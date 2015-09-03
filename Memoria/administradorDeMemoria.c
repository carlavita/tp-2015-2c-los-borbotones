/*
 * administradorDeMemoria.c
 *
 *  Created on: 30/8/2015
 *      Author: Fernando Rabinovich
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <commons/config.h>
#include <pthread.h>
#include <string.h>
#include <stdbool.h>
#include <commons/log.h>
#include <semaphore.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>

typedef struct
{
	int puertoEscucha;
	char* ipSwap;
	char * puertoSwap;
	int maximoMarcosPorProceso;
	int cantidadDeMarcos;
	int tamanioMarcos;
	int entradasTLB;
	int tlbHabilitada; /*Las commons no tiene un config_ge_bool.... => 0 false, 1 true*/
	int retardoMemoria;
}t_config_memoria;

t_config_memoria  configMemoria;
t_log * logMemoria;


void leerConfiguracion()
{
  log_info(logMemoria,"Inicio lectura de archivo de configuración");
  t_config * configuracionMemoria = config_create("/home/utnso/tp-2015-2c-los-borbotones/Memoria/memoria.conf");
  configMemoria.puertoEscucha = config_get_int_value(configuracionMemoria,"PUERTO_ESCUCHA");
  configMemoria.ipSwap = config_get_string_value(configuracionMemoria,"IP_SWAP");
  configMemoria.puertoSwap = config_get_string_value(configuracionMemoria,"PUERTO_SWAP");
  configMemoria.maximoMarcosPorProceso  = config_get_int_value(configuracionMemoria,"MAXIMO_MARCOS_POR_PROCESO");
  configMemoria.cantidadDeMarcos = config_get_int_value(configuracionMemoria,"CANTIDAD_MARCOS");
  configMemoria.tamanioMarcos = config_get_int_value(configuracionMemoria,"TAMANIO_MARCOS");
  configMemoria.entradasTLB = config_get_int_value(configuracionMemoria,"ENTRADAS_TLB");
  configMemoria.tlbHabilitada = config_get_int_value(configuracionMemoria,"TLB_HABILITADA");
  configMemoria.retardoMemoria = config_get_int_value(configuracionMemoria,"RETARDO_MEMORIA");
 log_info(logMemoria,"%d",configMemoria.puertoEscucha);
  log_info(logMemoria,"Finalizo lectura de archivo de configuración");
}

void crearServidores()
{
	servidorMultiplexor(configMemoria.puertoEscucha);
}


pthread_t * hiloServidor;
pthread_t hiloPrincipalDeMemoria;

void * funcionServidor()
{
	//crearServidores(configMemoria);

	pthread_join(hiloPrincipalDeMemoria,NULL);

	return NULL;
}
char * recibidoPorLaMemoria;
char mensaje[1024];



int main()
{
	remove("logMemoria.txt");//Cada vez que arranca el proceso borro el archivo de log.
//	configMemoria = malloc(sizeof(t_config_memoria));
	logMemoria = log_create("logMemoria.txt","Administrador de memoria",true,LOG_LEVEL_INFO);
	leerConfiguracion();
	//pthread_create(hiloServidor,NULL,&funcionServidor,NULL);
	log_info(logMemoria,"Puerto asignado: %d",configMemoria.puertoEscucha);

	int servidorCPU = servidorMultiplexor(configMemoria.puertoEscucha);
	recibidoPorLaMemoria = datosRecibidos();
	cliente(configMemoria.ipSwap,configMemoria.puertoSwap);
	int clienteSwap = clienteSeleccion();
	int envioDeMensajes = send(clienteSwap,recibidoPorLaMemoria,sizeof(recibidoPorLaMemoria),0);
	while(envioDeMensajes == -1) envioDeMensajes = send(clienteSwap,recibidoPorLaMemoria,sizeof(recibidoPorLaMemoria),0);
	log_info(logMemoria,"%d",envioDeMensajes);
    pthread_join(*hiloServidor,NULL);

    exit(0);
}


