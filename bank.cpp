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
#include <sstream>

#include "common.h"

void* client_thread(void* arg);
void* console_thread(void* arg);

// Create the map to contain the users
std::map< std::string, long > userBalance;
pthread_mutex_t userMutex;
std::map< std::string, std::string > userPIN;
std::map< std::string, std::string > userAccount;

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
    // Set up their account numbers.
    userAccount["Alice"] = "8765432187654321876543218765432187654321876543218765432187654321";
    userAccount["Bob"]   = "1234567812345678123456781234567812345678123456781234567812345678";
    userAccount["Eve"]   = "4130413041304130413041304130413041304130413041304130413041304130";

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

int balance( std::string& username, long& requestedBalance )
{
    // Attempt to acquire a lock.
    int lock;
    lock = pthread_mutex_lock( &userMutex );

    if (lock != 0) {
        return LOCK_ERROR;
    }
    
    // Gets the users balance, returns in requestedBalance.
    if( userBalance.find( username ) == userBalance.end() ){
        printf( "[bank] Error: nonexistant user.\n" );        
        lock = pthread_mutex_unlock( &userMutex );
        if (lock != 0) exit(1);
        return ERROR;
    }

    requestedBalance = userBalance[username];

    lock = pthread_mutex_unlock( &userMutex );
    if (lock != 0) {
        exit(1);
    }

	return TRANSACTED;
}

int deposit( std::string& username, long argument, long& newBalance )
{
    // Attempt to acquire a lock.
    int lock;
    lock = pthread_mutex_lock( &userMutex );
    if (lock != 0) {
        return LOCK_ERROR;
    }

    // Verify that the input is acceptable.
    if( userBalance.find( username ) == userBalance.end() ){
        printf( "[bank] Error: nonexistant user.\n" );     
        lock = pthread_mutex_unlock( &userMutex );
        if (lock != 0) exit(1);
        return ERROR;
    }
    if( argument < 0 ){
        printf( "[bank] Error: cannot deposit negative amounts.\n" );     
        lock = pthread_mutex_unlock( &userMutex );
        if (lock != 0) exit(1);
        return ERROR;
    }
    long store1 = userBalance[username];
    long store2 = store1;
    if( ( store1 + argument ) < store2 ){
        printf( "[bank] Error: deposit would cause overflow.\n" );     
        lock = pthread_mutex_unlock( &userMutex );
        if (lock != 0) exit(1);
        return ERROR;
    }
    if( argument < 0 ){
        printf( "[bank] Error: negative deposit amount.\n" );     
        lock = pthread_mutex_unlock( &userMutex );
        if (lock != 0) exit(1);
        return ERROR;
    }

    // Do the deposit, return the new balance in newBalance.
    userBalance[username] += argument;
    newBalance = userBalance[username];

    lock = pthread_mutex_unlock( &userMutex );
    if (lock != 0) {
        exit(1);
    }
    return TRANSACTED;
}

int withdraw( std::string& username, long argument, long& newBalance )
{
    // Attempt to acquire a lock.
    int lock;
    lock = pthread_mutex_lock( &userMutex );

    if (lock != 0) {
        return LOCK_ERROR;
    }

    // Verify that the input is acceptable.
    if( userBalance.find( username ) == userBalance.end() ){
        printf( "[bank] Error: nonexistant user.\n" );     
        lock = pthread_mutex_unlock( &userMutex );
        if (lock != 0) exit(1);
        return ERROR;
    }
    if( argument < 0 ){
        printf( "[bank] Error: cannot deposit negative amounts.\n" );     
        lock = pthread_mutex_unlock( &userMutex );
        if (lock != 0) exit(1);
        return ERROR;
    }
    if( userBalance[username] < argument ){
        printf( "[bank] Error: cannot withdraw more than is in account.\n" );     
        lock = pthread_mutex_unlock( &userMutex );
        if (lock != 0) exit(1);
        return ERROR;
    }

    // Do the withdrawl, return the new balance in newBalance.
    userBalance[username] -= argument;
    newBalance = userBalance[username];

	lock = pthread_mutex_unlock( &userMutex );
    if (lock != 0) {
        exit(1);
    }
	return TRANSACTED;
}

int transfer( std::string& username1, std::string& username2, long argument, long& newBalance )
{
    // Attempt to acquire a lock.
    int lock;
    lock = pthread_mutex_lock( &userMutex );
    if (lock != 0) {
        return LOCK_ERROR;
    }

    // Verify that the input is acceptable
    if( userBalance.find( username1 ) == userBalance.end() ){
        printf( "[bank] Error: nonexistant user 1.\n" );     
        lock = pthread_mutex_unlock( &userMutex );
        if (lock != 0) exit(1);
        return ERROR;
    }
    if( userBalance.find( username2 ) == userBalance.end() ){
        printf( "[bank] Error: nonexistant user 2.\n" );     
        lock = pthread_mutex_unlock( &userMutex );
        if (lock != 0) exit(1);
        return ERROR;
    }
    if( argument < 0 ){
        printf( "[bank] Error: negative transfer amount.\n" );     
        lock = pthread_mutex_unlock( &userMutex );
        if (lock != 0) exit(1);
        return ERROR;
    }
    if( argument > userBalance[username1] ){
        printf( "[bank] Error: transfer exceeds host user's balance.\n" );     
        lock = pthread_mutex_unlock( &userMutex );
        if (lock != 0) exit(1);
        return ERROR;
    }
    long store1 = userBalance[username2];
    long store2 = store1;
    if( ( store1 + argument ) < store2 ){
        printf( "[bank] Error: deposit would cause overflow.\n" );     
        lock = pthread_mutex_unlock( &userMutex );
        if (lock != 0) exit(1);
        return ERROR;
    }

    // Do the transfer, then return the new balance for the first user in newBalance
    userBalance[username1] -= argument;
    userBalance[username2] += argument;
    newBalance = userBalance[username1];

    lock = pthread_mutex_unlock( &userMutex );
    if (lock != 0) {
        exit(1);
    }
    return TRANSACTED;
}

void* client_thread(void* arg)
{
    // A client thread for any ATM.
    int csock = (int)arg;
    
    std::cout << "\n[bank] Client ID #" << csock << " connected.\n>bank>";
    std::cout.flush();
    
    //input loop
    int err, errID;

    keyinfo conn_info;
    std::string empty("");
    std::string resp_type;
    std::string resp_message;
    std::string messageBody(""), messageType("");
    std::string random = readRand(64);
    std::string sent_salt;
    std::string username;
    err = send_message(messageBody, messageType, resp_type, resp_message, csock, conn_info);

    while(err == 0)
    {
        std::cout << resp_type << " " << resp_message << std::endl;
        // Initial setting up the connection.
        if( resp_type.compare("getsalt") == 0 ) {
            // Getting the salt.
            username = resp_message;
            messageType.assign("sendsalt");
            messageBody.assign(random);
            sent_salt.assign(random);
            random = readRand(64);
        } else if( resp_type.compare("login") == 0 ){
            // Handling for logging in with PIN and accout number verification.
            std::string combinedUserInfo(userAccount[username]);
            combinedUserInfo.append(userPIN[username]);      
            std::string cornedBeef = hashKey( sent_salt, combinedUserInfo );
            messageType.assign("loginresult");
            if(resp_message.compare(cornedBeef) == 0){
                messageBody.assign("0");
            }
            else{
                messageBody.assign("1");
                username.assign("");
            }
        } else if( resp_type.compare("logout") == 0 ){
            // Logging out.
            messageType.assign("logoutresult");
            messageBody.assign("0");
            username.assign("");
        }
        // Begin the actual ATM requests involving moneys.
        else if( resp_type.compare("balance") == 0 ){
            // Handle a balance request from the user.
            long requestedBalance;
            errID = balance( username, requestedBalance );

            messageType.assign("balanceresult");

            if( errID == TRANSACTED ){
                std::stringstream sBalance;
                sBalance << requestedBalance;
                messageBody.assign(sBalance.str());
            } else if( errID == ERROR )
                messageBody.assign("ERROR");
            else if( errID == LOCK_ERROR || errID == UNLOCK_ERROR )
                messageBody.assign("ERROR");
        } else if( resp_type.compare("withdraw") == 0 ){
            // Handle a withdrawl request from the user.
            messageType.assign("withdrawresult");
            
            long withdrawl;
            if ( str2int( withdrawl, resp_message.c_str() ) != SUCCESS ){
                messageBody.assign("ERROR");
                err = send_message( messageType, messageBody, resp_type, resp_message, csock, conn_info);
                continue;
            }
            
            long newBalance;
            errID = withdraw( username, withdrawl, newBalance );
            
            if( errID == TRANSACTED ){
                std::stringstream sBalance;
                sBalance << newBalance;
                messageBody.assign(sBalance.str());
            } else if( errID == ERROR )
                messageBody.assign("ERROR");
            else if( errID == LOCK_ERROR || errID == UNLOCK_ERROR )
                messageBody.assign("ERROR");

        } else if( resp_type.compare("transfer") == 0 ){
            // Handle a transfer request from the user.
            messageType.assign("transferresult");
            
            std::string recipient, amount;
            recipient = resp_message.substr( 0, (int)resp_message.find("|") );
            amount    = resp_message.substr( (int)resp_message.find("|") + 1, resp_message.length() );
            long transferAmount;
            if ( str2int( transferAmount, amount.c_str() ) != SUCCESS ){
                messageBody.assign("ERROR");
                err = send_message( messageType, messageBody, resp_type, resp_message, csock, conn_info);
                continue;
            }
            
            long newBalance;
            errID = transfer( username, recipient, transferAmount, newBalance );
            
            if( errID == TRANSACTED ){
                std::stringstream sBalance;
                sBalance << newBalance;
                messageBody.assign(sBalance.str());
            } else if( errID == ERROR )
                messageBody.assign("ERROR");
            else if( errID == LOCK_ERROR || errID == UNLOCK_ERROR )
                messageBody.assign("ERROR");
        }
        std::cout << messageType << " " << messageBody << std::endl;
        err = send_message( messageType, messageBody, resp_type, resp_message, csock, conn_info);
    }

    std::cout << "\n[bank] Client ID #" << csock << " disconnected.\n>bank>";
    std::cout.flush();
    
    close(csock);
    return NULL;
}

void* console_thread(void* arg)
{
    // A thread for the bank to handle direct requests.
    char buf[80];
    int errID;

    while(1)
    {
        printf(">bank> ");
        fgets(buf, 79, stdin);
        buf[strlen(buf)-1] = '\0';  //trim off trailing newline

        // Convert to c++ style string
        std::string input(buf);
        if (input.length() < 8) {
            continue;
        }

        if (input.substr(0,7).compare("deposit") == 0) {
            //Handle deposit
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

            long newBalance;
            errID = deposit( username, balance, newBalance );
            if( errID == TRANSACTED )
                std::cout << "[bank] " << username << " deposited $" << balance 
                          << ", new balance $" << newBalance << "." << std::endl;
            else if( errID == ERROR )
                std::cout << "[bank] Request failed.\n";
            else if( errID == LOCK_ERROR || errID == UNLOCK_ERROR )
                std::cout << "[bank] Critical failure.\n";

        } else if (input.substr(0,7).compare("balance") == 0) {
            //Handle balance
            std::string username = input.substr(8, input.length()-8);

            long requestedBalance;
            errID = balance( username, requestedBalance );
            if( errID == TRANSACTED )
                std::cout << "[bank] " << username << " has a balance of $" 
                          << requestedBalance << "." << std::endl;
            else if( errID == ERROR )
                std::cout << "[bank] Request failed.\n";
            else if( errID == LOCK_ERROR || errID == UNLOCK_ERROR )
                std::cout << "[bank] Critical failure.\n";
        }
    }
    return NULL;
}



