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
	int pagina;
}t_TLB;

typedef struct
{
	int pid;
	int marco;
	int pagina;
	int bitValidez;
	int presencia;
	int bitUso;
	int bitModificado;
	char * contenido;
}t_tablaDePaginas;

typedef struct
{
	int frame;
	int pagina;
}t_estructuraAlgoritmoReemplazoPaginas;

t_config_memoria configMemoria;
t_log * logMemoria;
int clienteSwap;
t_list * tablaDePaginas;
t_pidFrame * tablaAdministrativaProcesoFrame; //INICIALIZAR EN EL MAIN()  !!!!!!
int * memoriaReservadaDeMemPpal;
pthread_t * hiloSigUsr1;
pthread_t * hiloSigUsr2;
t_list * listaDePidFrames;
t_list * tlb;
char * recibidoPorLaMemoria;
char mensaje[1024];
t_list * estructuraAlgoritmos;





void leerConfiguracion();
void crearServidor();
void generarTLB(int entradasTLB);
void creacionTLB(const t_config_memoria* configMemoria, t_log* logMemoria);
int ConexionMemoriaSwap(t_config_memoria* configMemoria, t_log* logMemoria);
void generarTablaDePaginas(int * memoriaReservadaDeMemPpal,int pid, int cantidadDePaginas);
void generarCantidadDeFramesAsignadosAlProceso(int pid,int cantidadDePaginas);
//void avisarAlSwap(int clienteSwap);
void generarEstructuraAdministrativaPidFrame(int pid,int paginas);
void procesamientoDeMensajes(int cliente,int servidor);
void generarEstructuraAdministrativaPIDFrame();
void enviarIniciarSwap(int cliente, t_iniciarPID *estructuraCPU,
		t_mensajeHeader mensajeHeaderSwap, int servidor, t_log* logMemoria);


#endif /* ADMINISTRADORDEMEMORIA_H_ */
