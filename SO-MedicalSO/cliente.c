#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include <sys/wait.h>
#include <sys/select.h>
#include <sys/time.h>
#include <errno.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <signal.h>
#include <pthread.h>
#include "cliente.h"

#define TAMNPIPE 50



pthread_t ptid;
int fd_medico, fd_cliente=-1, fd_enviar=-1, fd_balcao=-1;




void buildNamePipe(char* namepipe,int pid){

    sprintf(namepipe,"/tmp/cli_%d", pid);
}
int enviaMsg(int fd, mensagem msgenv){

    int nbytes = write(fd, &msgenv, sizeof(msgenv));
    if(nbytes != sizeof(msgenv)){
        printf("Erro ao enviar. So consegui enviar %d bytes de %d bytes", nbytes, sizeof(msgenv));
 

        return 0;
    }
    return 1;

}




/*=======================================COM SELECT=========================================*/





void trataSinalSelect(int signum, siginfo_t *info, void *secret){

    
    mensagem msg;
    char namepipe[50];
    int nbytes;
    strcpy(msg.nome, "cliente");
    strcpy(msg.mensagem, "sair");
    msg.PID = getpid();
    buildNamePipe(namepipe, msg.PID);
    strcpy(msg.pipe, namepipe);
    msg.pipeaEnviar[0] = '\0';
    
    if(fd_balcao != fd_enviar){  //Esta a falar com o medico
        strcpy(msg.mensagem, "adeus");
        close(fd_balcao);
    }

    if(fd_enviar != -1){ //assim esta preparado para fazer control c antes de comunicar com o balcao
                        //manda para o cliente ou para o balcao, se tiver a falar com o medico ele responsabiliza se de eliminar o cliente
        
        enviaMsg(fd_enviar, msg);
        close(fd_enviar);
    }

    close(fd_cliente);

    unlink(namepipe);
    exit(EXIT_SUCCESS);

}


int trataTeclado(mensagem msgenv){
    int aux = fd_enviar;
    int nbytes;
    int ret = 1;



    fgets(msgenv.mensagem,sizeof(msgenv.mensagem), stdin);
    msgenv.mensagem[strlen(msgenv.mensagem)-1] = '\0'; 

              
    nbytes = write(fd_enviar, &msgenv, sizeof(msgenv));    

    if(nbytes != sizeof(msgenv)){
        printf("Erro ao enviar. So consegui enviar %d bytes de %d bytes\n\n", nbytes, sizeof(msgenv));
        return 0;
    }
        
    
    if(!strcmp(msgenv.mensagem, "sair") && fd_balcao == fd_enviar){
        return 1;          
    }
    if(!strcmp(msgenv.mensagem, "adeus")){  
        return 1;       
    }
    return 0;
    
}



int leitura2(){

        mensagem msgcli;
        int nbytes;

        nbytes = read(fd_cliente, &msgcli, sizeof(msgcli));
        if(nbytes != sizeof(msgcli)){
            printf("Erro de leitura");
            return 0;
        }

        if(strcmp(msgcli.mensagem,"sair")==0){    
            if(!strcmp(msgcli.nome, "balcao")){
                printf("O balcao mandou encerrar\n\n");
            }
            else
                printf("O medico cancelou a consulta\n\n");                
            return 1;
        }
        if(strcmp(msgcli.mensagem,"adeus")==0){
            if(!strcmp(msgcli.nome, "balcao")){
                printf("O balcao vai encerrar\n\n");
            }
            else
                printf("O medico cancelou a consulta\n\n");                
            return 1;
            
        }
        if(strlen(msgcli.pipeaEnviar) > 1){  //Se for enviado um pipe diferente significa que é 
            int fd_aux = open(msgcli.pipeaEnviar, O_WRONLY);                      //para comecar a falar com o medico
            if(fd_aux == -1){
                printf("O medico ficou indisponivel. Tente mais tarde\n\n");
                return 1;
            }
            else {
                fd_balcao = fd_enviar;
                close(fd_enviar); // fecha escrita balcao
                fd_enviar = fd_aux; //Passa para o fd_enviar o do medico
            }
        }
        printf("\n%s -> %s\n",msgcli.nome, msgcli.mensagem); //Impressao de mensagem

        return 0;
}

int main(int argc, char** argv, char** envp){

    setbuf(stdout, NULL);
    char namepipe[TAMNPIPE], str[256], resposta[256], sintoma[50];
    int pid = getpid(), res = 0, nbytes;
    int nfd;
    mensagem msgcli,msgenv;


    fd_set read_fds;
    struct timeval tv;
    
    struct sigaction sa;
	sa.sa_flags = SA_RESTART | SA_SIGINFO;
	sa.sa_sigaction = trataSinalSelect; // corre handler_funcSinal
	sigaction(SIGINT, &sa, NULL);


    if(argc !=2 ){
        fprintf(stderr, "Erro ao executar\nSintaxe: ./cliente nome\n");
        exit(EXIT_SUCCESS);
    }


    /********************Cricao do seu proprio namepipe********************+*/
    buildNamePipe(namepipe, pid);   //Nao está a ser usado por enquanto
    fprintf(stderr,"PID: [%d]\n", getpid());
    //Abre faz mkfifo do seu pipe
    res = mkfifo(namepipe, 0777);

    if(res == -1){
        perror("mkfifo do Cliente com pid falhou\n");
        exit(EXIT_FAILURE);
    }

    //fprintf(stderr,"Fifo do cliente criado");  
    //Abertura do seu pipe
    fd_cliente = open(namepipe, O_RDWR);
    if(fd_cliente == -1){
        perror("\nErro ao abrir o Fifo do cliente\n");
        unlink(namepipe);
        exit(EXIT_FAILURE);
    }


    /**************************Abertura do balcao****************/
    fd_balcao=open(PIPEBALCAO, O_WRONLY | O_NONBLOCK);
    if(fd_balcao == -1){
        fprintf(stderr,"\nO balcao nao esta disponivel, tente mais tarde. Obrigado Sr(a). %s\n", argv[1]);
        close(fd_cliente);
        unlink(namepipe);
        exit(EXIT_FAILURE);
    }


    /**********************Introducao de Sintoma**********************/
    fprintf(stderr,"Bem-vindo exmo(a) %s a MEDICALso\n", argv[1]);
    fprintf(stderr,"Introduza os seus sintomas: ");
    fgets(sintoma, sizeof(sintoma), stdin);
    sintoma[strlen(sintoma)-1]='\0';
    
    fd_enviar = fd_balcao; 

    /*******************Preenchimento da mensagem a enviar***************/
    strcpy(msgenv.nome, argv[1]);
    strcpy(msgenv.mensagem, sintoma);
    strcpy(msgenv.pipe, namepipe);
    msgenv.pipeaEnviar[0]='\0';
    msgenv.PID = getpid();

    /**********************Envio de mensagem com sintoma****************/
    nbytes = write(fd_enviar, &msgenv, sizeof(msgenv));

    if(nbytes != sizeof(msgcli)){
        printf("Erro de envio de mensagem\n");
    }
    

   /**********************Rececao de mensagem com sintoma****************/
    nbytes = read(fd_cliente, &msgcli, sizeof(msgcli));

    if(nbytes != sizeof(msgcli)){
        printf("Erro ao receber a mensagem\n");
    }

    printf("%s -> %s\n", msgcli.nome ,msgcli.mensagem); //Mensagem impresssa;


    /*******************SELECT******************************/

    int x = 1;
   
    while (1)
    {
        if(x)
            printf("-> ");
        tv.tv_sec = 20;
        tv.tv_usec = 0;


        FD_ZERO(&read_fds); //Coloca tudo a 0
        FD_SET(0,&read_fds);  //Ler do stdin
        FD_SET(fd_cliente, &read_fds);  //Ler do pipe dele

        nfd = select(
                    (fd_cliente)+1,
                    &read_fds,
                    NULL,
                    NULL,
                    &tv);


        if(nfd == 0){
            x = 0;
        }            

        if(nfd == -1){
            perror("Erro no select");
            close(fd_cliente);
            unlink(namepipe);


            exit(EXIT_FAILURE);
        }

        if(FD_ISSET(0, &read_fds)){

            if(trataTeclado(msgenv)){
                break;
            }
            x = 1;
        }

        if(FD_ISSET(fd_cliente, &read_fds)){
            if(leitura2()){

                break;
            }
            x = 1;
        }

    }
    close(fd_cliente);
    close(fd_enviar);
    printf("\n\nTerminando.......\n");
    unlink(namepipe);
}   