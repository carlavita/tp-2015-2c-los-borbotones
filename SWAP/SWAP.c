/*
 * SWAP.C
 *
 *  Created on: 30/8/2015
 *      Author: Martin Fleichman
 */


#include <stdlib.h>
#include <stdio.h>
#include <commons/log.h>
#include <commons/string.h>
#include <commons/config.h>
#include <commons/txt.h>
#include "ConfigSWAP.h"
#include "SWAP.h"
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <unistd.h>




t_log * logSWAP;
t_config_ProcesoSWAP configuracionSWAP;
FILE * archivoDisco;


void LeerArchivoConfiguracion()
{
	t_config* cfgSWAP = malloc(sizeof(t_config));

	log_info(logSWAP,"Leyendo Archivo de Configuracion");
	cfgSWAP = config_create("archivoConfig.conf");
	configuracionSWAP.PuertoEscucha = config_get_int_value(cfgSWAP,"PUERTO_ESCUCHA");
	configuracionSWAP.NombreSwap = config_get_string_value(cfgSWAP,"NOMBRE_SWAP");
	configuracionSWAP.CantidadPaginas = config_get_int_value(cfgSWAP,"CANTIDAD_PAGINAS");
	configuracionSWAP.TamanioPagina = config_get_int_value(cfgSWAP,"TAMANIO_PAGINA");
	configuracionSWAP.RetardoCompactacion = config_get_int_value(cfgSWAP,"RETARDO_COMPACTACION");
	log_info (logSWAP,"%d",configuracionSWAP.PuertoEscucha);
	log_info(logSWAP,"Archivo de Configuracion Leido correctamente");

}

void servidor_Memoria(){

	 //printf(" estoy en el hilo servidor de CPU\n");
	 //todo crear servidor para un cliente Memoria
	 //log_info(logSWAP,"Dentro del hilo conexion a cpu");

	 	struct addrinfo hints;
	 	struct addrinfo *serverInfo;

	 	memset(&hints, 0, sizeof(hints));
	 	hints.ai_family = AF_UNSPEC;		// No importa si uso IPv4 o IPv6
	 	hints.ai_flags = AI_PASSIVE;		// Asigna el address del localhost: 127.0.0.1
	 	hints.ai_socktype = SOCK_STREAM;	// Indica que usaremos el protocolo TCP

	 	//getaddrinfo(NULL, PUERTO, &hints, &serverInfo); // Notar que le pasamos NULL como IP, ya que le indicamos que use localhost en AI_PASSIVE
	 	//dejo la misma ip de la maquina porque el planificador y la cpu son la misma pc--sino cambiar por ip_planificador
	 	getaddrinfo(NULL,configuracionSWAP.PuertoEscucha, &hints, &serverInfo);

	 	int listenningSocket;
	 	listenningSocket = socket(serverInfo->ai_family, serverInfo->ai_socktype, serverInfo->ai_protocol);

	 	bind(listenningSocket,serverInfo->ai_addr, serverInfo->ai_addrlen);
	 	freeaddrinfo(serverInfo); // Ya no lo vamos a necesitar

	 	listen(listenningSocket, BACKLOG);// IMPORTANTE: listen() es una syscall BLOQUEANTE.

	 	struct sockaddr_in addr;			// Esta estructura contendra los datos de la conexion del cliente. IP, puerto, etc.
	 	socklen_t addrlen = sizeof(addr);

	 	int socketCliente = accept(listenningSocket, (struct sockaddr *) &addr, &addrlen);
	 	char package[PACKAGESIZE];
	 	int status = 1;		// Estructura que manejea el status de los recieve.

	 	printf("Memoria conectada. Esperando mensajes:\n");
	 	log_info(logSWAP,"Memoria conectada");

	 	while (status != 0){
	 			status = recv(socketCliente, (void*) package, PACKAGESIZE, 0);
	 			if (status != 0) printf("%s", package);

	 	}

	 	close(socketCliente);
	 	close(listenningSocket);
	 	printf("Cierro conexion con Memoria \n");
	 	log_info(logSWAP,"Cierro conexion con Memoria");


}

void * CreacionDisco() {

	remove(configuracionSWAP.NombreSwap);
	char command[1000];
	char * bytesCrear;
	int tamanioArchivo = configuracionSWAP.CantidadPaginas * configuracionSWAP.TamanioPagina;
	bytesCrear = string_itoa(tamanioArchivo);

	log_info(logSWAP,"Creando archivo de tamanio: %d",tamanioArchivo);
	/* ARMO COMANDO DD DE CREACION DE ARCHIVO DE TAMAÑO FIJO */
	strcpy(command,"dd if=/dev/zero of=");
	strcat(command,configuracionSWAP.NombreSwap);
	strcat(command, " bs=");
	strcat(command,bytesCrear);
	strcat(command," count=1");

	/* EJECUTO COMANDO*/
	system(command);
	log_info(logSWAP,"Archivo Creado correctamente");
return NULL;
}

void * iniciar(int idProceso ,int cantidadPaginas){
	int a = 0;
	while (a <= cantidadPaginas) {
	fseek(archivoDisco,(a*configuracionSWAP.TamanioPagina), SEEK_SET);
	fputs("/0",archivoDisco);
	a++;
	}
	return NULL;

}

int busquedaPosicionCaracter (int posicion,char *listaDeArchivos, char valorABuscar){

if (listaDeArchivos[posicion]== '\0')
	return -1; else if (listaDeArchivos[posicion]== valorABuscar)
		return posicion;
	else
	return busquedaPosicionCaracter(posicion+1,listaDeArchivos,valorABuscar);
}

char *parsearLinea(char * lineaLeida){
	int posicion = busquedaPosicionCaracter (0,lineaLeida,';');
	char lineaParseada[100] = "";
	strncpy(lineaParseada,&lineaLeida[0],posicion);
	//printf("Linea leida1: %s\n", lineaParseada);
	return lineaParseada;
}



int main ()  {
	remove("ArchivoLogueoSWAP.txt");
	logSWAP = log_create("ArchivoLogueoSWAP.txt","SWAP",true,LOG_LEVEL_INFO);
	log_info(logSWAP,"Inicio Proceso SWAP");


	LeerArchivoConfiguracion();
	CreacionDisco();
	int servidor = servidorMultiplexor(configuracionSWAP.PuertoEscucha);
	printf("Mensaje recibido: %s",datosRecibidos());
	//servidor_Memoria();


	exit(0);
	//return 0; //CARLA AMOR Y PAZ POR MI 0 :D
}
