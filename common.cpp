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

std::string hashKey( std::string salt, std::string PIN )
{
    //Hash the pin with the salt
    CryptoPP::SHA512 pin_hash;
    pin_hash.Update((byte *) salt.c_str(), salt.length());
    pin_hash.Update((byte *) PIN.c_str(), PIN.length());
    char pin_hash_buff[64];
    pin_hash.Final((byte *)pin_hash_buff);

    std::string hashedKey;
    hashedKey.assign( pin_hash_buff, 64 );

    return hashedKey;
}

std::string readRand( int desiredBytes )
{
    FILE* file = fopen("/dev/urandom", "r");
    char buffer[desiredBytes];
    fgets(buffer, desiredBytes, file);
    fclose(file);

    std::string randomString;
    randomString.assign(buffer, desiredBytes);
    return randomString;
}

int send_socket(std::string& data, std::string& recieved, int sock) {

    size_t length = data.length();
    const char * packet = data.c_str();
    char recvpacket[1025];

    //send the packet through the proxy to the bank
    if(length != 0 && sizeof(int) != send(sock, &length, sizeof(int), 0))
    {
        return -1;
    }
    if(length != 0 && (int)length != send(sock, (void*)packet, length, 0))
    {
        return -1;
    }

    //TODO: do something with response packet
    if(sizeof(int) != recv(sock, &length, sizeof(int), 0))
    {
        return -1;
    }
    if(length >= 1024 || length <= 0)
    {
        return -1;
    }
    if((int)length != recv(sock, recvpacket, length, 0))
    {
        return -1;
    }
    recieved.assign(recvpacket, length);
    return 0;
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
        printf("error1\n");
        return err;
    }
    size_t sep_pos = response.find('|');
    if (sep_pos == response.npos ){
        printf("error2\n");
        return -1;
    }
    response_type = response.substr(0, sep_pos);
    response_message = response.substr(sep_pos+1, response.length() - sep_pos);
    return 0;
}


std::string prev_nonce("");

int send_nonce(std::string& data_type, std::string& data, std::string& response_type, std::string& response_message, int sock) {
    if (prev_nonce.length() == 0) {
        //key = establish_key();
        //prev_nonce = 
    }
    return 0;
}
