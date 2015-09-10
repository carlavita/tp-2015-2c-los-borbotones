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
#include "SWAP.h"
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <unistd.h>
#include <commons/collections/list.h>




t_log * logSWAP;
t_config_ProcesoSWAP configuracionSWAP;
FILE * archivoDisco;
t_list * listaProcesos;
t_list * listaPaginasLibres;
int paginasLibres;



void leerArchivoConfiguracion()
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

void servidorMemoria(){

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


void * creacionDisco() {

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
	int nroPagina;
	int estado = controlInsercionPaginas(cantidadPaginas);
	switch (estado)  {
		case 0: /*ESPACIO PARA ASIGNAR PERO NO CONTIGUO -> EJECUTAR COMPACTACION*/
		compactacion();
		break;
	case -1: /* NO HAY ESPACIO PARA ASIGNAR PAGINAS DEVOLVER ERROR */
		break;
	default:
	 /*ESPACIO CONTIGUO EN DISCO PARA ASIGNAR PAGINAS*/
			list_add(listaProcesos,procesoCreate(idProceso,cantidadPaginas,estado));
			paginasLibres = paginasLibres - cantidadPaginas;

			for (nroPagina= 0; nroPagina <= cantidadPaginas;nroPagina++) {
			fseek(archivoDisco,(estado+(nroPagina*configuracionSWAP.TamanioPagina)), SEEK_SET);
			fputc('\0',archivoDisco);
			}

			break;
	}
	return NULL;
}

void * finalizar (int PID){
	int posicionPIDLista = busquedaPIDEnLista(PID);
	t_tablaProcesos* procesoFinalizado = list_get(listaProcesos,posicionPIDLista); //VER COMO BUSCAR LA POSICION EN LA LISTA
	int cantidadPaginas =procesoFinalizado->cantidadPaginas;
	int primerPagina = procesoFinalizado->primerPagina;
	int cantidadEscrituras = procesoFinalizado->cantidadEscrituras ;
	int cantidadLecturas = procesoFinalizado->cantidadLecturas ;
	int ultimaPagina = procesoFinalizado->ultimaPagina ;
	int bytesUsadosPorPID = ultimaPagina - primerPagina;
	char * contenido = '/0';
	//ACA VA LA ELIMINACION DEL CONTENIDO DEL S WAP DE ESAS PAGINAS
		/*	for (posicionArchivo = 0 ; posicionArchivo <= bytesUsadosPorPID; posicionArchivo++){
				fseek(archivoDisco,primerBytePID + posicionArchivo,SEEK_SET);
				fputc('\0',archivoDisco);
			}*/
			fseek(archivoDisco,primerPagina,SEEK_SET);
			fwrite(contenido,bytesUsadosPorPID,primerPagina,archivoDisco);

	//ELIMINO DE VECTOR
	list_add(listaPaginasLibres,paginasLibresCreate(primerPagina,ultimaPagina));
	list_remove(listaProcesos,posicionPIDLista);
	paginasLibres = paginasLibres + cantidadPaginas;
	ordenarLista(); //LUEGO DE INSERTAR ORDENO LA LISTA POR PAGINA BYTE OCUPADO

	return NULL;
}


char* leer (int PID,int nroPagina){
	int tamanioPagina = configuracionSWAP.TamanioPagina;
	int primerBytePagina = (nroPagina-1) * tamanioPagina;
	char * contenido;
	fseek(archivoDisco,primerBytePagina,SEEK_SET);
	fread(contenido,tamanioPagina,primerBytePagina,archivoDisco); //LEER CONTENIDO UBICADO EN LA PAGINA
	//DEVUELVO PAGINA LEIDA

	int posicionPID = busquedaPIDEnLista(PID);
	t_tablaProcesos *procesoObtenido = list_get(listaProcesos,posicionPID);
	procesoObtenido->cantidadLecturas = procesoObtenido->cantidadLecturas + 1;

	return contenido;
}

void * escribir (int PID, int nroPagina, char* contenidoPagina){
	int tamanioPagina = configuracionSWAP.TamanioPagina;
	int primerBytePagina = (nroPagina-1) * tamanioPagina;
	fseek(archivoDisco,primerBytePagina,SEEK_SET);
	//fputs(contenidoPagina,archivoDisco);
	fwrite(contenidoPagina,tamanioPagina,primerBytePagina,archivoDisco);

	int posicionPID = busquedaPIDEnLista(PID);
	t_tablaProcesos *procesoObtenido = list_get(listaProcesos,posicionPID);
	procesoObtenido->cantidadEscrituras = procesoObtenido->cantidadEscrituras + 1;
	return NULL;
}


void * compactacion(){

	return NULL;
}



int controlInsercionPaginas(int cantidadPaginas) {
	int contador = 0 ;
	int espacioRequerido = cantidadPaginas * configuracionSWAP.TamanioPagina;
	int tamanioLista = list_size(listaProcesos);
	t_tablaProcesos* procesoInicial;
	t_tablaProcesos* procesoFinal;
	procesoInicial = list_get(listaProcesos,contador);
	procesoFinal = list_get(listaProcesos,(contador+1));
	if (cantidadPaginas > paginasLibres){
			return -1;
		}
		while (contador < tamanioLista) {
	/*while ((procesoFinal->primerByte - procesoInicial->ultimoByte > 1)) {
			if (espacioRequerido <= (procesoFinal->primerByte - procesoInicial->ultimoByte > 1)){
				return espacioRequerido; // ACA DEVUELVO LA POSICION EN CUAL INGRESAR

	}}*/
		contador++;
		procesoInicial = list_get(listaProcesos,contador);
		procesoFinal = list_get(listaProcesos,(contador+1));
		}
	return 0;/* CONTROL DE ESPACIO CONTIGUO EN VECTOR*/
}

int busquedaPIDEnLista(int PID){
	int posicion = 0;
	t_tablaProcesos* proceso;
	proceso = list_get(listaProcesos,posicion);
	while (!proceso->pid == PID) {
		posicion++;
		proceso = list_get(listaProcesos,posicion);
		}
	return posicion;
}

void * ordenarLista(){
bool _ordenamiento_porBytes(t_tablaPaginasLibres* paginasLibres, t_tablaPaginasLibres* paginasLibresMenor) {
	return paginasLibres ->desdePagina < paginasLibresMenor->hastaPagina;
	}
	list_sort(listaPaginasLibres,(void*) _ordenamiento_porBytes);
	return NULL;
}

void * inicializarListas(){
	listaProcesos = list_create(); /* CREO MI LISTA ENLAZADA*/
	listaPaginasLibres = list_create();
	list_add(listaPaginasLibres,paginasLibresCreate(0,configuracionSWAP.CantidadPaginas));

	return NULL;
}

int procesoCreate(int PID, int cantidadPaginas, int primeraPagina) {
	t_tablaProcesos *proceso = malloc(sizeof(t_tablaProcesos));
proceso->pid = PID;
proceso->cantidadPaginas = cantidadPaginas;
proceso->primerPagina = 0;
proceso->ultimaPagina = cantidadPaginas - proceso->primerPagina;
proceso->cantidadEscrituras = 0;
proceso->cantidadLecturas = 0;
return proceso ->pid;
}

int paginasLibresCreate(int desdePagina, int hastaPagina) {
	t_tablaPaginasLibres *paginasLibres = malloc(sizeof(t_tablaPaginasLibres));
paginasLibres->desdePagina = desdePagina;
paginasLibres->hastaPagina = hastaPagina;
return paginasLibres->desdePagina;
}

int main ()  {
	remove("ArchivoLogueoSWAP.txt");
	logSWAP = log_create("ArchivoLogueoSWAP.txt","SWAP",true,LOG_LEVEL_INFO);
	log_info(logSWAP,"Inicio Proceso SWAP");


	leerArchivoConfiguracion();
	paginasLibres = configuracionSWAP.CantidadPaginas;
	creacionDisco();
	inicializarListas();
	int servidor = servidorMultiplexor(configuracionSWAP.PuertoEscucha);
	printf("Mensaje recibido: %s",datosRecibidos());
	//servidorMemoria();

	exit(0);
}
