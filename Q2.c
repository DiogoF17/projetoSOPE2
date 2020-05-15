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
#include <sys/types.h>
#include <sys/mman.h>

#define NUM_MAX_BATHROOM 10000
#define CHARCODE_ZERO 48 //48->code of '0'
#define CHARCODE_NINE 57 //57->code of '9'
#define PARAMETROS_FIFO (*(struct ParametrosParaFifo*) arg)
#define MS_2_US 100000
#define MAX_SPACE 100
#define SHM_MODE 0600
#define FIFO_MODE 0660

int clientEnd = 0;
int end = 0; //variavel que permite o ciclo
time_t begin; //instante inicial do programa
int closed = 0; //diz-nos se a casa de banho fechou

int pedidosRestantes = 0;

pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;

sem_t sem;
sem_t sem1;
int arrWC[NUM_MAX_BATHROOM];

int shmfd;
char *shm;

int t = -1;
int l = -1;
int n = -1;
char fifoName[MAX_SPACE];

char *validWords[3] = {"-t", "-n", "-l"};

int countThreadsRunning = 0;

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
        if(string[i] != CHARCODE_ZERO){
            zero = 0;
            break;
        }
    }
    
    if(zero == 1)
        return 0;

    for(int i = 0; i < strlen(string); i++){
        if (string[i] < CHARCODE_ZERO || string[i] > CHARCODE_NINE)
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
            else if(strcmp(argv[i-1], "-l") == 0){
                l = atoi(argv[i]);

                if(l > NUM_MAX_BATHROOM)
                    l = NUM_MAX_BATHROOM;
            
                countL++;
            }
            else{
                n = atoi(argv[i]);
                countN++;
            }
        }

        i++;
    }
    
    if(countT > 1 || countN > 1 || countL > 1|| countT == 0){
        printf("Invalid Format!\nFormat: Qn <-t nsecs> [-l nplaces] [-n nthreads] fifoname\n");
        exit(1);
    }

    if(l == -1)
        l = NUM_MAX_BATHROOM;

    //inicialização do array
    for(int i=0; i<l; i++){
        arrWC[i] = 0;
    }

}

int choose_WC() {
    if (end) 
        return -1;

    //verifica se alguma casa de banho está livre (a 0) e coloca ocupado (a 1) e retorna o numero da casa de banho (i)
    for (int i = 0; i < l; i++) {
        if (arrWC[i] == 0) {
            arrWC[i] = 1;
            return i;
        }
    }

    return -1;

}

void *thread_func(void *arg){
    int escritor;

    //nome do fifo usado para a resposta do servidor
    char file[MAX_SPACE];
    sprintf(file, "/tmp/%d.%ld", PARAMETROS_FIFO.pid, PARAMETROS_FIFO.tid);

    sem_wait(&sem1);
    pedidosRestantes++;
    sem_post(&sem1);

    PARAMETROS_FIFO.pid = getpid();
    PARAMETROS_FIFO.tid = pthread_self();

    //acusa a rececao do pedido feita pelo cliente
    printf("%ld ; %d ; %d ; %ld ; %d ; %d ; RECVD\n",
        time(NULL) - begin, PARAMETROS_FIFO.i, PARAMETROS_FIFO.pid, PARAMETROS_FIFO.tid, PARAMETROS_FIFO.dur, PARAMETROS_FIFO.p1);

    //espera que o cliente cria o fifo para a resposta
    do{
        escritor = open(file, O_WRONLY);
    } while(escritor == -1);

    //se o tempo de funcionamento da casa de banho chegar ao fim manda ao cliente a dizer que fechou
    if(closed){
        //Nao entrou logo a duracao foi -1
        (* (struct ParametrosParaFifo *)arg).dur = -1;

        if(write(escritor, (struct ParametrosParaFifo *)arg, sizeof(struct ParametrosParaFifo)) == -1){
             printf("%ld ; %d ; %d ; %ld ; %d ; %d ; GAVUP\n",
               time(NULL) - begin, PARAMETROS_FIFO.i, PARAMETROS_FIFO.pid, PARAMETROS_FIFO.tid, PARAMETROS_FIFO.dur, PARAMETROS_FIFO.p1);
        }

        printf("%ld ; %d ; %d ; %ld ; %d ; %d ; 2LATE\n",
        time(NULL) - begin, PARAMETROS_FIFO.i, PARAMETROS_FIFO.pid, PARAMETROS_FIFO.tid, PARAMETROS_FIFO.dur, PARAMETROS_FIFO.p1);
    }

    //manda para o cliente a resposta com a casa de banho atribuida
    else{
        sem_wait(&sem);

        //atribui o numero da casa de banho
        sem_wait(&sem1);
        pedidosRestantes--;
        int wc=choose_WC();
        PARAMETROS_FIFO.p1 = wc;
        sem_post(&sem1);

        if(PARAMETROS_FIFO.p1 == -1){
            printf("%ld ; %d ; %d ; %ld ; %d ; %d ; 2LATE\n",
                   time(NULL) - begin, PARAMETROS_FIFO.i, PARAMETROS_FIFO.pid, PARAMETROS_FIFO.tid, PARAMETROS_FIFO.dur, PARAMETROS_FIFO.p1);
        }

        else {
            //diz que o cliente entrou na casa de banho
            printf("%ld ; %d ; %d ; %ld ; %d ; %d ; ENTER\n",
                   time(NULL) - begin, PARAMETROS_FIFO.i, PARAMETROS_FIFO.pid, PARAMETROS_FIFO.tid, PARAMETROS_FIFO.dur, PARAMETROS_FIFO.p1);
        }

        //envia a resposta ao cliente a dizer qual a casa de banho atribuida
        if (write(escritor, (struct ParametrosParaFifo *) arg, sizeof(struct ParametrosParaFifo)) == -1) {
            printf("%ld ; %d ; %d ; %ld ; %d ; %d ; GAVUP\n",
                    time(NULL) - begin, PARAMETROS_FIFO.i, PARAMETROS_FIFO.pid, PARAMETROS_FIFO.tid, PARAMETROS_FIFO.dur, PARAMETROS_FIFO.p1);
        }

        if((*(struct ParametrosParaFifo *)arg).p1 != -1){

            //tempo de utilizacao da casa de banho
            usleep(PARAMETROS_FIFO.dur * MS_2_US);

            //coloca o numero da casa de banho livre
            sem_wait(&sem1);
            arrWC[PARAMETROS_FIFO.p1] = 0;
            sem_post(&sem1);

            //diz que o tempo do cliente na casa de banho acabou
            printf("%ld ; %d ; %d ; %ld ; %d ; %d ; TIMUP\n",
                    time(NULL) - begin, PARAMETROS_FIFO.i, PARAMETROS_FIFO.pid, PARAMETROS_FIFO.tid, PARAMETROS_FIFO.dur, PARAMETROS_FIFO.p1);
        }

        sem_post(&sem);
    }

    close(escritor);

    pthread_mutex_lock(&mutex);
    countThreadsRunning--;

    if (n!=-1)
        n++;

    pthread_mutex_unlock(&mutex);

    return NULL;
}

void signalHandler(int signal){
    //altera o valor para terminar o ciclo de tratamento de pedidos
    end = 1;

    //o fifo que era usado para a comunicacao entre o servidor e a casa de banho foi destruido
    closed = 1;

    *shm = 1;

    //-------------
    //Destroy shared memory region

    if(munmap(shm, 1)<0)
        perror("munmap");

    if(shm_unlink("status")<0)
        perror("shm_unlink");

    //-------------    
    //impede que processos que estejam empacados a espera saibam logo que a casa de banho fechou
    for(int i = 0; i < pedidosRestantes; i++)
        sem_post(&sem);

}

int main(int argc, char *argv[]){

    begin = time(NULL);

    pthread_t tid;

    int leitor;

    //verifica se o programa foi invocado com um formato correto
    validFormat(argc, argv);

    //vai buscar os valores para atribuir a n, t e l
    atribuiValores(argv, argc);

    //vai buscar qual o nome do fifo pelo qual se vai passar os argumentos
    find_fifo_name(argv, argc);

    //----------------

    //create the shared memory region
    shmfd = shm_open("status",O_CREAT|O_RDWR,SHM_MODE);
    if(shmfd<0)
        perror("shm_open");
        
    if (ftruncate(shmfd,1) < 0)
        perror("ftruncate");
        
    //attach this region to virtual 
    shm = (char *) mmap(0,1,PROT_READ|PROT_WRITE,MAP_SHARED,shmfd,0);
    if(shm == MAP_FAILED)
        perror("mmap");

    //----------------

    sem_init(&sem, 0, l);
    sem_init(&sem1, 0, 1);

    //gera e trata de um alarme para daqui a t segundos
    signal(SIGALRM, signalHandler);
    alarm(t);

    //cria o fifo pelo qual vai ser estabelecida a comunicacao entre a casa de banho e o cliente
    mkfifo(fifoName, FIFO_MODE);
    do{
        leitor = open(fifoName, O_RDONLY | O_NONBLOCK);
    }while(leitor == -1 && !end && !clientEnd);

    struct ParametrosParaFifo argFifo;

    int numLidos; //tamanho de informacao lida no read

    while(!end){

        pthread_mutex_lock(&mutex);
        if(n>0 || n== -1){
            
            numLidos = read(leitor, &argFifo, sizeof(struct ParametrosParaFifo));
            
            if (numLidos>0){
                //cria uma thread para tratar do pedido
                //reserva espaco para a passagem de argumentos
                void *arg = malloc(sizeof(struct ParametrosParaFifo));
                PARAMETROS_FIFO = argFifo;
                
                while(pthread_create(&tid, NULL, thread_func, arg)){
                    perror("pthread create");
                    usleep(5);
                }

                countThreadsRunning++;

                if (n!=-1)
                    n--;
            }
            
        }
        pthread_mutex_unlock(&mutex);
        
    } 

    //esvazia o fifo com eventuais pedidos que tenham restado
    while(1){
        pthread_mutex_lock(&mutex);
        if(n>0 || n== -1){

            numLidos = read(leitor, &argFifo, sizeof(struct ParametrosParaFifo));

            if (numLidos <=0){
                pthread_mutex_unlock(&mutex);
                break;
            }
        
            //cria uma thread para tratar do pedido
            //reserva espaco para a passagem de argumentos
            void *arg = malloc(sizeof(struct ParametrosParaFifo));
            PARAMETROS_FIFO = argFifo;
            
            while(pthread_create(&tid, NULL, thread_func, arg)){
                perror("pthread create");
                usleep(5);
            }

            countThreadsRunning++;

            if (n!=-1)
                n--;
        }
        pthread_mutex_unlock(&mutex);
    
    }

    //fecha e destroi o fifo usado para comunicacao
    close(leitor);
    unlink(fifoName);

    //Espera que todas as threads executem para depois destruir os semaforos
    while(countThreadsRunning){
    }

    sem_destroy(&sem);
    sem_destroy(&sem1);
    pthread_mutex_destroy(&mutex);

    pthread_exit(0);

}