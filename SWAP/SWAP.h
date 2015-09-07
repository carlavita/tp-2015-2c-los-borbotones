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
	int primerByte; //TAMAÑO QUE CARGO, PARA TODOS LOS CAMPOS DE LA TLB.
	int cantidadLecturas;
	int cantidadEscrituras;
}t_tablaProcesos;


int busquedaProximaPosicionLibreVector();
int busquedaPosicionPID (int pid);
void * CreacionDisco();
void * inicializarTablaPaginas(int cantidadPaginas);
void LeerArchivoConfiguracion();
void servidor_Memoria();
void * iniciar(int idProceso ,int cantidadPaginas);
void * finalizar (int PID);
void * leer (int PID ,int nroPagina);
void * escribir (int PID, int nroPagina, char* contenidoPagina);
void * compactacion();
int controlInsercionPaginas(int cantidadPaginas) ;
static t_tablaProcesos *proceso_create(int PID, int cantidadPaginas, int primerByte);
static void proceso_destroy(t_tablaProcesos *self);

#endif /* SWAP_H_ */
