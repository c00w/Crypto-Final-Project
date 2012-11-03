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

#include "cryptopp/sha.h"

#include "common.h"

std::string User("");

void handle_input(std::string & input, int sock) {
    if (input.length() <= 6) {
        return;
    }

    if (input.substr(0, 5).compare("login") == 0) {
        if (User.length() > 0) {
            return;
        }
        std::string username = input.substr(6, input.length()-6);
        if (username.length() <= 0) {
            return;
        }
        char *password = getpass("PIN: ");
        if (password == NULL) {
            return;
        }
        std::string PIN(password);
        if (PIN.length() != 4) {
            return;
        }

        //Get a salt from the server
        std::string msg_type("getsalt");
        std::string msg_data(username);
        std::string resp_type;
        std::string resp_data;
        int err = send_message(msg_type, msg_data, resp_type, resp_data, sock);

        std::cout << "Recieved salt" << std::endl;

        if (err != 0 || resp_type.compare("sendsalt") != 0 || resp_data.length() <28) {
            return;
        }

        //Hash the pin with the salt
        CryptoPP::SHA512 pin_hash;
        pin_hash.Update((byte *) resp_data.c_str(), resp_data.length());
        pin_hash.Update((byte *) PIN.c_str(), PIN.length());
        char pin_hash_buff[64];
        pin_hash.Final((byte *)pin_hash_buff);

        msg_type.assign("login");
        msg_data.assign(pin_hash_buff, 64);

        std::cout << "Sent salt to server" << std::endl;

        //Try and login
        err = send_message(msg_type, msg_data, resp_type, resp_data, sock);

        if (err != 0 || resp_type.compare("loginresult") != 0) {
            return;
        }
        
        if (resp_data.length() && resp_data.compare("0")) {
            std::cout << "Logged in" << std::endl;
        }
        User.assign(username);
    } else if (input.substr(0,6).compare("logout")) {
       if (User.length() == 0) {
            return;
       }

       std::string msg_type("logout"), msg_data(""), resp_type, resp_data;
       int err = send_message(msg_type, msg_data, resp_type, resp_data, sock);
       if (err != 0 || resp_type.compare("logout") != 0) {
            return;
       }
       if (resp_data.compare("0")) {
            std::cout << "Logged out" << std::endl;
            User.assign("");
       }
    }
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
    while(1)
    {
        printf("atm> ");
        fgets(buf, 79, stdin);
        buf[strlen(buf)-1] = '\0';  //trim off trailing newline
        
        //TODO: your input parsing code has to put data here
        char packet[1024];
        int length = 1;
       
        //Upcast string
        std::string input(buf);
        handle_input(input, sock);
    }
    
    //cleanup
    close(sock);
    return 0;
}

