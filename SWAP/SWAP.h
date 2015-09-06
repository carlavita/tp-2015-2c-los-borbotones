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
	int cantidadPagidas;
	int primerByte; //TAMAÑO QUE CARGO, PARA TODOS LOS CAMPOS DE LA TLB.
}t_tablaPaginas;


int busquedaProximaPosicionLibreVector();
int busquedaPosicionPID (int pid);
void * CreacionDisco();
void * inicializarTablaPaginas(int cantidadPaginas);
void LeerArchivoConfiguracion();
void servidor_Memoria();
void * iniciar(int idProceso ,int cantidadPaginas);
void * finalizar (int PID);
void * leer (int nroPagina);
void * escribir (int nroPagina, char* contenidoPagina);
void * compactacion();
int controlInsercionPaginas(int cantidadPaginas) ;

#endif /* SWAP_H_ */
