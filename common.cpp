#include "common.h"

STR2INT_ERROR str2int (long &i, char const *s)
{
    char *end;
    long  l;
    errno = 0;
    l = strtol(s, &end, 0); //Base = 10
    if ((errno == ERANGE && l == LONG_MAX) || l > INT_MAX) {
        return S_OVERFLOW;
    }
    if ((errno == ERANGE && l == LONG_MIN) || l < INT_MIN) {
        return S_UNDERFLOW;
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

std::string readRandInt()
{
    FILE* file = fopen("/dev/urandom", "r");
    int result;
    fgets((char *)&result, sizeof(int), file);
    fclose(file);

    return std::to_string(result);
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
    if(sizeof(int) != recv(sock, &length, sizeof(int), MSG_WAITALL))
    {
        return -1;
    }
    if(length >= 1024 || length <= 0)
    {
        return -1;
    }
    if((int)length != recv(sock, recvpacket, length, MSG_WAITALL))
    {
        return -1;
    }
    recieved.assign(recvpacket, length);
    return 0;
}

bool applyHMAC( std::string plain, std::string stringKey, std::string& hashed ) {
    CryptoPP::SecByteBlock key( (byte*)stringKey.c_str(), stringKey.length() );

    // Set up the required parts for the HMAC.
    std::string encoded, mac;
    encoded.clear();
    CryptoPP::StringSource( key, key.size(), true,
                            new CryptoPP::HexEncoder( new CryptoPP::StringSink(encoded) ) );

    // Attempt to HMAC.
    try{
        CryptoPP::HMAC<CryptoPP::SHA256> cornedBeef( key, key.size() );
        CryptoPP::StringSource( plain, true,
                                new CryptoPP::HashFilter( cornedBeef, new CryptoPP::StringSink(mac) ) );
    } catch( const CryptoPP::Exception& e ) {
        std::cerr << "[8R8K] D::::\n";
        return 1;
    }

    // Reapply it.
    encoded.clear();
    CryptoPP::StringSource( mac, true, new CryptoPP::HexEncoder( new CryptoPP::StringSink(encoded) ) );

    // Return the hashed plaintext and a success.
    hashed.assign(encoded);
    return 0;
}

bool validHMAC( std::string hash, std::string stringKey, std::string plain ) {
    // Verify if the HMAC comes out as identical.
    std::string attemptedHash;
    if( applyHMAC( plain, stringKey, attemptedHash ) ) return 0;
    if( hash.compare(attemptedHash) == 0 ) return 1;
    return 0;
}

bool compileHashedMessage( std::string plain, std::string key, std::string& compiled )
{
    // hash|time|message
    std::string hash, time;

    timeval currentTime;
    gettimeofday( &currentTime, NULL );
    double timeNow  = currentTime.tv_sec + ( currentTime.tv_usec / 1000000.0 );

    if( applyHMAC( plain, key, hash ) ) return 1;
    time.assign( std::to_string(timeNow) );

    compiled.assign(hash);
    compiled.append("|");
    compiled.append(time);
    compiled.append("|");
    compiled.append(plain);
    return 0;
}

bool extractData( std::string fullMessage, std::string stringKey, std::string& data )
{
    // Extract the message components, formatted as hash|time|message
    std::string hash, time, message;
    int pipeLocs[2];
    pipeLocs[0] = (int)fullMessage.find("|");
    pipeLocs[1] = (int)fullMessage.find( "|", pipeLocs[0]+1 );
    int subLengths[3];
    subLengths[0] = pipeLocs[0];
    subLengths[1] = ( pipeLocs[1] - pipeLocs[0] ) - 1;
    subLengths[2] = ( fullMessage.length() - pipeLocs[1] ) - 1;    
    hash.assign( fullMessage.substr( 0, subLengths[0] ) );
    time.assign( fullMessage.substr( pipeLocs[0] + 1, subLengths[1] ) );
    message.assign( fullMessage.substr( pipeLocs[1] + 1, subLengths[2] ) );

    // Verify the integrity
    if( !validHMAC( hash, stringKey, message ) ) return 1;
    
    // Verify that the timestamp is reasonable.
    timeval currentTime;
    gettimeofday( &currentTime, NULL );
    double timeNow  = currentTime.tv_sec + ( currentTime.tv_usec / 1000000.0 );
    double timeThen = atof( time.c_str() );
    if( timeThen > timeNow ) return 1;
    if( timeNow - timeThen > 0.1 ) return 1;
    
    data.assign(message);
    return 0;
}

int send_message(std::string & type, std::string& data, std::string&response_type, std::string& response_message, int sock, keyinfo & conn_info){

    //Construct message
    std::string message(type);
    if (type.length() != 0) {
        message.append("|");
        message.append(data);
    }

    //Send it and get response
    std::string response;
    int err = send_nonce(message, response, sock, conn_info);
    
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

int send_HMAC( std::string& data, std::string& response, int sock, keyinfo & conn_info ){
    // Attempt to HMAC and send the message
    std::string wrappedData;
    std::string wrappedResponse;
    std::string unwrappedResponse;
    std::string key = "1234567890123456";
    
    if( data.length() != 0 )
        if( compileHashedMessage( data, key, wrappedData ) != 0 ){
            std::cout << "Failed hash. br8kspider\n";
            return -1;
        }
    
    int err = send_socket( wrappedData, wrappedResponse, sock );
    if( err != 0 ) return err;
    
    if( extractData( wrappedResponse, key, unwrappedResponse ) != 0 ){
        std::cout << "Failed unwrap. br8kspider\n";
        return -1;
    }
    
    response.assign( unwrappedResponse );
    
    return 0;
}

int send_aes(std::string& data, std::string& response, int sock, keyinfo & conn_info) {
    // Attempt to initialize and send the message.
    try {
        if (conn_info.aes_initialized == false) {
            conn_info.aes_initialized = true;
            byte iv[CryptoPP::AES::BLOCKSIZE] = "123456789123456";
            byte key[32] = "1234567890123456789012345678901";
            CryptoPP::SecByteBlock fukey(key, 32);
            conn_info.aesdecrypt.SetKeyWithIV(fukey, fukey.size(), iv);
            conn_info.aesencrypt.SetKeyWithIV(fukey, fukey.size(), iv);
        }
        byte ciphertext[data.length()];
        conn_info.aesencrypt.ProcessData(ciphertext, (byte *)data.c_str(), data.length());
        data.assign((char* )ciphertext, data.length());
        
        int err = send_HMAC(data, response, sock, conn_info);
        
        if (err != 0) {
            return err;
        }
        byte plaintext[response.length()];
        conn_info.aesdecrypt.ProcessData(plaintext, (byte *)response.c_str(), response.length());
        response.assign((char *)plaintext, response.length());
    } catch( const CryptoPP::Exception& e) {
        return -1;
    }
    return 0;
}

int send_nonce(std::string& data, std::string& response, int sock, keyinfo& conn_info) {
    // Noncing the message
    std::string my_nonce, new_data;
    if (conn_info.there_nonce.length() == 0) {
        int err = establish_key(data.length()==0, sock, conn_info);
        if (err != 0) {
            return err;
        }
        //prev_nonce =
        conn_info.there_nonce.assign("asdasdasd");
    }
    if (data.length() != 0) {
        my_nonce.assign(readRandInt());
        new_data.assign(my_nonce);
        new_data.append("|");
        new_data.append(conn_info.there_nonce);
        new_data.append("|");
        new_data.append(data);
    } else {
       my_nonce.assign(conn_info.there_nonce);
       new_data.assign("");
    }
    int err = send_aes(new_data, response, sock, conn_info);
    if (err != 0) {
        return err;
    }

    std::string params(response);
    size_t sep_pos = params.find('|');
    if (sep_pos == params.npos) {
        return -1;
    };

    conn_info.there_nonce = params.substr(0, sep_pos);
    params.assign(params.substr(sep_pos+1, params.length()-sep_pos));
    sep_pos = params.find('|');
    if (sep_pos == params.npos) {
        return -1;
    }

    std::string sent_my_nonce = params.substr(0, sep_pos);
    if (my_nonce.compare(sent_my_nonce) != 0) {
        return -1;
    }
    response.assign(params.substr(sep_pos+1, params.length()-sep_pos));
    return 0;
}

int establish_key(bool server, int csock, keyinfo& conn_info) {
    std::string mydata = readRand(128);
    std::string their_data;
    int err = send_socket(mydata, their_data, csock);
    if (err != 0) {
        return -1;
    }
    std::string data;
    if (server) {
        data.assign(mydata);
        data.append(their_data);
    } else {
        data.assign(their_data);
        data.append(mydata);
    }
    CryptoPP::SHA512 hash_key;
    hash_key.Update((byte *) data.c_str(), data.length());
    char session_key_buff[64];
    hash_key.Final((byte *)session_key_buff);
    std::string aes_key(session_key_buff, 32);
    return 0;
}
