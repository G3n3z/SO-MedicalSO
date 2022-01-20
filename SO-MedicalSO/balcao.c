#include <stdio.h>
#include "balcao.h"
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/wait.h>
#include <string.h>
#include <sys/time.h>
#include <errno.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <signal.h>
#include <pthread.h>



int qmaxclientes, qmaxmedicos;


int sair = 0;

pthread_mutex_t* mControl;

void* thrd_func(void* arg); //Prototipo da funcao

void* alarmePrint(void* arg){
    sthread *t = (sthread *) arg; 
    int alr;
    int count = 0;
    while (1)
    {
        pthread_mutex_lock(t->al);
        alr = *t->alarme;
        pthread_mutex_unlock(t->al);
        sleep(1);
        ++count;
        pthread_mutex_lock(t->vSair);
        if(*t->sair){
            pthread_mutex_unlock(t->vSair);
            break;
        }
        pthread_mutex_unlock(t->vSair);

        if(count ==alr || count > alr){
            pthread_mutex_lock(t->listas);
            imprimeListaUtentesPorEspecialidade(*t->listU);
            pthread_mutex_unlock(t->listas);
            count = 0;
        }    
            
    }
    

}

void *recebeAlertas(void* arg){
    structTimes *t = (structTimes *) arg; 
    msgAlerta al;
    int res, fd_alerta, nbytes, aux;


    res = mkfifo(PIPEALERTA, 0777);
    if(res == -1){
        printf("Erro ao criar o pipe de alertas\n PIPEALERTA renicializado\n");
        if(errno == EEXIST){
            unlink(PIPEALERTA);
            res = mkfifo(PIPEALERTA, 0777);
            if(res == -1){
              union sigval s;
              sigqueue(getpid(),SIGINT, s);
              pthread_exit(NULL);  
            }
                
        }
        else
            pthread_exit(NULL);
    }

    fd_alerta = open(PIPEALERTA, O_RDWR);
    if(fd_alerta == -1){
        printf("Erro ao abrir o pipe de alertas");
    }
    t->s = NULL;
    while(1){
        
        nbytes = read(fd_alerta, &al, sizeof(al));
        
        if(nbytes !=  sizeof(al)){
            printf("Mensagem recebida incorretamente");
            continue;
        }

        pthread_mutex_lock(t->vSair);
        if(*t->sair){
            pthread_mutex_unlock(t->vSair);
            break;
        }
        pthread_mutex_unlock(t->vSair); 

        pthread_mutex_lock(t->vSinal);
        aux = verificaExistenciaEAtualiza(t->s, al.PID);
        pthread_mutex_unlock(t->vSinal);



        if(aux){  //Atualiza contador se ja existir;
            pthread_mutex_lock(t->vSinal);
            t->s = adicionaVecSinal(t->s, al);
            pthread_mutex_unlock(t->vSinal);
        }

    }

    close(fd_alerta);
    unlink(PIPEALERTA);
}

void *verificaAlertas(void* arg){
    structTimes *t = (structTimes *) arg; 
    int total = 22, count = 0;


    while (1)
    {
        sleep(2);
        pthread_mutex_lock(t->vSair);
        if(*t->sair){
            pthread_mutex_unlock(t->vSair);
            break;
        }
        pthread_mutex_unlock(t->vSair);


        pthread_mutex_lock(t->vSinal);
        t->s = verificaTempoAlerta(t->s, t->listas, t->listE, t->listU);
        pthread_mutex_unlock(t->vSinal);
        
    }
    pthread_mutex_lock(t->vSinal);
    t->s = libertaSinais(t->s);
    pthread_mutex_unlock(t->vSinal);

}




void trataControlC(int signum,siginfo_t *info, void *secret){

    pthread_mutex_lock(mControl);
    sair = 1;
    pthread_mutex_unlock(mControl);
    close(0);
    mensagem  msgenv;
    strcpy(msgenv.nome, "balcao");
    strcpy(msgenv.mensagem, "sair");
    msgenv.PID = getpid();
    enviaMsg(PIPEBALCAO, msgenv);
    
}

void retiraEspUrgencia(char* msg, char* especialidade, int* urgencia){
    char* aux, *aux2;

    aux = strtok(msg, " ");
    strcpy(especialidade, aux);
    aux = strtok(NULL, " ");
    *urgencia = atoi(aux);


}


void trataSigPIPE(int signum,siginfo_t *info, void *secret){
    return;
}


int main(int argc, char* argv[], char* envp[]){

    int id,x;
    int i = 0, classificadortobalcao[2], nbytes, balcaotoclassificador[2], pstoFather[2];
    char arg0[50], received[256];
    char sintoma[500], str[256];

    char *maxclientes = getenv("MAXCLIENTES");
    char *maxmedicos = getenv("MAXMEDICOS");

    if(maxmedicos == NULL || maxclientes == NULL){
                printf("Erro nas variaveis de ambiente MAXCLIENTES e/ou MAXMEDICOS\n");
                return 0;
            }
        qmaxclientes = atoi(maxclientes);
        qmaxmedicos = atoi(maxmedicos);

        printf("MAXCLIENTES: %d\nMAXMEDICOS: %d\n", qmaxclientes,qmaxmedicos);

        if(qmaxclientes <= 0 || qmaxmedicos <= 0){
            printf("Variavel de ambiente indicada com valor incorreto");
            return 0;
        }



    if(mkfifo(PIPEBALCAO, 0777) == -1){
            
        if(errno == EEXIST){
            printf("Servidor em execucao\n");
            exit(EXIT_FAILURE);
        }
        perror("Erro ao criar o pipe do proprio balcao\n");
        exit(EXIT_FAILURE);
    }

        int pid = getpid();


        pipe(classificadortobalcao);
        pipe(balcaotoclassificador);

        id=fork();
        if (id == 0)
        {
            close(0); //leitura
            dup(balcaotoclassificador[0]);
            close(balcaotoclassificador[0]);
            close(balcaotoclassificador[1]);


            close(1); //escrita
            dup(classificadortobalcao[1]);
            close(classificadortobalcao[0]);
            close(classificadortobalcao[1]);    
        
            execl("./classificador", "classificador",NULL); 
            union sigval sa;
            sleep(1);
            sigqueue(pid, SIGINT, sa);

            return 1;          

       }
        else{ //pai
            close(balcaotoclassificador[0]);
            close(classificadortobalcao[1]);
            struct sigaction sa;
            sa.sa_flags = SA_RESTART | SA_SIGINFO;
            sa.sa_sigaction = trataControlC;
            sigaction(SIGINT, &sa, NULL);
            
            struct sigaction sa1;
            sa1.sa_flags = SA_RESTART | SA_SIGINFO;
            sa1.sa_sigaction = trataSigPIPE;
            sigaction(SIGPIPE, &sa1, NULL);
            
            int alarme = 30;
            int fd_balcao = open(PIPEBALCAO, O_RDWR);
            pUtente listU = NULL;
            pEspecialista listE = NULL;

            if(fd_balcao == -1){
                perror("Erro ao abrir o name pipe do balcao\n");
                
                nbytes = write(balcaotoclassificador[1], "#fim\n", strlen("#fim\n"));
                if(nbytes != strlen("#fim\n")){
                    printf("Classificador nao foi terminado  %d - %d\n", nbytes, strlen("#fim\n"));
                }
                exit(EXIT_FAILURE);
            }
            pthread_t ptid, ptidAlarm, ptidRAlerta, ptidVAlerta;
            pthread_mutex_t listas = PTHREAD_MUTEX_INITIALIZER;
            pthread_mutex_t vSair = PTHREAD_MUTEX_INITIALIZER;
            pthread_mutex_t vAlarme = PTHREAD_MUTEX_INITIALIZER;
            pthread_mutex_t vSinal = PTHREAD_MUTEX_INITIALIZER;
            sthread t;
            mControl = &vSair;
            t.balcaotoclassificador1 = balcaotoclassificador[1];
            t.classificadortobalcao0 = classificadortobalcao[0];
            t.listas = &listas;
            t.vSair = &vSair;
            t.al = &vAlarme;
            t.fd_balcao = fd_balcao;
            t.sair = &sair;
            t.alarme = &alarme;
            t.listU = &listU;
            t.listE = &listE;
            
            structTimes stimes;
            stimes.listas = &listas;
            stimes.vSair = &vSair;
            stimes.vSinal = &vSinal;
            stimes.sair = &sair;
            stimes.listE = &listE;
            stimes.listU = &listU;
            

            if(pthread_create(&ptid, NULL, thrd_func, (void*) &t) != 0){
                printf("Erro a criar a thread de leitura\n");
                exit(1);
            }
            if(pthread_create(&ptidAlarm, NULL, alarmePrint, (void*) &t) != 0){
                printf("Erro a criar a thread de Alarme\n");
                union sigval s;
                sigqueue(getpid(), SIGINT, s);

            }
            if(pthread_create(&ptidRAlerta, NULL,recebeAlertas , (void*) &stimes) != 0){
                printf("Erro a criar a thread de receber sinais de vida do medico\n");
                union sigval s;
                sigqueue(getpid(), SIGINT, s);
            }
            if(pthread_create(&ptidVAlerta, NULL,verificaAlertas , (void*) &stimes) != 0){
                printf("Erro a criar a thread de verificar os sinais de vida\n");
                union sigval s;
                sigqueue(getpid(), SIGINT, s);
            }


            char nome[50], especialidade[50];

            int testC = 0, urgencia;
            

            setbuf(stdout, NULL);
            while(testC != 2 && !sair){

                imprimeComandos();


                fgets(sintoma,sizeof(sintoma)-1, stdin);
                sintoma[strlen(sintoma)]='\0';
                pthread_mutex_lock(&vSair); 
                if(sair){

                    pthread_mutex_unlock(&vSair); 
                    break;
                }
                pthread_mutex_unlock(&vSair);

                
                testC = funcTesteCommand(sintoma, &listU, &listE, &alarme, &t, &stimes); 
                if(testC == 0)
                    printf("Comando Invalido\n");

            }

            if(testC == 2){
                pthread_mutex_lock(&vSair);
                sair = 1;
                pthread_mutex_unlock(&vSair);
            }
            

            
            pthread_join(ptid, NULL);
            pthread_join(ptidAlarm, NULL);
            pthread_join(ptidRAlerta, NULL);
            pthread_join(ptidVAlerta, NULL);

            pthread_mutex_destroy(&listas);
            pthread_mutex_destroy(&vSinal);
            pthread_mutex_destroy(&vSair);
            pthread_mutex_destroy(&vAlarme);


            close(fd_balcao);
            unlink(PIPEBALCAO);
            libertaUtentes(listU);
            libertaMedicos(listE);
            fflush(stdout);

        }

    

    return 0;
}


void* thrd_func(void* arg){

    pUtente u;
    pEspecialista e;
    mensagem msg, msgenv;
    int nbytes, res, urgencia;
    strcpy(msgenv.nome, "balcao");
    strcpy(msgenv.pipe, PIPEBALCAO);
    msgenv.pipeaEnviar[0]='\0';
    msgenv.PID = getpid();
    char especialidadeObtida[256], especialidade[50];

    sthread *t = (sthread *) arg; 
    int x;


    while(1){
        
        printf("\nEsperando...\n\n");
        nbytes = read(t->fd_balcao, &msg, sizeof(msg));
        
        
        if(nbytes != sizeof(msg)){
            printf("Mensagem recebida com problemas");
            continue;
        }


        if(msg.PID == getpid()){
            printf("\nVamos encerrar");


            char fim[8] = "#fim\n";
            nbytes = write(t->balcaotoclassificador1, fim, strlen(fim));


            pthread_mutex_lock(t->listas);
            pEspecialista eAux = *t->listE;
            pUtente uAux = *t->listU;
            strcpy(msgenv.mensagem, "sair");
            while(eAux != NULL){
                enviaMsg(eAux->nPIPE,msgenv);
                eAux = eAux->prox;
            }
            while(uAux != NULL){
                enviaMsg(uAux->nPIPE,msgenv);
                uAux= uAux->prox;
            }
            pthread_mutex_unlock(t->listas);
            
            
            int fd_alerta = open(PIPEALERTA, O_WRONLY | O_NONBLOCK);
            if(fd_alerta != -1){
                msgAlerta m;
                m.PID = 000;
                nbytes = write(fd_alerta, &m, sizeof(m));
                if(nbytes != sizeof(m)){
                    printf("\nMensagem enviada para alerta incorretamente\n");
                }
            }
            break;



        }
            

        if(!strncmp(msg.pipe, "/tmp/cli", 8) ){
            

            //verifica se é um cliente novo
            pthread_mutex_lock(t->listas);
            x = verificaseExiteUtente(*t->listU, msg.PID);
            pthread_mutex_unlock(t->listas);

            if( x == 0){ //Ainda nao existe 
                
                pthread_mutex_lock(t->listas);
                x = verifcaMaxUtentes(*t->listU, qmaxclientes);
                pthread_mutex_unlock(t->listas);

                if(x){

                    if(!strcmp(msg.mensagem, "sair") || !strcmp(msg.mensagem, "adeus")){
                        continue;
                    } 
                    msg.mensagem[strlen(msg.mensagem)] = '\n';
                    msg.mensagem[strlen(msg.mensagem)] = '\0';

                    /*========================Envia os sintomas ao classificador*/
                    nbytes = write(t->balcaotoclassificador1, msg.mensagem,strlen(msg.mensagem)); //Escreve                   
                    nbytes=read(t->classificadortobalcao0, especialidadeObtida, sizeof(especialidadeObtida));  //Le do classificador                  
                        especialidadeObtida[nbytes-1] = '\0';
                        printf("\nEspecialidade: %s\n", especialidadeObtida);

                    
                    retiraEspUrgencia(especialidadeObtida, especialidade, &urgencia);

                    /*====================== Verifica se aceita especialidade ================*/
                     pthread_mutex_lock(t->listas);
                     x = verificaTamPorEspecialidade(*t->listU, especialidade);
                     pthread_mutex_unlock(t->listas);
                    if(!x){

                    /*====================== Impossibilidade de aceitar mais da especialidade e termino do cliente ================*/

                        printf("Numero maximo de utentes com a especialidade [%s] ja foi atingido.\nO utente [%d] sera terminado.\n",
                        especialidade, msg.PID);

                        strcpy(msgenv.mensagem, "Ja existem 5 utentes com a sua especialidade.\nPor favor, tente mais tarde.\n");

                        if(!enviaMsg(msg.pipe, msgenv)){
                            continue;
                        }

                        strcpy(msgenv.mensagem, "sair");
                        if(!enviaMsg(msg.pipe, msgenv)){
                            continue;
                        }
                    }
                    else{
                        /*====================== Insere na lista de utentes =====================*/
                        pthread_mutex_lock(t->listas);
                        *t->listU = insereUtente(*t->listU, msg, especialidade, urgencia);
                        pthread_mutex_unlock(t->listas);

                        /*=========================Envia resposta ao cliente*******************/
                        
                       
                        sprintf(msgenv.mensagem, "Foi-lhe atribuida a especialidade %s, com grau de urgencia %d.\nEsta(o) %d utente(s) a sua frente. Esta(o) %d utente a ser atendido(s) e %d medico(s) para a sua especialidade\n\n",
                        especialidade, urgencia, verificaPosicaoU(*t->listU, especialidade, msg.PID),verificaUtentesConsulta(*t->listU,especialidade,msg.PID),verificaEsp(*t->listE, especialidade));
                        
                        if(!enviaMsg(msg.pipe, msgenv)){
                            continue;
                        }

                        /*==============================Imprime para o admin a informacao=====================*/
                        printf("Foi inserido o Utente [%s] com os sintomas: %sFoi lhe atribuido a especialidade [%s] com grau de urgencia [%d]\n\n",
                        msg.nome, msg.mensagem, especialidade, urgencia);
                        
                        /*==========================Coloca a ser atendido caso exista medico=================*/
                        pthread_mutex_lock(t->listas);
                        u = returnUtente(*t->listU, msg.PID);
                        x = podeSerAtendido(&u, t->listE, msg.PID);
                        pthread_mutex_unlock(t->listas);

                        if(x){
                            printf("O utente [%s] vai ser atendido pelo medico [%s]", u->nome, u->esp->nome);
                            pthread_mutex_lock(t->listas);
                            sprintf(msgenv.mensagem, "Vai ser atendido pelo medico %s", u->esp->nome);
                            strcpy(msgenv.pipeaEnviar, u->esp->nPIPE);  //Coloca no pipe a enviar o pipe do medico
                            pthread_mutex_unlock(t->listas);
                            /*==================Envia ao utente o named pipe do medico====================*/
                            
                            if(!enviaMsg(msg.pipe, msgenv)){
                                continue;
                            }

                            /*==================Envia ao medico o named pipe do utente====================*/
                            pthread_mutex_lock(t->listas);
                            strcpy(msgenv.pipeaEnviar, u->nPIPE);
                            sprintf(msgenv.mensagem, "Vai atender o utente %s", u->nome);
                            *t->listE = ordenaEspe(*t->listE, u->esp);
                            if(!enviaMsg(u->esp->nPIPE, msgenv)){
                                continue;
                            }
                            pthread_mutex_unlock(t->listas);
                            msgenv.pipeaEnviar[0]='\0';
                        }
                    }                                    
                }
                else{
                    printf("Numero maximo de utentes ja foi atingido.\nO utente [%d] sera terminado.\n\n",
                        msg.PID);

                        
                        strcpy(msgenv.mensagem, "O numero maximo de Utentes ja foi atingido.\nPor favor, tente mais tarde.\n");
                        if(!enviaMsg(msg.pipe, msgenv)){
                                continue;
                        }

                        strcpy(msgenv.mensagem, "sair");
                        if(!enviaMsg(msg.pipe, msgenv)){
                            continue;
                        }

                }
            }    
            else if(!strcmp(msg.mensagem, "sair") || !strcmp(msg.mensagem, "adeus")){
                printf("O utente [%s] com PID: [%d] decidiu abandonar o sistema\n", msg.nome, msg.PID);
                pthread_mutex_lock(t->listas);  
                *t->listU = eliminaUtenteID(*t->listU, msg.PID);
                pthread_mutex_unlock(t->listas);                    
            } 
                
            else {
                strcpy(msgenv.mensagem, "Ira ser atendido logo que possivel.\nPor favor aguarde...\n");
                if(!enviaMsg(msg.pipe, msgenv)){
                    continue;
                }
            }
        }
        
        if(!strncmp(msg.pipe, "/tmp/esp", 8) ){ 
        

            //verifica se é um especialista novo
            pthread_mutex_lock(t->listas);  
            x = verificaseExisteEsp(*t->listE, msg.PID); 
            pthread_mutex_unlock(t->listas);  
            if( x == 0){ //Ainda nao existe 
                pthread_mutex_lock(t->listas);
                x =verifcaMaxMedicos(*t->listE, qmaxmedicos);
                pthread_mutex_unlock(t->listas);

                if(x){

                    if(strcmp(msg.mensagem, "sair") == 0){
                        continue;
                    }
                    pthread_mutex_lock(t->listas);
                    x = verificaTamPorEspecialidadeMedicos(*t->listE, msg.mensagem); 
                    pthread_mutex_unlock(t->listas);
                    if(!x){

                    /*====================== Impossibilidade de aceitar mais da especialidade e termino do cliente ================*/

                        printf("Numero maximo de Especialistas com a especialidade [%s] ja foi atingido.\nO Especialista [%d] sera terminado.\n\n",
                        msg.mensagem, msg.PID);

                        strcpy(msgenv.mensagem, "Ja existem 5 especialistas com a sua especialidade.\nPor favor, tente mais tarde.\n");
                        if(!enviaMsg(msg.pipe, msgenv)){
                            continue;
                        }

                        strcpy(msgenv.mensagem, "sair");
                        if(!enviaMsg(msg.pipe, msgenv)){
                            continue;
                        }
                    }
                    else{
                        
                        pthread_mutex_lock(t->listas);
                        *t->listE = insereEspecialista(*t->listE, msg);
                        pthread_mutex_unlock(t->listas);
                        strcpy(msgenv.mensagem, "Bem-vindo a MEDICALso");
                        
                        if(!enviaMsg(msg.pipe, msgenv)){
                            continue;
                        }
                        /*============== Verifica se existe algum utente em espera para ser atender ================*/
                        pthread_mutex_lock(t->listas);
                        e = returnEsp(*t->listE, msg.PID);
                        x = podeAtender(&e, t->listU, msg.PID);
                        pthread_mutex_unlock(t->listas);
                        if(x){

                            printf("O utente [%s] vai ser atendido pelo medico [%s]", e->ute->nome, e->nome);
            
                        /*============== Enviar para o especialista o named pipe do utente =========================*/
                            pthread_mutex_lock(t->listas);
                            *t->listE = ordenaEspe(*t->listE, e);
                            
                            sprintf(msgenv.mensagem, "Vai atender o utente %s", e->ute->nome);
                            strcpy(msgenv.pipeaEnviar, e->ute->nPIPE);
                            pthread_mutex_unlock(t->listas);    

                            if(!enviaMsg(msg.pipe, msgenv)){
                               continue;
                            }

                        /*================= Enviar ao utente o named pipe de especialista =========================*/
                            pthread_mutex_lock(t->listas);
                            strcpy(msgenv.pipeaEnviar, e->nPIPE);
                            sprintf(msgenv.mensagem, "Vai ser atendido pelo medico %s", e->nome);

                            if(!enviaMsg(e->ute->nPIPE, msgenv)){
                                continue;
                            }
                            pthread_mutex_unlock(t->listas);
                            msgenv.pipeaEnviar[0]='\0';
                        }


                    }
                }
                else{  //Se ja atingiu o maximo de especialistas
                    printf("Numero maximo de Especialistas ja foi atingido.\nO Especialista [%d] sera terminado.\n\n",
                    msg.PID);

                    
                    strcpy(msgenv.mensagem, "O numero maximo de Especialistas ja foi atingido.\nPor favor, tente mais tarde.\n");
                    if(!enviaMsg(msg.pipe, msgenv)){
                       continue;
                    }

                    strcpy(msgenv.mensagem, "sair");
                    if(!enviaMsg(msg.pipe, msgenv)){
                       continue;
                    }
                }
            }
            else if(strcmp(msg.mensagem, "sair") == 0){
                pthread_mutex_lock(t->listas);
                e = returnEsp(*t->listE, msg.PID);
                if(e->ute!= NULL){
                    printf("O medico [%s] PID: %d terminou a consulta com o utente [%s] PID: %d\n\n",  e->nome, e->PIDMedico, e->ute->nome, e->ute->PIDUtente);
                    *t->listU = eliminaUtenteID(*t->listU, e->ute->PIDUtente);
                }else
                    printf("O medico [%s] PID: %d abandonou o sistema\n\n", e->nome, e->PIDMedico);
                
                *t->listE = eliminaMedicoID(*t->listE, msg.PID);
                pthread_mutex_unlock(t->listas);
            }

            else if(strcmp(msg.mensagem, "adeus") == 0){ //Acabou a consulta ---
                pthread_mutex_lock(t->listas);
                e = returnEsp(*t->listE, msg.PID); 
                
                if(e->ute == NULL){
                    strcpy(msgenv.mensagem,"Nao existe utentes da sua especialidade neste momento");

                    enviaMsg(msg.pipe, msgenv);
                    pthread_mutex_unlock(t->listas);
                    continue;
                }
                pthread_mutex_unlock(t->listas);

                nbytes = read(t->fd_balcao, &msg, sizeof(msg));
                if(nbytes != sizeof(msg)){
                    printf("Alguem terminou um consulta Mensagem recebida com problemas\n");
                    
                }
                pthread_mutex_lock(t->listas);
                if(!strcmp(msg.mensagem, "utente"))
                    printf("O utente [%s] PID: %d terminou a consulta com o medico [%s] PID: %d\n\n", e->ute->nome, e->ute->PIDUtente, e->nome, e->PIDMedico);
                else if(!strcmp(msg.mensagem, "medico"))
                    printf("O medico [%s] PID: %d terminou a consulta com o utente [%s] PID: %d\n\n",  e->nome, e->PIDMedico, e->ute->nome, e->ute->PIDUtente);
                else 
                    printf("Consulta terminada pelo admnistrador entre medico %s e utente %s\n\n", e->nome, e->ute->nome);
                if(e->ute!= NULL){
                    
                    *t->listU = eliminaUtenteID(*t->listU, e->ute->PIDUtente);
                }
                e->ute = NULL;

                x =podeAtender(&e, t->listU, msg.PID); 
                pthread_mutex_unlock(t->listas);
                
                if(x){
                    /*============== Enviar para o especialista o named pipe do utente =========================*/
                        pthread_mutex_lock(t->listas);
                        *t->listE = ordenaEspe(*t->listE, e);

                        strcpy(msgenv.mensagem, "Ira atender um utente");
                        strcpy(msgenv.pipeaEnviar, e->ute->nPIPE);
                        pthread_mutex_unlock(t->listas);
                        if(!enviaMsg(msg.pipe, msgenv)){
                           continue;
                        }

                    /*================= Enviar ao utente o named pipe de especialista =========================*/
                        pthread_mutex_lock(t->listas);
                        strcpy(msgenv.pipeaEnviar, e->nPIPE);
                        strcpy(msgenv.mensagem, "Ira ser atendido dentro de breves instantes");

                        if(!enviaMsg(e->ute->nPIPE, msgenv)){
                           continue;
                        }
                        pthread_mutex_unlock(t->listas);
                        msgenv.pipeaEnviar[0]='\0';
                }
                else{
                        strcpy(msgenv.mensagem,"Nao existe utentes da sua especialidade neste momento");

                        if(!enviaMsg(msg.pipe, msgenv)){
                           continue;
                        }


                }
            }  

            else {
                strcpy(msgenv.mensagem, "Nao existem utentes para a sua especialidade.\nPor favor aguarde...\n");
                if(!enviaMsg(msg.pipe, msgenv)){
                    continue;
                }
            }
        }

    }


}