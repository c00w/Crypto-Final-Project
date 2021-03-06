#include "common.h"
#include "bankpub.h"
#include "atmpub.h"
#include "bankpriv.h"
#include "atmpriv.h"


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
    char * res = fgets(buffer, desiredBytes, file);
    if (res == NULL) {
        exit(1);
    }
    fclose(file);

    std::string randomString;
    randomString.assign(buffer, desiredBytes);
    return randomString;
}

std::string readRandInt()
{
    FILE* file = fopen("/dev/urandom", "r");
    int result;
    char * res = fgets((char *)&result, sizeof(int), file);
    if (res == NULL) {
        exit(1);
    }
    fclose(file);

    return std::to_string(result);
}

int send_socket(std::string data, std::string& recieved, int sock) {

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
    time.assign( std::to_string(timeNow) );

    if(applyHMAC( plain + time, key, hash ) ) return 1;

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
    if( !validHMAC( hash, stringKey, message+time ) ) return 1;
    
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

int send_message(std::string & type, std::string data, std::string&response_type, std::string& response_message, int sock, keyinfo & conn_info){

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

int send_HMAC( std::string data, std::string& response, int sock, keyinfo &conn_info ){
    // Attempt to HMAC and send the message
    std::string wrappedData;
    std::string wrappedResponse;
    std::string unwrappedResponse;
    
    if( data.length() != 0 )
        if( compileHashedMessage( data, conn_info.hmackey, wrappedData ) != 0 ){
            return -1;
        }
    
    int err = send_socket( wrappedData, wrappedResponse, sock );
    if( err != 0 ) return err;
    
    if( extractData( wrappedResponse, conn_info.hmackey, unwrappedResponse ) != 0 ){
        return -1;
    }
    
    response.assign( unwrappedResponse );
    
    return 0;
}

int send_aes(std::string data, std::string& response, int sock, keyinfo & conn_info) {
    // Attempt to initialize and send the message.
    try {
        if (conn_info.aes_initialized == false) {
            conn_info.aes_initialized = true;
            CryptoPP::SecByteBlock fukey((byte *)conn_info.aeskey.c_str(), conn_info.aeskey.length());
            conn_info.aesdecrypt.SetKeyWithIV(fukey, fukey.size(), (byte *)conn_info.aesiv.c_str());
            conn_info.aesencrypt.SetKeyWithIV(fukey, fukey.size(), (byte *)conn_info.aesiv.c_str());
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

int send_nonce(std::string data, std::string& response, int sock, keyinfo& conn_info) {
    // Noncing the message
    std::string my_nonce, new_data;
    if (conn_info.there_nonce.length() == 0) {
        int err = establish_key( data.length()==0, sock, conn_info );
        if (err != 0) {
            return err;
        }
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
    int err = send_rsa(server, mydata, their_data, csock);
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
    char key_buff[64];
    hash_key.Final((byte *)key_buff);
    conn_info.aeskey.assign(key_buff, 32);

    CryptoPP::SHA512 nonce_key;
    nonce_key.Update((byte *)key_buff, 32);
    nonce_key.Final((byte *)key_buff);
    conn_info.there_nonce.assign(std::to_string(*(int*)key_buff));

    CryptoPP::SHA512 hmac_key;
    hmac_key.Update((byte *)key_buff, 32);
    hmac_key.Final((byte *)key_buff);
    conn_info.hmackey.assign(key_buff, 16);

    CryptoPP::SHA512 aes_iv;
    aes_iv.Update((byte *)key_buff, 32);
    aes_iv.Final((byte *)key_buff);
    conn_info.aesiv.assign(key_buff, CryptoPP::AES::BLOCKSIZE);
    
    return 0;
}


int send_rsa(bool server, std::string data, std::string& recieved, int sock) {
	
	CryptoPP::AutoSeededRandomPool rng;
    //Do RSA Encryption here
   
	std::string plain, encrypted, decrypted, signature, recovered, tdecoded;
    bool result;

	CryptoPP::RSAES_OAEP_SHA_Encryptor other_pub;
	CryptoPP::RSAES_OAEP_SHA_Encryptor my_pub;
    CryptoPP::RSAES_OAEP_SHA_Decryptor my_priv;

	if(server)
	{
	    {
		    CryptoPP::HexDecoder decoder;
		    decoder.Put( (byte*)bank_priv, bank_priv_size );
		    decoder.MessageEnd();

		     my_priv.AccessKey().Load( decoder );
		 }

	    {

	        CryptoPP::HexDecoder decoder;
	        decoder.Put( (byte*)atm_pub, atm_pub_size );
	        decoder.MessageEnd();

	        other_pub.AccessKey().Load( decoder );
	    }
	    {

	        CryptoPP::HexDecoder decoder;
	        decoder.Put( (byte*)bank_pub, bank_pub_size);
	        decoder.MessageEnd();

	        my_pub.AccessKey().Load( decoder );
	    }


	 }
	 else
	 {
        {
	        CryptoPP::HexDecoder decoder;
	        decoder.Put( (byte*)atm_pub, atm_pub_size);
	        decoder.MessageEnd();

	        my_pub.AccessKey().Load( decoder );
	    }
	    {
	        CryptoPP::HexDecoder decoder;
	        decoder.Put( (byte*)bank_pub, bank_pub_size );
	        decoder.MessageEnd();

	        other_pub.AccessKey().Load( decoder );
	    }
	    {
	        CryptoPP::HexDecoder decoder;
	        decoder.Put( (byte*)atm_priv, atm_priv_size );
	        decoder.MessageEnd();

	        my_priv.AccessKey().Load( decoder );
	    }
	
	  }

	CryptoPP::RSASS<CryptoPP::PSSR, CryptoPP::SHA1>::Signer signer( my_priv );

	//Run the data through the RSA encryption
	CryptoPP::StringSource( data, true, new CryptoPP::PK_EncryptorFilter( rng, other_pub, new CryptoPP::StringSink( encrypted )));
	
	//Sign the encrypted data	
    size_t length = signer.MaxSignatureLength(); 
    byte sig_buff[length];

    signer.SignMessage(rng, (byte *)encrypted.c_str(), encrypted.length(), sig_buff);
    signature.assign((char *)sig_buff, length);

	CryptoPP::RSASS<CryptoPP::PSSR, CryptoPP::SHA1>::Verifier verifiermine( my_pub);

    data.assign(std::to_string(signature.length()));
    data.append("|");
    data.append(signature);
    data.append(encrypted);


    int err = send_socket(data, recieved, sock);
    if (err != 0) {
        return -1;
    }

    if (recieved.find("|") == recieved.npos) {
        return -1;
    }
    int sig_length = atoi(recieved.substr(0, recieved.find("|")).c_str());
    if (sig_length == 0) {
        return -1;
    }
    if (recieved.length() < sig_length + recieved.find("|")+1) {
        return -1;
    }
    recieved = recieved.substr(recieved.find("|")+1, recieved.length()-recieved.find("|")-1);
    std::string other_signature = recieved.substr(0, sig_length);
    std::string other_encrypted = recieved.substr(sig_length, recieved.length() - sig_length);
    if (other_signature.length() != (unsigned int)sig_length) {
        return -1;
    }

	CryptoPP::RSASS<CryptoPP::PSSR, CryptoPP::SHA1>::Verifier verifier( other_pub );

    result = verifier.VerifyMessage((byte *)other_encrypted.c_str(), other_encrypted.length(), (byte *)other_signature.c_str(), other_signature.length());
    if (result == false)
	{
        return -1;//Signature failed if it returns -1
    }

	CryptoPP::StringSource( other_encrypted, true, new CryptoPP::PK_DecryptorFilter( rng, my_priv, new CryptoPP::StringSink( decrypted )));

    recieved.assign(decrypted);
    return 0;
}
