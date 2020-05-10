#include <stdio.h> 
#include <stdlib.h> 
#include <fcntl.h> 
#include <unistd.h>
#include <sys/stat.h>
#include <string.h>
#include <pthread.h>
#include <signal.h>
#include <limits.h>
#include<semaphore.h>

#define NUM_MAX_BATHROOM 10000

int clientEnd = 0;
int end = 0; //variavel que permite o ciclo
time_t begin; //instante inicial do programa
int closed = 0; //diz-nos se a casa de banho fechou
int numCasasBanho=1; //nplaces na casa de banho

sem_t sem;
sem_t sem1;
int arrWC[NUM_MAX_BATHROOM];

int t = -1;
int l = -1;
int n = -1;
char fifoName[100];

int threadVerifyWorking = 0;

char *validWords[3] = {"-t", "-n", "-l"};

struct ParametrosParaFifo{
    int i; //identificador de cada pedido
    int pid; //pid do processo
    unsigned long tid; //tid do processo
    int dur; //milisegundos
    int p1; //nº da casa de banho que foi atribuida
};

int validNumber(char *string){
    if(string == NULL)
        return 0;
    if(strcmp(string, "") == 0)
        return 0;

    int zero = 1;

    for(int i = 0; i < strlen(string); i++){
        if(string[i] != 48){//48->code of '0'
            zero = 0;
            break;
        }
    }
    
    if(zero == 1)
        return 0;

    for(int i = 0; i < strlen(string); i++){
        if (string[i] < 48 || string[i] > 57) //48->code of '0', 57->code of '9'
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
        printf("Invalid Format!\nFormat: Qn <-t nsecs> [-l nplaces] [-n nthreads] fifoname\n");
        exit(1);
    }

    while(i < (argc - 1)){
        if(inValidWords(argv[i]) == 1){
            if((i+1) >= (argc-1) || validNumber(argv[i+1]) == 0){
                printf("Invalid Format!\nFormat: Qn <-t nsecs> [-l nplaces] [-n nthreads] fifoname\n");
                exit(1);
            } 
        }
        else if (validNumber(argv[i]) == 1){
            if(i == 1 || inValidWords(argv[i-1]) == 0){
                printf("Invalid Format!\nFormat: Qn <-t nsecs> [-l nplaces] [-n nthreads] fifoname\n");
                exit(1);
            }
        }
        else{
            printf("Invalid Format!\nFormat: Qn <-t nsecs> [-l nplaces] [-n nthreads] fifoname\n");
            exit(1);
        }
        
        i++;
    }
    
}

void find_fifo_name(char *argv[], int argc){
    if(inValidWords(argv[argc-1]) == 1){
        printf("Invalid Format!\nFormat: Qn <-t nsecs> [-l nplaces] [-n nthreads] fifoname\n");
        exit(1);
    }

    if(strcmp("clientStatus", argv[argc-1]) == 0 || strcmp("bathroomStatus", argv[argc-1]) == 0){
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

                if(n > NUM_MAX_BATHROOM)
                    n = NUM_MAX_BATHROOM;
                //inicialização do array
                for(int i=0; i<n; i++){
                    arrWC[i] = 0;
                }
                countN++;
            }
            else{
                l = atoi(argv[i]);
                countL++;
            }
        }

        i++;
    }
    
    if(countT > 1 || countN > 1 || countL > 1|| countT == 0){
        printf("Invalid Format!\nFormat: Qn <-t nsecs> [-l nplaces] [-n nthreads] fifoname\n");
        exit(1);
    }

    if(n==-1){
        n = NUM_MAX_BATHROOM;
    }

}

int choose_WC() {
    if (end) {
        return -1;
    }
    //verifica se alguma casa de banho está livre (a 0) e coloca ocupado (a 1) e retorna o numero da casa de banho (i)
    for (int i = 0; i < n; i++) {
        //printf("\n%d\n\n",i);
        if (arrWC[i] == 0) {
            arrWC[i] = 1;
            return i;
        }
    }

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

        //atribui o numero da casa de banho
        sem_wait(&sem);
        sem_wait(&sem1);
        int wc=choose_WC();
        (*(struct ParametrosParaFifo *)arg).p1 = wc;
        sem_post(&sem1);

        if(wc==-1){
            printf("%ld ; %d ; %d ; %ld ; %d ; %d ; 2LATE\n",
                   time(NULL) - begin, (* (struct ParametrosParaFifo *)arg).i,
                   (*(struct ParametrosParaFifo*) arg).pid, (*(struct ParametrosParaFifo*) arg).tid,
                   (* (struct ParametrosParaFifo *)arg).dur, (* (struct ParametrosParaFifo *)arg).p1);
        }
        else {
            //diz que o cliente entrou na casa de banho
            printf("%ld ; %d ; %d ; %ld ; %d ; %d ; ENTER\n",
                   time(NULL) - begin, (*(struct ParametrosParaFifo *) arg).i,
                   (*(struct ParametrosParaFifo *) arg).pid, (*(struct ParametrosParaFifo *) arg).tid,
                   (*(struct ParametrosParaFifo *) arg).dur, (*(struct ParametrosParaFifo *) arg).p1);

            //envia a resposta ao cliente a dizer qual a casa de banho atribuida
            if (write(escritor, (struct ParametrosParaFifo *) arg, sizeof(struct ParametrosParaFifo)) == -1) {
                printf("%ld ; %d ; %d ; %ld ; %d ; %d ; GAVUP\n",
                       time(NULL) - begin, (*(struct ParametrosParaFifo *) arg).i,
                       (*(struct ParametrosParaFifo *) arg).pid, (*(struct ParametrosParaFifo *) arg).tid,
                       (*(struct ParametrosParaFifo *) arg).dur, (*(struct ParametrosParaFifo *) arg).p1);
            }


            //tempo de utilizacao da casa de banho
            usleep((*(struct ParametrosParaFifo *) arg).dur * 1000);
            //coloca o numero da casa de banho livre
            sem_wait(&sem1);
            arrWC[(*(struct ParametrosParaFifo *) arg).p1] = 0;
            sem_post(&sem1);
            sem_post(&sem);

            //diz que o tempo do cliente na casa de banho acabou
            printf("%ld ; %d ; %d ; %ld ; %d ; %d ; TIMUP\n",
                   time(NULL) - begin, (*(struct ParametrosParaFifo *) arg).i,
                   (*(struct ParametrosParaFifo *) arg).pid, (*(struct ParametrosParaFifo *) arg).tid,
                   (*(struct ParametrosParaFifo *) arg).dur, (*(struct ParametrosParaFifo *) arg).p1);
        }
    }

    close(escritor);

    return NULL;
}

void signalHandler(int signal){
    //altera o valor para terminar o ciclo de tratamento de pedidos
    end = 1;

    //o fifo que era usado para a comunicacao entre o servidor e a casa de banho foi destruido
    closed = 1;

    //caso o cliente nao tenha terminado
    //e escrito no fifo do status da casa de banho
    //que esta ja terminou 
    if(!clientEnd){
        mkfifo("bathroomStatus", 0660);
        int escritor = open("bathroomStatus", O_WRONLY);

        if(write(escritor, "destroyed", 9) == -1)
            perror("write");

        close(escritor);
    }
    
}

void* verifyClientEnd(void *arg){
    threadVerifyWorking = 1;

    int leitor;

    do{
        leitor = open("clientStatus", O_RDONLY);
    }while(leitor == -1 && !end && !clientEnd);

    if(leitor != -1 && !clientEnd){
        char string[4];
        strcpy(string, "");
        do{
            if(read(leitor, string, 4) == -1)
                perror("read");
        }while(strcmp("end", string) != 0);

        clientEnd = 1;

        close(leitor);
    }
    
    unlink("clientStatus");

    return NULL;
}

int main(int argc, char *argv[]){

    begin = time(NULL);

    sem_init(&sem, 0, n);
    sem_init(&sem1, 0, 1);

    pthread_t tid, tidClienteEnd;

    int leitor;

    //verifica se o programa foi invocado com um formato correto
    validFormat(argc, argv);

    //vai buscar os valores para atribuir a n, t e l
    atribuiValores(argv, argc);

    //vai buscar qual o nome do fifo pelo qual se vai passar os argumentos
    find_fifo_name(argv, argc);

    //gera e trata de um alarme para daqui a t segundos
    signal(SIGALRM, signalHandler);
    alarm(t);

    //thread que vai verificar se o cliente terminou a geracao de pedidos
    //se sim entao nao e necessario escrever quando se destruir
    if(pthread_create(&tidClienteEnd, NULL, verifyClientEnd, NULL))
        perror("pthread_create");

    //cria o fifo pelo qual vai ser estabelecida a comunicacao entre a casa de banho e o cliente
    mkfifo(fifoName, 0660);
    leitor = open(fifoName, O_RDONLY);

    struct ParametrosParaFifo argFifo;

    int numLidos; //tamanho de informacao lida no read

    while(!end){

        numLidos = read(leitor, &argFifo, sizeof(struct ParametrosParaFifo));

        //reserva espaco para a passagem de argumentos
        void *arg = malloc(sizeof(struct ParametrosParaFifo));
        *(struct ParametrosParaFifo *)arg = argFifo;

        //cria uma thread para tratar do pedido
        if(numLidos > 0){
            if(pthread_create(&tid, NULL, thread_func, arg))
                perror("pthread create");
        }
    } 

    //esvazia o fifo com eventuais pedidos que tenham restado
    while(read(leitor, &argFifo, sizeof(struct ParametrosParaFifo)) > 0){
        //reserva espaco para a passagem de argumentos
        void *arg = malloc(sizeof(struct ParametrosParaFifo));
        *(struct ParametrosParaFifo *)arg = argFifo;

        //cria uma thread para tratar do pedido
        if(numLidos > 0){
            if(pthread_create(&tid, NULL, thread_func, arg))
                perror("pthread create");
        }
    }

    //fecha e destroi o fifo usado para comunicacao
    close(leitor);
    unlink(fifoName);

    pthread_exit(0);

    sem_destroy(&sem);
    sem_destroy(&sem1);

}