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


int main()
{


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

	
    CryptoPP::HexEncoder bpubSink( new CryptoPP::StringSink(bpub) );
    bankPub.DEREncode( bpubSink );
    bpubSink.MessageEnd();

	
    CryptoPP::HexEncoder aprivSink( new CryptoPP::StringSink(apriv) );
    atmPriv.DEREncode( aprivSink );
    aprivSink.MessageEnd();

	
    CryptoPP::HexEncoder apubSink( new CryptoPP::StringSink(apub) );
    atmPub.DEREncode( apubSink );
    apubSink.MessageEnd();


	FILE * bprFile;
	FILE * bpbFile, *aprFile, *apbFile;
	bprFile = fopen( "bankpriv.h", "w" );
	fprintf(bprFile, "char bank_priv[%d] = \"", bpriv.length()+1);
	fprintf(bprFile, "%s", bpriv.c_str());
	fprintf(bprFile, "\";");
	fprintf(bprFile, "\nint bank_priv_size = %d;", bpriv.length()+1);
	fclose(bprFile);

	bpbFile = fopen( "bankpub.h", "w" );
	fprintf(bpbFile, "char bank_pub[%d] = \"", bpub.length()+1);
	fprintf(bpbFile, "%s", bpub.c_str());
	fprintf(bpbFile, "\";");
	fprintf(bprFile, "\nint bank_pub_size = %d;", bpub.length()+1);
	fclose(bpbFile);

	aprFile = fopen( "atmpriv.h", "w" );
	fprintf(aprFile, "char atm_priv[%d] = \"", apriv.length()+1);
	fprintf(aprFile, "%s", apriv.c_str());
	fprintf(aprFile, "\";");
	fprintf(bprFile, "\nint atm_priv_size = %d;", apriv.length()+1);
	fclose(aprFile);

	apbFile = fopen( "atmpub.h", "w" );
	fprintf(apbFile, "char atm_pub[%d] = \"", apub.length()+1);
	fprintf(apbFile, "%s", apub.c_str());
	fprintf(apbFile, "\";");
	fprintf(bprFile, "\nint atm_pub_size = %d;", apub.length()+1);
	fclose(apbFile);

	return 0;
} 





//Set the public keys to be e = 257


// try 16384 for RSA key size
// else try 8192

//byte[2048] = "key strings"
