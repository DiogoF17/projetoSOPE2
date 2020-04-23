exe:
	gcc -o Q1 Q1.c -pthread -Wall
	gcc -o U1 U1.c -pthread -Wall
clean:
	rm Q1 U1