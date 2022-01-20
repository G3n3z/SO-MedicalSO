#ifndef BALCAO_H
#define BALCAO_H

#include "servico.h"
//#include "cliente.h"
//#include "medico.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

typedef struct sinal sinais, *psinal;

struct sinal 
{
    int pid;
    int time;
    psinal prox;
};

typedef struct 
{
    psinal s;
    pthread_mutex_t *listas;
    pthread_mutex_t *vSair;
    pthread_mutex_t *vSinal;
    int* sair;
    pUtente *listU;
    pEspecialista *listE;

}structTimes;

typedef struct structofthreads sthread;

struct structofthreads{
    int classificadortobalcao0;
    int balcaotoclassificador1;
    int fd_balcao;
    pthread_mutex_t *listas;
    pthread_mutex_t *vSair;
    pthread_mutex_t *al;
    int* sair;
    int* alarme;
    pUtente *listU;
    pEspecialista *listE;
    
};



void flush_in();
void imprimeComandos();

void imprimeUtenteEspera(pUtente lista); //Imprime os utentes a espera

void imprimeUtenteAtendido(pUtente lista); //Imprime os utentes a serem atendidos

void comandoUtentes(pUtente lista); //Imprime todos os utentes divididos por estarem a espera ou a serem atendidos

pUtente insereUtente(pUtente lista, mensagem msg, char* especialidade, int urg); //Insere utente ordenado pela urgencia
 
pUtente eliminaUtenteID(pUtente lista, int id); //Elimina utente por ide

int funcTesteCommand(char *sintomaorigem, pUtente *listU, pEspecialista *listE, int* alarme, sthread *t, structTimes *st); //Funcao de teste e execucao dos comandos do utilizador

int verifcaMaxUtentes(pUtente lista, int max); //Verifica o numero de utentes e devolve o true(1) or false(0), true se poder adicionar, false caso contrario

int verificaTamPorEspecialidade(pUtente lista, char* espec); //Verifica o numero de utentes com aquela especialidade e devolve true(1) or false(0). True se for inferior a
                                                             // 5 e caso contrario false
void imprimeListaUtentesPorEspecialidade(pUtente lista);


pUtente libertaUtentes(pUtente lista);
pEspecialista libertaMedicos(pEspecialista lista);

pEspecialista insereEspecialista(pEspecialista lista, mensagem msg);
void imprimeMedicosEspera(pEspecialista lista);
void imprimeMedicoAtender(pEspecialista lista);
int verificaTamPorEspecialidadeMedicos(pEspecialista lista, char* espec);
int verifcaMaxMedicos(pEspecialista lista, int max);

pEspecialista eliminaMedicoID(pEspecialista lista, int id);

int verificaseExiteUtente(pUtente lista, int pid);
int verificaseExisteEsp(pEspecialista lista, int pid);

pUtente returnUtente(pUtente lista, int PID);
pEspecialista returnEsp(pEspecialista lista, int PID);

int podeSerAtendido(pUtente *utente, pEspecialista *listaE, int PID);
int podeAtender(pEspecialista *espec, pUtente *listaU, int PID);

int verificaPosicaoU(pUtente, char*, int);
int verificaEsp(pEspecialista, char*);

int verificaUtentesConsulta(pUtente, char*, int);

void medicoIsFree(pEspecialista listaE, int pid);
int enviaMsg(char* pipe, mensagem msgenv);

pEspecialista ordenaEspe(pEspecialista lista, pEspecialista e);

int verificaExistenciaEAtualiza(psinal s, int pid);
psinal adicionaVecSinal(psinal s, msgAlerta al);
psinal verificaTempoAlerta(psinal s, pthread_mutex_t *listas, pEspecialista *lE, pUtente *lU);
psinal libertaSinais(psinal s);
void imprimeSinais(structTimes st);
#endif
