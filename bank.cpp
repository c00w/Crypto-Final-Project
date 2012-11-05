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

#include "common.h"

void* client_thread(void* arg);
void* console_thread(void* arg);

// Create the map to contain the users
std::map< std::string, long > userBalance;
pthread_mutex_t userMutex;
std::map< std::string, std::string > userPIN;

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
    
    //Set up the users.
    userBalance["Alice"] = 100;
    userBalance["Bob"]   = 50;
    userBalance["Eve"]   = 0;
    // Set up their keys.
    userPIN["Alice"] = "4321";
    userPIN["Bob"]   = "1234";
    userPIN["Eve"]   = "4130";
    
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
    int err;
    
    std::string empty("");
    std::string resp_type;
    std::string resp_message;
    err = send_message(empty, empty, resp_type, resp_message, csock); 
    printf("Returned from 1st recieve\n");
    if (err != 0) {
        close(csock);
        return NULL;
    }
    
    std::string random = readRand(64);
    std::string username;
    while(1)
    {
        std::cout << resp_type;
        if (resp_type.compare("getsalt") == 0) {
            username = resp_message;
            std::string messageType("sendsalt");
            err = send_message(messageType, random, resp_type, resp_message, csock);
        }
        else if(resp_type.compare("login") == 0){
            std::string cornedBeef = hashKey( random, userPIN[username] );
            std::string messageType("loginresult");
            std::string messageBody;
            if(resp_message.compare(cornedBeef) == 0)
            {
                messageBody.assign("0");
            }
            else
                messageBody.assign("1");
            err = send_message(messageType, messageBody, resp_type, resp_message, csock);
        }
    }

    printf("[bank] client ID #%d disconnected\n", csock);

    close(csock);
    return NULL;
}

bool balance( std::string& username )
{
    if( userBalance.find( username ) == userBalance.end() ){
        printf( "[bank] Error: nonexistant user\n" );
        return 1;
    }
    
	std::cout << "[bank] User " << username << " has $" << userBalance[username] << std::endl;
	return 0;
}

bool deposit( std::string& username, long argument )
{
    // Attempt to acquire a lock.
    int lock;
    lock = pthread_mutex_lock( &userMutex );
    if (lock != 0) {
        return 1;
    }
    
    if( userBalance.find( username ) == userBalance.end() ){
        printf( "[bank] Error: nonexistant user\n" );
        return 1;
    }
    long store1 = userBalance[username];
    long store2 = store1;
    if( ( store1 + argument ) < store2 ){
        printf( "[bank] Error: deposit would cause overflow\n" );
        return 1;
    }
    if( argument < 0 ){
        printf( "[bank] Error: negative deposit amount\n" );
        return 1;
    }
        
    else
    {
        // Success
        userBalance[username] += argument;
        std::cout << "[bank] User " << username << " deposited $" << argument << std::endl;
    }

    lock = pthread_mutex_unlock( &userMutex );
    if (lock != 0) {
        return 1;
    }
    return 0;
}

bool withdraw( std::string& username, long argument )
{
    // Attempt to acquire a lock.
    int lock;
    lock = pthread_mutex_lock( &userMutex );

    if (lock != 0) {
        return 1;
    }

    if( userBalance.find( username ) == userBalance.end() ){
        printf( "[bank] Error: nonexistant user\n" );
        return 1;
    }
    if( argument < 0 ){
        printf( "[bank] Error: negative withdrawl amount\n" );
        return 1;
    }
	
    userBalance[username] -= argument;
    std::cout << "[bank] User " << username << " withdrew $" << argument << std::endl;
	    
	lock = pthread_mutex_unlock( &userMutex );
    if (lock != 0) {
        return 1;
    }
	return 0;
}

bool transfer( std::string& username1, std::string& username2, long argument )
{
    // Attempt to acquire a lock.
    int lock;
    lock = pthread_mutex_lock( &userMutex );
    if (lock != 0) {
        return 1;
    }

    if( userBalance.find( username1 ) == userBalance.end() ){
        printf( "[bank] Error: nonexistant user 1\n" );
        return 1;
    }
    if( userBalance.find( username2 ) == userBalance.end() ){
        printf( "[bank] Error: nonexistant user 2\n" );
        return 1;
    }
    if( argument < 0 ){
        printf( "[bank] Error: negative transfer amount\n" );
        return 1;
    }
    if( argument > userBalance[username1] ){
        printf( "[bank] Error: transfer exceeds host user's balance\n" );
        return 1;
    }
    long store1 = userBalance[username2];
    long store2 = store1;
    if( ( store1 + argument ) < store2 ){
        printf( "[bank] Error: deposit would cause overflow\n" );
        return 1;
    }
    
    userBalance[username1] -= argument;
    userBalance[username2] += argument;
    
    std::cout << "[bank] User " << username1 << " transferred $" << argument
              << " to " << username2 << std::endl;
              
    lock = pthread_mutex_unlock( &userMutex );
    if (lock != 0) {
        return 1;
    }
    return 0;
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
        if (input.length() < 8) {
            continue;
        }

        //Handle deposit
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
            //std::cout << username << std::endl <<  balance << std::endl;
            
            deposit( username, balance );

        //Handle balance
        } else if (input.substr(0,7).compare("balance") == 0) {
            std::string username = input.substr(8, input.length()-8);
            //std:: cout << username << std::endl;
            
            balance( username );
        }
    }
    return NULL;
}


