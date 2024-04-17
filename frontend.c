#include "frontend.h"
#include "users_lib.h"

//novo - criação de pipes e estruturas
#define ADMIN_FIFO "serverPipe"
#define HEARTBEAT_FIFO "heartbeatPipe"
#define USER_FIFO "CLIENTE%d"

char USER_FIFO_F[20];



//Thread que envia o Heartbeat
void *fEnviaHeartbeat (void *loginData){
	user *u = (user *) loginData;
	heartbeat u2;
	int fdHeartbeat;
	int hb = 0;
	int size = 0;


	if(getenv ("HEARTBEAT")  == NULL)
        hb = 10;
    	else
    	hb = atoi(getenv ("HEARTBEAT"));
	

	strcpy(u2.nome,u->nome);
	do{

		fdHeartbeat = open(HEARTBEAT_FIFO, O_RDWR);
		size = write(fdHeartbeat,&u2,sizeof(u2));
		close(fdHeartbeat);	
		sleep(hb);
	}while(u->online == 1);
	pthread_exit(NULL);

}


//Thread para comunicar com o cliente, enviar
void *fEnviaServer (void *loginData){
	user *u = (user *) loginData;
	user u2;
	int fd;
	int verificaC = 0;
	char comando[50];
	char auxComando[50];
	int size = 0;

	//inicializar estrutura
	strcpy(u2.nome,u->nome);
	strcpy(u2.pass,u->pass);
	strcpy(u2.nPipe,u->nPipe);
	strcpy(u2.comando,"\0");
	u2.pid = u->pid;
	u2.saldo = u->saldo;
	u2.online = u->online;
	u2.logout = u->logout;


	do{
		printf("\nIndique o comando: ");
		scanf(" %[^\n]",comando);
		strcpy(auxComando,comando);
		verificaC=validaComandosUser(auxComando);  
		if (verificaC == 0){
				strcpy(u2.comando, comando);
				fd = open(ADMIN_FIFO,O_WRONLY);
				size = write(fd,&u2,sizeof(u2));
				close(fd);
		}
	} while(verificaC!=2 && u->online == 1);
	u->online = 0;
	u2.logout = 1;
	fd = open(ADMIN_FIFO,O_RDWR);
	size = write(fd,&u2,sizeof(u2));
	close(fd);
	pthread_exit(NULL);
}


//Thread para comunicar com o cliente, receber, por fazer
void *fRecebeServer (void *loginData){
	user *u = (user *) loginData;
	int fd;
	int size = 0;
	mensagem m;
	item it[20];


    signal(SIGUSR1, signalHandler);
	do{
		m.resposta = 0;	
		m.saldo = 0;
		m.kick = 0;
		m.time = 0;
		m.mudaSaldo = 0;
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


		fd = open(u->nPipe,O_RDONLY);
		size=read(fd,&m,sizeof(mensagem));
		switch(m.resposta){
			// COMANDOS SELL / BUY
			case 1:
				printf("\n%s\n", m.mensagem);
				break;
			// COMANDOS LIST E PARECIDOS
			case 2:
				for(int i=0;i<m.nitems;i++){
					if(m.it[i].idItem != 0) {
						printf("\nITEM: ID - %d | NOME - %s | CATEGORIA - %s | PRECO ATUAL - %d | PRECO COMPRE JA - %d | VENDEDOR - %s | LICITADOR MAIS ELEVADO - %s\n",m.it[i].idItem,
																		m.it[i].nome,m.it[i].categoria,m.it[i].vlicitacao,m.it[i].vcompreja,m.it[i].clientevenda,m.it[i].clientecompra);
					}
    			}
				break;
			//COMANDO TIME
			case 3:
				printf("\nTempo de execucao plataforma : %d\n", m.time);
				break;
			//COMANDO CASH	
			case 4:
				printf("\nSaldo Atual: %d\n", m.saldo);
				break;
			//COMANDO ADD
			case 5:
				printf("\nExecutado com Sucesso, saldo atual: %d\n", m.saldo);
				break;
			//COMANDO KICK	
			case 6:
				printf("\nUTILIZADOR EXPULSO PELO ADMIN\n");
				u->online = 0;
                break;
            case 7:
                printf("\nMensagem recebida do promotor \n %s\n", m.mensagem);
                break;
		}
	}while(u->online == 1);
	pthread_exit(NULL);

}


int validaComandosUser(char *comando) {	
		

	printf("\n comando no valida comandos :%s\n", comando);

	int numargumento=1,totalargumentos=0;
	char *token=strtok(comando," ");


	if(strcmp(token,"sell")==0) {
		while(token!=NULL){            //percorrer o comando até ao fim
			token=strtok(NULL," ");

			if(token != NULL) {
				if(isdigit(token[0])>0 && numargumento==1)
					printf("\nArgumento %d: Invalido!\n",numargumento);
				else if(isdigit(token[0])>0 && numargumento==2)
					printf("\nArgumento %d: Invalido!\n",numargumento);
				else if(isdigit(token[0])==0 && numargumento==3)
					printf("\nArgumento %d: Invalido!\n",numargumento);
				else if(isdigit(token[0])==0 && numargumento==4)
					printf("\nArgumento %d: Invalido!\n",numargumento);
				else if(isdigit(token[0])==0 && numargumento==5)
					printf("\nArgumento %d: Invalido!\n",numargumento);
				else {
					totalargumentos++;
				}
			}
			numargumento++;
			

		}
		if(totalargumentos==5) {
					printf("\n[SELL] Comando reconhecido!\n");
					return 0;
				}
		else{
			printf("\n[SELL] ERRO NOS ARGUMENTOS! Insira novamente!\n");
			return 1;}	
	}


	else if(strcmp(token,"list")==0){
		while(token!=NULL){            
			token=strtok(NULL," ");

			if(token != NULL) 
				totalargumentos++;
			if(totalargumentos==0){
				printf("\n[LIST] Comando reconhecido!\n");
				return 0;}
			else{
				printf("\n[LIST] ERRO NOS ARGUMENTOS! Insira novamente!\n");
				return 1;}
		}
	}



	else if(strcmp(token,"licat")==0){
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
			printf("\n[LICAT] Comando reconhecido!\n");
			return 0;}

		else{
			printf("\n[LICAT] ERRO NOS ARGUMENTOS!\nInsira novamente!\n");
			return 1;}
	}

	else if(strcmp(token,"lisel")==0){
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
			printf("\n[LISEL] Comando reconhecido!\n");
			return 0;}

		else{
			printf("\n[LISEL] ERRO NOS ARGUMENTOS!\nInsira novamente!\n");
			return 1;}
	}


	else if(strcmp(token,"lival")==0){
		while(token!=NULL) {
			token=strtok(NULL," ");
			if(token!=NULL){
				if(isdigit(token[0])==0 && numargumento==1)
					printf("\nArgumento %d: Invalido!\n",numargumento);
				else {
					totalargumentos++;
				}
			}
			numargumento++;
		}
		if(totalargumentos==1) {
			printf("\n[LIVAL] Comando reconhecido!\n");
			return 0;}

		else{
			printf("\n[LIVAL] ERRO NOS ARGUMENTOS!\nInsira novamente!\n");
			return 1;}
	}

	


	else if(strcmp(token,"litime")==0) {
		while(token!=NULL) {
			token=strtok(NULL," ");
			if(token!=NULL){
				if(isdigit(token[0])==0 && numargumento==1)
					printf("\nArgumento %d: Invalido!\n",numargumento);
				else {
					totalargumentos++;
				}
			}
			numargumento++;
		}
		if(totalargumentos==1) {
			printf("\n[LITIME] Comando reconhecido!\n");
			return 0;}

		else{
			printf("\n[LITIME] ERRO NOS ARGUMENTOS!\nInsira novamente!\n");
			return 1;}
	}
	
	else if(strcmp(token,"time")==0) {
		while(token!=NULL){            
			token=strtok(NULL," ");

			if(token != NULL) 
				totalargumentos++;
			if(totalargumentos==0){
				printf("\n[TIME] Comando reconhecido!\n");
				return 0;}
			else{
				printf("\n[TIME] ERRO NOS ARGUMENTOS! Insira novamente!\n");
				return 1;}
		}
	}
		
	
	else if(strcmp(token,"buy")==0) {
		while(token!=NULL){            
			token=strtok(NULL," ");

			if(token != NULL) {
				if(isdigit(token[0])==0 && numargumento==1)
					printf("\nArgumento %d: Invalido!\n",numargumento);
				else if(isdigit(token[0])==0 && numargumento==2)
					printf("\nArgumento %d: Invalido!\n",numargumento);
				else {
					totalargumentos++;
				}
			}
			numargumento++;
		}
		if(totalargumentos==2){
			printf("\n[BUY] Comando reconhecido!\n");
			return 0;}

		else{
			printf("\n[BUY] ERRO NOS ARGUMENTOS!\nInsira novamente!\n");
			return 1;}
		
	}
		
	
	else if(strcmp(token,"cash")==0) {
		while(token!=NULL){            
			token=strtok(NULL," ");

			if(token != NULL) 
				totalargumentos++;
			if(totalargumentos==0){
				printf("\n[CASH] Comando reconhecido!\n");
				return 0;}
			else{
				printf("\n[CASH] ERRO NOS ARGUMENTOS! Insira novamente!\n");
				return 1;}
		}
	}
		
	else if(strcmp(token,"add")==0) {
		while(token!=NULL) {
			token=strtok(NULL," ");
			if(token!=NULL){
				if(isdigit(token[0])==0 && numargumento==1)
					printf("\nArgumento %d: Invalido!\n",numargumento);
				else {
					totalargumentos++;
				}
			}
			numargumento++;
		}
		if(totalargumentos==1) {
			printf("\n[ADD] Comando reconhecido!\n");
			return 0;}

		else{
			printf("\n[ADD] ERRO NOS ARGUMENTOS!\nInsira novamente!\n");
			return 1;}
	}

	
	else if(strcmp(token,"exit")==0) {
		while(token!=NULL){            
			token=strtok(NULL," ");

			if(token != NULL) 
				totalargumentos++;
			if(totalargumentos==0){
				printf("\n[EXIT] Comando reconhecido!\nA encerrar sessao!\n");
				return 2;}
			else{
				printf("\n[EXIT] ERRO NOS ARGUMENTOS! Insira novamente!\n");
				return 1;}
		}
	}
	
	else
		printf("\nComando invalido! Insira novo comando!\n");
	totalargumentos=0;
	numargumento=1;
	return 0;
	

}

//REFAZER
void recebeUserKick() {
	int fdrecebe;
	mensagem kickUser;
	fdrecebe=open(USER_FIFO,O_RDONLY);
	int size3=read(fdrecebe,&kickUser,sizeof(kickUser));
	if(size3>0){
		if(kickUser.resposta==1){
			printf("\nAdministrador expulsou-o da sessao!\n");
			close(fdrecebe);
			unlink(USER_FIFO);
			exit(0);
		}
	}
}
//REFAZER
void recebeEncerraPlat() {
	mensagem encerraPlat;
	int fdrecebe;

	fdrecebe=open(USER_FIFO,O_RDWR);
	int size4=read(fdrecebe,&encerraPlat,sizeof(encerraPlat));
	if(size4>0){
		if(encerraPlat.resposta==1) {
			printf("\nFoi desconectado automaticamente.\nA encerrar plataforma!\n");
			close(fdrecebe);
			unlink(USER_FIFO);
			exit(0);
		}
	}
}


void signalHandler(int sig)
{
    pthread_exit(NULL);

}


int main(int argc, char **argv) {

	setbuf(stdout,NULL);
	mensagem m;
	user loginData;
	//mensagem userResp; vamos receber a resposta num inteiro so
	char comando[100];
	//int fdenvia, fdrecebe; thread para cada um 
	int fdServer;
	int fdRecebe;
	int verificaU=0,verificaC=0; //verificar utilizador e credenciais
	int size = 0, size2 = 0;
	
	pthread_t enviaServer;
	pthread_t recebeServer;
	pthread_t enviaHeartbeat;
	
		m.resposta = 2031;	
		m.saldo = 0;
		m.kick = 0;

	if(argc!=3) {
		printf("\nErro no numero de argumentos para iniciar sessao!");
		exit(0);
	}	
	
	//Copiar dados para a estrutura para depois enviar
	strcpy(loginData.nome,argv[1]);  
	strcpy(loginData.pass,argv[2]);  
	loginData.pid=getpid();   
	loginData.online = 0;
	loginData.logout = 0;
	strcpy(loginData.comando,"LOGIN"); 

	//Criar o proprio fifo e copiar para a estrutura
	sprintf(USER_FIFO_F,USER_FIFO,getpid());	
	if(mkfifo(USER_FIFO_F,0666)== -1)
		printf("\nErro ao abrir o pipe\n");

	strcpy(loginData.nPipe, USER_FIFO_F);
	//enviar a estrutura para o Servidor 
	fdServer=open(ADMIN_FIFO,O_WRONLY);	
	size=write(fdServer,&loginData,sizeof(user));	
	close(fdServer);

	//Receber a Mensagem de volta (Realocar possivelmente)
	fdRecebe=open(loginData.nPipe,O_RDONLY);
	m.resposta = 0;
	size2=read(fdRecebe,&m,sizeof(mensagem));


	if(m.resposta == 0){
		//Se a verificaçao nao funcionar, encerrar
		printf("\nUtilizador inexistente ou ja tem sessao iniciada!\n\nA encerrar sessao!\n");
		close(fdRecebe);
		unlink(USER_FIFO);
		exit(0);
	}

	else{
		//No caso de dar lançar as threads
		loginData.online = 1;
		close(fdServer);
		printf("\nBem Vindo ao Leilao %s", argv[1]);
		pthread_create(&enviaServer, NULL, fEnviaServer, &loginData);
		pthread_create(&recebeServer, NULL, fRecebeServer, &loginData);
		pthread_create(&enviaHeartbeat, NULL, fEnviaHeartbeat, &loginData);

	}

		pthread_join(recebeServer, NULL);
		pthread_kill(enviaServer, SIGUSR1);
		pthread_kill(enviaHeartbeat, SIGUSR1);
		pthread_join(enviaServer, NULL);
		pthread_join(enviaHeartbeat, NULL);
		unlink(USER_FIFO);
		printf("\nVOU ENCERRAR O USER\n");
		exit(2);
}
