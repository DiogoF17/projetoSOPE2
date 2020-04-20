#include <stdio.h> 
#include <stdlib.h> 
#include <fcntl.h> 
#include <unistd.h>
#include <sys/stat.h>
#include <string.h>
#include <pthread.h>
#include <signal.h>

int end = 0; //variavel que permite o ciclo
time_t begin; //instante inicial do programa
int closed = 0; //diz-nos se a casa de banho fechou

struct ParametrosParaFifo{
    int i; //identificador de cada pedido
    int pid; //pid do processo
    int tid; //tid do processo
    int dur; //milisegundos
    int p1; //nº da casa de banho que foi atribuida
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

    //acusa a rececao do pedido feita pelo cliente
    printf("%ld ; %d ; %d ; %d ; %d ; %d ; RECVD\n",
        time(NULL) - begin, (* (struct ParametrosParaFifo *)arg).i,
        (* (struct ParametrosParaFifo *)arg).pid, (* (struct ParametrosParaFifo *)arg).tid,
        (* (struct ParametrosParaFifo *)arg).dur, (* (struct ParametrosParaFifo *)arg).p1);

    //diz que o pedido chegou tarde de mais
    if(closed){
        printf("%ld ; %d ; %d ; %d ; %d ; %d ; 2LATE\n",
        time(NULL) - begin, (* (struct ParametrosParaFifo *)arg).i,
        (* (struct ParametrosParaFifo *)arg).pid, (* (struct ParametrosParaFifo *)arg).tid,
        (* (struct ParametrosParaFifo *)arg).dur, (* (struct ParametrosParaFifo *)arg).p1);
    }
    //diz que o cliente entrou na casa de banho
    else{
        printf("%ld ; %d ; %d ; %d ; %d ; %d ; ENTER\n",
        time(NULL) - begin, (* (struct ParametrosParaFifo *)arg).i,
        (* (struct ParametrosParaFifo *)arg).pid, (* (struct ParametrosParaFifo *)arg).tid,
        (* (struct ParametrosParaFifo *)arg).dur, (* (struct ParametrosParaFifo *)arg).p1);
    }

    //nome do fifo usado para a resposta do servidor
    char file[100];
    sprintf(file, "/tmp/%d.%d", (*(struct ParametrosParaFifo*) arg).pid, (*(struct ParametrosParaFifo*) arg).tid);

    //espera que o cliente cria o fifo para a resposta
    do{
        escritor = open(file, O_WRONLY);
    } while(escritor == -1);

    //se o tempo de funcionamento da casa de banho chegar ao fim manda ao cliente a dizer que fechou
    if(closed)
        write(escritor, (struct ParametrosParaFifo *)arg, sizeof(struct ParametrosParaFifo));
    //manda para o cliente a resposta com a casa de banho atribuida
    else{
        (*(struct ParametrosParaFifo *)arg).p1 = 2;
        write(escritor, (struct ParametrosParaFifo *)arg, sizeof(struct ParametrosParaFifo));
    }

    close(escritor);

    return NULL;
}

void signalHandler(int signal){
    //altera o valor para terminar o ciclo de tratamento de pedidos
    end = 1;
}

int main(int argc, char *argv[]){

    begin = time(NULL);

    pthread_t tid;

    //verifica se o programa foi invocado com um formato correto
    validFormat(argc, argv);

    //gera e trata de um alarme para daqui a argv[2] segundos
    signal(SIGALRM, signalHandler);
    alarm(atoi(argv[2]));

    //vai buscar qual o nome do fifo pelo qual se vai passar os argumentos
    char dirFifoPed[100], fifo_ped[100];
    find_fifo_name(argv, fifo_ped);
    sprintf(dirFifoPed, "/tmp/%s", fifo_ped);

    //cria o fifo pelo qual vai ser estabelecida a comunicacao entre a casa de banho e o cliente
    mkfifo(dirFifoPed, 0660);
    int leitor = open(dirFifoPed, O_RDONLY);

    struct ParametrosParaFifo argFifo;

    int numLidos; //tamanho de informacao lida no read

    do{

        numLidos = read(leitor, &argFifo, sizeof(struct ParametrosParaFifo));

        //reserva espaco para a passagem de argumentos
        void *arg = malloc(sizeof(struct ParametrosParaFifo));
        *(struct ParametrosParaFifo *)arg = argFifo;

        //cria uma thread para tratar do pedido
        if(numLidos != 0)
            pthread_create(&tid, NULL, thread_func, arg);

    } while(!end);

    //o fifo que era usado para a comunicacao entre o servidor e a casa de banho foi destruido
    closed = 1;

    //fecha e destroi o fifo usado para comunicacao
    close(leitor);
    unlink(dirFifoPed);

    pthread_exit(0);

}