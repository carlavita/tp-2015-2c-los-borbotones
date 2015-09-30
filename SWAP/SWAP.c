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
#include <sys/stat.h>
#include <sys/socket.h>
#include <netdb.h>
#include <protocolo.h>
#include <unistd.h>
#include <fcntl.h>



int main ()  {
	remove("ArchivoLogueoSWAP.txt");
	logSWAP = log_create("ArchivoLogueoSWAP.txt","SWAP",true,LOG_LEVEL_INFO);
	log_info(logSWAP,"Inicio Proceso SWAP");

	leerArchivoConfiguracion();
	paginasLibres = configuracionSWAP.CantidadPaginas;
	creacionDisco();
	archivoDisco = fopen(configuracionSWAP.NombreSwap, "r+");
	inicializarListas();

	inicializarDisco();
/*
  PRUEBAS SWAP CON COMPACTACION
	iniciar(1,10);
	iniciar(2,10);
	iniciar(3,10);
	iniciar(4,32);
	finalizar(2);
	escribir(3,1,"HOLA");
	escribir(4,3,"BUNE");
	escribir(4,5,"PEPE");
	iniciar(5,12);
	leer(3,1);
	leer(4,3);
	leer(4,5);
*/

	int servidor = servidorMultiplexor(configuracionSWAP.PuertoEscucha);
	for (;;){
		escucharMensajes(servidor);
			}



	log_info(logSWAP,"Fin Proceso SWAP");
	//servidorMemoria();
	fclose(archivoDisco);
	exit(0);
}



/******************* FUNCIONES ELEMENTALES *****************/

int iniciar(int idProceso ,int cantidadPaginas){

	/* CASO -1 : HAY ESPACIO PERO NO CONSECUTIVO PARA ASIGNAR PAGINAS
	 * CASO -2: NO HAY ESPACIO PARA ASIGNAR LA CANTIDAD SOLICITADA
	 * CASO 2(DEVUELVE LA PRIMER PAGINA ASIGNADA) : HAY ESPACIO Y SE ASIGNA
	 */
	//int nroPagina;
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
		return ERROR;
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

	/*	char* contenido;
			contenido = string_repeat(' ', cantidadPaginas * configuracionSWAP.TamanioPagina);
			fseek(archivoDisco,paginaInicial*configuracionSWAP.TamanioPagina,SEEK_SET);
			fputs(contenido,archivoDisco);*/

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

			/* LOGEO EN INICIAR */
				log_info(logSWAP,"Proceso mPRoc asignado");
				log_info(logSWAP,"PID asignado: %d",idProceso);
				log_info(logSWAP,"Byte Inicial %d",paginaInicial*configuracionSWAP.TamanioPagina);
				log_info(logSWAP,"Tamaño en bytes de asignacion %d",cantidadPaginas*configuracionSWAP.TamanioPagina);
				return OK;
			break;
	}
		return 0;
}
int finalizar (int PID){

	/* OBTENDO LA INFORMACION DEL PRCESO A FINALIZAR Y ALMACENO EN VARIABLES LOCALES */
	int posicionPIDLista = 1;//busquedaPIDEnLista(PID);
	t_tablaProcesos* procesoFinalizado = list_get(listaProcesos,posicionPIDLista);
	int cantidadPaginas =procesoFinalizado->cantidadPaginas;
	int primerPagina = procesoFinalizado->primerPagina;
	int cantidadEscrituras = procesoFinalizado->cantidadEscrituras ;
	int cantidadLecturas = procesoFinalizado->cantidadLecturas ;
	int ultimaPagina = procesoFinalizado->ultimaPagina ;
	int bytesUsadosPorPID = ((ultimaPagina + 1 - primerPagina) * configuracionSWAP.TamanioPagina);
	//char * contenido = '\0';
	char * contenido = string_repeat('\0',bytesUsadosPorPID);


	//ACA VA LA ELIMINACION DEL CONTENIDO DEL SWAP DE ESAS PAGINAS
			fseek(archivoDisco,primerPagina* configuracionSWAP.TamanioPagina,SEEK_SET);
			//fwrite(contenido,bytesUsadosPorPID,primerPagina,archivoDisco);
			fputs(contenido,archivoDisco);


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
					log_info(logSWAP,"Byte Inicial %d",primerPagina*configuracionSWAP.TamanioPagina);
					log_info(logSWAP,"Tamaño en bytes liberados %d",bytesUsadosPorPID);
					log_info(logSWAP,"Cantidad de Escrituras solicitadas: %d", cantidadEscrituras);
					log_info(logSWAP,"Cantidad de Lecturas solicitadas: %d", cantidadLecturas);

	ordenarLista();//LUEGO DE INSERTAR ORDENO LA LISTA POR NRO PAGINA LIBRE
	unificacionEspacioContiguo();


	return OK;
}


char * leer (int PID,int nroPagina){

	int tamanioPagina = configuracionSWAP.TamanioPagina;
	int posicionPID = 1;//busquedaPIDEnLista(PID);
		t_tablaProcesos *procesoObtenido = list_get(listaProcesos,posicionPID);
	int primerBytePagina = (nroPagina + /*procesoObtenido->primerPagina*/1) * tamanioPagina;
	char * contenido = malloc(tamanioPagina);


	//rewind(archivoDisco);

	fseek(archivoDisco,primerBytePagina,SEEK_SET);
	fgets(contenido,tamanioPagina + 1,archivoDisco); //LEER CONTENIDO UBICADO EN LA PAGINA



	/* LOGEO EN ESCRITURA */
		log_info(logSWAP,"Lectura solicitada");
		log_info(logSWAP,"PID solicitado: %d",PID);
		log_info(logSWAP,"Byte Inicial %d",primerBytePagina);
		log_info(logSWAP,"Tamaño solicitado para lectura %d",string_length(contenido));
		log_info(logSWAP,"El contenido de lectura es %s", contenido);

	//DEVUELVO PAGINA LEIDA

	//procesoObtenido->cantidadLecturas = procesoObtenido->cantidadLecturas + 1;
	sleep(configuracionSWAP.RetardoSWAP);
	return contenido;
}

int escribir (int PID, int nroPagina, char* contenidoPagina){

	int tamanioPagina = configuracionSWAP.TamanioPagina;
	int posicionPID = busquedaPIDEnLista(PID);
			t_tablaProcesos *procesoObtenido = list_get(listaProcesos,posicionPID);
		int primerBytePagina = (nroPagina + procesoObtenido->primerPagina) * tamanioPagina;
	/* LOGEO EN ESCRITURA */
	log_info(logSWAP,"Escritura solicitada");
	log_info(logSWAP,"PID solicitado: %d",PID);
	log_info(logSWAP,"Byte Inicial %d",primerBytePagina);
	log_info(logSWAP,"Tamaño solicitado para escritura %d",string_length(contenidoPagina));
	log_info(logSWAP,"El contenido de escritura es %s", contenidoPagina);



	char* contenido = malloc(tamanioPagina - string_length(contenidoPagina));
	contenido = string_repeat('\0', tamanioPagina - string_length(contenidoPagina) );


	fseek(archivoDisco,primerBytePagina,SEEK_SET);
	//fwrite(contenidoPagina,tamanioPagina,primerBytePagina,archivoDisco);
	fputs(contenidoPagina,archivoDisco);
	fputs(contenido,archivoDisco);



	procesoObtenido->cantidadEscrituras = procesoObtenido->cantidadEscrituras + 1;
	sleep(configuracionSWAP.RetardoSWAP);
	return 14;
}


void * compactacion(){

		ordenarListaPorPagina();
	while (list_size(listaPaginasLibres) > 1){
		t_tablaPaginasLibres* pagLibre = list_get(listaPaginasLibres,0);
		t_tablaPaginasLibres* paginaSiguiente = list_get(listaPaginasLibres,1);

		int DesdeBusqueda = pagLibre->desdePagina;
		int BloquesLibres = pagLibre->hastaPagina - pagLibre->desdePagina;
		int HastaBusqueda = paginaSiguiente->desdePagina;
		bool buscarEnPid = true;
		while(buscarEnPid){
			/* ACA HAGO ALGO*/
			int pos = buscarBloqueAMover(DesdeBusqueda,HastaBusqueda);

			if (pos > -1){
			t_tablaProcesos *procesoObtenido = list_get(listaProcesos,pos);
			int primerBytePID = procesoObtenido->primerPagina * configuracionSWAP.TamanioPagina;
			int cantidadPaginas = procesoObtenido->ultimaPagina - procesoObtenido->primerPagina;
			int tamanioProceso = cantidadPaginas * configuracionSWAP.TamanioPagina;
			char * contenido = malloc(tamanioProceso);


				//rewind(archivoDisco);

				fseek(archivoDisco,primerBytePID,SEEK_SET);
				fgets(contenido,tamanioProceso,archivoDisco); //LEER CONTENIDO UBICADO EN LA PAGINA

				fseek(archivoDisco,primerBytePID,SEEK_SET);
				char * contenidoNuevo = string_repeat('\0',tamanioProceso);
				fputs(contenidoNuevo,archivoDisco);

				fseek(archivoDisco,DesdeBusqueda*configuracionSWAP.TamanioPagina,SEEK_SET);
				fputs(contenido,archivoDisco);
				procesoObtenido->primerPagina = procesoObtenido->primerPagina - BloquesLibres - 1;
				procesoObtenido->ultimaPagina = procesoObtenido->ultimaPagina - BloquesLibres - 1;
				free(contenido);
				free(contenidoNuevo);


			DesdeBusqueda = procesoObtenido->ultimaPagina + 1;

			}
			else
			{
				buscarEnPid = false;
			}
		}
			pagLibre->desdePagina = DesdeBusqueda;
			pagLibre->hastaPagina = DesdeBusqueda + BloquesLibres;
			ordenarLista();
			unificacionEspacioContiguo();
		}

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
	configuracionSWAP.RetardoSWAP = config_get_int_value(cfgSWAP,"RETARDO_SWAP");
	configuracionSWAP.RetardoCompactacion = config_get_int_value(cfgSWAP,"RETARDO_COMPACTACION");
	log_info (logSWAP,"%d",configuracionSWAP.PuertoEscucha);
	log_info(logSWAP,"Archivo de Configuracion Leido correctamente");

}



void * creacionDisco() {

	remove(configuracionSWAP.NombreSwap);
	char command[1000];
	char * page;
	char * count;
	int sizePage = configuracionSWAP.TamanioPagina;
	int countPage = configuracionSWAP.CantidadPaginas;
	page = string_itoa(sizePage);
	count = string_itoa(countPage);

	log_info(logSWAP,"Creando archivo de tamanio: %d",sizePage*countPage);
	/* ARMO COMANDO DD DE CREACION DE ARCHIVO DE TAMAÑO FIJO */
	strcpy(command,"dd if=/dev/zero of=");
	strcat(command,configuracionSWAP.NombreSwap);
	strcat(command, " bs=");
	strcat(command,page);
	strcat(command," count=");
	strcat(command,count);

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

void inicializarDisco() {
	int tamanioDisco = configuracionSWAP.CantidadPaginas
			* configuracionSWAP.TamanioPagina;
	char* contenido;
	contenido = string_repeat('A', tamanioDisco);

	fputs(contenido,archivoDisco);

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
	while (proceso->pid != PID) {
		posicion++;
		proceso = list_get(listaProcesos,posicion);
		}
	return posicion;
}

int busquedaPaginaEnLista(int numeroPagina){
	int poss = 0;
	t_tablaPaginasLibres* paginaLibre;
	paginaLibre = list_get(listaPaginasLibres,poss);
	while (paginaLibre->desdePagina != numeroPagina) {
		poss++;
		paginaLibre = list_get(listaProcesos,poss);
		}
	return poss;

}

void * ordenarLista(){
bool _ordenamiento_porPaginasLibres(t_tablaPaginasLibres* paginasLibres, t_tablaPaginasLibres* paginasLibresMenor) {
	return paginasLibres ->desdePagina < paginasLibresMenor->desdePagina;
	}
	list_sort(listaPaginasLibres,(void*) _ordenamiento_porPaginasLibres);
	return NULL;
}

void ordenarListaPorPagina(){
	bool _ordenamiento_porPaginaInicial(t_tablaProcesos* procesoMayor, t_tablaProcesos* procesoMenor) {
		return procesoMayor ->primerPagina < procesoMenor->primerPagina;
		}
		list_sort(listaPaginasLibres,(void*) _ordenamiento_porPaginaInicial);
		}


void * unificacionEspacioContiguo(){
	t_tablaPaginasLibres* pagLibre;
	t_tablaPaginasLibres* paginaSiguiente;
	int contador = 0;
	while ((contador + 1 < list_size(listaPaginasLibres)) && (list_size(listaPaginasLibres) > 1)){
	pagLibre = list_get(listaPaginasLibres,contador);
	paginaSiguiente = list_get(listaPaginasLibres,contador+1);
	if (paginaSiguiente->desdePagina - pagLibre->hastaPagina == 1){
		pagLibre->hastaPagina = paginaSiguiente->hastaPagina;
		list_remove(listaPaginasLibres,contador+1);
	}
	else
	{
		contador++;
	}
	}
	return NULL;
}

int buscarBloqueAMover (int DesdePosicion, int HastaPosicion){
	t_tablaProcesos *procesoObtenido;
	int posicion = -1;
	int contador = 0;
	while (contador < list_size(listaProcesos)){
					procesoObtenido = list_get(listaProcesos,contador);
						if ((procesoObtenido->primerPagina > DesdePosicion) && (procesoObtenido->ultimaPagina < HastaPosicion) ){
							posicion = contador;
							contador = list_size(listaProcesos);
						}
						else{
							contador++;
						}
				}

	return posicion;
}


/****************** FUNCIONES SOCKETS *****************/

void escucharMensajes(int servidor){
		int mensaje1 = 0;
		int pid, paginas,status;
		t_mensajeHeader mensajeHeader;
		t_iniciarPID estructuraMemoria;
		t_finalizarPID estructuraMemoriaFinalizar;
		t_leer estructuraMemoriaLeer;
	    mensaje1 = recv(servidor, &mensajeHeader, sizeof(t_mensajeHeader), 0);
	    printf("mensaje recibido: %d",mensajeHeader.idmensaje);
	    fflush(stdout);
	    switch (mensajeHeader.idmensaje)
		{
			case INICIAR:
				log_info(logSWAP,"Se recibio mensaje INICIAR");
				//sendACK(servidor);
				recv(servidor,&estructuraMemoria,sizeof(t_iniciarPID),0);
				//sendACK(servidor);
			 status = iniciar(estructuraMemoria.pid,estructuraMemoria.paginas);
				t_mensajeHeader iniciar;
					iniciar.idmensaje = status;
					send(servidor,&iniciar,sizeof(t_mensajeHeader),0);
				//sleep(10);
				break;
			case LEER:
				log_info(logSWAP,"Se recibio mensaje INICIAR");
				recv(servidor,&estructuraMemoriaLeer,sizeof(t_leer),0);

				char * contenido = malloc(configuracionSWAP.TamanioPagina);
				contenido = leer(estructuraMemoriaLeer.pid,estructuraMemoriaLeer.pagina);
				int tamanio = string_length(contenido);
				send(servidor,&tamanio, sizeof(int),0);

				send(servidor,contenido,string_length(contenido),0);

				break;
			case ESCRIBIR:
				log_info(logSWAP,"Se recibio el mensaje ESCRIBIR");
				//sendACK(servidor);
				recv(servidor,&pid,sizeof(int),0);
				//sendACK(servidor);
				recv(servidor,&paginas,sizeof(int),0);
				//sendACK(servidor);
				int sizeContenido;
				recv(servidor,&sizeContenido, sizeof(int),0);
				//sendACK(servidor);
				char * contenidoEscribir = malloc(sizeContenido);
				recv(servidor,&contenidoEscribir,sizeof(char*),0);
				int status = escribir(pid,paginas,contenidoEscribir);
				send(servidor,status,sizeof(int),0);

				break;
			case FINALIZAR:
				log_info(logSWAP,"Se recibio mensaje FINALIZAR");
				recv(servidor,&estructuraMemoriaFinalizar,sizeof(t_finalizarPID),0);
				status = finalizar(estructuraMemoriaFinalizar.pid);
				printf("FINALIZAR!!!!!: %d",status);
				t_mensajeHeader finalizar;
				finalizar.idmensaje = status;
				send(servidor,&finalizar,sizeof(t_mensajeHeader),0);
				break;
			default:
				log_info(logSWAP,"Mensaje incorrecto");
		}

}
