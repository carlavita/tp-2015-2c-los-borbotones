/*
 * administradorDeMemoria.h
 *
 *  Created on: 16/9/2015
 *      Author: utnso
 */

#ifndef ADMINISTRADORDEMEMORIA_H_
#define ADMINISTRADORDEMEMORIA_H_
#define PACKAGESIZE 1024


typedef struct
{
	int pid;
	int frameAsignado;
}t_pidFrame;

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

typedef struct
{
	int pid;
	int numeroPagina;
	int frame;
}t_TLB;    //LA TLB VA A SER UN ARRAY DE N entradas.(por ahora)

typedef struct
{
	int pid;
	int marco;
	int pagina;
	int bitUso;
	int bitModificado;
}t_tablaDePaginas;

void leerConfiguracion();
void crearServidor();
t_TLB * generarTLB(int entradasTLB);
void creacionTLB(const t_config_memoria* configMemoria, t_log* logMemoria,t_TLB* tlb);
int ConexionMemoriaSwap(t_config_memoria* configMemoria, t_log* logMemoria);
void generarTablaDePaginas(int * memoriaReservadaDeMemPpal,int pid, int cantidadDePaginas);
void generarCantidadDeFramesAsignadosAlProceso(int pid,int cantidadDePaginas);
void avisarAlSwap(int clienteSwap);
void generarEstructurasAdministrativas(int pid,int paginas);
void procesamientoDeMensajes(int cliente,int servidor);
void generarEstructuraAdministrativaPIDFrame();


#endif /* ADMINISTRADORDEMEMORIA_H_ */
