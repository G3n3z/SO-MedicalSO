#include "balcao.h"

void flush_in(){
    int ch;
    while( (ch = fgetc(stdin)) != EOF && ch != '\n' ){}
}

void imprimeComandos(){

    printf("\nComandos: \n\n");
    printf("utentes  <=> Para consultar a lista de utentes em espera e a ser atendidos\n");
    printf("especialistas <=> Para consultar a lista dos Especialistas em espera e em consultas\n");
    printf("delut <id> <=> Para eliminar um utente com determinado id\n");
    printf("delesp <id> <=> Para eliminar um especialista com determinado id\n");
    printf("freq <seg> <=> Altera a frequencia com que sao impressas as listas (em seg)\n");
    printf("encerra <=> Para encerrar o programa, o classificador e todos os medicos e clientes\n");
    printf("sinais <=> Para mostrar a lista de sinais de vida\n\n");
}


void imprimeUtenteEspera(pUtente lista){
    int conta = 0;
    printf("\n\nUtentes em espera: \n");    
    while (lista!=NULL){
        if(lista->esp == NULL){
            printf("Utente %s | %s | Urgencia: %d | PID: %d\n", lista->nome, lista->especialidade, lista->urgencia, lista->PIDUtente);
            conta++;
        }
        lista = lista->prox;
    }
    if(conta == 0){
        printf("Nao existem utentes em lista de espera.\n");
        return;
    }

}

void imprimeUtenteAtendido(pUtente lista){
    int conta = 0;
    printf("\n\nUtentes a ser Atendidos: \n");
    while (lista!=NULL){
        if(lista->esp != NULL){
            printf("Utente %s | %s a ser atendido por: %d %s\n", lista->nome, lista->especialidade, lista->esp->PIDMedico, lista->esp->nome);
            conta++;    
        }
        lista = lista->prox;

    }
    if(conta == 0){
        printf("Nao existem utentes a serem atendidos.\n");
        return;
    } 
}

void comandoUtentes(pUtente lista){

    imprimeUtenteEspera(lista);
    imprimeUtenteAtendido(lista);
}

pUtente insereUtente(pUtente lista, mensagem msg, char* especialidade, int urg){   //Insercao ordenada de Utentes
    pUtente novo, aux, auxAnt;

    static int id;
    novo = malloc(sizeof(utente));
    if(novo == NULL){
        printf("Erro de alocacao");
        return lista;
    }  

    if(lista == NULL)
        id=0;
    
    strcpy(novo->nome, msg.nome);
    novo->urgencia = urg;
    novo->prox =NULL;
    novo->PIDUtente = msg.PID;
    novo->esp = NULL;
    strcpy(novo->especialidade,especialidade); 
    strcpy(novo->nPIPE, msg.pipe);
    

    if(lista == NULL){ //1 insercao
        lista = novo;
        return lista;
    }

    auxAnt = NULL;
    aux = lista;
    
    while (aux!=NULL && novo->urgencia >= aux->urgencia)
    {
        auxAnt = aux;
        aux = aux->prox;
    }

    if(auxAnt == NULL){  // Caso seja insercao a cabeca
        novo->prox = lista;
        lista = novo;
        return lista;
    }

    if(aux == NULL){ //Insercao a cauda
        auxAnt->prox = novo;
        return lista;
    }

    auxAnt->prox = novo;
    novo->prox = aux;


    return lista;
    

}

pUtente eliminaUtenteID(pUtente lista, int pid){
    pUtente aux, auxAnt;

    if(lista == NULL){
        printf("Ainda nao existe utentes");
        return NULL;
    }

    if(lista->PIDUtente == pid){  //Eliminacao a cabeca
        aux = lista;
        lista = lista->prox;
        if(aux->esp != NULL)
            aux->esp->ute = NULL; 
        free(aux);
        printf("Utente: %d eliminado.\n", pid);
        return lista;
    }

    auxAnt = NULL;
    aux = lista;
    while(aux != NULL && aux->PIDUtente != pid){
        
        auxAnt = aux;
        aux = aux->prox;

    }

    if(aux == NULL){  // Nao encontrou nenhum utente com aquele id
        printf("Nao existe nenhum utente com este id, tente outro\n");
        return lista;
    }
    

    auxAnt->prox = aux->prox;
    if(aux->esp != NULL)
        aux->esp->ute = NULL; 
    free(aux);
    printf("Utente: %d eliminado.\n", pid);
    return lista;

}




int funcTesteCommand(char *sintomaorigem, pUtente *listU, pEspecialista *listE, int *alarme, sthread* t, structTimes *st){
    

    char *aux;
    int num;
    char sintoma[500];
    strcpy(sintoma, sintomaorigem);
    sintoma[strlen(sintoma)-1]='\0';
 
    aux=strtok(sintoma, " ");
    
    if(aux== NULL)
        aux = " " ;
    
    if(strcmp(sintoma, "utentes")==0){
        pthread_mutex_lock(t->listas);
        pUtente p = *listU; 
        imprimeUtenteEspera(p);
        imprimeUtenteAtendido(p);
        pthread_mutex_unlock(t->listas);
        return 1;
    } 
    else if(!strcmp(sintoma, "especialistas")){
        pthread_mutex_lock(t->listas);
        imprimeMedicosEspera(*listE);
        imprimeMedicoAtender(*listE);
        fflush(stdout);
        pthread_mutex_unlock(t->listas);
        return 1;
    }
    else if(!strcmp(aux, "delut")){
      
        aux = strtok(NULL," ");
        if(aux == NULL){
            printf("Comando Invalido\n");
            return 1;
        }

        num = atoi(aux);
        if(num <= 0){
            printf("Id invalido\n");
            return 1;
        }     
        mensagem m;
        strcpy(m.nome, "balcao");
        strcpy(m.mensagem, "sair");
        strcpy(m.pipe, PIPEBALCAO);
        m.pipeaEnviar[0] = '\0';
        m.PID = getpid();

        pthread_mutex_lock(t->listas);
        pUtente p = *listU , u;
        u = returnUtente(*listU, num);
        if(u != NULL){
            enviaMsg(u->nPIPE, m);
            if(u->esp != NULL){ //Se estiver a ser atendido
                strcpy(m.mensagem, "adeus");
                enviaMsg(u->esp->nPIPE, m);
            }
            else{ //Se nao estiver a ser atendido
                p = eliminaUtenteID(p,num);
                *listU = p;
            }
        }      
         
        pthread_mutex_unlock(t->listas);
        return 1;
        
    }
    else if(!strcmp(aux, "delesp")){
        aux = strtok(NULL," ");

        if(aux == NULL){
            printf("Comando Invalido\n");
            return 1;
        }

        num = atoi(aux);
        if(num <= 0){
            printf("Id invalido\n");
            return 1;
        } 
        
        mensagem m;
        strcpy(m.nome, "balcao");
        strcpy(m.mensagem, "sair");
        strcpy(m.pipe, PIPEBALCAO);
        m.PID = getpid();
        m.pipeaEnviar[0] = '\0';
        pthread_mutex_lock(t->listas);    
        pEspecialista p = *listE, e;
        e = returnEsp(p, num);
        if(e != NULL){
            if(e->ute != NULL){
                enviaMsg(e->ute->nPIPE,m);
                *listU = eliminaUtenteID(*listU, e->ute->PIDUtente);
            }
            enviaMsg(e->nPIPE, m);
        }
        p = eliminaMedicoID(p,num);
        *listE = p; 
        pthread_mutex_unlock(t->listas);
        return 1;
        
    }
    else if(!strcmp(aux, "freq")){
        aux = strtok(NULL," ");
        if(aux == NULL){
            printf("Comando Invalido\n");
            return 1;
        }

        num = atoi(aux);
        if(num <= 0){
            printf("Id invalido\n");
            return 1;
        }
        pthread_mutex_lock(t->al);     
        *alarme = num;
        pthread_mutex_unlock(t->al);
        return 1;
    }
    else if (!strcmp(sintoma, "encerra")){
        //Manda mensagem para o proprio name pipe
        mensagem msg;
        msg.mensagem[0]='\0';
        msg.PID = getpid();
        enviaMsg(PIPEBALCAO, msg);
        return 2;    
    }
    else if (!strcmp(sintoma, "sinais"))
    {
        pthread_mutex_lock(st->vSinal);
        imprimeSinais(*st);
        pthread_mutex_unlock(st->vSinal);
        return 1;
    }
    
    
    
    return 0;

}

int verificaTamPorEspecialidade(pUtente lista, char* espec){
    int count = 0;
    pUtente aux = lista;
    while(aux != NULL){
        if(strcmp(aux->especialidade, espec) == 0)
            count++;
        aux = aux->prox;    
    }

    if(count < 5)
        return 1;

    else
        return 0;
}


int verifcaMaxUtentes(pUtente lista, int max){
    int count = 0;
    pUtente aux = lista;
    while (aux!=NULL)
    {
        aux = aux->prox;
        count++;
    }

    if(count < max)
        return 1;

    else
        return 0;    
    
}



pUtente libertaUtentes(pUtente lista){

    pUtente aux = lista;
    while(lista!=NULL){
        lista = lista->prox;
        free(aux);
        aux= lista;
    }

    return NULL;
}

int verificaseExiteUtente(pUtente lista, int pid){

    pUtente aux = lista;
    
    while(aux!= NULL){
        if(aux->PIDUtente == pid)
            return 1;

         aux = aux->prox;   
    }

    return 0;

}

int verificaPosicaoU(pUtente lista, char* espec, int pid){
    int count = 0;
    pUtente aux = lista;
    while(aux != NULL && aux->PIDUtente != pid){
        if(strcmp(aux->especialidade, espec) == 0 )
            count++;
        aux = aux->prox;    
    }

    return count;
}

int verificaUtentesConsulta(pUtente lista, char* espec, int pid){
    int count = 0;
    pUtente aux = lista;
    while(aux != NULL ){
        if((strcmp(aux->especialidade, espec) == 0) && aux->esp != NULL && aux->PIDUtente != pid)
            count++;
        aux = aux->prox;    
    }

    return count;
}

void imprimeListaUtentesPorEspecialidade(pUtente lista){
    int count, tot = 0;
    pUtente aux = lista;
    char especialidades[5][50] = {{"geral"},{"neurologia"},{"ortopedia"},{"oftalmologia"},{"estomatologia"}};
    
    for(int i = 0 ; i < 5 ; i++){
        
        count = 0;
        while(aux != NULL){
            if(!strcmp(aux->especialidade,especialidades[i]) && aux->esp == NULL){
                count++;
                ++tot;
            }
            aux = aux->prox;
        }
        if(count > 0){
            printf("\n\nLista de espera da especialidade %s\n", especialidades[i]);
            printf("Existem %d utentes em espera\n", count);
        }

        aux = lista;
    }

    if(tot == 0)
        printf("Nao existe filas de espera\n\n");

}

/*



          ////////////////////////////////    FUNCOES DOS MEDICOS     ///////////////////////////////





*/

pEspecialista insereEspecialista(pEspecialista lista, mensagem msg){   //Insercao ordenada de Utentes
    pEspecialista novo, aux, auxAnt;

    
    novo = (pEspecialista)malloc(sizeof(especialista));
    if(novo == NULL){
        printf("Erro de alocacao");
        return lista;
    }  


    strcpy(novo->nome, msg.nome);
    novo->prox =NULL;
    novo->PIDMedico = msg.PID;

    novo->ute = NULL;
    strcpy(novo->especialidade,msg.mensagem); 
    strcpy(novo->nPIPE, msg.pipe);
    
    printf("O especialista [%s] com PID: [%d] entrou no sistema\n", novo->nome, novo->PIDMedico);

    if(lista == NULL){ //1 insercao
        lista = novo;
        return lista;
    }

    auxAnt = NULL;
    aux = lista;
    
    while (aux!=NULL && novo->PIDMedico >= aux->PIDMedico) 
    {
        auxAnt = aux;
        aux = aux->prox;
    }

    if(auxAnt == NULL){  // Caso seja insercao a cabeca
        novo->prox = lista;
        lista = novo;
        return lista;
    }

    if(aux == NULL){ //Insercao a cauda
        auxAnt->prox = novo;
        return lista;
    }

    auxAnt->prox = novo;
    novo->prox = aux;


    return lista;
}

void imprimeMedicosEspera(pEspecialista lista){
    int conta = 0;
 
    printf("\n\nMedicos em espera: \n");    
    while (lista!=NULL){
        if(lista->ute == NULL){
            printf("%s | %s | ID: %d\n", lista->nome, lista->especialidade, lista->PIDMedico);
            conta++;
        }
        lista = lista->prox;
    }
    if(conta == 0){
        printf("Nao existem medicos em espera.\n");
        return;
    } 
}

void imprimeMedicoAtender(pEspecialista lista){
    int conta = 0;

    printf("\n\nMedicos a atender: \n");    
    while (lista!=NULL){
        if(lista->ute != NULL){
            printf("%s | %s  a atender: %d %s\n", lista->nome, lista->especialidade, lista->ute->PIDUtente, lista->ute->nome);
            conta++;
        }
        lista = lista->prox;

    }
    if(conta == 0){
        printf("Nao existem medicos a atender.\n");
        return;
    }
}


int verifcaMaxMedicos(pEspecialista lista, int max){
    int count = 0;
    pEspecialista aux = lista;
    while (aux!=NULL)
    {
        aux = aux->prox;
        count++;
    }

    if(count < max)
        return 1;

    else
        return 0;    
    
}

int verificaTamPorEspecialidadeMedicos(pEspecialista lista, char* espec){
    int count = 0;
    pEspecialista aux = lista;
    while(aux != NULL){
        if(strcmp(aux->especialidade, espec) == 0)
            count++;
        aux = aux->prox;    
    }

    if(count < 5)
        return 1;

    else
        return 0;
}

pEspecialista libertaMedicos(pEspecialista lista){

    pEspecialista aux = lista;
    while(lista!=NULL){
        lista = lista->prox;
        free(aux);
        aux= lista;
    }

    return NULL;
}

pEspecialista eliminaMedicoID(pEspecialista lista, int id){
    pEspecialista aux, auxAnt;
 
    if(lista == NULL){
       printf("Ainda nao existe medicos");
       return NULL;
    }
 
    if(lista->PIDMedico == id){  //Eliminacao a cabeca
        aux = lista;
        
        if(aux->ute != NULL){
           printf("Medico em consulta, tente mais tarde.");
           return lista;
        }
        lista = lista->prox;
        free(aux);
        printf("Medico: %d eliminado.\n", id);
        return lista;
    }
 
    auxAnt = NULL;
    aux = lista;
    while(aux != NULL && aux->PIDMedico != id){
      
       auxAnt = aux;
       aux = aux->prox;
 
    }
    if(aux == NULL){  // Nao encontrou nenhum utente com aquele id
       printf("Nao existe nenhum medico com este id, tente outro\n");
       return lista;
    }


    if(aux->ute != NULL){
           printf("Medico em consulta, tente mais tarde.");
           return lista;
    }


 
    auxAnt->prox = aux->prox;
    if(aux->ute!= NULL){
       aux->ute->esp = NULL;
    }
 
    free(aux);
    printf("Medico: %d eliminado.\n", id);
    return lista;
 
}
int verificaseExisteEsp(pEspecialista lista, int pid) {

    pEspecialista aux = lista;
    
    while(aux!= NULL){
        if(aux->PIDMedico == pid)
            return 1;

         aux = aux->prox;   
    }

    return 0;
}

pUtente returnUtente(pUtente lista, int PID){

    pUtente aux = lista;
    while (aux!= NULL && aux->PIDUtente != PID){
        aux = aux->prox;
    }
    
    if(aux == NULL)
        return NULL;

    return aux;    

}


int podeSerAtendido(pUtente *utente, pEspecialista *listaE, int PID){

    pEspecialista l = *listaE;
    pUtente u = *utente;

    if(u == NULL){
        return 0;
    }

    while(l != NULL && (strcmp(u->especialidade,l->especialidade) || l->ute != NULL)){
        l = l->prox;
    }

    if(l != NULL){
        u->esp = l;
        l->ute = *utente;
        return 1;
    }
      
    return 0;  
}

void medicoIsFree(pEspecialista listaE, int pid){

    while(listaE != NULL && listaE->ute->PIDUtente != pid){
        listaE = listaE->prox;
    }

    if(listaE!=NULL)
        listaE->ute = NULL;
 
}

pEspecialista returnEsp(pEspecialista lista, int PID){

    pEspecialista aux = lista;
    while (aux!= NULL && aux->PIDMedico != PID){
        aux = aux->prox;
    }
    
    if(aux == NULL)
        return NULL;

    return aux;    
}


int podeAtender(pEspecialista *espec, pUtente *listaU, int PID){

    pUtente l = *listaU;
    pEspecialista e = *espec;


    while(l != NULL && (strcmp(e->especialidade,l->especialidade) || l->esp != NULL)){
        l = l->prox;
    }

    if(l != NULL){
        e->ute = l;
        l->esp = *espec;
        return 1;
    }
      
    return 0;  
}

int verificaEsp(pEspecialista lista, char* esp){
    int count = 0;
    pEspecialista aux = lista;
    while(aux != NULL){
        if(strcmp(aux->especialidade, esp) == 0)
            count++;
        aux = aux->prox;
    }

    return count;
}


int enviaMsg(char* pipe, mensagem msgenv){

    int fd_enviar = open(pipe, O_WRONLY| O_NONBLOCK);
    if(fd_enviar == -1){
        printf("Nao foi possivel abrir o pipe");
        
        return 0;
    }

    int nbytes = write(fd_enviar, &msgenv, sizeof(msgenv));
    if(nbytes != sizeof(msgenv)){
        printf("Erro ao enviar. So consegui enviar %d bytes de %d bytes", nbytes, sizeof(msgenv));
 
        close(fd_enviar);
        return 0;
    }
    close(fd_enviar);
    return 1;


}


pEspecialista ordenaEspe(pEspecialista lista, pEspecialista e){

    pEspecialista aux = lista, auxAnt = NULL, temp;

    if(lista == NULL)
        return NULL;
    

    while(aux != NULL && aux->PIDMedico != e->PIDMedico){
        auxAnt = aux;
        aux = aux->prox;
        printf("Aqui");
    }
    //1 3 5 
    if(aux->prox == NULL) //Sefor o ultimo
        return lista;

    if(auxAnt == NULL){ //Se for a cabeca
        lista = lista->prox;
        auxAnt = lista;
    }
    else
        auxAnt->prox = aux->prox;

    while (auxAnt->prox != NULL)
    {
        auxAnt= auxAnt->prox;
    }

    auxAnt->prox = aux;
    aux->prox= NULL;

    return lista;

}

/*=====================|Funcoes do sinal de vida===========================*/

int verificaExistenciaEAtualiza(psinal s, int pid){

    if(s == NULL){
        return 1;
    }
        

    psinal aux = s;

    while (aux != NULL && aux->pid != pid)
    {
        aux = aux->prox;
    }

    if(aux != NULL){
        aux->time += 20;
        return 0;
    }

    return 1;
    
        
}


psinal adicionaVecSinal(psinal s, msgAlerta al){

    psinal aux = s, novo;

    novo = malloc(sizeof(sinais));
    if(novo == NULL){
        printf("Nao foi possivel alocar memoria\n");
        return s;
    }

    novo->time = 20;
    novo->pid = al.PID;
    novo->prox = NULL;


    if(s == NULL){ //Primeira Insercao
        s = novo;
        return s;
    }

    while(aux->prox != NULL){ //Insercao à cauda
        aux = aux->prox;
    }

    aux->prox = novo;


    return s;   
}

psinal verificaTempoAlerta(psinal s, pthread_mutex_t *listas, pEspecialista * lE, pUtente *lU){
    
    if(s == NULL)
        return NULL;

    psinal aux = s, aux2 = NULL;
    mensagem msgenv;
    pEspecialista e;
    
    while (aux != NULL)
    {
        aux->time -= 2;
        if(aux->time < -2){
            pthread_mutex_lock(listas);
            e = returnEsp(*lE, aux->pid);
            if(e != NULL){
                printf("Medico [%s] saiu abrutamente\n", e->nome);
                if (e->ute != NULL)
                {   
                    msgenv.PID = e->PIDMedico;
                    strcpy(msgenv.nome, e->nome);
                    strcpy(msgenv.mensagem, "adeus");
                    msgenv.pipe[0] = '\0';
                    msgenv.pipeaEnviar[0] = '\0';
                    if(!enviaMsg(e->ute->nPIPE, msgenv))
                        unlink(e->ute->nPIPE);
                    *lU = eliminaUtenteID(*lU,e->ute->PIDUtente);
                }
                unlink(e->nPIPE);
                *lE = eliminaMedicoID(*lE,aux->pid);
                
            }
            pthread_mutex_unlock(listas);
            if(aux2 == NULL){
                aux2 = aux;
                aux = aux->prox;
                free(aux2);
                s = aux;
            }
            else{
                aux2->prox = aux->prox;
                free(aux);
                aux = aux2->prox;
            }
        }else{
            aux2 = aux; 
            aux = aux->prox;
        }
        

    }

    return s; 
}

psinal libertaSinais(psinal s){

    psinal aux = s;
    while (s != NULL)
    {
        aux = s;
        s = s->prox;
        free(aux);
    }
    
    return NULL;
}

void imprimeSinais(structTimes st){
   
    psinal aux = st.s;

    if(aux == NULL){
        printf("Ainda nao existe especialistas\n");
        return;
    }

    printf("\n\nLista de Sinais:\n");

    while (aux != NULL)
    {
        printf("Especialista: [%d] com Time: [%d]\n", aux->pid, aux->time);
        aux = aux->prox;
    }
    

}