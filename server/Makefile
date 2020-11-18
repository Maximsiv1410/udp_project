std = -std=c++17

default:
	g++ main.cpp -o server.out $(std) -lboost_system -lpthread

santhread:
	g++ main.cpp -o serversan.out $(std) -lboost_system -lpthread -fsanitize=thread

sanaddr:
	g++ main.cpp -o serversan.out $(std) -lboost_system -lpthread -fsanitize=address

opt:
	g++ main.cpp -o server.out $(std) -lboost_system -lpthread -O2  

debug:
	g++ main.cpp -o server.out $(std) -lboost_system -lpthread -g3

clean:
	rm -rf *.out