/*
 * administradorDeMemoria.h
 *
 *  Created on: 16/9/2015
 *      Author: utnso
 */

#ifndef ADMINISTRADORDEMEMORIA_H_
#define ADMINISTRADORDEMEMORIA_H_
#define PACKAGESIZE 1024

#include <socket.h>
#include <protocolo.h>
#include <errno.h>

typedef struct {
	int pid;
	int frameAsignado;
	int frameUsado;
} t_pidFrame;

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
} t_config_memoria;

typedef struct {
	int pid;
	int pagina;
	int frame;
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

typedef struct
{
	int pid;
	t_tablaDePaginas * estructura;
} t_estructurasDelProceso;

t_config_memoria configMemoria;
t_log * logMemoria;
t_pidFrame * tablaAdministrativaProcesoFrame; //INICIALIZAR EN EL MAIN()  !!!!!!
pthread_t * hiloSigUsr1;
pthread_t * hiloSigUsr2;


t_list * listaDePidFrames;
t_list * tlb;
t_list * estructuraAlgoritmos;
t_list * estructurasPorProceso;

int clienteSwap;
char * recibidoPorLaMemoria;
char * memoriaReservadaDeMemPpal;

void leerConfiguracion();
void crearServidor();
void generarTLB(int entradasTLB);
void creacionTLB(const t_config_memoria* configMemoria, t_log* logMemoria);
int ConexionMemoriaSwap(t_config_memoria* configMemoria, t_log* logMemoria);
void generarTablaDePaginas(char* memoriaReservadaDeMemPpal, int pid,
int cantidadDePaginas, t_list  * tablaDePaginas);
void AsignarFrameAlProceso(int pid, int cantidadDePaginas);
void generarEstructuraAdministrativaPidFrame(int pid, int paginas);
void procesamientoDeMensajes(int cliente, int servidor);
void generarEstructuraAdministrativaPIDFrame();
void enviarIniciarSwap(int cliente, t_iniciarPID *estructuraCPU,t_mensajeHeader mensajeHeaderSwap, int servidor, t_log* logMemoria);
int busquedaPidPaginaEnLista(int PID, int pagina,t_list* tablaDePaginas);
int buscarEnTablaDePaginas(int pid, int pagina, t_list * tablaDePaginas,int posicion);
void RealizarVolcadoMemoriaLog();
void ActualizarFrame(t_tablaDePaginas* paginaAAsignar, int pid,char * contenido, int frame);
void buscarContenidoPagina(int pid, int pagina, int socketCPU, t_list* tablaDePaginas);
int buscarPaginasProceso(int pid,int pagina);
/*ALGORITMO FIFO*/
int algoritmoFIFO(int pid);
int llamar(int pid);
int CantidadDeFrames(int pid);
#endif /* ADMINISTRADORDEMEMORIA_H_ */

