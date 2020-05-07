#include <stdio.h> 
#include <stdlib.h> 
#include <fcntl.h> 
#include <unistd.h>
#include <sys/stat.h>
#include <string.h>
#include <pthread.h>
#include <time.h>
#include <signal.h>
#include <limits.h>

int destroyed = 0;  //diz-nos se o fifo ja foi criado alguma vez, pois pode ja ter sido destruido e nesse caso nao podemos enviar mais pedidos
int end = 0; //variavel que permite o ciclo
time_t begin; //instante inicial do programa

int t = -1;
char fifoName[100];

struct ParametrosParaFifo{
    int i; //identificador de cada pedido
    int pid; //pid do processo
    unsigned long tid; //tid do processo
    int dur; //milisegundos
    int p1; //nº da casa de banho que foi atribuida
};

struct ParametrosParaThread{
    int identificador; //identificador de cada thread na linha de lancamento
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
    if(strcmp(string, "") == 0 || isZero(string))
        return 0;
    for(int i = 0; i < strlen(string); i++){
        if (string[i] < 48 || string[i] > 57)
            return 0;
    }
    return 1;
}

void printInvalidFormat(){
    printf("Invalid Format!\nFormat: Un <-t nsecs> fifoname\n");
    exit(1);
}

void validFormat(int argc, char *argv[]){
    if(argc != 4 || argc % 2 == 1)
        printInvalidFormat();
    if(strcmp(argv[1], "-t") != 0)
        printInvalidFormat();
    if(validNumber(argv[2]) != 1)
        printInvalidFormat();

    t = atoi(argv[2]);

    if(strcmp(argv[3], "-t") == 0 || strcmp(argv[3], "-n") == 0 || strcmp(argv[3], "-l") == 0)
        printInvalidFormat();

    if(strcmp("client", argv[argc-1]) == 0 || strcmp("status", argv[argc-1]) == 0){
        printf("O nome: %s e usado para outro fifo interno ao programa!\nPor favor escolha outro nome!\n", argv[argc-1]);
        exit(1);
    }

    strcpy(fifoName, argv[3]);    
}

void *thread_func(void *arg){
    int escritor = -1, leitor;
    
    struct ParametrosParaFifo argFifo;

    //-----------------------
/*
    if(!destroyed){

        int leitor1 = open("status", O_RDONLY | O_NONBLOCK);
        
        if(leitor1 != -1){
            char string[10];
            if(read(leitor1, string, 10) == -1)
                perror("read");
            if(strcmp("destroyed", string) == 0)
                destroyed = 1;

            close(leitor1);
        }

    }
    else
        unlink("status");*/

    //---------------
    
    //fica a espera que o fifo seja criado caso nao tenha sido criado nenhuma vez
    do{
        escritor = open((*(struct ParametrosParaThread *)arg).fifo_ped, O_WRONLY);
    }while(escritor == -1 && destroyed == 0);

    //o fifo ja foi criado e ainda nao foi detsruido
    if(escritor != -1 && destroyed == 0){

        argFifo.i = (*(struct ParametrosParaThread *)arg).identificador;
        argFifo.pid = getpid();
        argFifo.tid = pthread_self();
        argFifo.dur = (rand() % 5) + 1;
        argFifo.p1 = -1;

        //o cliente faz o pedido ao servidor(casa de banho)
        printf("%ld ; %d ; %d ; %ld ; %d ; %d ; IWANT\n",
                time(NULL) - begin, argFifo.i,
                argFifo.pid, argFifo.tid,
                argFifo.dur, argFifo.p1);
        
        if(write(escritor, &argFifo, sizeof(struct ParametrosParaFifo)) == -1)
            perror("write");

        close(escritor);

        //o cliente cria o fifo onde vai receber as respostas do servidor
        char file[100];
        sprintf(file, "/tmp/%d.%ld", argFifo.pid, argFifo.tid);

        mkfifo(file, 0660);
        leitor = open(file, O_RDONLY);

        //le a resposta do servidor
        if(read(leitor, &argFifo, sizeof(struct ParametrosParaFifo))==-1){
            printf("%ld ; %d ; %d ; %ld ; %d ; %d ; FAILD\n",
                time(NULL) - begin, argFifo.i,
                argFifo.pid, argFifo.tid,
                argFifo.dur, argFifo.p1);
        }
        else{

            argFifo.pid = getpid();
            argFifo.tid = pthread_self();

            //a casa de banho que foi atribuida ao cliente foi -1 logo ja fechou
            //neste caso teve tempo de fazer o pedido porque ainda estava aberta mas entretanto fechou
            if(argFifo.p1 == -1){
                printf("%ld ; %d ; %d ; %ld ; %d ; %d ; CLOSD\n",
                        time(NULL) - begin, argFifo.i,
                        argFifo.pid, argFifo.tid,
                        argFifo.dur, argFifo.p1);
            }
            //foi atribuida uma casa de banho > -1
            else{
                printf("%ld ; %d ; %d ; %ld ; %d ; %d ; IAMIN\n",
                        time(NULL) - begin, argFifo.i,
                        argFifo.pid, argFifo.tid,
                        argFifo.dur, argFifo.p1);
            }
        }
        
        close(leitor);
        unlink(file);
    }
    //caso o fifo ja tenha sido destruido, ou seja, se a casa de banho ja fechou
    //o cliente nao teve tempo de fazer o pedido
    else if(destroyed == 1){
        
        argFifo.i = (*(struct ParametrosParaThread *)arg).identificador;
        argFifo.pid = getpid();
        argFifo.tid = pthread_self();
        argFifo.dur = -1;
        argFifo.p1 = -1;

        printf("%ld ; %d ; %d ; %ld ; %d ; %d ; CLOSD\n",
                    time(NULL) - begin, argFifo.i,
                    argFifo.pid, argFifo.tid,
                    argFifo.dur, argFifo.p1);
    }

    //libertacao de recursos utilizados
    free(arg);

    return NULL;
}

void writeClientEnd(){
    int escritor;
    mkfifo("client", 0660);
    do{
        escritor = open("client", O_WRONLY | O_NONBLOCK);
    }while(escritor == -1);

    if(write(escritor, "end", 3) == -1)
        perror("write");

    close(escritor);
}

void readDestroyed(int leitor){
    char string[10];

    do{
        if(read(leitor, string, 10) == -1)
            perror("read");
    }while(strcmp("destroyed", string) != 0);

    destroyed = 1;

    close(leitor);
}

void signalHandler(int signal){
    //altera o valor para terminar o ciclo de geracao de pedidos
    end = 1;

    int leitor = open("status", O_RDONLY | O_NONBLOCK);
    
    if(leitor != -1){
        char string[10];
        if(read(leitor, string, 10) == -1)
            perror("read");
        if(strcmp("destroyed", string) == 0)
            destroyed = 1;

        close(leitor);
    }

    if(!destroyed)
        writeClientEnd();
}

void* verifyDestroyed(void *arg){
    int leitor;

    do{
        leitor = open("status", O_RDONLY | O_NONBLOCK);
    }while(leitor == -1 && !end);

    if(leitor != -1 && !destroyed)
        readDestroyed(leitor);
    
    unlink("status");

    return NULL;
}

int main(int argc, char *argv[]){

    begin = time(NULL);

    //verifica se o programa foi invocado com um formato correto
    validFormat(argc, argv);

    //gera e trata de um alarme para daqui a argv[2] segundos
    signal(SIGALRM, signalHandler);
    alarm(atoi(argv[2]));

    srand(time(NULL));

    //intervalo pelo qual se vai gerar pedidos
    int intervalo = (rand() % 100) + 1;

    int identificador = 1;
    
    pthread_t tid, tidStatus;

    //thread que vai durante o program verificar se a casa de banho fechou
    //quando tiver fechado ativa a flag destroyed e abandona a thread
    if(pthread_create(&tidStatus, NULL, verifyDestroyed, NULL))
        perror("pthread_create");

    while(!end){
        //intervalo pelo qual vao ser gerados os pedidos
        usleep(intervalo * 1000);
        
        //reserva espaco para a passagem de argumentos
        void * arg = malloc (sizeof(struct ParametrosParaThread));
        (*(struct ParametrosParaThread *) arg).identificador = identificador;
        strcpy((*(struct ParametrosParaThread *) arg).fifo_ped, fifoName);

        //cria uma thread para fazer o pedido
        if(pthread_create(&tid, NULL, thread_func, arg))
            perror("pthread_create");
        
        //identificador de cada pedido
        identificador++;

    /* if(destroyed == 0){
            int leitor = leitor = open("status", O_RDONLY | O_NONBLOCK);

            if(leitor != -1){
                char string[10];
                if(read(leitor, string, 10) == -1)
                    perror("read");

                if(strcmp("destroyed", string) == 0){
                    destroyed = 1;
                    unlink("status");
                }
            }
        }*/
    }

    pthread_exit(0);

}