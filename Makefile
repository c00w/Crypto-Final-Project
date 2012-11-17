FLAGS = -m32 -g -Wall -std=c++0x
COMMON_FILES = common.cpp

all: atm bank proxy
	rm bank*.h
	rm atm*.h

atm: atm.cpp common.cpp keygen
	g++ atm.cpp $(COMMON_FILES) $(FLAGS) -o atm -lcrypto++
bank: bank.cpp common.cpp keygen
	g++ bank.cpp $(COMMON_FILES) $(FLAGS) -o bank -lpthread -lcrypto++
proxy:
	g++ proxy.cpp $(FLAGS) -o proxy -lpthread 
keygen:
	g++ keygen.cpp $(FLAGS) -o keygen -lcrypto++ 
	./keygen
	rm keygen


bankpub.h: keygen
bankpriv.h: keygen
atmpub.h: keygen
atmpriv.h: keygen

clean:
	rm atm bank proxy keygen bankpub.h bankpriv.h atmpub.h atmpriv.h
