FLAGS = -m32 -g -Wall -std=c++0x
COMMON_FILES = common.cpp

all: atm bank proxy

atm: atm.cpp common.cpp
	g++ atm.cpp $(COMMON_FILES) $(FLAGS) -o atm -lcrypto++
bank: bank.cpp common.cpp
	g++ bank.cpp $(COMMON_FILES) $(FLAGS) -o bank -lpthread -lcrypto++
proxy:
	g++ proxy.cpp $(FLAGS) -o proxy -lpthread 

clean:
	rm atm bank proxy
