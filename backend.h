#include "headers.h"

#define PIPE_BACKEND "pipeBackend"

int verificaLogin(tdados *tdata, user *loginDataR, int hb);
void *threadComandosAdmin(void *tdata);
void *threadComunicaClientes(void *tdata);
void *threadLancarPromotores(void *prom);
void *threadRecebeHeartbeat(void *tdata);
int processComandoUser(char* comando);
void executaAdminComCancel(tdados * data, char* comando);
void executaAdminComReprom(tdados* data);
void executaAdminComList(tdados *data);
void executaAdminComUser(tdados* data);
void executaAdminComProms(tdados* data);
void executaAdminComKick(tdados *data, char *comando);
void encerrarPlataforma(tdados * data);
int validaComandosUser(char *comando);
int validaComandosAdmin(char *comando);
int lerFicheiroPromotores(char* promsfile, promotor *p);
int lerFicheiroItens(char* itemsfile, item* c);
void executaUserComList(tdados *data,char * pipe);
void executaUserComLicat(tdados *data,char *comando,char * pipe);
void executaUserComLisel(tdados *data,char *comando,char * pipe);
void executaUserComLival(tdados *data,char *comando,char * pipe);
void executaUserComLitime(tdados *data,char *comando,char * pipe);
void executaUserComSell(tdados *data,char *comando,char * pipe, char *utilizador);
void executaUserComTime(tdados *data,char * pipe);
void executaUserComBuy(tdados *data,char *comando,char * pipe,char *nomeutilizador);
void executaUserComCash(tdados *data,char * pipe,char *nomeutilizador);
void executaUserComAdd(tdados *data,char *comando,char * pipe,char *nomeutilizador);
void cancelPromotores(tdados * data, char* nome);
void copiarProms(tdados *data, promotor *aux, int npromotores);
void comparaProms(tdados *data, promotor *aux, int npromotores, int j);
void criaThreadsPromotores(pthread_t *lancaPromotores, tdados *data, int npromotores);
void reorganizaArrayUsers(tdados *data);
void reorganizaArrayItems(tdados *data);
void reorganizaArrayHeartbeat(tdados *data);
void removeUserlista(tdados *data, char * nome);
int verificaUserArray(tdados *data, char* nome);
void executaAdminComClose(tdados * data);
void saveTimeFile(char *nome,int time);
void saveItensFile(char *nome, item* c,int nitens);
void signalHandler(int sig);
void atualizaPromocao(tdados *data, char* categoria, int valor, int duracao);
void reorganizaArrayProms(tdados *data);