all: ttt ttts

ttt: ttt.c
	gcc -fsanitize=address -Wall -Wextra -o ttt ttt.c

ttts: ttts.c
	gcc -pthread ttts.c -o ttts

