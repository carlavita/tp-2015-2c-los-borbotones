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
#include <protocolo.h>




int main ()  {
	remove("ArchivoLogueoSWAP.txt");
	logSWAP = log_create("ArchivoLogueoSWAP.txt","SWAP",true,LOG_LEVEL_INFO);
	log_info(logSWAP,"Inicio Proceso SWAP");


	leerArchivoConfiguracion();
	paginasLibres = configuracionSWAP.CantidadPaginas;
	creacionDisco();
	inicializarListas();

	int servidor = servidorMultiplexor(configuracionSWAP.PuertoEscucha);
	escucharConexiones();



	//servidorMemoria();

	exit(0);
}



/******************* FUNCIONES ELEMENTALES *****************/

void * iniciar(int idProceso ,int cantidadPaginas){

	/* CASO -1 : HAY ESPACIO PERO NO CONSECUTIVO PARA ASIGNAR PAGINAS
	 * CASO -2: NO HAY ESPACIO PARA ASIGNAR LA CANTIDAD SOLICITADA
	 * CASO 2(DEVUELVE LA PRIMER PAGINA ASIGNADA) : HAY ESPACIO Y SE ASIGNA
	 */
	int nroPagina;
	int paginaInicial;
	t_tablaProcesos* proceso = malloc(sizeof(t_tablaProcesos));
	int posicionEnLista;
	paginaInicial = controlInsercionPaginas(cantidadPaginas);
		switch (paginaInicial)  {
		case -1: /*ESPACIO PARA ASIGNAR PERO NO CONTIGUO -> EJECUTAR COMPACTACION*/
		log_info(logSWAP,"Iniciando Compactacion");
		compactacion();
		log_info(logSWAP,"Compactacion finalizada");
		break;
	case -2: /* NO HAY ESPACIO PARA ASIGNAR PAGINAS DEVOLVER ERROR */
		log_info(logSWAP,"Proceso mProc rechazado por falta de espacido, su PID es %d", idProceso);
		break;
	default:
	 /*ESPACIO CONTIGUO EN DISCO PARA ASIGNAR PAGINAS*/
		posicionEnLista = busquedaPaginaEnLista(paginaInicial);
		/* AGREGO EL PROCESO A LA LISTA*/
		proceso->pid = idProceso;
		proceso->cantidadPaginas = cantidadPaginas;
		proceso->primerPagina = paginaInicial;
		proceso->ultimaPagina = cantidadPaginas + paginaInicial - 1;
		proceso->cantidadEscrituras = 0;
		proceso->cantidadLecturas = 0;
		list_add(listaProcesos,proceso);
			paginasLibres = paginasLibres - cantidadPaginas;

			/* ACTUALIZO MI LISTA DE PAGINAS LIBRES */

			t_tablaPaginasLibres * paginas = list_get(listaPaginasLibres,posicionEnLista);


				if (paginas->hastaPagina + 1 - paginas->desdePagina == cantidadPaginas){
					list_remove(listaPaginasLibres,0);

				}
				else
				{
					paginas->desdePagina = paginas->desdePagina + cantidadPaginas;
				}

			/* INSERTO EN EL DISCO SWAP LAS PAGINAS */

				for (nroPagina= 0; paginaInicial +nroPagina <= cantidadPaginas+paginaInicial;nroPagina++) {
			fseek(archivoDisco,((paginaInicial + nroPagina -1)*configuracionSWAP.TamanioPagina), SEEK_SET);
			fputc('\0',archivoDisco);
			}
			/* LOGEO EN INICIAR */
				log_info(logSWAP,"Proceso mPRoc asignado");
				log_info(logSWAP,"PID asignado: %d",idProceso);
				log_info(logSWAP,"Byte Inicial %d",(paginaInicial-1)*configuracionSWAP.TamanioPagina);
				log_info(logSWAP,"Tamaño en bytes de asignacion %d",cantidadPaginas*configuracionSWAP.TamanioPagina);

			break;
	}
	return NULL;
}

void * finalizar (int PID){

	/* OBTENDO LA INFORMACION DEL PRCESO A FINALIZAR Y ALMACENO EN VARIABLES LOCALES */
	int posicionPIDLista = busquedaPIDEnLista(PID);
	t_tablaProcesos* procesoFinalizado = list_get(listaProcesos,posicionPIDLista);
	int cantidadPaginas =procesoFinalizado->cantidadPaginas;
	int primerPagina = procesoFinalizado->primerPagina;
	int cantidadEscrituras = procesoFinalizado->cantidadEscrituras ;
	int cantidadLecturas = procesoFinalizado->cantidadLecturas ;
	int ultimaPagina = procesoFinalizado->ultimaPagina ;
	int bytesUsadosPorPID = ((ultimaPagina - primerPagina) * configuracionSWAP.TamanioPagina) - 1;
	char * contenido = '\0';





	//ACA VA LA ELIMINACION DEL CONTENIDO DEL SWAP DE ESAS PAGINAS
			fseek(archivoDisco,(primerPagina-1)* configuracionSWAP.TamanioPagina,SEEK_SET);
			fwrite(contenido,bytesUsadosPorPID,primerPagina,archivoDisco);

	//ELIMINO DE LISTA DE PRCESOS Y AGREGO A LISTA DE PAGINAS LIBRES
		//AGREGO LAS PAGINAS LIBERADAS A LA LISTA
			t_tablaPaginasLibres *paginas = malloc(sizeof(t_tablaPaginasLibres));
			paginas->desdePagina = primerPagina;
			paginas->hastaPagina = ultimaPagina;
			list_add(listaPaginasLibres,paginas);

		//LIBERO EL PROCESO DE LA LISTA

	list_remove(listaProcesos,posicionPIDLista);
	paginasLibres = paginasLibres + cantidadPaginas;

	/* LOGEO EN FINALIZAR */
					log_info(logSWAP,"Proceso mPRoc liberado");
					log_info(logSWAP,"PID liberado: %d",PID);
					log_info(logSWAP,"Byte Inicial %d",(primerPagina-1)*configuracionSWAP.TamanioPagina);
					log_info(logSWAP,"Tamaño en bytes liberados %d",bytesUsadosPorPID);

	ordenarLista(); //LUEGO DE INSERTAR ORDENO LA LISTA POR NRO PAGINA LIBRE

	return NULL;
}


char* leer (int PID,int nroPagina){
	int tamanioPagina = configuracionSWAP.TamanioPagina;
	int primerBytePagina = (nroPagina-1) * tamanioPagina;
	char * contenido;
	fseek(archivoDisco,primerBytePagina,SEEK_SET);
	fread(contenido,tamanioPagina,primerBytePagina,archivoDisco); //LEER CONTENIDO UBICADO EN LA PAGINA

	/* LOGEO EN ESCRITURA */
		log_info(logSWAP,"Lectura solicitada");
		log_info(logSWAP,"PID solicitado: %d",PID);
		log_info(logSWAP,"Byte Inicial %d",primerBytePagina);
		log_info(logSWAP,"Tamaño solicitado para lectura %d",strlen(contenido));
		log_info(logSWAP,"El contenido de escritura es %s", contenido);

	//DEVUELVO PAGINA LEIDA
	int posicionPID = busquedaPIDEnLista(PID);
	t_tablaProcesos *procesoObtenido = list_get(listaProcesos,posicionPID);
	procesoObtenido->cantidadLecturas = procesoObtenido->cantidadLecturas + 1;

	return contenido;
}

void * escribir (int PID, int nroPagina, char* contenidoPagina){
	int tamanioPagina = configuracionSWAP.TamanioPagina;
	int primerBytePagina = (nroPagina-1) * tamanioPagina;
	/* LOGEO EN ESCRITURA */
	log_info(logSWAP,"Escritura solicitada");
	log_info(logSWAP,"PID solicitado: %d",PID);
	log_info(logSWAP,"Byte Inicial %d",primerBytePagina);
	log_info(logSWAP,"Tamaño solicitado para escritura %d",strlen(contenidoPagina));
	log_info(logSWAP,"El contenido de escritura es %s", contenidoPagina);

	fseek(archivoDisco,primerBytePagina,SEEK_SET);
	fwrite(contenidoPagina,tamanioPagina,primerBytePagina,archivoDisco);

	int posicionPID = busquedaPIDEnLista(PID);
	t_tablaProcesos *procesoObtenido = list_get(listaProcesos,posicionPID);
	procesoObtenido->cantidadEscrituras = procesoObtenido->cantidadEscrituras + 1;
	return NULL;
}


void * compactacion(){
	sleep(configuracionSWAP.RetardoCompactacion);//SIMULO TIEMPO DE COMPACTACION
	return NULL;
}






/************** FUNCION CONFIGURACION E INICIALIZACION *************/

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

void * inicializarListas(){
	listaProcesos = list_create(); /* CREO MI LISTA ENLAZADA*/
	listaPaginasLibres = list_create();
	t_tablaPaginasLibres *paginasLibres = malloc(sizeof(t_tablaPaginasLibres));
	paginasLibres->desdePagina = 0;
	paginasLibres->hastaPagina = configuracionSWAP.CantidadPaginas -1 ;
	list_add(listaPaginasLibres,paginasLibres);

	return NULL;
}






/***************** FUNCIONES AUXILIARES Y DE BUSQUEDA **************/

int controlInsercionPaginas(int cantidadPaginas) {
	int estadoDevuelto = -1;
	t_tablaPaginasLibres* pagLibres;
	if (cantidadPaginas > paginasLibres){
			estadoDevuelto = -2;
		}
	else
	{
		if (list_size(listaPaginasLibres) == 1){
			    pagLibres = list_get(listaPaginasLibres,0);
				estadoDevuelto = pagLibres->desdePagina;
		}
		else {
			int contador = 0;
			while (contador < list_size(listaPaginasLibres)){
				pagLibres = list_get(listaPaginasLibres,contador);
					if ((pagLibres->hastaPagina - pagLibres->desdePagina) >= cantidadPaginas){
						estadoDevuelto = pagLibres->desdePagina;
						contador = list_size(listaPaginasLibres);
					}
					else{
						contador++;
					}
			}
		}
	}
	return estadoDevuelto;
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

int busquedaPaginaEnLista(int numeroPagina){
	int posicion = 0;
	t_tablaPaginasLibres* paginaLibre;
	paginaLibre = list_get(listaPaginasLibres,posicion);
	while (!paginaLibre->desdePagina == numeroPagina) {
		posicion++;
		paginaLibre = list_get(listaProcesos,posicion);
		}
	return posicion;

}

void * ordenarLista(){
bool _ordenamiento_porPaginasLibres(t_tablaPaginasLibres* paginasLibres, t_tablaPaginasLibres* paginasLibresMenor) {
	return paginasLibres ->desdePagina < paginasLibresMenor->desdePagina;
	}
	list_sort(listaPaginasLibres,(void*) _ordenamiento_porPaginasLibres);
	return NULL;
}



/****************** FUNCIONES SOCKETS *****************/

void * escucharConexiones(){
	char *mensajeRecibido = malloc(sizeof(char *));
	strncpy(mensajeRecibido,datosRecibidos(),sizeof(PACKAGESIZE));
	switch ((int)&mensajeRecibido)
		{
			case INICIAR:

				break;
			case LEER:
					//&pid = PID;

				break;
			case ESCRIBIR:

				break;
			case FINALIZAR:
				//BORRAR TODAS LAS ESTRUCTURAS ADMINISTRATIVAS PARA ESE mProc.
				//AVISAR A SWAP, PARA QUE LIBERE TAMBIEN.
				break;
			default:
				log_info(logSWAP,"Mensaje incorrecto");
		}
		free(mensajeRecibido);

	return NULL;
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

