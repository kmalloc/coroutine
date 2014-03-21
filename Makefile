all : run

run : comain.cc coroutine.cc
	g++ -g -Wall -o $@ $^

clean :
	rm run
