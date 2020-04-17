#include <stdio.h> 
#include <stdlib.h> 
#include <fcntl.h> 
#include <unistd.h>
#include <sys/stat.h>
#include <string.h>

struct ParametrosParaFifo{
    int identificador; //identificador de cada pedido
    int tempo; //milisegundos
    char fifo_resp[100]; //nome do fifo que vai recolher a resposta
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

int optionalArgs(char *argv[]){
    int i = 3;
    while(argv[i] != NULL){
        if(strcmp(argv[i], "-l") == 0 || strcmp(argv[i], "-n") == 0){
            if(validNumber(argv[i-1]) != 1 || validNumber(argv[i+1]) != 1)
                return 0;
        }
        else if(validNumber(argv[i]) == 1){
            if(validNumber(argv[i-1]) == 1 || validNumber(argv[i+1]) == 1 || argv[i+1] == NULL)
                return 0;
        }
        else{
            if(validNumber(argv[i-1]) != 1 || argv[i+1] != NULL)
                return 0;
        }
        i++;
    }
    return 1;
}

void validFormat(int argc, char *argv[]){
    if(argc < 4 || argc > 8 || argc % 2 != 0){
        printf("Invalid Format!\nFormat: Qn <-t nsecs> [-l nplaces] [-n nthreads] fifoname\n");
        exit(1);
    }
    if(strcmp(argv[1], "-t") != 0){
        printf("Invalid Format!\nFormat: Qn <-t nsecs> [-l nplaces] [-n nthreads] fifoname\n");
        exit(1);
    }
    if(validNumber(argv[2]) != 1){
        printf("Invalid Format!\nFormat: Qn <-t nsecs> [-l nplaces] [-n nthreads] fifoname\n");
        exit(1);
    }
    
    //Usar apenas na primeira parte
    if(validNumber(argv[3]) == 1){
        printf("Invalid Format!\nFormat: Qn <-t nsecs> [-l nplaces] [-n nthreads] fifoname\n");
        exit(1);
    }
    
    /*
    //Nao e suposto estar na primeira parte
    if(optionalArgs(argv) != 1){
        printf("Invalid Format!\nFormat: Qn <-t nsecs> [-l nplaces] [-n nthreads] fifoname\n");
        exit(1);
    }
    */
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
/*
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
}*/

int main(int argc, char *argv[]){

    pthread_t tid;

    validFormat(argc, argv);

    char dirFifoPed[100], fifo_ped[100];
    find_fifo_name(argv, fifo_ped);
    sprintf(dirFifoPed, "/tmp/%s", fifo_ped);

    mkfifo(dirFifoPed, 0660);
    int leitor = open(dirFifoPed, O_RDONLY);

    struct ParametrosParaFifo argFifo;

    int numLidos;

    do{

        numLidos = read(leitor, &argFifo, sizeof(struct ParametrosParaFifo));

        /*void *arg = malloc(sizeof(struct ParametrosParaFifo));
        *(struct ParametrosParaFifo *)arg = argFifo;
    
        pthread_create(&tid, NULL, thread_func, arg);*/

        if(argFifo.identificador != 0)
            printf("Lanca %d -- %d -- %s\n", argFifo.identificador, argFifo.tempo, argFifo.fifo_resp);

    } while(numLidos > 0);

    close(leitor);
    unlink(dirFifoPed);

    return 0;;

}