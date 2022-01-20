#include "medico.h"
void buildNamePipe(char* namepipe,int pid);
int enviaMsg(int fd, mensagem msgenv);

int fd_cliente = -1, fd_medico = -1, fd_enviar = -1, fd_balcao = -1, sair = 0;



void trataCntrlC(int signum, siginfo_t *info, void *secret){
    mensagem msg;
    char namepipe[50];
    int nbytes;
    strcpy(msg.nome, "medico");
    strcpy(msg.mensagem, "sair");
    msg.PID = getpid();
    buildNamePipe(namepipe, msg.PID);
    strcpy(msg.pipe, namepipe);
    msg.pipeaEnviar[0] = '\0';
    

    enviaMsg(fd_enviar, msg);
 
    
    if(fd_balcao!= fd_enviar){    //Se estiver a falar com o utente 
        enviaMsg(fd_balcao, msg);
        close(fd_balcao);
        close(fd_enviar);
    }

    unlink(namepipe);
    exit(EXIT_SUCCESS);
}


void buildNamePipe(char* namepipe,int pid){

    sprintf(namepipe,"/tmp/esp_%d", pid);
}

int enviaMsg(int fd, mensagem msgenv){

    int nbytes = write(fd, &msgenv, sizeof(msgenv));
    if(nbytes != sizeof(msgenv)){
        printf("Erro ao enviar. So consegui enviar %d bytes de %d bytes", nbytes, sizeof(msgenv));
 

        return 0;
    }

    return 1;

}


/*===========================================SELECT=====================================*/
void* mandaSinal(void* arg);


int leitura2(mensagem msgenviada, mensagem msgrecebida){
    

    char str[256];
    int nbytes;


    nbytes = read(fd_medico, &msgrecebida, sizeof(msgrecebida));
        

    if(strlen(msgrecebida.pipeaEnviar) > 0){
        int fd_teste;
        fd_teste = open(msgrecebida.pipeaEnviar, O_WRONLY);
        if(fd_teste == -1){
            printf("Utente nao esta disponivel");
            return 1;
        }
        //lock
        fd_enviar = fd_teste;
        fd_cliente = fd_enviar;

    }
    if(!strcmp(msgrecebida.mensagem,"sair") && !strcmp(msgrecebida.nome,"balcao")){  //Se lhe disserem sair
        
        sair = 1;

        printf("O balcao mandou encerrar\n");
        return 1;

    }    
    else if(!strcmp(msgrecebida.mensagem,"adeus")){  //Se lhe disserem adeus
        if(strcmp(msgrecebida.nome,"balcao")!= 0 ){  //Se esta a falar com o utente e recebe "adeus", envia para o balcao a dizer que a consulta acabou
            printf("O utente saiu da consulta\n");
            strcpy(msgenviada.mensagem, "adeus");
            enviaMsg(fd_balcao, msgenviada);
            strcpy(msgenviada.mensagem,"utente");
            enviaMsg(fd_balcao, msgenviada);

            close(fd_enviar); //fecha o cliente   
            fd_cliente = -1;
            fd_enviar = fd_balcao;
        }
        else{ //Se for recebida pelo balcao
            if(fd_enviar != fd_balcao){
                
                close(fd_enviar);
                fd_cliente = -1;
                fd_enviar = fd_balcao;
                strcpy(msgenviada.mensagem, "adeus");
                enviaMsg(fd_enviar, msgenviada);
                strcpy(msgenviada.mensagem, "balcao");
                enviaMsg(fd_enviar, msgenviada);
                return 0;
            }
            
        }
            
        }
    else{

        printf("\n%s -> %s\n", msgrecebida.nome, msgrecebida.mensagem);
    }
        
    
    return 0;
}

int escreve2(mensagem msgenviada, mensagem msgrecebida){
    

    fgets(msgenviada.mensagem,sizeof(msgenviada), stdin);
    msgenviada.mensagem[strlen(msgenviada.mensagem)-1] = '\0';

    write(fd_enviar, &msgenviada, sizeof(msgenviada));

    if(!strcmp(msgenviada.mensagem,"sair")){

        if(fd_enviar == fd_cliente){ //Se estiver a conversar com o cliente, envia ao balcao a dizer que vai sair e que acaba a consulta
            write(fd_balcao, &msgenviada, sizeof(msgenviada));
        }

        enviaMsg(fd_medico, msgenviada);
        return 1;
    }

    if(!strcmp(msgenviada.mensagem,"adeus")){ //Se for para acabar a consulta avisa o balcao que é para voltar estar disponivel

        if(fd_enviar == fd_cliente){
            enviaMsg(fd_balcao, msgenviada);
            strcpy(msgenviada.mensagem,"medico");
            enviaMsg(fd_balcao, msgenviada);
            fd_enviar = fd_balcao;  
            close(fd_cliente);
        }

        return 0;         
    }

    return 0;    

}


int main(int argc, char** argv, char** envp){

    setbuf(stdout, NULL);
    char namepipe[TAMNPIPE];
    int pid = getpid(), res = 0, nbytes;
    mensagem msgrecebida, msgenviada;
    struct sigaction sa;
    sa.sa_flags = SA_RESTART | SA_SIGINFO;
    sa.sa_sigaction = trataCntrlC;
    sigaction(SIGINT, &sa, NULL);
    int nfd;
    fd_set read_fds;
    struct timeval tv;
    pthread_t ptid;
    /********************Verifica a o processo foi iniciado correctamente**********/

    if(argc != 3){
        fprintf(stderr, "Erro ao executar\nSintaxe: ./medico nome especialidade\n");
        exit(EXIT_SUCCESS);
    }

    /**********************Mensagem Inicial de rececao*****************************/
    fprintf(stderr,"Bem-vindo Dr(a). %s.\n", argv[1]);
    fprintf(stderr,"Por favor aguarde enquanto lhe atribuimos um utente: \n");


    /********************Cricao do seu proprio namepipe***************************/
    buildNamePipe(namepipe, pid);
    fprintf(stderr,"PID: [%d]\n", pid);
    
    //Abre faz mkfifo do seu pipe
    res = mkfifo(namepipe, 0777);

    if(res == -1){
        perror("Fifo do especialista com pid falhou\n");
        exit(EXIT_FAILURE);
    }


    
    //Abertura do seu pipe
    fd_medico = open(namepipe, O_RDWR);
    if(fd_medico == -1){
        perror("Erro ao abrir o fifo do especialista\n");
        exit(EXIT_FAILURE);
    }
    


    /*******************Preenchimento da mensagem a enviar***************/
    strcpy(msgenviada.nome, argv[1]);
    strcpy(msgenviada.mensagem, argv[2]);
    strcpy(msgenviada.pipe, namepipe);
    msgenviada.pipeaEnviar[0]='\0';
    msgenviada.PID = pid;


    /**************************Abertura do balcao*****************************/
    fd_enviar=open(PIPEBALCAO, O_WRONLY | O_NONBLOCK);
    if(fd_enviar == -1){
        fprintf(stderr,"O balcao nao esta disponivel Dr(a). %s, por favor tente mais tarde.\nObrigado!\n\n", argv[1]);
        unlink(namepipe);
        exit(EXIT_FAILURE);
    }
    fd_balcao = fd_enviar;
    fd_cliente = -1;
    /**********************Envio de mensagem com especialidade****************/
    nbytes = write(fd_enviar, &msgenviada, sizeof(msgenviada));

    if(nbytes != sizeof(msgenviada)){
        printf("Erro de envio de mensagem\n\n");
    }
    

   /**********************Rececao de mensagem com utente a atender************/
    nbytes = read(fd_medico, &msgrecebida, sizeof(msgrecebida));

    if(nbytes != sizeof(msgrecebida)){
        printf("Erro ao receber a mensagem\n\n");
    }


    int x = 1;
    pthread_mutex_t vSair = PTHREAD_MUTEX_INITIALIZER;
    if(pthread_create(&ptid,NULL, mandaSinal, (void*) &vSair)!= 0){
        printf("Erro ao criar thread de sinais de vida\n");
        return 0;
    }

    while (1)
    {   
        
        if(x)
            printf("-> ");

        tv.tv_sec = 20; tv.tv_usec = 0;
        FD_ZERO(&read_fds);
        FD_SET(0, &read_fds);
        FD_SET(fd_medico, &read_fds);

        nfd = select((fd_medico) +1, &read_fds, NULL, NULL,&tv);
        
        if(nfd == -1){
            printf("Erro de Select");
            exit(EXIT_FAILURE);
        }

        if(nfd == 0){
            x = 0;
        }

        if(FD_ISSET(0, &read_fds)){
            if(escreve2(msgenviada, msgrecebida))
            {
                break;
            }
            x = 1;
        }

        if(FD_ISSET(fd_medico, &read_fds)){
            if(leitura2(msgenviada, msgrecebida)){
                break;
            }
            x = 1;
        }
    }
    pthread_mutex_lock(&vSair);
    sair = 1;
    pthread_mutex_unlock(&vSair);
    pthread_join(ptid, NULL);
    pthread_mutex_destroy(&vSair);
    close(fd_medico);
    close(fd_cliente);
    printf("\n\nTerminando.......\n");
    unlink(namepipe);

}

void* mandaSinal(void* arg){

    pthread_mutex_t* vSair = (pthread_mutex_t*) arg;
    int count = 18, total = 20, nbytes;
   
    int fd_alerta = open(PIPEALERTA, O_WRONLY | O_NONBLOCK);

    if(fd_alerta == -1){
        mensagem  msgenv;
        strcpy(msgenv.mensagem,"sair");
        msgenv.pipe[0] = '\0';
        msgenv.pipeaEnviar[0]='\0';
        msgenv.PID = getpid();
        enviaMsg(fd_cliente, msgenv);
        pthread_exit(NULL);
    }
    msgAlerta m;
    m.PID = getpid();
    while (1)
    {
        sleep(2);
        count += 2;
        pthread_mutex_lock(vSair);
        if(sair){
            pthread_mutex_unlock(vSair);
            break;
        }
        pthread_mutex_unlock(vSair);
        if(count == total){
            //printf("Enviou Sinal\n");
            nbytes = write(fd_alerta, &m, sizeof(m));
            if(nbytes != sizeof(m)){
                printf("Sinal de vida enviado com insucesso %d\n", nbytes);
            }
            count = 0;
        }    

    }
    


}
