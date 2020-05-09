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

int threadVerifyWorking = 0;

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

int validNumber(char *string){
    if(string == NULL)
        return 0;
    if(strcmp(string, "") == 0)
        return 0;

    int zero = 1;

    for(int i = 0; i < strlen(string); i++){
        if(string[i] != 48){ //48->code of '0'
            zero = 0;
            break;
        }
    }
    
    if(zero == 1)
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
    if(validNumber(argv[2]) != 1){
        printf("Invalid Format!\nFormat: Un <-t nsecs> fifoname\n");
        exit(1);
    }

    t = atoi(argv[2]);

    if(strcmp(argv[3], "-t") == 0 || strcmp(argv[3], "-n") == 0 || strcmp(argv[3], "-l") == 0){
        printf("Invalid Format!\nFormat: Un <-t nsecs> fifoname\n");
        exit(1);
    }

    if(strcmp("clientStatus", argv[argc-1]) == 0 || strcmp("bathroomStatus", argv[argc-1]) == 0){
        printf("O nome: %s e usado para outro fifo interno ao programa!\nPor favor escolha outro nome!\n", argv[argc-1]);
        exit(1);
    }

    strcpy(fifoName, argv[3]);    
}

void *thread_func(void *arg){
    int escritor = -1, leitor;
    
    struct ParametrosParaFifo argFifo;

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
        if(write(escritor, &argFifo, sizeof(struct ParametrosParaFifo)) == -1)
            perror("write");

        close(escritor);

        printf("%ld ; %d ; %d ; %ld ; %d ; %d ; IWANT\n",
                time(NULL) - begin, argFifo.i,
                argFifo.pid, argFifo.tid,
                argFifo.dur, argFifo.p1);

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
    /*
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
    }*/

    //libertacao de recursos utilizados
    free(arg);

    return NULL;
}

void signalHandler(int signal){
    //altera o valor para terminar o ciclo de geracao de pedidos
    end = 1;

    //caso a casa de banho nao tenha terminado
    //e escrito no fifo do status da casa do cliente
    //que este ja terminou a geracao de pedidos
    if(!destroyed){
        mkfifo("clientStatus", 0660);
        int escritor = open("clientStatus", O_WRONLY);
        
        if(write(escritor, "end", 3) == -1)
            perror("write");

        close(escritor);
    }
}

void* verifyDestroyed(void *arg){
    threadVerifyWorking = 1;
    
    int leitor;

    do{
        leitor = open("bathroomStatus", O_RDONLY);
    }while(leitor == -1 && !end && !destroyed);

    if(leitor != -1 && !destroyed){
        char string[10];

        do{
            if(read(leitor, string, 10) == -1)
                perror("read1");
        }while(strcmp("destroyed", string) != 0);

        destroyed = 1;

        close(leitor);
    }
    
    unlink("bathroomStatus");

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
    int intervalo = (rand() % 10) + 1;

    int identificador = 1;
    
    pthread_t tid, tidStatus;

    //thread que vai durante o programa verificar se a casa de banho fechou
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
    }

    pthread_exit(0);

}