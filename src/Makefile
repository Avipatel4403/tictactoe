all: ttt ttts

ttt: ttt.c
	gcc -fsanitize=address -Wall -Wextra -o ttt ttt.c

ttts: ttts.c protocol.c
	gcc -fsanitize=address -pthread ttts.c protocol.c -o ttts

protocol.o: protocol.c protocol.h
	gcc -c protocol.c

clean:
	rm -f ttt ttts protocol.o

