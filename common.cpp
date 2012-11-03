
#include "common.h"

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

int send_socket(std::string& data, std::string& recieved, int sock) {

    size_t length = data.length();
    const char * packet = data.c_str();
    char recvpacket[1025];

    //send the packet through the proxy to the bank
    if(length != 0 && sizeof(int) != send(sock, &length, sizeof(int), 0))
    {
        printf("fail to send packet length\n");
        return -1;
    }
    if(length != 0 && length != send(sock, (void*)packet, length, 0))
    {
        printf("fail to send packet\n");
        return -1;
    }
    
    //TODO: do something with response packet
    if(sizeof(int) != recv(sock, &length, sizeof(int), 0))
    {
        printf("fail to read packet length\n");
        return -1;
    }
    if(length >= 1024 || length <= 0)
    {
        printf("packet too long\n");
        return -1;
    }
    if(length != recv(sock, recvpacket, length, 0))
    {
        printf("fail to read packet\n");
        return -1;
    }
    recieved.assign(recvpacket, length);
}

int send_message(std::string & type, std::string& data, std::string&response_type, std::string& response_message, int sock){

    //Construct message
    std::string message(type);
    if (type.length() != 0) {
        message.append("|");
        message.append(data);
    }

    //Send it and get response
    std::string response;
    int err = send_socket(message, response, sock);    
    if (err != 0) {
        return err;
    }
    size_t sep_pos = response.find('|');
    if (sep_pos == response.npos ){
        return -1;
    }
    response_type = response.substr(0, sep_pos); 
    response_message = response.substr(sep_pos+1, response.length() - sep_pos);
    return 0;
}
