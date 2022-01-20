all:balcao.c
	@gcc -o balcao balcao.c balcaofunc.c -pthread balcao.h servico.h
	@gcc -o medico medico.c -pthread servico.h medico.h
	@gcc -o cliente cliente.c servico.h
	@echo "tudo compilado"

medic:medico.c
	@rm medico
	@echo "medico compilado"
	@gcc -o medico medico.c -pthread servico.h medico.h

client:
	@rm cliente
	@gcc -o cliente cliente.c servico.h
	@echo "cliente compilado"
clean:
	@rm balcao medico cliente /tmp/balcaopipe

cleanpipes:
	@rm /tmp/balcaopipe /tmp/esp_*  /tmp/cli_* /tmp/alerta