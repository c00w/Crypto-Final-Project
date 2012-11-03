FLAGS = -m32 -g 

all:
	g++ atm.cpp $(FLAGS) -o atm -lcrypto++
	g++ bank.cpp $(FLAGS) -o bank -lpthread -lcrypto++
	g++ proxy.cpp $(FLAGS) -o proxy -lpthread 

clean:
	rm atm bank proxy
