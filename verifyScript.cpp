#include <iostream>
#include <fstream>
#include <string.h>

using namespace std;

int main(){
    
    ifstream casaDeBanho, cliente;

    casaDeBanho.open("q2.log");
    cliente.open("u2.log");

    if(!casaDeBanho.is_open() || !cliente.is_open()){
        cout << "Erro na abertura dos ficheitos!\n";
        return 1;
    }

    int countReceived = 0, countEnter = 0, count2Late = 0, countTimeUp = 0;
    int countIWant = 0, countIAmIn = 0, countClosed = 0;

    string aux;
    while(getline(casaDeBanho, aux)){
        int ind = aux.find_last_of(";");

        aux = aux.substr(ind+2);

        if(aux == "RECVD")
            countReceived++;
        else if(aux == "ENTER")
            countEnter++;
        else if(aux == "TIMUP")
            countTimeUp++;
        else
            count2Late++;
    }

    while(getline(cliente, aux)){
        int ind = aux.find_last_of(";");

        aux = aux.substr(ind+2);

        if(aux == "IWANT")
            countIWant++;
        else if(aux == "IAMIN")
            countIAmIn++;
        else 
            countClosed++;
    }

    cout << "RECVD: " << countReceived << "\nENTER: " << countEnter << "\nTIMUP: " << countTimeUp << "\n2LATE: " << count2Late << "\nENTER + 2LATE: " << count2Late + countEnter << endl;
    cout << "\nIWANT: " << countIWant << "\nIAMIN: " << countIAmIn << "\nCLOSD: " << countClosed << "\nIAMIN + CLOSD: " << countIAmIn + countClosed << endl;

    casaDeBanho.close();
    cliente.close();

    return 0;
}