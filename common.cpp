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

bool applyHMAC( std::string plain, std::string stringKey, std::string& hashed ) {
    CryptoPP::SecByteBlock key( (byte*)stringKey.c_str(), stringKey.length() );

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

    encoded.clear();
    CryptoPP::StringSource( mac, true, new CryptoPP::HexEncoder( new CryptoPP::StringSink(encoded) ) );

    hashed.assign(encoded);
    return 0;
}

bool validHMAC( std::string hash, std::string stringKey, std::string plain ) {
    std::string attemptedHash;
    if( applyHMAC( plain, stringKey, attemptedHash ) ) return 0;
    if( hash.compare(attemptedHash) == 0 ) return 1;
    return 0;
}

bool extractData( std::string fullMessage, std::string stringKey, std::string& data )
{
    // hash|time|message
    std::string hash, time, message;
    int pipeLocs[2];
    pipeLocs[0] = (int)fullMessage.find("|");
    pipeLocs[1] = (int)fullMessage.find( "|", pipeLocs[0]+1 );
    hash = fullMessage.substr( 0, pipeLocs[0] );
    time = fullMessage.substr( pipeLocs[0] + 1, pipeLocs[1] );
    message = fullMessage.substr( pipeLocs[1] + 1, fullMessage.length() );

    // Verify the integrity
    if( !validHMAC( hash, stringKey, message ) ){
        std::cout << "Invalid hash.";
        return 1;
    }
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

bool initialized = false;
CryptoPP::CFB_Mode<CryptoPP::AES >::Decryption aesdecrypt;
CryptoPP::CFB_Mode<CryptoPP::AES >::Encryption aesencrypt;

int send_aes(std::string& data, std::string& response, int sock) {
    try {
        if (initialized == false) {
            byte iv[CryptoPP::AES::BLOCKSIZE] = "123456789123456";
            byte key[32] = "1234567890123456789012345678901";
            CryptoPP::SecByteBlock fukey(key, 32);
            aesdecrypt.SetKeyWithIV(fukey, fukey.size(), iv);
            aesencrypt.SetKeyWithIV(fukey, fukey.size(), iv);
        }
        byte ciphertext[data.length()];
        aesencrypt.ProcessData((byte *)data.c_str(), ciphertext, data.length());
        data.assign((char* )ciphertext, data.length());
        int err = send_socket(data, response, sock);
        if (err != 0) {
            return err;
        }
        byte plaintext[response.length()];
        aesdecrypt.ProcessData((byte *)response.c_str(), plaintext, response.length());
        response.assign((char *)plaintext, response.length());
    } catch( const CryptoPP::Exception& e) {
        return -1;
    }
    return 0;
}

std::string there_nonce("");

int send_nonce(std::string& data, std::string& response, int sock) {
    std::string my_nonce, new_data;
    if (there_nonce.length() == 0) {
        //key = establish_key();
        //prev_nonce =
        there_nonce.assign("asdasdasd");
    }
    if (data.length() != 0) {
        my_nonce.assign(readRand(32));
        new_data.assign(my_nonce);
        new_data.append("|");
        new_data.append(there_nonce);
        new_data.append("|");
        new_data.append(data);
    } else {
       my_nonce.assign(there_nonce);
       new_data.assign("");
    }
    int err = send_aes(new_data, response, sock);
    if (err != 0) {
        return err;
    }
    std::string params(response);
    size_t sep_pos = params.find('|');
    if (sep_pos == params.npos) {
        return -1;
    };
    there_nonce = params.substr(0, sep_pos);
    params.assign(params.substr(sep_pos, params.length()-sep_pos));
    sep_pos = params.find('|');
    std::string sent_my_nonce = params.substr(0, sep_pos);
    if (my_nonce.compare(sent_my_nonce) != 0) {
        return -1;
    }
    response.assign(params.substr(sep_pos, params.length()-sep_pos));
    return 0;
}


