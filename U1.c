#include <stdio.h> 
#include <stdlib.h> 
#include <fcntl.h> 
#include <unistd.h>
#include <sys/stat.h>
#include <string.h>
#include <pthread.h>

int isZero(char *string){
    for(int i = 0; i < strlen(string); i++){
        if(string[i] != 48)
            return 0;
    }
    return 1;
}

int validNumber(char *string){
    if(string == NULL)
        return 0;
    if(strcmp(string, "") == 0 || isZero(string) == 1)
        return 0;
    for(int i = 0; i < strlen(string); i++){
        if (string[i] < 48 || string[i] > 57)
            return 0;
    }
    return 1;
}

void validFormat(int argc, char *argv[]){
    if(argc != 4 || argc % 2 == 1){
        printf("Invalid Format!\nFormat: Un <-t nsecs> fifoname\n");
        exit(1);
    }
    if(strcmp(argv[1], "-t") != 0){
        printf("Invalid Format!\nFormat: Un <-t nsecs> fifoname\n");
        exit(1);
    }
    if(validNumber(argv[2]) != 1 || validNumber(argv[3]) == 1){
        printf("Invalid Format!\nFormat: Un <-t nsecs> fifoname\n");
        exit(1);
    }
}

void *thread_func(void *arg){
    printf("Lanca %d\n", *(int *)arg);
    free(arg);
    return NULL;
}

int main(int argc, char *argv[]){

    validFormat(argc, argv);

    int identificador = 0;
    double num = 0, numSecs = atoi(argv[2]) * 1000;
    pthread_t tid;

    while(num < numSecs){
        usleep(50000); //sleep de 50 milisegundo
        //printf("Lanca %d\n", identificador);
        void * arg = malloc (sizeof(int));
        *(int *) arg = identificador;
        pthread_create(&tid, NULL, thread_func, arg);
        num+=50;
        identificador++;
    }

    pthread_exit(0);

}