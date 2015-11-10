/*
 * administradorDeMemoria.h
 *
 *  Created on: 16/9/2015
 *      Author: utnso
 */

#ifndef ADMINISTRADORDEMEMORIA_H_
#define ADMINISTRADORDEMEMORIA_H_
#define PACKAGESIZE 1024
#include <errno.h>

typedef struct {
	int pid;
	int frameAsignado;
	int frameUsado;
	int frameModificado;
	int puntero;
} t_pidFrame;

typedef struct {
	int puertoEscucha;
	char* ipSwap;
	char * puertoSwap;
	int maximoMarcosPorProceso;
	int cantidadDeMarcos;
	int tamanioMarcos;
	int entradasTLB;
	int tlbHabilitada; /*Las commons no tiene un config_ge_bool.... => 0 false, 1 true*/
	int retardoMemoria;
} t_config_memoria;

typedef struct {
	int pid;
	int pagina;
} t_TLB;

typedef struct {
	int pid;
	int marco;
	int pagina;
	int bitValidez;
	int presencia;
	int bitUso;
	int bitModificado;
	char * direccion;
} t_tablaDePaginas;

typedef struct {
	int frame;
	int pid; //POR EL MULTIHILO
	int pagina;
} t_estructuraAlgoritmoReemplazoPaginas;

typedef struct
{
	int pid;
	void * estructura;
}t_estructurasDelProceso;


t_config_memoria configMemoria;
t_log * logMemoria;
int clienteSwap;
t_list * tablaDePaginas;
t_pidFrame * tablaAdministrativaProcesoFrame; //INICIALIZAR EN EL MAIN()  !!!!!!
char * memoriaReservadaDeMemPpal;
pthread_t * hiloSigUsr1;
pthread_t * hiloSigUsr2;
t_list * listaDePidFrames;
t_list * tlb;
char * recibidoPorLaMemoria;
char mensaje[1024];
t_list * estructuraAlgoritmos;
t_list * estructurasPorProceso;


void leerConfiguracion();
void crearServidor();
void generarTLB(int entradasTLB);
void creacionTLB(const t_config_memoria* configMemoria, t_log* logMemoria);
int ConexionMemoriaSwap(t_config_memoria* configMemoria, t_log* logMemoria);
void generarTablaDePaginas(char * memoriaReservadaDeMemPpal, int pid,
		int cantidadDePaginas);
void AsignarFrameAlProceso(int pid, int cantidadDePaginas);
//void avisarAlSwap(int clienteSwap);
void generarEstructuraAdministrativaPidFrame(int pid, int paginas);
void procesamientoDeMensajes(int cliente, int servidor);
void generarEstructuraAdministrativaPIDFrame();
void enviarIniciarSwap(int cliente, t_iniciarPID *estructuraCPU,
		t_mensajeHeader mensajeHeaderSwap, int servidor, t_log* logMemoria);
int busquedaPIDEnLista(int PID, int pagina);
void RealizarVolcadoMemoriaLog();
/*ALGORITMO FIFO*/
int algoritmoFIFO(int pid);
int llamar(int pid);
int CantidadDeFrames(int pid);
t_list * busquedaListaFramesPorPid (int pid);
int colaParaReemplazo(int frameAReemplazar, int cantidadDeFrames, int pid);
char * pedirContenidoAlSwap(int cliente, int pid, int pagina, int servidor);
void escribir(t_escribir * estructuraEscribir, int socketSwap);
void escribirContenidoSwap(t_escribir * estructEscribir,int socketSwap);
void escribirContenido(t_escribir * estructEscribir,int frame);

//ALGORITMO CLOCK
int ejecutarAlgoritmoClock (int pid, t_list * listaARemplazar);
int algoritmoClockModificado (int pid);
int busquedaPosicionAlgoritmo (t_list * listaBusqueda);

#endif /* ADMINISTRADORDEMEMORIA_H_ */

