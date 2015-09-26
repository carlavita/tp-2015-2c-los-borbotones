/*
 * SWAP.h
 *
 *  Created on: 30/8/2015
 *      Author: utnso
 */

#ifndef SWAP_H_
#define SWAP_H_

#define PUERTO "6008"
#define PACKAGESIZE 1024
#define BACKLOG 5
#include <commons/collections/list.h>

typedef
struct
{
	int PuertoEscucha;
	char * NombreSwap;
	int CantidadPaginas ;
	int TamanioPagina;
	int RetardoSWAP;
	int RetardoCompactacion;

} t_config_ProcesoSWAP;

typedef struct
{
	int pid;
	int cantidadPaginas;
	int primerPagina; //PRIMER BYTE EN EL QUE ESCRIBE EL PID;
	int ultimaPagina;
	int cantidadLecturas;
	int cantidadEscrituras;
}t_tablaProcesos;

typedef struct
{
	int desdePagina;
	int hastaPagina;
}t_tablaPaginasLibres;

t_log * logSWAP;
t_config_ProcesoSWAP configuracionSWAP;

t_list * listaProcesos;
t_list * listaPaginasLibres;
int paginasLibres;
FILE * archivoDisco;

/*** FUNCIONES DE INICIALIZACION ***/
void * creacionDisco();
void leerArchivoConfiguracion();
void * inicializarListas();
void inicializarDisco();

/*** FUNCIONES ELEMENTALES ***/
int iniciar(int idProceso ,int cantidadPaginas);
int finalizar (int PID);
char * leer (int PID ,int nroPagina);
int escribir (int PID, int nroPagina, char* contenidoPagina);
void * compactacion();

/*** FUNCIONES DE BUSQUEDA Y AUXILIARES***/
int controlInsercionPaginas(int cantidadPaginas) ;
int busquedaPIDEnLista(int PID);
int busquedaPaginaEnLista(int numeroPagina);
void * ordenarLista();
void * unificacionEspacioContiguo();

/*** FUNCIONES DE SOCKETS ***/
void escucharMensajes(int servidor);
void servidorMemoria();


#endif /* SWAP_H_ */
