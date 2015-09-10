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

typedef
struct
{
	int PuertoEscucha;
	char * NombreSwap;
	int CantidadPaginas ;
	int TamanioPagina;
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

void * creacionDisco();
void leerArchivoConfiguracion();
void servidorMemoria();
void * iniciar(int idProceso ,int cantidadPaginas);
void * finalizar (int PID);
char * leer (int PID ,int nroPagina);
void * escribir (int PID, int nroPagina, char* contenidoPagina);
void * compactacion();
int controlInsercionPaginas(int cantidadPaginas) ;
int procesoCreate(int PID, int cantidadPaginas, int primeraPagina);
int paginasLibresCreate(int desdePagina, int hastaPagina);
int busquedaPIDEnLista(int PID);
void * ordenarLista();
void * inicializarListas();

#endif /* SWAP_H_ */
