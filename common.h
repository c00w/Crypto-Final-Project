#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <limits.h>
#include <iostream>

#include <string>

#include "cryptopp/sha.h"

enum STR2INT_ERROR { SUCCESS, OVERFLOW, UNDERFLOW, INCONVERTIBLE };
STR2INT_ERROR str2int (long &i, char const *s);

enum TRANSACTION_RESULT { TRANSACTED, REQUEST_ERROR, LOCK_ERROR, UNLOCK_ERROR };

std::string readRand( int desiredBytes );

std::string hashKey( std::string salt, std::string PIN );

int send_message(std::string & type, std::string& data, std::string&response_type, std::string& response_message, int sock);
