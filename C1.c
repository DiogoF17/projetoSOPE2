#include <stdio.h> 
#include <stdlib.h> 
#include <fcntl.h> 
#include <unistd.h>
#include <sys/stat.h>
#include <string.h>
#include <pthread.h>
#include <signal.h>

int end;
time_t begin;
int closed = 0;

struct ParametrosParaFifo{
    int i; //identificador de cada pedido
    int pid;
    int tid;
    int dur; //milisegundos
    int p1;
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

void *thread_func(void *arg){
    int escritor;

    printf("%ld ; %d ; %d ; %d ; %d ; %d ; RECVD\n",
        time(NULL) - begin, (* (struct ParametrosParaFifo *)arg).i,
        (* (struct ParametrosParaFifo *)arg).pid, (* (struct ParametrosParaFifo *)arg).tid,
        (* (struct ParametrosParaFifo *)arg).dur, (* (struct ParametrosParaFifo *)arg).p1);

    if(closed){
        printf("%ld ; %d ; %d ; %d ; %d ; %d ; 2LATE\n",
        time(NULL) - begin, (* (struct ParametrosParaFifo *)arg).i,
        (* (struct ParametrosParaFifo *)arg).pid, (* (struct ParametrosParaFifo *)arg).tid,
        (* (struct ParametrosParaFifo *)arg).dur, (* (struct ParametrosParaFifo *)arg).p1);
    }
    else{
        printf("%ld ; %d ; %d ; %d ; %d ; %d ; ENTER\n",
        time(NULL) - begin, (* (struct ParametrosParaFifo *)arg).i,
        (* (struct ParametrosParaFifo *)arg).pid, (* (struct ParametrosParaFifo *)arg).tid,
        (* (struct ParametrosParaFifo *)arg).dur, (* (struct ParametrosParaFifo *)arg).p1);
    }

    char file[100];
    sprintf(file, "/tmp/%d.%d", (*(struct ParametrosParaFifo*) arg).pid, (*(struct ParametrosParaFifo*) arg).tid);

    do{
        escritor = open(file, O_WRONLY);
    } while(escritor == -1);

   // char string[100];
    //sprintf(string, "Resposta Pedido %d -- %d\n", )
    if(closed)
        write(escritor, (struct ParametrosParaFifo *)arg, sizeof(struct ParametrosParaFifo));
    else{
        (*(struct ParametrosParaFifo *)arg).p1 = 2;
        write(escritor, (struct ParametrosParaFifo *)arg, sizeof(struct ParametrosParaFifo));
    }

    //printf("Lanca %d -- %d -- %s\n", (*(struct ParametrosParaFifo*) arg).identificador, (*(struct ParametrosParaFifo*) arg).tempo, (*(struct ParametrosParaFifo*) arg).fifo_resp);

    //do{

    //}
    close(escritor);
    return NULL;
}

void signalHandler(int signal){
    end = 1;
}

int main(int argc, char *argv[]){

    begin = time(NULL);

    pthread_t tid;

    validFormat(argc, argv);

    signal(SIGALRM, signalHandler);
    alarm(atoi(argv[2]));

    char dirFifoPed[100], fifo_ped[100];
    find_fifo_name(argv, fifo_ped);
    sprintf(dirFifoPed, "/tmp/%s", fifo_ped);

    mkfifo(dirFifoPed, 0660);
    int leitor = open(dirFifoPed, O_RDONLY);

    struct ParametrosParaFifo argFifo;

    int numLidos;

    do{

        numLidos = read(leitor, &argFifo, sizeof(struct ParametrosParaFifo));

        void *arg = malloc(sizeof(struct ParametrosParaFifo));
        *(struct ParametrosParaFifo *)arg = argFifo;

        if(numLidos != 0)
            pthread_create(&tid, NULL, thread_func, arg);

    } while(!end);

    closed = 1;

    close(leitor);
    unlink(dirFifoPed);

    //printf("Bathroom is no longer in service!\n");

    pthread_exit(0);

}