all: ttt ttts

ttt: ttt.c
	gcc -g -fsanitize=address -Wall -Wextra -o ttt ttt.c

ttts: ttts.c protocol.c
	gcc -g -fsanitize=address -pthread ttts.c protocol.c -o ttts

protocol.o: protocol.c protocol.h
	gcc -c protocol.c

clean:
	rm -f ttt ttts protocol.o

