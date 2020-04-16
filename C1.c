#include <stdio.h> 
#include <stdlib.h> 
#include <fcntl.h> 
#include <unistd.h>
#include <sys/stat.h>
#include <string.h>

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

int main(int argc, char *argv[]){

    validFormat(argc, argv);

    return 0;

}