exe:
	gcc -o Q2 Q2.c -pthread -Wall
	gcc -o U2 U2.c -pthread -Wall
clean:
	rm Q2 U2
