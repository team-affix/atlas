all:
	g++ -std=c++20 -DDEBUG ./test/main.cpp ./cpp/* -o main

algo:
	g++ -std=c++20 ./test/a01_main.cpp ./cpp/* -o a01_main

clean:
	rm -f main
