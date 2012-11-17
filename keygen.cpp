//Key Generation
//Compile with: g++ keygen.cpp -m32 -g -Wall -o keygen -lcrypto++



#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <stdlib.h>
#include <stdio.h>
#include <pthread.h>
#include <string.h>

#include "cryptopp/sha.h"

#include <iostream>
#include "cryptopp/rsa.h"
#include <string>

#include "cryptopp/randpool.h"
#include "cryptopp/cryptlib.h"
#include "cryptopp/config.h"
#include "cryptopp/modes.h"
#include "cryptopp/files.h"
#include "cryptopp/rng.h"
#include "cryptopp/aes.h"
#include "cryptopp/fips140.h"
#include "cryptopp/osrng.h"
#include "cryptopp/base64.h"
#include "cryptopp/queue.h"
#include "cryptopp/hex.h"


int bank()
{

	CryptoPP::AutoSeededRandomPool rng;

	FILE* randFile = fopen("/dev/random", "r");
    char buffer[16];
    fgets(buffer, 16, randFile);//Should be reading 128 bits
    fclose(randFile);
    
    std::string bankRandData;
    bankRandData.assign(buffer, 16);
/////////// Imaginary Bank /////////////////////////
	std::string plain, cipher, decrypted;
	plain = "hello";
    CryptoPP::RSA::PrivateKey priv;
	priv.GenerateRandomWithKeySize( rng, 2048 );
	CryptoPP::RSA::PublicKey pub( priv );
	CryptoPP::RSASSA_PKCS1v15_SHA_Signer signer(priv);
	CryptoPP::RSASSA_PKCS1v15_SHA_Verifier verifier(signer);

	CryptoPP::RSAES_OAEP_SHA_Encryptor e( pub );
	CryptoPP::StringSource( bankRandData, true, new CryptoPP::PK_EncryptorFilter( rng, e, new CryptoPP::StringSink( cipher )));
	
	CryptoPP::RSAES_OAEP_SHA_Decryptor d( priv );
	CryptoPP::StringSource( cipher, true, new CryptoPP::PK_DecryptorFilter( rng, d, new CryptoPP::StringSink( decrypted )));

	std::cout << plain << std::endl;




	std::string signature;

	CryptoPP::StringSource(bankRandData, true, new CryptoPP::SignerFilter(rng, signer,
		    new CryptoPP::StringSink(signature)
	   ) 
	);

	try{
	CryptoPP::StringSource(bankRandData+signature, true, new CryptoPP::SignatureVerificationFilter(
		    verifier, NULL, CryptoPP::SignatureVerificationFilter::THROW_EXCEPTION
	   ) 
	);
	} catch( const CryptoPP::Exception& e) {
        return -1;//Signature failed if it returns -1
    }

	std::cout << "Verified signature on message" << std::endl;

//////////// End of Bank //////////////////////////////

	return 0;
}


int atm()
{
	
	CryptoPP::AutoSeededRandomPool rng;

	FILE* randFile = fopen("/dev/random", "r");
    char buffer[16];
    fgets(buffer, 16, randFile);//Should be reading 128 bits
    fclose(randFile);
    
    std::string atmRandData;
    atmRandData.assign(buffer, 16);
//////////// Imaginary ATM /////////////////////////////


	std::string plain, cipher, decrypted;
	plain = "hello";
    CryptoPP::RSA::PrivateKey priv;
	priv.GenerateRandomWithKeySize( rng, 2048 );
	CryptoPP::RSA::PublicKey pub( priv );
	CryptoPP::RSASSA_PKCS1v15_SHA_Signer signer(priv);
	CryptoPP::RSASSA_PKCS1v15_SHA_Verifier verifier(signer);

	CryptoPP::RSAES_OAEP_SHA_Encryptor e( pub );
	CryptoPP::StringSource( atmRandData, true, new CryptoPP::PK_EncryptorFilter( rng, e, new CryptoPP::StringSink( cipher )));
	
	CryptoPP::RSAES_OAEP_SHA_Decryptor d( priv );
	CryptoPP::StringSource( cipher, true, new CryptoPP::PK_DecryptorFilter( rng, d, new CryptoPP::StringSink( decrypted )));

	std::cout << plain << std::endl;




	std::string signature;

	CryptoPP::StringSource(atmRandData, true, new CryptoPP::SignerFilter(rng, signer,
		    new CryptoPP::StringSink(signature)
	   ) 
	);

	try{
	CryptoPP::StringSource(atmRandData+signature, true, new CryptoPP::SignatureVerificationFilter(
		    verifier, NULL, CryptoPP::SignatureVerificationFilter::THROW_EXCEPTION
	   ) 
	);
	} catch( const CryptoPP::Exception& e) {
        return -1;//Signature failed if it returns -1
    }

	std::cout << "Verified signature on message" << std::endl;

/////////// End of ATM //////////////////////////////////	


	return 0;
}

int main()
{
	//CryptoPP::AutoSeededRandomPool rng;
/*
    // Create a private bank key, encode it with DER (X.507)
    CryptoPP::RSAES_OAEP_SHA_Decryptor bpriv( rng, 4096 );
    CryptoPP::TransparentFilter bprivFile( new CryptoPP::FileSink("bankpriv.der",true) );
    bpriv.DEREncode( bprivFile );
    bprivFile.MessageEnd();

	// Create banks corresponding public key
	CryptoPP::RSAES_OAEP_SHA_Encryptor bpub( bpriv );
	CryptoPP::TransparentFilter bpubFile( new CryptoPP::FileSink("bankpub.der", true));
	bpub.DEREncode( bpubFile );
	bpubFile.MessageEnd();

    CryptoPP::RSAES_OAEP_SHA_Decryptor apriv( rng, 4096 );
    CryptoPP::TransparentFilter aprivFile( new CryptoPP::FileSink("atmpriv.der",true) );
    apriv.DEREncode( aprivFile );
    aprivFile.MessageEnd();


	CryptoPP::RSAES_OAEP_SHA_Encryptor apub( apriv );
	CryptoPP::TransparentFilter apubFile( new CryptoPP::FileSink("atmpub.der", true));
	apub.DEREncode( apubFile );
	apubFile.MessageEnd();
*/

	CryptoPP::AutoSeededRandomPool rng2;

	CryptoPP::RSA::PrivateKey bankPriv;
	bankPriv.GenerateRandomWithKeySize( rng2, 2048 );
	CryptoPP::RSA::PublicKey bankPub( bankPriv );

	CryptoPP::RSA::PrivateKey atmPriv;
	atmPriv.GenerateRandomWithKeySize( rng2, 2048 );
	CryptoPP::RSA::PublicKey atmPub( atmPriv );

	std::string bpriv,bpub,apriv,apub;
    //CryptoPP::TransparentFilter bprivSink( new CryptoPP::StringSink(bpriv) );
    CryptoPP::HexEncoder bprivSink(new CryptoPP::StringSink(bpriv));    
	bankPriv.DEREncode( bprivSink );
    bprivSink.MessageEnd();

	
    CryptoPP::HexEncoder bpubSink( new CryptoPP::StringSink(bpriv) );
    bankPub.DEREncode( bpubSink );
    bpubSink.MessageEnd();

	
    CryptoPP::HexEncoder aprivSink( new CryptoPP::StringSink(bpriv) );
    atmPriv.DEREncode( aprivSink );
    aprivSink.MessageEnd();

	
    CryptoPP::HexEncoder apubSink( new CryptoPP::StringSink(bpriv) );
    atmPub.DEREncode( apubSink );
    apubSink.MessageEnd();


	FILE * bprFile;
	FILE * bpbFile, *aprFile, *apbFile;
	bprFile = fopen( "bankpriv.h", "w" );
	fprintf(bprFile, "char bank_priv[%d] = \"", bpriv.length());
	fprintf(bprFile, "%s", bpriv.c_str());
	fprintf(bprFile, "\";");
	fclose(bprFile);

	bpbFile = fopen( "bankpub.h", "w" );
	fprintf(bpbFile, "char bank_pub[%d] = \"", bpub.length());
	fprintf(bpbFile, "%s", bpub.c_str());
	fprintf(bpbFile, "\";");
	fclose(bpbFile);

	aprFile = fopen( "atmpriv.h", "w" );
	fprintf(aprFile, "char atm_priv[%d] = \"", apriv.length());
	fprintf(aprFile, "%s", apriv.c_str());
	fprintf(aprFile, "\";");
	fclose(aprFile);

	apbFile = fopen( "atmpub.h", "w" );
	fprintf(apbFile, "char atm_pub[%d] = \"", apub.length());
	fprintf(apbFile, "%s", apub.c_str());
	fprintf(apbFile, "\";");
	fclose(apbFile);

	















	//bank();
	//atm();

	return 0;
} 





//Set the public keys to be e = 257


// try 16384 for RSA key size
// else try 8192

//byte[2048] = "key strings"
