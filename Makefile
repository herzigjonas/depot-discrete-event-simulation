all:
	g++ -Wall -pedantic -std=c++11 main.cpp -o main -lsimlib -lm
run:
	./main
clean:
	rm -f main