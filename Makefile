FLAGS = -m32 -g -Wall
COMMON_FILES = common.cpp

all: atm bank proxy

atm:
	g++ atm.cpp $(COMMON_FILES) $(FLAGS) -o atm -lcrypto++
bank:
	g++ bank.cpp $(COMMON_FILES) $(FLAGS) -o bank -lpthread -lcrypto++
proxy:
	g++ proxy.cpp $(FLAGS) -o proxy -lpthread 

clean:
	rm atm bank proxy
