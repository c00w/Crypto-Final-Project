/**
    @file atm.cpp
    @brief Top level ATM implementation file
 */
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>


#include <string>
#include <iostream>
#include <fstream>

#include "cryptopp/sha.h"

#include "common.h"

std::string User("");

void error() {
    std::cout<< "[atm] Error. Please try again." << std::endl;
}

int error_guard(std::string resp_data) {
    if( resp_data.compare("ERROR") == 0 ){
        return 1;
    }
    return 0;
}

int handle_input(std::string & input, int sock, keyinfo& conn_info) {
    // Handles user input
    if (input.length() < 6) {
        return 1;
    }

    if (input.substr(0, 5).compare("login") == 0) {
        // Handle a login request
        if (User.length() > 0) {
            return 1;
        }
        std::string username = input.substr(6, input.length()-6);
        if (username.length() <= 0) {
            return 1;
        }

        std::string cardFile(username);
        cardFile.append(".card");
        std::ifstream cardReader;
        cardReader.open(cardFile.c_str());
        if( !cardReader.good() ){
            std::cout << "[atm] Bad login." << std::endl;
            return 1;
        }
        std::string userCard;
        cardReader >> userCard;
        if( userCard.length() != 64 ){
            std::cout << "[atm] Bad login." << std::endl;
            return 1;
        }

        char *password = getpass("      PIN: ");
        if (password == NULL) {
            std::cout << "[atm] Bad login." << std::endl;
            return 1;
        }
        std::string PIN(password);
        if (PIN.length() != 4) {
            std::cout << "[atm] Bad login." << std::endl;
            return 1;
        }

        //Get a salt from the server
        std::string msg_type("getsalt");
        std::string msg_data(username);
        std::string resp_type;
        std::string resp_data;
        int err = send_message(msg_type, msg_data, resp_type, resp_data, sock, conn_info);

        if (err != 0 || resp_type.compare("sendsalt") != 0 || resp_data.length() <28) {
            return 1;
        }

        //Hash the login info with the salt
        msg_type.assign("login");
        std::string loginData(userCard);
        loginData.append(PIN);
        msg_data = hashKey( resp_data, loginData );

        //Try and login
        err = send_message(msg_type, msg_data, resp_type, resp_data, sock, conn_info);

        if (err != 0 || resp_type.compare("loginresult") != 0) {
            return 1;
        }

        if (resp_data.compare("0") == 0) {
            std::cout << "[atm] Logged in as " << username << "." << std::endl;
        }
        else{
            std::cout << "[atm] Bad login." << std::endl;
            return 1;
        }
        User.assign(username);
        return 0;
    }

    //User guard
    if (User.length() == 0) {
        return 1;
    }

    if (input.substr(0,6).compare("logout") == 0) {
       // Handle a logout request
       std::string msg_type("logout"), msg_data(""), resp_type, resp_data;
       int err = send_message(msg_type, msg_data, resp_type, resp_data, sock, conn_info);
       if( err != 0 || resp_type.compare("logoutresult") != 0 ) {
            return 1;
       }
       if( resp_data.compare("0") == 0 ) {
            std::cout << "[atm] Logged out." << std::endl;
            User.assign("");
       }
    } else if (input.substr(0,7).compare("balance") == 0) {
        // Handle a balance request
        std::string msg_type("balance"), msg_data(""), resp_type, resp_data;
        int err = send_message(msg_type, msg_data, resp_type, resp_data, sock, conn_info);
        if (err != 0 || resp_type.compare("balanceresult") != 0) {
            return 1;
        }
        if (error_guard(resp_data)!= 0) {
            return 1;
        }

        std::cout << "[atm] User has a balance of $" << resp_data << "." << std::endl;

    } else if (input.substr(0,8).compare("transfer") == 0) {
        // Handle a transfer request
        if (input.length() < 9) {
            return 1;
        }
        std::string params = input.substr(9, input.length() -9);

        size_t space_index = params.find(' ');
        if (space_index == params.npos) {
            return 1;
        }
        std::string username = params.substr(0, space_index);
        std::string amount = params.substr(space_index+1, params.length()-space_index-1);

        std::string msg_type("transfer"), msg_data(username), resp_type, resp_data;
        msg_data.append("|");
        msg_data.append(amount);
        int err = send_message(msg_type, msg_data, resp_type, resp_data, sock, conn_info);
        if (err != 0 || resp_type.compare("transferresult") != 0) {
            return 1;
        }

        if (error_guard(resp_data) != 0) {
            return 1;
        }
        std::cout << "[atm] User transferred $" << amount << " to " <<username 
                  << ", leaving a final balance of $" << resp_data << "." << std::endl;

    } else if (input.substr(0,8).compare("withdraw") == 0) {
        // Handle a withdrawl request
        if (input.length() < 9) {
            return 1;
        }
        std::string amount = input.substr(9, input.length()-9);
        std::string msg_type("withdraw"), msg_data(amount), resp_type, resp_data;
        int err = send_message(msg_type, msg_data, resp_type, resp_data, sock, conn_info);
        if (err != 0 || resp_type.compare("withdrawresult") != 0) {
            return 1;
        }

        if (error_guard(resp_data) != 0) {
            return 1;
        }
        std::cout << "[atm] $" << amount << " withdrawn. New balance $" << resp_data << "." << std::endl;

    } else {
        return 1;
    }
    return 0;
}



int main(int argc, char* argv[])
{
    if(argc != 2)
    {
        printf("Usage: atm proxy-port\n");
        return -1;
    }

    //socket setup
    unsigned short proxport = atoi(argv[1]);
    int sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if(!sock)
    {
        printf("fail to create socket\n");
        return -1;
    }
    sockaddr_in addr;
    addr.sin_family = AF_INET;
    addr.sin_port = htons(proxport);
    unsigned char* ipaddr = reinterpret_cast<unsigned char*>(&addr.sin_addr);
    ipaddr[0] = 127;
    ipaddr[1] = 0;
    ipaddr[2] = 0;
    ipaddr[3] = 1;
    if(0 != connect(sock, reinterpret_cast<sockaddr*>(&addr), sizeof(addr)))
    {
        printf("fail to connect to proxy\n");
        return -1;
    }

    //input loop
    char buf[80];
    keyinfo conn_info;
    while(1)
    {
        printf(">atm> ");
        char * res = fgets(buf, 79, stdin);
        if (res == NULL) {
            break;
        }
        buf[strlen(buf)-1] = '\0';  //trim off trailing newline

        //Upcast string
        std::string input(buf);
        int err = handle_input(input, sock, conn_info);
        if (err != 0) {
            error();
        }
    }

    //cleanup
    close(sock);
    return 0;
}



