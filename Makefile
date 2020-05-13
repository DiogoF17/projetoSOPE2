exe:
	gcc -o Q2 Q2.c -pthread -lrt -Wall
	gcc -o U2 U2.c -pthread -lrt -Wall
clean:
	rm Q2 U2
