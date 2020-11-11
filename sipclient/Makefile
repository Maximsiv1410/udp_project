default:
	g++ main.cpp -o client.out -lboost_system -lpthread

san:
	g++ main.cpp -o clientsan.out -lboost_system -lpthread -fsanitize=thread

opt:
	g++ main.cpp -o client.out -lboost_system -lpthread -O2

debug:
	g++ main.cpp -o client.out -lboost_system -lpthread -g3

clean:
	rm -rf *.out