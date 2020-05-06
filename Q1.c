#include <stdio.h> 
#include <stdlib.h> 
#include <fcntl.h> 
#include <unistd.h>
#include <sys/stat.h>
#include <string.h>
#include <pthread.h>
#include <signal.h>
#include <limits.h>

int clientEnd = 0;
int end = 0; //variavel que permite o ciclo
time_t begin; //instante inicial do programa
int closed = 0; //diz-nos se a casa de banho fechou
int numCasasBanho=1; //nplaces na casa de banho
pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;

int t = -1;
int l = -1;
int n = -1;
char fifoName[100];

char *validWords[3] = {"-t", "-n", "-l"};

struct ParametrosParaFifo{
    int i; //identificador de cada pedido
    int pid; //pid do processo
    unsigned long tid; //tid do processo
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

int inValidWords(char *string){
    for(int i = 0; i < 3; i++){
        if(strcmp(string, validWords[i]) == 0)
            return 1;
    }
    return 0;
}

void printInvalidFormat(){
    printf("Invalid Format!\nFormat: Qn <-t nsecs> [-l nplaces] [-n nthreads] fifoname\n");
    exit(1);
}

void validFormat(int argc, char *argv[]){
    int i = 1;
    
    if(argc < 4 || argc > 8 || argc % 2 != 0){
        printf("1");
        printInvalidFormat();
    }

    while(i < (argc - 1)){
        if(inValidWords(argv[i]) == 1){
            if((i+1) >= (argc-1) || validNumber(argv[i+1]) == 0){
                printf("2");
                printInvalidFormat();   
            }
        }
        else if (validNumber(argv[i]) == 1){
            if(i == 1 || inValidWords(argv[i-1]) == 0){
                printf("3");
                printInvalidFormat();
            }
        }
        else{
            printf("4");
            printInvalidFormat();
        }
        
        i++;
    }

    /*if(strcmp(argv[1], "-t") != 0){
        printf("Invalid Format!\nFormat: Qn <-t nsecs> [-l nplaces] [-n nthreads] fifoname\n");
        exit(1);
    }
    if(validNumber(argv[2]) != 1){
        printf("Invalid Format!\nFormat: Qn <-t nsecs> [-l nplaces] [-n nthreads] fifoname\n");
        exit(1);
    }
    
    
    //Nao e suposto estar na primeira parte
    if(optionalArgs(argv) != 1){
        printf("Invalid Format!\nFormat: Qn <-t nsecs> [-l nplaces] [-n nthreads] fifoname\n");
        exit(1);
    }*/
    
}

void find_fifo_name(char *argv[], int argc){
    if(inValidWords(argv[argc-1]) == 1)
        printInvalidFormat();

    if(strcmp("client", argv[argc-1]) == 0 || strcmp("status", argv[argc-1]) == 0){
        printf("O nome: %s e usado para outro fifo interno ao programa!\nPor favor escolha outro nome!\n", argv[argc-1]);
        exit(1);
    }
    
    if(strcpy(fifoName, argv[argc-1]) == NULL){
        perror("strcpy");
    }
}

void atribuiValores(char *argv[], int argc){
    int i = 1;
    int countT = 0, countN = 0, countL = 0;

    while(i < (argc - 1)){
        if(inValidWords(argv[i]) == 0){
            if(strcmp(argv[i-1], "-t") == 0){
                t = atoi(argv[i]);
                countT++;
            }
            else if(strcmp(argv[i-1], "-n") == 0){
                n = atoi(argv[i]);
                countN++;
            }
            else{
                l = atoi(argv[i]);
                countL++;
            }
        }

        i++;
    }
    
    if(countT > 1 || countN > 1 || countL > 1|| countT == 0)
        printInvalidFormat();

}

int choose_WC(){
    numCasasBanho++;
    return numCasasBanho;
}

void *thread_func(void *arg){
    int escritor;

    //nome do fifo usado para a resposta do servidor
    char file[100];
    sprintf(file, "/tmp/%d.%ld", (*(struct ParametrosParaFifo*) arg).pid, (*(struct ParametrosParaFifo*) arg).tid);
    
    (*(struct ParametrosParaFifo*) arg).pid = getpid();
    (*(struct ParametrosParaFifo*) arg).tid = pthread_self();

    //acusa a rececao do pedido feita pelo cliente
    printf("%ld ; %d ; %d ; %ld ; %d ; %d ; RECVD\n",
        time(NULL) - begin, (* (struct ParametrosParaFifo *)arg).i,
        (*(struct ParametrosParaFifo*) arg).pid, (*(struct ParametrosParaFifo*) arg).tid,
        (* (struct ParametrosParaFifo *)arg).dur, (* (struct ParametrosParaFifo *)arg).p1);

    //diz que o pedido chegou tarde de mais
    if(closed){
        //Nao entrou logo a duracao foi -1
        (* (struct ParametrosParaFifo *)arg).dur = -1;

        printf("%ld ; %d ; %d ; %ld ; %d ; %d ; 2LATE\n",
        time(NULL) - begin, (* (struct ParametrosParaFifo *)arg).i,
        (*(struct ParametrosParaFifo*) arg).pid, (*(struct ParametrosParaFifo*) arg).tid,
        (* (struct ParametrosParaFifo *)arg).dur, (* (struct ParametrosParaFifo *)arg).p1);
    }

    //espera que o cliente cria o fifo para a resposta
    do{
        escritor = open(file, O_WRONLY);
    } while(escritor == -1);

    //se o tempo de funcionamento da casa de banho chegar ao fim manda ao cliente a dizer que fechou
    if(closed){
        if(write(escritor, (struct ParametrosParaFifo *)arg, sizeof(struct ParametrosParaFifo)) == -1){
             printf("%ld ; %d ; %d ; %ld ; %d ; %d ; GAVUP\n",
               time(NULL) - begin, (* (struct ParametrosParaFifo *)arg).i,
               (*(struct ParametrosParaFifo*) arg).pid, (*(struct ParametrosParaFifo*) arg).tid,
               (* (struct ParametrosParaFifo *)arg).dur, (* (struct ParametrosParaFifo *)arg).p1);
        }
    }
    //manda para o cliente a resposta com a casa de banho atribuida
    else{

        pthread_mutex_lock(&mutex);
        //atribuicao da casa de banho
        (*(struct ParametrosParaFifo *)arg).p1 = choose_WC();
        pthread_mutex_unlock(&mutex);

        //diz que o cliente entrou na casa de banho
        printf("%ld ; %d ; %d ; %ld ; %d ; %d ; ENTER\n",
               time(NULL) - begin, (* (struct ParametrosParaFifo *)arg).i,
               (*(struct ParametrosParaFifo*) arg).pid, (*(struct ParametrosParaFifo*) arg).tid,
               (* (struct ParametrosParaFifo *)arg).dur, (* (struct ParametrosParaFifo *)arg).p1);
        
        //envia a resposta ao cliente a dizer qual a casa de banho atribuida
        if(write(escritor, (struct ParametrosParaFifo *)arg, sizeof(struct ParametrosParaFifo)) == -1){
            printf("%ld ; %d ; %d ; %ld ; %d ; %d ; GAVUP\n",
               time(NULL) - begin, (* (struct ParametrosParaFifo *)arg).i,
               (*(struct ParametrosParaFifo*) arg).pid, (*(struct ParametrosParaFifo*) arg).tid,
               (* (struct ParametrosParaFifo *)arg).dur, (* (struct ParametrosParaFifo *)arg).p1);
        }

        //tempo de utilizacao da casa de banho
        usleep((*(struct ParametrosParaFifo *)arg).dur*1000);

        //diz que o tempo do cliente na casa de banho acabou
        printf("%ld ; %d ; %d ; %ld ; %d ; %d ; TIMUP\n",
               time(NULL) - begin, (* (struct ParametrosParaFifo *)arg).i,
               (*(struct ParametrosParaFifo*) arg).pid, (*(struct ParametrosParaFifo*) arg).tid,
               (* (struct ParametrosParaFifo *)arg).dur, (* (struct ParametrosParaFifo *)arg).p1);

    }

    close(escritor);

    return NULL;
}

void writeDestroyed(){
    int escritor;
    
    mkfifo("status", 0660);
    do{
        escritor = open("status", O_WRONLY | O_NONBLOCK);
   }while(escritor == -1);

    if(escritor != -1){
        if(write(escritor, "destroyed", 9) == -1)
            perror("write");

        close(escritor);
    }
}

void readClientEnd(int leitor){
    char string[10];

    do{
        if(read(leitor, string, 4) == -1)
            perror("read");
    }while(strcmp("end", string) != 0);

    clientEnd = 1;

    close(leitor);
}

void signalHandler(int signal){
    //altera o valor para terminar o ciclo de tratamento de pedidos
    end = 1;

    int leitor = open("client", O_RDONLY | O_NONBLOCK);
    
    if(leitor != -1){
        char string[10];
        if(read(leitor, string, 4) == -1)
            perror("read");
        if(strcmp("end", string) == 0)
            clientEnd = 1;

        close(leitor);
    }

    if(!clientEnd)
        writeDestroyed();
    
}

void* verifyClientEnd(void *arg){
    int leitor;

    do{
        leitor = open("client", O_RDONLY);
    }while(leitor == -1 && !end);

    if(leitor != -1 && !clientEnd)
        readClientEnd(leitor);
    
    unlink("client");

    return NULL;
}

int main(int argc, char *argv[]){

    begin = time(NULL);

    pthread_t tid, tidClienteEnd;

    //verifica se o programa foi invocado com um formato correto
    validFormat(argc, argv);

    //vai buscar os valores para atribuir a n, t e l
    atribuiValores(argv, argc);

    //vai buscar qual o nome do fifo pelo qual se vai passar os argumentos
    find_fifo_name(argv, argc);

    //gera e trata de um alarme para daqui a argv[2] segundos
    signal(SIGALRM, signalHandler);
    alarm(t);

    if(pthread_create(&tidClienteEnd, NULL, verifyClientEnd, NULL))
        perror("pthread_create");

    //cria o fifo pelo qual vai ser estabelecida a comunicacao entre a casa de banho e o cliente
    mkfifo(fifoName, 0660);
    int leitor = open(fifoName, O_RDONLY);

    struct ParametrosParaFifo argFifo;

    int numLidos; //tamanho de informacao lida no read

    do{

        numLidos = read(leitor, &argFifo, sizeof(struct ParametrosParaFifo));

        //reserva espaco para a passagem de argumentos
        void *arg = malloc(sizeof(struct ParametrosParaFifo));
        *(struct ParametrosParaFifo *)arg = argFifo;

        //cria uma thread para tratar do pedido
        if(numLidos != 0){
            if(pthread_create(&tid, NULL, thread_func, arg))
                perror("pthread create");
        }

    } while(!end);

    //o fifo que era usado para a comunicacao entre o servidor e a casa de banho foi destruido
    closed = 1;

    //fecha e destroi o fifo usado para comunicacao
    close(leitor);
    unlink(fifoName);

    pthread_exit(0);

}