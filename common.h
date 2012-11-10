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
#include <sys/time.h>

#include "cryptopp/cryptlib.h"
#include "cryptopp/filters.h"
#include "cryptopp/hex.h"
#include "cryptopp/hmac.h"
#include "cryptopp/sha.h"
#include "cryptopp/modes.h"
#include "cryptopp/aes.h"

enum STR2INT_ERROR { SUCCESS, S_OVERFLOW, S_UNDERFLOW, INCONVERTIBLE };
STR2INT_ERROR str2int (long &i, char const *s);

enum TRANSACTION_RESULT { TRANSACTED, REQUEST_ERROR, LOCK_ERROR, UNLOCK_ERROR };

bool applyHMAC( std::string plain, std::string stringKey, std::string& hashed );
bool validHMAC( std::string hash, std::string stringKey, std::string plain );
bool extractData( std::string fullMessage, std::string stringKey, std::string& data );
bool compileHashedMessage( std::string plain, std::string key, std::string& compiled );

std::string readRand( int desiredBytes );

std::string hashKey( std::string salt, std::string PIN );

int send_message(std::string & type, std::string& data, std::string&response_type, std::string& response_message, int sock);
