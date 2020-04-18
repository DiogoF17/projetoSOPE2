#include <stdio.h> 
#include <stdlib.h> 
#include <fcntl.h> 
#include <unistd.h>
#include <sys/stat.h>
#include <string.h>
#include <pthread.h>
#include <time.h>
#include <signal.h>

int created = 0;  //diz-nos se o fifo ja foi criado alguma vez, pois pode ja ter sido destruido e nesse caso nao podemos enviar mais pedidos
int end = 0;
time_t begin;

struct ParametrosParaFifo{
    int i; //identificador de cada pedido
    int pid;
    int tid;
    int dur; //milisegundos
    int p1;
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
    int escritor = -1, leitor;
    
    do{
        escritor = open((*(struct ParametrosParaThread *)arg).fifo_ped, O_WRONLY);
    }while(escritor == -1 && created == 0);

    if(escritor != -1){
        created = 1;

        struct ParametrosParaFifo argFifo;

        argFifo.i = (*(struct ParametrosParaThread *)arg).identificador;
        argFifo.pid = getpid();
        argFifo.tid = pthread_self();
        argFifo.dur = (rand() % 5) + 1;
        argFifo.p1 = -1;

        printf("%ld ; %d ; %d ; %d ; %d ; %d ; IWANT\n",
                time(NULL) - begin, argFifo.i,
                argFifo.pid, argFifo.tid,
                argFifo.dur, argFifo.p1);
        
        write(escritor, &argFifo, sizeof(struct ParametrosParaFifo));

        close(escritor);

        char file[100];
        sprintf(file, "/tmp/%d.%d", argFifo.pid, argFifo.tid);
        mkfifo(file, 0660);
        leitor = open(file, O_RDONLY);
        read(leitor, &argFifo, sizeof(struct ParametrosParaFifo));
        if(argFifo.p1 == -1){
            printf("%ld ; %d ; %d ; %d ; %d ; %d ; CLOSD\n",
                    time(NULL) - begin, argFifo.i,
                    argFifo.pid, argFifo.tid,
                    argFifo.dur, argFifo.p1);
        }
        else{
             printf("%ld ; %d ; %d ; %d ; %d ; %d ; IAMIN\n",
                    time(NULL) - begin, argFifo.i,
                    argFifo.pid, argFifo.tid,
                    argFifo.dur, argFifo.p1);
        }
        
        close(leitor);
        unlink(file);
    }

    free(arg);

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

void signalHandler(int signal){
    end = 1;
}

int main(int argc, char *argv[]){

    begin = time(NULL);

    validFormat(argc, argv);

    signal(SIGALRM, signalHandler);
    alarm(atoi(argv[2]));

    srand(time(NULL));

    int intervalo = (rand() % 100) + 1;

    int identificador = 1;
    pthread_t tid;

    char dirFifoPed[100], fifo_ped[100];
    find_fifo_name(argv, fifo_ped);
    sprintf(dirFifoPed, "/tmp/%s", fifo_ped);

    while(!end){
        usleep(intervalo * 1000);
        
        void * arg = malloc (sizeof(struct ParametrosParaThread));
        (*(struct ParametrosParaThread *) arg).identificador = identificador;
        strcpy((*(struct ParametrosParaThread *) arg).fifo_ped, dirFifoPed);

        pthread_create(&tid, NULL, thread_func, arg);
        identificador++;
    }

    pthread_exit(0);

}