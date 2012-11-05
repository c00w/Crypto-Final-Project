FLAGS = -m32 -g -Wall
COMMON_FILES = common.cpp

all:
	g++ atm.cpp $(COMMON_FILES) $(FLAGS) -o atm -lcrypto++
	g++ bank.cpp $(COMMON_FILES) $(FLAGS) -o bank -lpthread -lcrypto++
	g++ proxy.cpp $(FLAGS) -o proxy -lpthread 

clean:
	rm atm bank proxy
