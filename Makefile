FLAGS = -m32 -g

all:
	g++ atm.cpp $(FLAGS) -o atm
	g++ bank.cpp $(FLAGS) -o bank -lpthread
	g++ proxy.cpp $(FLAGS) -o proxy -lpthread
