#include "backend.h"
#include "users_lib.h"
#define ADMIN_FIFO "serverPipe"
#define HEARTBEAT_FIFO "heartbeatPipe"
#define USER_FIFO "CLIENTE%d"
char USER_FIFO_F[20];
//int usersOnline = 0;  //Numero de utilizadores online, talvez nao usar como global
pthread_mutex_t readProms;//Mutex para ler dos varios promotores, talvez nao preciso

#pragma region THREADS

void *threadgerePromocao(void *tdata){
    tdados *ptd = (tdados *) tdata;


    do{

        sleep(1);
        
        
    }while(ptd->encerraPlat == 0);
    pthread_exit(NULL);

}

//thread recebe Heartbeats
void *threadRecebeHeartbeat(void *tdata){
    tdados *ptd = (tdados *) tdata;
    int hb = 0;
    int fdRecebe;
    int size = 0;
    heartbeat aux;

    printf("\nENTREI NA THREAD HEARTBEAT\n");


    //tratar da var de ambiente
    if(getenv ("HEARTBEAT")  == NULL){
        hb = 10;
    }

    else{
        hb = atoi(getenv ("HEARTBEAT"));
    }

    
    do{
        fdRecebe = open(HEARTBEAT_FIFO,O_RDONLY);
            if(fdRecebe==-1)
                printf("\nErro\n");
        size = read(fdRecebe,&aux,sizeof(heartbeat));
        pthread_mutex_lock(ptd->usersTrinco);
        for(int i = 0; i<ptd->usersOnline; i++){
            if(strcmp(ptd->h[i].nome,aux.nome)==0){
                ptd->h[i].time = hb;
            }
        }
        pthread_mutex_unlock(ptd->usersTrinco);
    close(fdRecebe);
    }while(ptd->encerraPlat == 0);
    printf("\n Encerrei a Thread recebe Heartbeat\n");
    pthread_exit(NULL);
}

 void *threadGereHeartbeat(void *tdata) {
    tdados *ptd = (tdados *) tdata;
    heartbeat aux;

    signal(SIGUSR1, signalHandler);


    do{ 
        sleep(1);
        for(int i = 0; i<ptd->usersOnline;i++){
            ptd->h[i].time --;
            if(ptd->h[i].time < 0){
                pthread_mutex_lock(ptd->usersTrinco);
                removeUserlista(ptd,ptd->h[i].nome);
                strcpy(ptd->h[i].nome,"\0");
                reorganizaArrayHeartbeat(ptd);
                pthread_mutex_unlock(ptd->usersTrinco);
            }
        }
    }while(ptd->encerraPlat == 0);
    printf("\n Encerrei a Thread GERE Heartbeat\n");
    pthread_exit(NULL);

}
//thread para lançar promotores, voltar a ver sobre terminar o processo filho
void *threadLancarPromotores(void *tdata){
    tdados *ptd = (tdados *) tdata;
    promotor *p;
    mensagem m;
    int fdEnvia;
    char aux[30];

    char str[100];
    char categoria[50];
    int duracao;
    int valor;
    int i = 0;
    


    //pesquisar pelo primeiro promotor que nao esta ativo
    pthread_mutex_lock(ptd->promsTrinco);
    for(int i = 0; i<ptd->npromotores; i++){
        if(ptd->p[i].ativo!=1){
            p = &ptd->p[i];
            ptd->p[i].ativo = 1;
            strcpy(aux,ptd->p[i].nome);
            break;
        }
    }
    pthread_mutex_unlock(ptd->promsTrinco);

    char *token;
    int res,idalvo=0,tam;
    int fd[2];
    int status = 0;
    pid_t wpid;
    if(pipe(fd)==-1){
        printf("\nErro no pipe\n");
        exit(3);
    }
    res=fork();
    if(res == 0)
    {
        close(1);
        dup(fd[1]);
        close(fd[1]);
        close(fd[0]);
        execl(aux,aux,NULL);
        exit(0);
    } else{
        p->pid = res;
        close(fd[1]);
        do{
            printf("\nNome: %s\n", p->nome);
            if(tam=read(fd[0],str,100) > 0){
                printf("%s\n", str);
                 //TODO STRTOK DAS STRINGS OU ASSIM
                token = strtok(str," ");
                strcpy(categoria,token);

                while(token!=NULL){
                    token=strtok(NULL," "); 
                    if(i==0)
                        valor=atoi(token);
                    else if (i==1)
                        duracao=atoi(token);
                    i++;
                }
                m.resposta = 7;
                m.saldo = 0;
                m.kick = 0;
                m.time = 0;
                strcpy(m.mensagem, str);
                for(int i = 0; i<ptd->usersOnline; i++){
                    fdEnvia=open(ptd->u[i].nPipe,O_WRONLY);
                    write(fdEnvia,&m,sizeof (m));
                    close(fdEnvia);
                }
            }
        }while(((wpid = wait(&status)) > 0));
        printf("\nENCERREI THREAD PROMOTORES\n");
        pthread_exit(NULL);
    }
}

//rasco como o crlh mas pronto logo se ve se mudamos
void *threadTempoExecucaoPlataforma(void *tdata){
    tdados *ptd = (tdados *) tdata;
    FILE *r;
    int time = 0;
    printf("\nEntrei na thread tempo\n");

    r=fopen("PlatTimeFile.txt","rt");
    if(r == NULL) {
        printf("\nNao existe tempo guardado anteriormente\n");
    }
    fscanf(r, "%d", &time);
    fclose(r);
    do{
        time ++;
        pthread_mutex_lock(ptd->timeTrinco);
        ptd->tempoPlataforma = time;
        pthread_mutex_unlock(ptd->timeTrinco);

        sleep(1);
    }while(ptd->encerraPlat == 0);

    printf("\nENCERREI THREAD TEMPO\n");
    pthread_exit(NULL);
}

void *threadGereItens(void *tdata){
    tdados* ptd = (tdados *) tdata;
    
    do{
        sleep(1);
        for(int i=0;i<ptd->nItens;){
            pthread_mutex_lock(ptd->itemsTrinco);
            ptd->it[i].duracao--;
            pthread_mutex_unlock(ptd->itemsTrinco);
            if(ptd->it[i].duracao == 0 ){
                if(strcmp(ptd->it[i].clientecompra,"\0")==0){
                    pthread_mutex_lock(ptd->itemsTrinco);
                    strcpy(ptd->it[i].nome,"\0");
                    reorganizaArrayItems(ptd);
                    pthread_mutex_unlock(ptd->itemsTrinco);
                }
                else if(strcmp(ptd->it[i].clientecompra,"\0")!=0){
                    for(int j=0;j<ptd->usersOnline;j++){
                            if(strcmp(ptd->it[i].clientecompra,ptd->u[j].nome)==0){

                            pthread_mutex_lock(ptd->usersTrinco);
                            ptd->u[j].saldo=ptd->u[j].saldo-ptd->it[i].vlicitacao;
                            pthread_mutex_unlock(ptd->usersTrinco);
                            pthread_mutex_lock(ptd->itemsTrinco);
                            strcpy(ptd->it[i].nome,"\0");
                            reorganizaArrayItems(ptd);
                            pthread_mutex_unlock(ptd->itemsTrinco);
                        } 
                    }
                }
            }
            else 
                i++;
        }
    }while(ptd->encerraPlat==0);

    printf("\nENCERREI THREAD ITEMS\n");
    pthread_exit(NULL);
}

void *threadComandosAdmin(void *tdata){
    tdados* pt = (tdados *) tdata;
    int commandType = 0;
    int parar = 0;
    int fd;
    char auxComando[50];
    char comando[50];
    mensagem m;



    char* usersfile;
    union sigval s;

    printf("\nEntrei thread comandos admin\n");

    if(getenv("FUSERS")!= NULL)
        usersfile=getenv("FUSERS");
    else
        strcpy(usersfile,"FUSERS.txt");
    do{

        m.resposta = 0;
        m.saldo = 0;
        m.kick = 0;
        m.time = 0;

        printf("\nInsira um comando Valido: \n");
        scanf(" %[^\n]",comando);

        strcpy(auxComando,comando);
        printf("\n comando: %s \n", comando);
        commandType = validaComandosAdmin(auxComando);

        if(commandType > 0){
            switch (commandType){
                case 1:
                    executaAdminComUser(pt);
                    break;

                case 2:
                    executaAdminComList(pt);
                    break;

                case 3:
                    executaAdminComKick(pt, comando);
                    break;

                case 4:
                    executaAdminComProms(pt);
                    break;

                case 5:
                    executaAdminComReprom(pt);
                    break;

                case 6:
                    executaAdminComCancel(pt,comando);
                    break;

                case 7:
                    executaAdminComClose(pt);
                    printf("\n Encerrei a Thread Admin Coms\n");
                    pthread_exit(NULL);
                    break;
            }
        }
    }while(pt->encerraPlat == 0);

    printf("\nENCERREI THREAD COM ADMIN\n");

}
void *threadComunicaClientes(void *tdata){
    tdados* pt = (tdados *) tdata;
    int verificaU;
    int fdRecebe;
    int fdEnvia;
    int size = 0;
    int size2 = 0;
    int comandType = 0;
    char auxComando[50];
    user loginDataR;
    mensagem m;
    item it;
    printf("\nENTREI THREAD COMUNICA\n");

    int hb = 0;
    if(getenv ("HEARTBEAT")  == NULL){
        hb = 10;
    }

    else{
        hb = atoi(getenv ("HEARTBEAT"));
    }
    
    signal(SIGUSR1, signalHandler);


    do {
        //Inicializar vars e estrutura
        verificaU = 0;
        m.resposta = 0;
        m.saldo = 0;
        m.kick = 0;
        m.time = 0;
        loginDataR.online = -1;
        loginDataR.pid = 0;
        loginDataR.saldo = 0;
        loginDataR.logout = 0;
        strcpy(loginDataR.nome, "\0");
        strcpy(loginDataR.pass, "\0");
        strcpy(loginDataR.nPipe, "\0");
        strcpy(loginDataR.comando, "\0");

        fdRecebe=open(ADMIN_FIFO,O_RDONLY);
            if(fdRecebe==-1)
                printf("\nErro\n");
        size=read(fdRecebe,&loginDataR,sizeof(user));
        printf("\nCOMANDO :%s", loginDataR.comando);
           
        //Verificação de Login ou se é comando
        if(strcmp(loginDataR.comando,"LOGIN")== 0){
            //Verificar se o usar ja esta logado (ta mal feito temos de percorrer no array dos clientes pelo nome)
            if(verificaUserArray(pt,loginDataR.nome) == 0) {
                m.resposta = verificaLogin(pt, &loginDataR, hb);
                fdEnvia=open(loginDataR.nPipe,O_WRONLY);
                size2=write(fdEnvia,&m,sizeof(mensagem));
                close(fdEnvia);
            }
            else{
                m.resposta = 0;
                fdEnvia=open(loginDataR.nPipe,O_WRONLY);
                size2=write(fdEnvia,&m,sizeof(mensagem));
            }
        }
        else if(loginDataR.logout == 1){
            //verificar o logout
            printf("\nUser %s fez logout\n",loginDataR.nome);
            removeUserlista(pt,loginDataR.nome);
        }

        else{
            //Caso de ser um comando
            strcpy(auxComando,loginDataR.comando);
            comandType=processComandoUser(auxComando);
            switch (comandType){

                case 1:
                    executaUserComSell(pt,loginDataR.comando,loginDataR.nPipe, loginDataR.nome);
                    break;

                case 2:
                    executaUserComList(pt,loginDataR.nPipe);
                    break;

                case 3:

                    executaUserComLicat(pt,loginDataR.comando,loginDataR.nPipe);
                    break;

                case 4:
                    executaUserComLisel(pt,loginDataR.comando,loginDataR.nPipe);
                    break;

                case 5:
                    executaUserComLival(pt,loginDataR.comando,loginDataR.nPipe);
                    break;

                case 6:
                    executaUserComLitime(pt,loginDataR.comando,loginDataR.nPipe);
                    break;

                case 7:
                    executaUserComTime(pt,loginDataR.nPipe);
                    break;

                case 8:
                    executaUserComBuy(pt,loginDataR.comando,loginDataR.nPipe,loginDataR.nome);
                    break;

                case 9:
                    executaUserComCash(pt,loginDataR.nPipe, loginDataR.nome);
                    break;

                case 10:
                    executaUserComAdd(pt,loginDataR.comando,loginDataR.nPipe,loginDataR.nome);
                    break;
            }
        }
        close(fdRecebe);
    }while(pt->encerraPlat == 0);
    printf("\nENCERREI THREAD COM USERS\n");
    pthread_exit(NULL);
}
#pragma endregion
int verificaLogin(tdados *tdata, user *loginDataR, int hb){
    int verificaU=0;
    printf("\nHEARTBEAT VALUE = %d", hb);
    //Verificaçao de login, envia-se o verificaU, o user se receber 0 desliga
    verificaU = isUserValid(loginDataR->nome,loginDataR->pass);
    if(verificaU == 1){
        //adicionar ao Array dos clientes
        pthread_mutex_lock(tdata->usersTrinco);
        strcpy(tdata->u[tdata->usersOnline].nome,loginDataR->nome);
        strcpy(tdata->u[tdata->usersOnline].pass,loginDataR->pass);
        strcpy(tdata->u[tdata->usersOnline].nPipe,loginDataR->nPipe);
        tdata->u[tdata->usersOnline].pid = loginDataR->pid;
        tdata->u[tdata->usersOnline].saldo = getUserBalance(loginDataR->nome);
        tdata->u[tdata->usersOnline].online = 1;
        strcpy(tdata->h[tdata->usersOnline].nome,loginDataR->nome);
        tdata->h[tdata->usersOnline].time = hb;
        printf("\nNOME: %s, Heartbeat :%d\n",tdata->h[tdata->usersOnline].nome, tdata->h[tdata->usersOnline].time);
        tdata->usersOnline ++;
        pthread_mutex_unlock(tdata->usersTrinco);
    }
    return verificaU;

}

int verificaUserArray(tdados *data, char* nome){
    if(data->usersOnline >= 20)
        return 1;

    for(int i=0; i<data->usersOnline; i++){
        if(strcmp(nome,data->u[i].nome)==0)
            return 1;
    }

    return 0;
}

#pragma region COMANDOS USER
int processComandoUser(char* comando){

    char *token=strtok(comando," ");
    if(strcmp(token,"sell")==0)
        return 1;
    else if(strcmp(token,"list")==0)
        return 2;
    else if(strcmp(token,"licat")==0)
        return 3;
    else if(strcmp(token,"lisel")==0)
        return 4;
    else if(strcmp(token,"lival")==0)
        return 5;
    else if(strcmp(token,"litime")==0)
        return 6;
    else if(strcmp(token,"time")==0)
        return 7;
    else if(strcmp(token,"buy")==0)
        return 8;
    else if(strcmp(token,"cash")==0)
        return 9;
    else if(strcmp(token,"add")==0)
        return 10;
}

void executaUserComList(tdados *data,char * pipe){
    item itemsPipe[data->nItens];
    int fdenvia;
    int nItems;
    mensagem m;

    pthread_mutex_lock(data->itemsTrinco);
    for(int i=0;i<data->nItens;i++){
        itemsPipe[i]=data->it[i];
    }
    pthread_mutex_unlock(data->itemsTrinco);

    m.resposta = 0;
    m.saldo = 0;
    m.kick = 0;
    m.time = 0;

    for(int j=0;j<data->nItens;j++){
        itemsPipe[j]=data->it[j];
        nItems = j+1;
    }

    pthread_mutex_lock(data->itemsTrinco);
    for(int i=0;i<nItems;i++){
        m.it[i]=itemsPipe[i];
    }
    pthread_mutex_unlock(data->itemsTrinco);
    m.resposta = 2;
    m.nitems = nItems;
    m.nitems =data->nItens;
    fdenvia=open(pipe,O_RDWR);
    int size=write(fdenvia,&m,sizeof(m));
    close(fdenvia);
}

void executaUserComLicat(tdados *data,char *comando,char * pipe) {
    char *token=strtok(comando," ");
    printf("\ntoken :%s\n", token);
    int aux=0;
    int fdenvia;
    int nItems = 0;
    item itemsPipe[30];
    mensagem m;

    token=strtok(NULL," ");
    printf("\ntoken :%s\n", token);

    m.resposta = 0;
    m.saldo = 0;
    m.kick = 0;
    m.time = 0;
    //m.it = NULL;
    for(int i=0;i<20;i++){
        m.it[i].idItem= 0;
        strcpy(m.it[i].nome,"\0");
        strcpy(m.it[i].categoria,"\0");
        m.it[i].vlicitacao = 0;
        m.it[i].vcompreja = 0;
        m.it[i].duracao = 0;
        m.it[i].truepreco = 0;
        m.it[i].duraprom = 0; 
        strcpy(m.it[i].clientevenda,"\0");
        strcpy(m.it[i].clientecompra, "\0");
    }


    pthread_mutex_lock(data->itemsTrinco);

    for(int i=0;i<data->nItens;i++){
            if(strcmp(data->it[i].categoria,token) ==0){
                itemsPipe[nItems]=data->it[i];
                nItems ++;
            }
     }
    
    pthread_mutex_unlock(data->itemsTrinco);
    for(int i=0;i<nItems;i++){
        m.it[i]=itemsPipe[i];
    }

    m.resposta = 2;
    m.nitems = nItems;
    fdenvia=open(pipe,O_RDWR);
    int size=write(fdenvia,&m,sizeof(m));
    close(fdenvia);
}

void executaUserComLisel(tdados *data,char *comando,char * pipe) {
    char *token=strtok(comando," ");
    int fdenvia;
    int nItems = 0;
    item itemsPipe[data->nItens];
    mensagem m;

    m.resposta = 0;
    m.saldo = 0;
    m.kick = 0;
    m.time = 0;

    token=strtok(NULL," ");

    pthread_mutex_lock(data->itemsTrinco);
    for(int i=0;i<data->nItens;i++){
            if(strcmp(data->it[i].clientevenda,token) ==0){
                itemsPipe[nItems]=data->it[i];
                nItems ++;
            }
    }

    for(int i=0;i<nItems;i++){
        m.it[i]=itemsPipe[i];
    }

    pthread_mutex_unlock(data->itemsTrinco);
    m.resposta = 2;
    m.nitems = nItems;
    fdenvia=open(pipe,O_RDWR);
    int size=write(fdenvia,&m,sizeof(m));
    close(fdenvia);
}

void executaUserComLival(tdados *data,char *comando,char * pipe) {
    char *token=strtok(comando," ");
    int valorNum = 0;
    int fdenvia;
    int nItems = 0;
    item itemsPipe[data->nItens];
    mensagem m;

    m.resposta = 0;
    m.saldo = 0;
    m.kick = 0;
    m.time = 0;

    token=strtok(NULL," ");
    valorNum = atoi(token);

    pthread_mutex_lock(data->itemsTrinco);
    for(int j=0;j<data->nItens;j++){
        if(data->it[j].vlicitacao <= valorNum){
            itemsPipe[nItems]=data->it[j];
            nItems ++;
        }

    }

    for(int i=0;i<nItems;i++){
        m.it[i]=itemsPipe[i];
    }

    pthread_mutex_unlock(data->itemsTrinco);
    m.resposta = 2;
    m.nitems = nItems;
    fdenvia=open(pipe,O_RDWR);
    int size=write(fdenvia,&m,sizeof(m));
    close(fdenvia);
}

void executaUserComLitime(tdados *data,char *comando,char * pipe) {
    char *token=strtok(comando," ");
    int tempoNum;
    int fdenvia;
    int nItems = 0;
    item itemsPipe[data->nItens];
    mensagem m;

    printf("\n ENTREI NO LITIME\n");

    m.resposta = 0;
    m.saldo = 0;
    m.kick = 0;
    m.time = 0;

    token=strtok(NULL," ");
    tempoNum = atoi(token);

    pthread_mutex_lock(data->itemsTrinco);
    printf("\nENTREI NO FOR\n");

    for(int j=0;j<data->nItens;j++){
        if(data->it[j].duracao <=tempoNum){
            itemsPipe[nItems]=data->it[j];
            nItems ++;
        }
    }
    pthread_mutex_unlock(data->itemsTrinco);

    printf("\nFORA DO FOR\n");
    for(int i=0;i<nItems;i++){
        m.it[i]=itemsPipe[i];
    }

    m.resposta = 2;
    m.nitems = nItems;
    fdenvia=open(pipe,O_RDWR);
    int size=write(fdenvia,&m,sizeof(m));
    close(fdenvia);
}

void executaUserComSell(tdados *data,char *comando,char * pipe, char *utilizador){
    char *token=strtok(comando," ");
    char nomeIt[50];
    char categoriaIt[50];
    int precoIt = 0, precoCJ = 0, duracaoIt = 0;
    int fdEnvia;
    int aux = 0;
    mensagem m;

    m.resposta = 0;
    m.saldo = 0;
    m.kick = 0;
    m.time = 0;
    m.mudaSaldo = 0;
    strcpy(m.mensagem,"\0");


    token=strtok(NULL," ");

    strcpy(nomeIt,token);
    printf("\nNOME:%s\n", nomeIt);
    token=strtok(NULL," ");

    strcpy(categoriaIt, token);

    token=strtok(NULL," ");

    precoIt = atoi(token);

    token=strtok(NULL," ");

    precoCJ = atoi(token);

    token=strtok(NULL," ");

    duracaoIt = atoi(token);

    if(data->nItens == 20){
        m.resposta = 1;
        printf("\nNumero maximo de itens atingido\n");
        strcpy(m.mensagem,"Numero maximo de itens atingido\n");
        fdEnvia = open(pipe, O_WRONLY);
        write(fdEnvia,&m,sizeof(m));
        close(fdEnvia);
    }
    else{

        pthread_mutex_lock(data->itemsTrinco);
        aux = data->nItens;
        data->it[aux].idItem = 1 + (data->idMaisAlto);
        strcpy(data->it[aux].nome, nomeIt);
        strcpy(data->it[aux].categoria, categoriaIt);
        data->it[aux].vlicitacao = precoIt;
        data->it[aux].vcompreja = precoCJ;
        data->it[aux].duracao = duracaoIt;
        strcpy(data->it[aux].clientevenda, utilizador);
        strcpy(data->it[aux].clientecompra, "\0");

        data->idMaisAlto++;
        data->nItens++;

        pthread_mutex_unlock(data->itemsTrinco);
        printf("\nNITENS:%d\n",data->nItens);
        m.resposta = 1;
        printf("\nItem posto a vendo com sucesso\n");
        strcpy(m.mensagem,"Item posto a vendo com sucesso\n");
        fdEnvia = open(pipe, O_WRONLY);
        write(fdEnvia,&m,sizeof(m));
        close(fdEnvia);

        
    }
}

//aceder ao tempo guardado na plataforma
void executaUserComTime(tdados *data,char * pipe){
    int fdenvia;
    mensagem m;

    m.resposta = 0;
    m.saldo = 0;
    m.kick = 0;
    m.time = 0;

    pthread_mutex_lock(data->timeTrinco);
    m.time = data->tempoPlataforma;

    pthread_mutex_unlock(data->timeTrinco);
    m.resposta = 3;
    fdenvia=open(pipe,O_RDWR);
    int size=write(fdenvia,&m,sizeof(m));
    close(fdenvia);
}


void executaUserComCash(tdados *data,char * pipe,char *nomeutilizador){
    int fdenvia;

    mensagem m;

    m.resposta = 0;
    m.saldo = 0;
    m.kick = 0;
    m.time = 0;


    pthread_mutex_lock(data->usersTrinco);
    for(int i=0;i<data->usersOnline;i++){
        if(strcmp(data->u[i].nome,nomeutilizador)==0)
            m.saldo=data->u[i].saldo;
        break;//ver se é break ou continue
    }
    pthread_mutex_unlock(data->usersTrinco);
    m.resposta = 4;
    fdenvia=open(pipe,O_RDWR);
    int size=write(fdenvia,&m,sizeof(m));
    close(fdenvia);
}

void executaUserComAdd(tdados *data,char *comando,char * pipe,char *nomeutilizador){
    char *token=strtok(comando," ");
    int fdenvia;
    int aux=0;
    char valorAddStr[10];
    int valorAddNum;
    mensagem m;

    m.resposta = 0;
    m.saldo = 0;
    m.kick = 0;
    m.time = 0;

    token=strtok(NULL," ");
    valorAddNum=atoi(token);

    for(int i=0;i<data->usersOnline;i++){
        printf("\nNomeComando %s \n Nome Userlist %s\n Nusers %d",nomeutilizador, data->u[i].nome, data->usersOnline);
        if(strcmp(data->u[i].nome,nomeutilizador)==0){
            pthread_mutex_lock(data->usersTrinco);  
            data->u[i].saldo=data->u[i].saldo+valorAddNum;
            pthread_mutex_unlock(data->usersTrinco);
            m.saldo = data->u[i].saldo;
            printf("\nSALDO DEPOIS DO ADD:%d",m.saldo );
        }
    }
    m.resposta = 5;
    fdenvia=open(pipe,O_RDWR);
    int size=write(fdenvia,&m,sizeof(m));
    close(fdenvia);
    printf("\nSAI DO ADD\n");
}

void executaUserComBuy(tdados *data,char *comando,char * pipe,char *nomeutilizador){
    printf("\nCOMANDO: %s\n", comando);
    
    char *token=strtok(comando," ");
    int idNum,valorNum;
    int aux=0,aux2=0;
    char idStr[10],valorStr[10];
    int fdenvia;
    char stringSaldoNegativo[100]="Saldo negativo! Impossivel realizar pedido!";
    char stringLicitacaoPosta[100]="Licitacao feita!";
    char stringSaldoInfValor[100]="Saldo inferior ao valor proposto! Impossivel realizar pedido!";
    char stringCompraImediata[100]="Efetuou a compra do item pelo valor de compra instantanea! Valor retirado do seu saldo!";
    int i=0;
    int j=0;

    user *comprador = NULL;
    user *vendedor = NULL;
    item *itemsale = NULL;

    mensagem m;
    m.resposta = 0;
    m.saldo = 0;
    m.kick = 0;
    m.time = 0;
    m.mudaSaldo = 0;
    strcpy(m.mensagem,"\0");



    token=strtok(NULL," ");
    idNum=atoi(token);

    token=strtok(NULL," ");
    valorNum=atoi(token);

    for(i;i<data->usersOnline;i++){
        if(strcmp(data->u[i].nome,nomeutilizador)==0){
            comprador = &data->u[i];
            break;
        }
    }

    if(comprador->saldo < 0){
        m.resposta = 1;
        strcpy(m.mensagem,stringSaldoNegativo);
        fdenvia=open(pipe,O_RDWR);
        int size=write(fdenvia,&m,sizeof(m));
        close(fdenvia);
        return;
    }

    //saldo menor que o valor proposto para comprar o item
    if(comprador->saldo < valorNum){
        m.resposta = 1;
        strcpy(m.mensagem,stringSaldoInfValor);
        fdenvia=open(pipe,O_RDWR);
        int size=write(fdenvia,&m,sizeof(m));
        close(fdenvia);
        return;
    }
    else{ 
        //encontrar o item
         for( j;j<data->nItens;j++){
            if(data->it[j].idItem == idNum){
                itemsale = &data->it[j];
                break;
            }   
        }
        //encontrar o cliente que vai receber dinheiro possivelmente
        for(int k = 0; k < data->usersOnline; k ++){
            if(strcmp(itemsale->clientevenda, data->u[k].nome)==0){
                vendedor = &data->u[k];
                break;
            }
        }

    }
        if(valorNum>=itemsale->vcompreja && itemsale->vcompreja !=0){
            //Retirar o saldo do comprador e adicionar ao vendedor

            if(vendedor == NULL){
            m.resposta = 1;
            strcpy(m.mensagem,"\nVendedor nao esta online\n");
            fdenvia=open(pipe,O_RDWR);
            int size=write(fdenvia,&m,sizeof(m));
            close(fdenvia);
            return;
            }

            pthread_mutex_lock(data->usersTrinco);
            comprador->saldo = comprador->saldo - itemsale->vcompreja;
            vendedor->saldo = vendedor->saldo + itemsale->vcompreja;
            pthread_mutex_unlock(data->usersTrinco);

            pthread_mutex_lock(data->itemsTrinco);
            strcpy(itemsale->nome,"\0");
            reorganizaArrayItems(data);
            pthread_mutex_unlock(data->itemsTrinco);


            m.resposta = 1;
            strcpy(m.mensagem,stringCompraImediata);

            fdenvia=open(pipe,O_RDWR);
            int size=write(fdenvia,&m,sizeof(m));
            close(fdenvia);
            return;
        }
        else{
            if(valorNum>itemsale->vlicitacao){
                
                pthread_mutex_lock(data->itemsTrinco);
                itemsale->vlicitacao=valorNum;
                itemsale->truepreco=valorNum;
                strcpy(itemsale->clientecompra,nomeutilizador);
                pthread_mutex_unlock(data->itemsTrinco);
                m.resposta = 1;
                strcpy(m.mensagem,stringLicitacaoPosta);

                fdenvia=open(pipe,O_RDWR);
                int size=write(fdenvia,&m,sizeof(m));
                close(fdenvia);
                return;
            }

            else{
                m.resposta = 1;
                strcpy(m.mensagem,"Valor abaixo do atual preco do item");
                fdenvia=open(pipe,O_RDWR);
                int size=write(fdenvia,&m,sizeof(m));
                close(fdenvia);
            }

        }
    }

#pragma endregion

void saveTimeFile(char *nome,int time){
    FILE *f;
    f = fopen(nome, "w");
    if(f == NULL) {
        printf("\nFicheiro de Items nao encontrado\n");
        return;
    }
    fprintf(f,"%d",time);

}


void saveItensFile(char *nome, item* c, int nitens){
    int i = 0;
    FILE *f;
    f = fopen(nome, "w");
    if(f == NULL) {
        printf("\nFicheiro de Items nao encontrado\n");
        return;
    }

    for(int i = 0; i < nitens; i++ ){
        fprintf(f, "%d %s %s %d %d %d %s %s", c[i].idItem,
                  c[i].nome,c[i].categoria,c[i].vlicitacao,c[i].vcompreja,c[i].duracao,
                  c[i].clientecompra,c[i].clientevenda);
    }        
}

void reorganizaArrayProms(tdados *data){
    promotor aux;
    for (int contador = 0; contador < data->npromotores; ++contador) {
        if(strcmp(data->p[contador].nome,"\0")==0){
            for (int j = contador + 1; j < data->npromotores; ++j) {
                aux = data->p[contador];
                data->p[contador] = data->p[j];
                data->p[j] = aux;
            }
        }
    }
    data->npromotores --;
}


void reorganizaArrayHeartbeat(tdados *data){
    heartbeat aux;
    for (int contador = 0; contador < data->usersOnline; ++contador) {
        if(strcmp(data->h[contador].nome,"\0")==0){
            for (int j = contador + 1; j < data->usersOnline; ++j) {
                aux = data->h[contador];
                data->h[contador] = data->h[j];
                data->h[j] = aux;
            }
        }
    }
}

void reorganizaArrayItems(tdados *data){
    item aux;

    printf("\nENTRE NO REORGANIZA\n");
    for (int contador = 0; contador < data->nItens; ++contador) {
        if(strcmp(data->it[contador].nome,"\0")==0){
            for (int i = contador + 1; i < data->nItens; ++i) {
                aux = data->it[contador];
                data->it[contador] = data->it[i];
                data->it[i] = aux;
            }
        }
    }
    data->nItens--;
}

void reorganizaArrayUsers(tdados *data){
    user aux;


    for (int contador = 0; contador < data->usersOnline; ++contador) {
        if(strcmp(data->u[contador].nome,"\0")==0){
            for (int i = contador + 1; i < data->usersOnline; ++i) {
                aux = data->u[contador];
                data->u[contador] = data->u[i];
                data->u[i] = aux;
            }
        }
    }
}

void removeUserlista(tdados *data, char * nome){
    for(int i = 0; i<data->usersOnline; i++){
        if(strcmp(data->u[i].nome, nome)== 0){
            strcpy(data->u[i].nome,"\0");
        }
    }
    reorganizaArrayUsers(data);
    data->usersOnline --;
}

#pragma region ComandosAdmin
//Validaçao dos Comandos do admin
int validaComandosAdmin(char *comando) {

    int numargumento=1,totalargumentos=0;
    char *token=strtok(comando," ");



    if(strcmp(token,"users")==0) {
        while(token!=NULL){
            token=strtok(NULL," ");
            if(token != NULL)
                totalargumentos++;
            if(totalargumentos==0){
                //printf("\n[USERS] Comando reconhecido!\n");
                //listarUsers();
                return 1;
            }
            else{
                printf("\n[USERS] SINTAXE DO COMANDO : USERS\n");
                return 0;
            }
        }
    }


    else if(strcmp(token,"list")==0) {
        while(token!=NULL){
            token=strtok(NULL," ");

            if(token != NULL)
                totalargumentos++;
            if(totalargumentos==0){
                printf("\n[LIST] Comando reconhecido!\n");
                return 2;}
            else{
                printf("\n[LIST] ERRO NOS ARGUMENTOS! Insira novamente!\n");
                return 0;}
        }
    }


    else if(strcmp(token,"kick")==0) {
        printf("\nENTREI NO IF DO KICK\n");
        while(token!=NULL){
            token=strtok(NULL," ");

            if(token != NULL) {
                if(isdigit(token[0])>0 && numargumento==1)
                    printf("\nArgumento %d: Invalido!\n",numargumento);
                else {
                    totalargumentos++;
                }
            }
            numargumento++;
        }
        if(totalargumentos==1){
            printf("\n[KICK] Comando reconhecido!\n");
            return 3;}

        else{
            printf("\n[KICK] ERRO NOS ARGUMENTOS!\nInsira novamente!\n");
            return 0;}

    }

    else if(strcmp(token,"prom")==0) {
        while(token!=NULL){
            token=strtok(NULL," ");

            if(token != NULL)
                totalargumentos++;
            if(totalargumentos==0){
                printf("\n[PROM] Comando reconhecido!\n");
                return 4;}
            else{
                printf("\n[PROM] ERRO NOS ARGUMENTOS! Insira novamente!\n");
                return 0;}
        }
    }

    else if(strcmp(token,"reprom")==0) {
        while(token!=NULL){
            token=strtok(NULL," ");

            if(token != NULL)
                totalargumentos++;
            if(totalargumentos==0){
                printf("\n[REPROM] Comando reconhecido!\n");
                return 5;}
            else{
                printf("\n[REPROM] ERRO NOS ARGUMENTOS! Insira novamente!\n");
                return 0;}
        }
    }

    else if(strcmp(token,"cancel")==0) {
        while(token!=NULL) {
            token=strtok(NULL," ");
            if(token!=NULL){
                if(isdigit(token[0])>0 && numargumento==1)
                    printf("\nArgumento %d: Invalido!\n",numargumento);
                else {
                    totalargumentos++;
                }
            }
            numargumento++;
        }
        if(totalargumentos==1) {
            printf("\n[CANCEL] Comando reconhecido!\n");
            return 6;}

        else{
            printf("\n[CANCEL] ERRO NOS ARGUMENTOS!\nInsira novamente!\n");
            return 0;}
    }

    else if(strcmp(token,"close")==0) {
        while(token!=NULL){
            token=strtok(NULL," ");

            if(token != NULL)
                totalargumentos++;
            if(totalargumentos==0){
                printf("\n[CLOSE] Comando reconhecido!\nA encerrar plataforma!\n");
                //encerrarPlataforma();
                return 7;}
            else{
                printf("\n[CLOSE] ERRO NOS ARGUMENTOS! Insira novamente!\n");
                return 0;}
        }
    }

    else
        printf("\nComando invalido! Insira novo comando!\n");
    totalargumentos=0;
    numargumento=1;
    return 0;
}

//cancela um promotor, receber o nome  do promotor
void executaAdminComCancel(tdados * data, char* comando){
    char *token=strtok(comando," ");
    char nome[20];
    int aux2=0;  
    promotor aux;
    union sigval s;
    token=strtok(NULL," ");

    strcpy(nome,token);
    pthread_mutex_lock(data->promsTrinco);
    printf("\n N PROMOTORES%d\n", data->npromotores);
    for(int i=0; i<10; i++){
        //printf("\n NOME PROM1:%s\nNOME PROM1%s\n",data->p[i].nome, nome);
        if(strcmp(nome,data->p[i].nome)==0 && strcmp(data->p[i].nome,"\0")!=0){
            //printf("\nPID NO CANCEL :%d",data->p[i].pid);
            if (sigqueue(data->p[i].pid, SIGUSR1, s) == -1) {
                perror("sigqueue");
                return;
            }
            else{
                strcpy(data->p[i].nome, "\0");
                reorganizaArrayProms(data);
            }
        }
    }
    pthread_mutex_unlock(data->promsTrinco);

}
//cancelar promotores na funçao reprom
void cancelPromotores(tdados * data, char* nome){
    promotor aux;
    union sigval s;
    printf("\n N PROMOTORES%d\n", data->npromotores);
    for(int i=0; i<10; i++){
        //printf("\n NOME PROM1:%s\nNOME PROM1%s\n",data->p[i].nome, nome);
        if(strcmp(nome,data->p[i].nome)==0 && strcmp(data->p[i].nome,"\0")!=0){
            //printf("\nPID NO CANCEL :%d",data->p[i].pid);
            if (sigqueue(data->p[i].pid, SIGUSR1, s) == -1) {
                perror("sigqueue");
                return;
            }
        }
    }
    data->npromotores --;
}

//compara os promtores do ficheiro com a lista
void comparaProms(tdados *data, promotor *aux, int npromotores, int j){

    printf("\n npromotores %d\n", npromotores);
    int contador = 0;
    printf("\nvalor j:%d\n", j);
    for(int i=0; i<10; i++){
        for (int l=0; l<10;l++){
            //se nao existir no ficheiro novo
            //printf("\nNOME LISTA :%s   NOME AUX:%s/n",data->p[i].nome,aux[l].nome);
            if(strcmp(data->p[i].nome,aux[l].nome)==0){
                contador = 1;
            }
        }

        if(contador == 0){
            printf("\nENTREI cancel\n");
            //nao existe no ficheiro
            cancelPromotores(data,data->p[i].nome);
            strcpy(data->p[i].nome, "\0");
        }
        contador = 0;
    }
}



//Copia para o array de proms
void copiarProms(tdados *data, promotor *aux, int npromotores){

    int contador = 0;
    for(int i = 0; i<10; i++){
        for(int k = 0; k<10; k++){
            if(strcmp(aux[i].nome,data->p[k].nome)==0 && (strcmp(aux[i].nome,"\0")!=0)){
                contador++;
            }
        }
        if(contador == 0){
            for(int m = 0; m<npromotores; m++){
                if(strcmp(data->p[m].nome,"\0")==0){
                    strcpy(data->p[m].nome,aux[i].nome);
                    data->p[m].ativo = 0;
                    data->p[m].valorprom = 0;
                    data->p[m].duraprom = -1;
                    data->npromotores++;
                    break;
                }
            }
        }
        contador = 0;
    }
}



//Comando Reprom, rever para ver se a lista esta a ser bem atualizada, reencaminhar para o main para relançar as threads
void executaAdminComReprom(tdados* data){
    //promotor aux[10];
    FILE *f;
    int j = 0;
    int contador = 0;
    char * promsfile;
    char nome[50];
    promotor aux[10]={};

    //vars ambiente
    if(getenv("FPROMOTERS")!= NULL)
        promsfile=getenv("FPROMOTERS");
    else
        strcpy(promsfile,"FPROMOTERS.txt");

    //ler ficheiro para estrutura aux
    f = fopen("FPROMOTERS.txt", "r");
    if(f == NULL) {
        printf("\nErro ao abrir o ficheiro");
        return;
    }
    while(fscanf(f, "%s", aux[j].nome) == 1){
        printf ("\nPromotor encontrado :%s", aux[j].nome);
        j++;
    }
    rewind(f);
    fclose(f);
    //comparar promotores no arry com ficheiro
    pthread_mutex_lock(data->promsTrinco);
    data->npromotores = j;
    comparaProms(data,aux,data->npromotores, j);


    //Copiar do aux para a data
    copiarProms(data,aux,j);
    pthread_mutex_unlock(data->promsTrinco);


    //lançar novas threads
    criaThreadsPromotores(data->t, data, data->npromotores);

}
//Imprime Lista de Items
void executaAdminComList(tdados *data){
    pthread_mutex_lock(data->itemsTrinco);

    printf("\nNITENS:%d", data->nItens);
    for(int i=0;i<data->nItens;i++){
        if(data->it[i].idItem != 0) {
            printf("\nITEM: ID - %d | NOME - %s | CATEGORIA - %s | PRECO ATUAL - %d | PRECO COMPRE JA - %d | VENDEDOR - %s | LICITADOR MAIS ELEVADO - %s\n",data->it[i].idItem,
                   data->it[i].nome,data->it[i].categoria,data->it[i].vlicitacao,data->it[i].vcompreja,data->it[i].clientevenda,data->it[i].clientecompra);
        }
    }
    pthread_mutex_unlock(data->itemsTrinco);

}

//imprime lista de utilizadores ativos
void executaAdminComUser(tdados* data){

    //pthread_mutex_lock(data->usersTrinco);
    for(int i = 0; i<data->usersOnline; i++){
        printf("\nNome do Utilizador :%s\n", data->u[i].nome);
    }
    //pthread_mutex_unlock(data->usersTrinco);

}
//Imprima lista de promotores, verificar a chamda da funçao adicionar o inteiro npromotores
void executaAdminComProms(tdados* data){

    pthread_mutex_lock(data->promsTrinco);
    for(int i = 0; i<data->npromotores; i++){
        printf("\nNome do Promotor :%s\n", data->p[i].nome);
    }
    pthread_mutex_unlock(data->promsTrinco);
}

//Remove um utilizador do array de utilizadores voltar a este comando por causa da comunicaçao 
void executaAdminComKick(tdados *data, char *comando) {
    char *token=strtok(comando," ");
    mensagem m;
    int fdEnvia;
    int size = 0;
    int aux = 0;
    char username[20];


    token=strtok(NULL," ");


    m.resposta = 0;
    m.saldo = 0;
    m.kick = 0;
    m.time = 0;
    for (int i = 0; i < data->usersOnline; ++i) {
        if(strcmp(token,data->u[i].nome)==0){
            m.resposta = 6;
            fdEnvia=open(data->u[i].nPipe,O_RDWR);
            int size=write(fdEnvia,&m,sizeof(m));
            close(fdEnvia);
            removeUserlista(data, token);
            aux = 1;
        }
    }
}

//Envia mensagems aos utilizadores que a plataforma vai encerrar, talvez enviar um sinal as threads para encerrar e terminar processos, also terminar o loop no mains
void executaAdminComClose(tdados * data){
    int fd;
    mensagem m;
    union sigval s;

    m.resposta = 0;
    m.saldo = 0;
    m.kick = 0;
    m.time = 0;
    m.resposta = 6;

    data->encerraPlat=1;    

    printf("\nEntrei no close\n");


    for(int i = 0; i < data->usersOnline; i++){
        fd = open(data->u[i].nPipe,O_WRONLY);
        write(fd,&m,sizeof(mensagem));
        close(fd);
    }

    for(int i=0; i<data->npromotores; i++){
        sigqueue(data->p[i].pid,SIGUSR1,s);
    }
    //saveUsersFile(usersfile);
    //guardFicheiroItens(data);

}
#pragma endregion ComandoAdmin

void signalHandler(int sig)
{
    pthread_exit(NULL);

}

#pragma region FICHEIROS
//Recebe o ficheiro dos promotores, lê e adiciona ao array,
int lerFicheiroPromotores(char* promsfile, promotor *p) {
    int i=0;
    FILE *f;

    f = fopen(promsfile, "r");
    if(f == NULL) {
        printf("\nErro ao abrir o ficheiro");
        return -1;
    }
    while(fscanf(f, "%s", p[i].nome) == 1){
        printf ("\nPromotor encontrado :%s", p[i].nome);
        p[i].ativo = 0;
        p[i].duraprom = -1;
        p[i].valorprom = 0;
        i++;
    }
    rewind(f);

    fclose(f);
    return i;
}


//Recebe o ficheiro de intens, lê adiciona ao array
int lerFicheiroItens(char* itemsfile, item* c) {
    int i=0;
    FILE *f;

    f = fopen(itemsfile, "rt");
    if(f == NULL) {
        printf("\nFicheiro de Items nao encontrado\n");
        return -1;
    }
    while(fscanf(f, "%d %s %s %d %d %d %s %s", &c[i].idItem,
                 c[i].nome,c[i].categoria,&c[i].vlicitacao,&c[i].vcompreja,&c[i].duracao,
                 c[i].clientecompra,c[i].clientevenda)== 8){
                c[i].truepreco = c[i].vlicitacao;
                c[i].duraprom = -1;
                
                
                /*printf("\n%d %s %s %d %d %d %s %s %d %d\n", c[i].idItem,
                 c[i].nome,c[i].categoria,c[i].vlicitacao,c[i].vcompreja,c[i].duracao,
                 c[i].clientecompra,c[i].clientevenda, c[i].truepreco, c[i].duraprom);    
                 */     
                 i++;       
                 }



    return i;
}

#pragma endregion




void criaThreadsPromotores(pthread_t *lancaPromotores, tdados *data, int npromotores) {
    for (int i = 0; i < npromotores; i++) {
        if (data->p[i].ativo == 0 && strcmp(data->p[i].nome,"\0")!=0) {
            pthread_create(&lancaPromotores[i], NULL, threadLancarPromotores, data);
        }
    }
}


int main(int argc, char **argv) {

    setbuf(stdout,NULL);

    //estruturas essenciais
    user utilizadores[20];
    item itens [30];
    promotor prom [10];
    tdados tdata;
    heartbeat lista[20];
    tdata.usersOnline = 0;
    tdata.idMaisAlto = 0;


    //char username[20], password[20];
    char comando[100], str[100];
    int opcao=0;
    int fileUsers = 0;
    //int verificaU=0; //verificar utilizadores
    int verificaC=0; //verificar comandos nao esta a ser usado
    int verificaAS=0; //verificar atualização do saldo, nao esta a ser usado
    int verificaP=0; //nao esta a ser usado
    int res, saldo, controlo;
    int fdRecebe,fdEnvia;  //novo
    int npromotores; //numero de promtores lidos no ficheiro, usado nas threads
    int verificaThreadJoin;
    int idMaisAlto=0;




    //Verificar que o backend foi lançado corretamente
    if(argc!=1) {
        printf("\nErro no numero do argumentos!");
        exit(0);
    }

    //verifica se o admin já está a correr
    if(mkfifo(ADMIN_FIFO,0666)==-1) {
        if(errno==EEXIST){
            printf("\nFIFO ja existe\n");
            return 1;
        }
        else
            printf("\nErro ao abrir fifo\n");
        return 1;
    }

    if(mkfifo(HEARTBEAT_FIFO,0666)==-1) {
        /*if(errno==EEXIST){
            printf("\nFIFO ja existe\n");
            return 1;
        }*/
        printf("\nErro ao abrir fifo\n");
        return 1;
    }


    //VARS AMBIENTE
    int hb;
    char *promsfile;
    char* itemsfile;
    char* usersfile;
    if(getenv ("HEARTBEAT")  == NULL)
        hb = 10;
    else
        hb = atoi(getenv ("HEARTBEAT"));

    if(getenv("FPROMOTERS")!= NULL)
        promsfile=getenv("FPROMOTERS");
    else
        strcpy(promsfile,"FPROMOTERS.txt");

    if(getenv("FITEMS")!= NULL)
        itemsfile=getenv("FITEMS");
    else
        strcpy(itemsfile,"FITEMS.txt");

    if(getenv("FUSERS")!= NULL)
        usersfile=getenv("FUSERS");
    else
        strcpy(usersfile,"FUSERS.txt");


    //Ler Ficheiro Promotores
    tdata.npromotores = lerFicheiroPromotores(promsfile, prom);


    //Ler Ficheiro Items
    tdata.nItens=lerFicheiroItens(itemsfile, itens);
    printf("\nTDATA NITENS: %d\n",tdata.nItens);

    

    //THREADS
    pthread_t lancaPromotores[10];//thread lançamento promotores
    pthread_t comunicaClientes; //Lida com a leitura de comandos e verificaçao login
    pthread_t comandosAdmin;
    pthread_t gereTempoPlataforma;
    pthread_t gereItens;
    pthread_t recebeHeartbeat;
    pthread_t GereHeartbeat;
    pthread_t gerePromocao;
    pthread_t allThreads[5];


    printf("\n\n\nBem Vindo!\nA executar Administrador!\n\n\n");


    //Carregar o ficheiro de Utilizadores, usado para verificar o login
    fileUsers = loadUsersFile(usersfile);
    if(fileUsers==0)
        printf("\nPlataforma sem utilizadores registados!\n\n\n");
    else if(fileUsers==-1) {
        printf("\nErro na leitura do ficheiro dos utilizadores!\n\n\n");
    }
    else
        printf("Numero de Utilizadores lidos :%d\n ", fileUsers);


    
    //Guardar ids das threads para poder terminar;
    allThreads[0] = comunicaClientes;
    allThreads[1] = gereTempoPlataforma;
    allThreads[2] = gereItens;
    allThreads[3] = recebeHeartbeat;
    allThreads[4] = GereHeartbeat;

    printf("\nCheguei aqui\n");

    //Inicializar a estrutura principal
    tdata.u = utilizadores;
    tdata.it = itens;
    tdata.p = prom;
    tdata.t = lancaPromotores;
    tdata.h = lista;
    tdata.allThreads = allThreads;

    //declarar e iniciar mutex
    pthread_mutex_t usersTrinco;
    pthread_mutex_t itemsTrinco;
    pthread_mutex_t promsTrinco;
    pthread_mutex_t printTrinco;
    pthread_mutex_t timeTrinco;
    pthread_mutex_init(&usersTrinco, NULL);
    pthread_mutex_init(&itemsTrinco, NULL);
    pthread_mutex_init(&promsTrinco, NULL);
    pthread_mutex_init(&printTrinco, NULL);
    pthread_mutex_init(&timeTrinco, NULL);

    //ligaçao do mutex
    tdata.usersTrinco = &usersTrinco;
    tdata.itemsTrinco = &itemsTrinco;
    tdata.usersTrinco = &usersTrinco;
    tdata.promsTrinco = &promsTrinco;
    tdata.timeTrinco = &timeTrinco;



    //Encontrar o ID mais alto
    for(int i = 0; i<tdata.nItens; i++){
        if(tdata.it[i].idItem > idMaisAlto)
        idMaisAlto = tdata.it[i].idItem;
    }
    tdata.idMaisAlto = idMaisAlto;


    //Lançar promotores
    criaThreadsPromotores(tdata.t, &tdata, tdata.npromotores);


    //Lançar as threads
    pthread_create(&gereTempoPlataforma, NULL,threadTempoExecucaoPlataforma, &tdata);
    pthread_create(&comunicaClientes, NULL, threadComunicaClientes, &tdata);
    pthread_create(&comandosAdmin, NULL, threadComandosAdmin, &tdata);
    pthread_create(&gereItens, NULL, threadGereItens, &tdata);
    pthread_create(&recebeHeartbeat, NULL, threadRecebeHeartbeat, &tdata);
    pthread_create(&GereHeartbeat, NULL, threadGereHeartbeat, &tdata);
    pthread_create(&gerePromocao, NULL, threadgerePromocao, &tdata);






    //Esperar que as Threads terminem
    for(int i = 0; i < tdata.npromotores; i++){
        pthread_join(lancaPromotores[i], NULL);
    }


    //Esperar que as threads terminem

    verificaThreadJoin = pthread_join(comandosAdmin, NULL);
    pthread_kill(comunicaClientes, SIGUSR1);
    pthread_kill(recebeHeartbeat, SIGUSR1 );

    verificaThreadJoin = pthread_join(comunicaClientes,NULL);
    verificaThreadJoin = pthread_join(gereTempoPlataforma, NULL);
    verificaThreadJoin = pthread_join(gereItens, NULL);
    verificaThreadJoin = pthread_join(recebeHeartbeat, NULL);
    verificaThreadJoin = pthread_join(GereHeartbeat, NULL);
    verificaThreadJoin = pthread_join(gerePromocao, NULL);

    //atualizar biblioteca e guardar no ficheiro
    for (int i = 0; i < tdata.usersOnline; ++i) {
        updateUserBalance( tdata.u[i].nome, tdata.u[i].saldo);
    }
    //Guardar no ficheiro
    saveUsersFile(usersfile);
    saveItensFile(itemsfile, tdata.it, tdata.nItens);
    saveTimeFile("PlatTimeFile.txt", tdata.tempoPlataforma);

    printf("\n ENCERREI A PLATAFORMA\n");

    //Destruir mutex
    pthread_mutex_destroy(&usersTrinco);
    pthread_mutex_destroy(&itemsTrinco);
    pthread_mutex_destroy(&promsTrinco);
    pthread_mutex_destroy(&printTrinco);
    pthread_mutex_destroy(&timeTrinco);
    //destruir fifo
    unlink(ADMIN_FIFO);
    unlink(HEARTBEAT_FIFO);
    return 1;
}
