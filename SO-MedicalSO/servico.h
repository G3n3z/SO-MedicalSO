#ifndef SERVICO_H
#define SERVICO_H



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
#define TAMSTRINGS 256
#define TAMNPIPE 50
#define PIPEBALCAO "/tmp/balcaopipe"
#define PIPEALERTA "/tmp/alerta"

typedef struct medico especialista, *pEspecialista;
typedef struct cliente utente, *pUtente;
typedef struct MSG mensagem;

struct cliente
{
    
    char nome[TAMSTRINGS];
    char especialidade[TAMSTRINGS];
    int urgencia;
    int PIDUtente;
    char nPIPE[TAMNPIPE];
    pUtente prox;
    pEspecialista esp;

};



struct medico
{
    
    char nome[TAMSTRINGS];
    char especialidade[TAMSTRINGS];
    int PIDMedico;
    char nPIPE[TAMNPIPE];
    pUtente ute;
    pEspecialista prox;

};


struct MSG
{
    char nome[TAMSTRINGS];
    char mensagem[TAMSTRINGS];
    char pipeaEnviar[TAMNPIPE];
    char pipe[TAMNPIPE];
    int PID;
};

typedef struct 
{
    int PID;
}msgAlerta;






#endif