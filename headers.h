#include <stdio.h>
#include <sys/wait.h>
#include <unistd.h>
#include <string.h>
#include <signal.h>
#include <stdlib.h>
#include <time.h>
#include <ctype.h>
#include "users_lib.h"
#include <fcntl.h>
#include <sys/types.h>
#include <unistd.h>
#include <sys/stat.h>
#include <errno.h>
#include <pthread.h>

typedef struct{
    int idItem; 
    char nome[20];
    char categoria[20];
    int vlicitacao;
    int vcompreja;
    int duracao;
    int truepreco; //valor que foi posto a venda
    int duraprom; //duraçao da promoçao
    char clientevenda[20];//Cliente que pos a venda
    char clientecompra[20];//cliente com licitaçao mais elevada

}item;

typedef struct{
    char nome[20];
	char pass[20];
    int pid;
	int saldo;
    int online;  //0 se não estiver e 1 se estiver ligado à plataforma
    int logout;
    char nPipe[30];//assim ja temos o nome do pipe guardado
    char comando [100];//vamos enviar a mensagem pela estrutura cliente
}user;


typedef struct {
    int resposta;
    int time;
    int saldo;
    int mudaSaldo;//saldo
    int nitems;
    int kick;
    item it[30];
    char mensagem[100];
}mensagem;


typedef struct {
    char nome[20];
    int pid;
    int ativo;
    int valorprom;
    int duraprom;
    int promativo;
    char categoria[30];
}promotor;


typedef struct {
    char nome[20];
    int time;
}heartbeat;



typedef struct {
    user* u;
    item* it;
    promotor* p;
    pthread_t *t;    //aponta para a thread dos promotores
    pthread_t *allThreads;      //aponta para as restantes threads
    heartbeat *h;
    int npromotores;
    int usersOnline;
    int nItens;
    int tempoPlataforma;
    int idMaisAlto;
    int encerraPlat;
    pthread_mutex_t *usersTrinco;
    pthread_mutex_t *itemsTrinco;
    pthread_mutex_t *promsTrinco;
    pthread_mutex_t *printTrinco;
    pthread_mutex_t *comTrinco;
    pthread_mutex_t *timeTrinco;
    }tdados;
		


