#ifndef CLIENTE_H
#define CLIENTE_H

#include "servico.h"

//protopipos
int enviaMsg(int fd, mensagem msgenv);
void buildNamePipe(char* namepipe,int pid);

#endif