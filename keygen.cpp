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

//These are just straight from the cryptopp library
void SavePrivateKey(const std::string& filename, const CryptoPP::RSA::PrivateKey& key);
void SavePublicKey(const std::string& filename, const CryptoPP::RSA::PublicKey& key);
void Save(const std::string& filename, const CryptoPP::BufferedTransformation& bt);
void LoadPrivateKey(const std::string& filename, CryptoPP::RSA::PrivateKey& key);
void LoadPublicKey(const std::string& filename, CryptoPP::RSA::PublicKey& key);
void Load(const std::string& filename, CryptoPP::BufferedTransformation& bt);

int main()
{
	/*
	AutoSeededRandomPool rng;

	InvertibleRSAFunction keys;
	keys.GenerateRandomWithKeySize(rng, 2048);
	std::cout << keys.GetValueNames();
	std::cout << "\n\n\n";

	if(!keys.Validate(rng, 3))
	{
		std::cerr << "RSA private key validation failed";
    	return 1;
	}

	
	RSA::PrivateKey privKey;
	privKey.GenerateRandomWithKeySize(rng, 2048);
	
	if(!privKey.Validate(rng, 3))
		{
			std::cerr << "RSA private key validation failed";
			return 1;
		}


	RSAFunction pubKey(privKey);


	//Pretty sure that cout is not properly displaying these, they are getting casted as ints?
	std::cout << "n is: " << privKey.GetModulus() << std::cout << "\n\n\n";
	std::cout << "Private Exponent is: " << privKey.GetPrivateExponent() 	<< std::cout << "\n\n\n";
	std::cout << "Public Exponent is: " << privKey.GetPublicExponent() << 	std::cout << "\n\n\n";
	//std::cout << privKey.GetModulus() << std::endl;
	//std::cout << privKey.GetModulus() << std::endl;	

	std::cout << "*************************************************************\n\n";


	std::cout << "n is: " << keys.GetModulus() << std::cout << "\n\n\n";
	std::cout << "Private Exponent is: " << keys.GetPrivateExponent() << 	std::cout << "\n\n\n";
	std::cout << "Public Exponent is: " << keys.GetPublicExponent() << 		std::cout << "\n\n\n";

	std::cout << "Public key e is: " << pubKey.GetPublicExponent() << std::endl;
	std::cout << "Public Key n is: " << pubKey.GetModulus() << std::endl;

*/


	CryptoPP::AutoSeededRandomPool rng;
/*
    // Create a private RSA key and write it to a file using DER.
    CryptoPP::RSAES_OAEP_SHA_Decryptor priv( rng, 4096 );
    CryptoPP::TransparentFilter privFile( new CryptoPP::FileSink("rsakey.der") );
    priv.DEREncode( privFile );
    privFile.MessageEnd();

    // Create a private RSA key and write it to a string using DER (also write to a file to check it with OpenSSL).
    std::string the_key;
    CryptoPP::RSAES_OAEP_SHA_Decryptor pri( rng, 2048 );
    CryptoPP::TransparentFilter privSink( new CryptoPP::StringSink(the_key) );
    pri.DEREncode( privSink );
    privSink.MessageEnd();

    std::ofstream file ( "key.der", std::ios::out | std::ios::binary );
    file.write( the_key.data(), the_key.size() );
    file.close();

	CryptoPP::RSAES_OAEP_SHA_Encryptor pub( priv );
	CryptoPP::TransparentFilter pubFile( new CryptoPP::FileSink("pubkey.pb"));
	pub.DEREncode( pubFile );
	pubFile.MessageEnd();

	std::string s;
	CryptoPP::FileSource( "pubkey.pb", true, new StringSink( s ));
	std::cout << s << std::endl;
*/

    // Example Encryption & Decryption
    CryptoPP::InvertibleRSAFunction params;
    params.GenerateRandomWithKeySize( rng, 1536 );

    std::string plain = "RSA Encryption", cipher, decrypted_data;
	
	
	

    CryptoPP::RSA::PrivateKey privateKey( params );
    CryptoPP::RSA::PublicKey publicKey( params );

	SavePrivateKey( "rsaPrivKey.key", privateKey );
	SavePublicKey( "rsaPubKey.key", publicKey );


    CryptoPP::RSAES_OAEP_SHA_Encryptor e( publicKey );
    CryptoPP::StringSource( plain, true, new CryptoPP::PK_EncryptorFilter( rng, e, new CryptoPP::StringSink( cipher )));

    CryptoPP::RSAES_OAEP_SHA_Decryptor d( privateKey );
    CryptoPP::StringSource( cipher, true, new CryptoPP::PK_DecryptorFilter( rng, d, new CryptoPP::StringSink( decrypted_data )));

    assert( plain == decrypted_data );







	return 0;
}




void SavePublicKey(const std::string& filename, const CryptoPP::RSA::PublicKey& key)
{
	// http://www.cryptopp.com/docs/ref/class_byte_queue.html
	CryptoPP::ByteQueue queue;
	key.Save(queue);

	Save(filename, queue);
}

void SavePrivateKey(const std::string& filename, const CryptoPP::RSA::PrivateKey& key)
{
	// http://www.cryptopp.com/docs/ref/class_byte_queue.html
	CryptoPP::ByteQueue queue;
	key.Save(queue);

	Save(filename, queue);
}

void LoadPrivateKey(const std::string& filename, CryptoPP::RSA::PrivateKey& key)
{
	// http://www.cryptopp.com/docs/ref/class_byte_queue.html
	CryptoPP::ByteQueue queue;

	Load(filename, queue);
	key.Load(queue);	
}

void LoadPublicKey(const std::string& filename, CryptoPP::RSA::PublicKey& key)
{
	// http://www.cryptopp.com/docs/ref/class_byte_queue.html
	CryptoPP::ByteQueue queue;

	Load(filename, queue);
	key.Load(queue);	
}



void Save(const std::string& filename, const CryptoPP::BufferedTransformation& bt)
{
	// http://www.cryptopp.com/docs/ref/class_file_sink.html
	CryptoPP::FileSink file(filename.c_str());

	bt.CopyTo(file);
	file.MessageEnd();
}

void Load(const std::string& filename, CryptoPP::BufferedTransformation& bt)
{
	// http://www.cryptopp.com/docs/ref/class_file_source.html
	CryptoPP::FileSource file(filename.c_str(), true /*pumpAll*/);

	file.TransferTo(bt);
	bt.MessageEnd();
}







//Set the public keys to be e = 257


// try 16384 for RSA key size
// else try 8192


/* The commented out zone






*/
