/*
 * CPU.c
 *
 *  Created on: 1/9/2015
 *      Author: Fernando Sanchez
 */

#include "CPU.h"
#define CHECKPOINT 1// todo los mensajes van a ir al protocolo

typedef struct
{
	int codMje;

}t_mensaje_header; // todo, esto va a ir en el protocolo.h

void  LeerArchivoConfiguracion()
{
	t_config* cfgCPU = malloc(sizeof(t_config));
	log_info(logCPU,"Leyendo Archivo de Configuracion");
	cfgCPU = config_create(PATH_CONFIG);
	configuracionCPU.IPPlanificador = config_get_string_value(cfgCPU,"IP_PLANIFICADOR");
	//configuracionCPU.PuertoPlanificador = config_get_int_value(cfgCPU,"PUERTO_PLANIFICADOR");
	configuracionCPU.PuertoPlanificador = config_get_string_value(cfgCPU,"PUERTO_PLANIFICADOR");
	configuracionCPU.IPMemoria = config_get_string_value(cfgCPU,"IP_MEMORIA");
	configuracionCPU.PuertoMemoria = config_get_int_value(cfgCPU,"PUERTO_MEMORIA");
	configuracionCPU.CantidadHilos = config_get_int_value(cfgCPU,"CANTIDAD_HILOS");
	configuracionCPU.Retardo = config_get_int_value(cfgCPU,"RETARDO");
	log_info (logCPU,"%s",configuracionCPU.PuertoPlanificador);
	log_info(logCPU,"Archivo de Configuracion Leido correctamente");

	}

void Conexion_con_planificador(){

	printf("Conectando a planificador \n");
	log_info(logCPU,"Conectando a planificador");

		struct addrinfo hints;
		struct addrinfo *serverInfo;

		memset(&hints, 0, sizeof(hints));
		hints.ai_family = AF_UNSPEC;		// Permite que la maquina se encargue de verificar si usamos IPv4 o IPv6
		hints.ai_socktype = SOCK_STREAM;	// Indica que usaremos el protocolo TCP


		getaddrinfo(configuracionCPU.IPPlanificador, configuracionCPU.PuertoPlanificador, &hints, &serverInfo);
		int serverSocket;
		serverSocket = socket(serverInfo->ai_family, serverInfo->ai_socktype, serverInfo->ai_protocol);

		connect(serverSocket, serverInfo->ai_addr, serverInfo->ai_addrlen);
		freeaddrinfo(serverInfo);	// No lo necesitamos mas

		int enviar = 1;
		char message[PACKAGESIZE];
		int mensaje;
		printf("Conectado al servidor. Bienvenido al sistema, ya puede enviar mensajes. Escriba 'exit' para salir\n");

		int status = recv(serverSocket, &mensaje, sizeof(mensaje), 0);
			if (!status){
				if (mensaje == CHECKPOINT){
				printf("recibido el mensaje correr path desde el planificador");
				}
			}else{
				printf("error al recibir");
			}
			while(enviar){
				fgets(message, PACKAGESIZE, stdin);			// Lee una linea en el stdin (lo que escribimos en la consola) hasta encontrar un \n (y lo incluye) o llegar a PACKAGESIZE.
				if (!strcmp(message,"exit\n")) enviar = 0;			// Chequeo que el usuario no quiera salir
				if (enviar) send(serverSocket, message, strlen(message) + 1, 0); 	// Solo envio si el usuario no quiere salir.
			}

		close(serverSocket);
		printf("Conexion a planificador cerrada \n");
		log_info(logCPU,"Conexion a planificador cerrada");

}


int main ()  {
	remove(PATH_LOG);
	logCPU = log_create(PATH_LOG,"CPU",true,LOG_LEVEL_INFO);
	log_info(logCPU,"Inicio Proceso CPU");

	LeerArchivoConfiguracion();

	Conexion_con_planificador();

	return 0; //CARLA AMOR Y PAZ POR MI 0 :D
}
