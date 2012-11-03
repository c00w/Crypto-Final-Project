#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include <string>

int send_socket(std::string& data, std::string& recieved, int sock);
int send_message(std::string & type, std::string& data, std::string&response_type, std::string& response_message, int sock);
