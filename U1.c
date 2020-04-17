#include <stdio.h> 
#include <stdlib.h> 
#include <fcntl.h> 
#include <unistd.h>
#include <sys/stat.h>
#include <string.h>
#include <pthread.h>
#include <time.h>

struct ParametrosParaFifo{
    int identificador; //identificador de cada pedido
    int tempo; //milisegundos
    char fifo_resp[100]; //nome do fifo que vai recolher a resposta
};

struct ParametrosParaThread{
    int identificador; //identificador de cada thread na linha de lanacamento
    char fifo_ped[100]; //nome do fifo para o qual metemos os parametros
};

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
    int escritor;

    int time = (rand() % 5) + 1;

    char fifo_resp[100];
    sprintf(fifo_resp, "/tmp/fifo_resp_%ld", pthread_self());

    struct ParametrosParaFifo argFifo;

    argFifo.identificador = (*(struct ParametrosParaThread *)arg).identificador;
    argFifo.tempo = time;
    strcpy(argFifo.fifo_resp, fifo_resp);
    
    do{
        escritor = open((*(struct ParametrosParaThread *)arg).fifo_ped, O_WRONLY);
    }while(escritor == -1);

    //printf("Lanca %d -- %d -- %s\n", argFifo.identificador, argFifo.tempo, argFifo.fifo_resp);
    write(escritor, &argFifo, sizeof(struct ParametrosParaFifo));
    
    free(arg);
    close(escritor);

    return NULL;
}

void find_fifo_name(char *argv[], char *string){
    int i = 3;

    while(argv[i] != NULL){
        if(strcmp(argv[i], "-l") == 0 || strcmp(argv[i], "-n") == 0 || validNumber(argv[i]) == 1)
            continue;
        else{
            strcpy(string, argv[i]);
            break;
        }
    }
}

int main(int argc, char *argv[]){

    validFormat(argc, argv);

    srand(time(NULL));

    int identificador = 1;
    double num = 0, numSecs = atoi(argv[2]) * 1000;
    pthread_t tid;

    char dirFifoPed[100], fifo_ped[100];
    find_fifo_name(argv, fifo_ped);
    sprintf(dirFifoPed, "/tmp/%s", fifo_ped);

    while(num < numSecs){
        usleep(50000); //sleep de 50 milisegundo
        
        void * arg = malloc (sizeof(struct ParametrosParaThread));
        (*(struct ParametrosParaThread *) arg).identificador = identificador;
        strcpy((*(struct ParametrosParaThread *) arg).fifo_ped, dirFifoPed);

        pthread_create(&tid, NULL, thread_func, arg);
        num+=50;
        identificador++;
    }

    void * arg = malloc (sizeof(struct ParametrosParaThread));
    (*(struct ParametrosParaThread *) arg).identificador = 0;
    strcpy((*(struct ParametrosParaThread *) arg).fifo_ped, dirFifoPed);

    pthread_create(&tid, NULL, thread_func, arg);


    pthread_exit(0);

}