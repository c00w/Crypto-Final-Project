/**
    @file bank.cpp
    @brief Top level bank implementation file
 */
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <stdlib.h>
#include <stdio.h>
#include <pthread.h>
#include <string.h>
#include <errno.h>
#include <limits.h>

#include <string>
#include <iostream>
#include <stdexcept>
#include <map>

void* client_thread(void* arg);
void* console_thread(void* arg);
enum STR2INT_ERROR { SUCCESS, OVERFLOW, UNDERFLOW, INCONVERTIBLE };

STR2INT_ERROR str2int (long &i, char const *s)
{
    char *end;
    long  l;
    errno = 0;
    l = strtol(s, &end, 0); //Base = 10
    if ((errno == ERANGE && l == LONG_MAX) || l > INT_MAX) {
        return OVERFLOW;
    }
    if ((errno == ERANGE && l == LONG_MIN) || l < INT_MIN) {
        return UNDERFLOW;
    }
    if (*s == '\0' || *end != '\0') {
        return INCONVERTIBLE;
    }
    i = l;
    return SUCCESS;
}

// Create the map to contain the users
std::map< std::string, unsigned long int > users;
pthread_mutex_t userMutex;

int main(int argc, char* argv[])
{
    if(argc != 2)
    {
        printf("Usage: bank listen-port\n");
        return -1;
    }
    
    unsigned short ourport = atoi(argv[1]);
    
    //socket setup
    int lsock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if(!lsock)
    {
        printf("fail to create socket\n");
        return -1;
    }
    
    //listening address
    sockaddr_in addr_l;
    addr_l.sin_family = AF_INET;
    addr_l.sin_port = htons(ourport);
    unsigned char* ipaddr = reinterpret_cast<unsigned char*>(&addr_l.sin_addr);
    ipaddr[0] = 127;
    ipaddr[1] = 0;
    ipaddr[2] = 0;
    ipaddr[3] = 1;
    if(0 != bind(lsock, reinterpret_cast<sockaddr*>(&addr_l), sizeof(addr_l)))
    {
        printf("failed to bind socket\n");
        return -1;
    }
    if(0 != listen(lsock, SOMAXCONN))
    {
        printf("failed to listen on socket\n");
        return -1;
    }
    
    pthread_t cthread;
    pthread_create(&cthread, NULL, console_thread, NULL);
    
    //loop forever accepting new connections
    while(1)
    {
        sockaddr_in unused;
        socklen_t size = sizeof(unused);
        int csock = accept(lsock, reinterpret_cast<sockaddr*>(&unused), &size);
        if(csock < 0)   //bad client, skip it
            continue;
            
        pthread_t thread;
        pthread_create(&thread, NULL, client_thread, (void*)csock);
    }
}

void* client_thread(void* arg)
{
    int csock = (int)arg;
    
    printf("[bank] client ID #%d connected\n", csock);
    
    //input loop
    int length;
    char packet[1024];
    int lock;
    
    while(1)
    {
        //read the packet from the ATM
        if(sizeof(int) != recv(csock, &length, sizeof(int), 0))
            break;
        if(length >= 1024)
        {
            printf("packet too long\n");
            break;
        }
        if(length != recv(csock, packet, length, 0))
        {
            printf("[bank] fail to read packet\n");
            break;
        }
        
        //TODO: process packet data
		// Branch based on the requested operation.
		int request; enum kRequests{ kBalance, kDeposit, kWithdraw };
		int argument;
		std::string username;
		
		// Error: user doesn't exist
		if( users.find( username ) == users.end() ) printf( "[bank] Error 41302\n" );
		// Error: negative requested balance change
		if( argument < 0 ) printf( "[bank] Error 41389\n" );
		// Otherwise, attempt the request
		else if( request == kBalance )
		{
			std::cout << "[bank] User " << username << " has $" 
			          << static_cast<double>( users[username] ) / 100 << std::endl;
		}
		else if( request == kDeposit )
		{
		    // Attempt to acquire a lock.
		    lock = pthread_mutex_lock( &userMutex );
			
            int store1 = users[username];
            int store2 = store1;
            if( ( store1 + argument ) < store2 )
                printf( "[bank] Error 41372\n" );
            else
            {
                // Success
                users[username] += argument;
                std::cout << "[bank] User " << username << " deposited $"
		                  << static_cast<double>( argument ) / 100 << std::endl;
            }	
            
		    lock = pthread_mutex_unlock( &userMutex );
		}
		else if( request == kWithdraw )
		{
			// Attempt to acquire a lock.
			lock = pthread_mutex_lock( &userMutex );
			
		    if( users[username] - argument < 0 )
		        printf( "[bank] Error 41322\n" );
		    else
		    {
		        // Success
		        users[username] -= argument;
                std::cout << "[bank] User " << username << " withdrew $"
		                  << static_cast<double>( argument ) / 100 << std::endl;
		    }
			    
			lock = pthread_mutex_unlock( &userMutex );
		}
		else
		{
			// Invalid request.
			printf( "[bank] Error 41307\n" );
		}
        
        //TODO: put new data in packet
        
        //send the new packet back to the client
        if(sizeof(int) != send(csock, &length, sizeof(int), 0))
        {
            printf("[bank] fail to send packet length\n");
            break;
        }
        if(length != send(csock, (void*)packet, length, 0))
        {
            printf("[bank] fail to send packet\n");
            break;
        }

    }

    printf("[bank] client ID #%d disconnected\n", csock);

    close(csock);
    return NULL;
}

void* console_thread(void* arg)
{
    char buf[80];
    while(1)
    {
        printf("bank> ");
        fgets(buf, 79, stdin);
        buf[strlen(buf)-1] = '\0';  //trim off trailing newline

        // Convert to c++ style string
        std::string input(buf);
        if (input.length() < 7) {
            continue;
        }
        if (input.substr(0,7).compare("deposit") == 0) {
            std::string params = input.substr(8, input.length()-8);

            size_t space_index = params.find(' ');
            if (space_index == input.npos){
                continue;
            }

            std::string username = params.substr(0, space_index);
            std::string balance_str = params.substr(space_index+1, params.length()-space_index);
            if (username.length() == 0 or balance_str.length() == 0) {
                continue;
            }

            long balance;
            if (str2int(balance, balance_str.c_str()) != SUCCESS) {
                continue;
            }
            if (balance <= 0) {
                continue;
            }

            std::cout << username << std::endl <<  balance << std::endl;
        } else if (input.substr(0,7).compare("balance") == 0) {
            std::string username = input.substr(8, input.length()-8);
            std:: cout << username << std::endl;
        }
    }
}


