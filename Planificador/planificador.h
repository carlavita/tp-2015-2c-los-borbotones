/*
 * planificador.h
 *
 *  Created on: 30/8/2015
 *      Author: utnso
 */

#ifndef PLANIFICADOR_H_
#define PLANIFICADOR_H_


#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <arpa/inet.h>
#include <commons/log.h>
#include <commons/string.h>
#include <commons/config.h>
#include <commons/txt.h>
#include <commons/collections/list.h>
#include <string.h>

#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <unistd.h>



#define PATH_CONFIG "Planificador.config"
//#define PATH_CONFIG "/home/utnso/workspace/Planificador/Planificador.config"

#define PATHLOG "LOGPlanificador.config"
#define PATH_SIZE 256

//constantes de conexion
#define PUERTO "6008"
#define BACKLOG 5			// Define cuantas conexiones vamos a mantener pendientes al mismo tiempo
#define PACKAGESIZE 1024	// Define cual va a ser el size maximo del paquete a enviar
#define PUERTO_SIZE 7 //(MAXIMO 6 MAS EL FIN DE CADENA)


//todo pasar a un protocolo.h
#define SALUDO 0
#define CHECKPOINT 1

//mensajes del Planificador a CPU
#define EJECUTARPROC 2

//mensajes del CPU a Planificador
#define FINALIZAPROCOK 3
#define PROCFALLA 4
#define PROCIO 5



// constantes para algoritmos
#define FIFO 1
#define RR 2
typedef struct
{
	char* puertoEscucha;
	int algoritmo;
	int quantum;

}t_configPlanificador;

typedef struct
{
	int pid;
	char* pathProc;
	int proxInst;

}t_pcb;

typedef struct
{
	int estado;//(0 libre, 1 ocupada)
	int id;
	int socket;
	int pid;
}t_cpu;



typedef struct
{
	int codMje;

}t_mensajeHeader;

typedef struct
{
	char* pathProc;
	int proxInst;
	int quantum;

}t_contextoEjecucion;


int PID = 0; // Para numerar los procesos
int servidor = 0;//todo socket cpu, solo por pruebas
t_configPlanificador configPlanificador;
t_log *logger;

/*Listas de planificacion*/

t_list* LISTOS;
t_list* EJECUTANDO;
t_list* BLOQUEADOS;
t_list* FINALIZADOS;
t_list* lista_CPU;

// Semáforo para listas de pcbs
pthread_mutex_t mutex_listas;
pthread_mutex_t mutex_lista_cpu;
/**
* @NAME: levantarConfiguracion()
* @DESC:Levanta parametros de configuracion
*/
void levantarConfiguracion();/*Levanta parametros de configuración*/
/**
* @NAME: mostrar_consola()
* @DESC: Muestra consola para planificador
*/
void mostrarConsola( void *ptr );/*Fucnionalidad de consola*/
/**
* @NAME: espera_enter()
* @DESC: Espera enter para continuar (Limpia y vuelve a mostrar opciones de consola)
*/
void esperaEnter ();
/**
* @NAME: servidor_CPU()
* @DESC: Función para hilo de servidor
*/
void servidorCPU( void *ptr );//servidor de cpus
/**
* @NAME: inicializar_listas()
* @DESC: Crea las listas del planificador
*/
void inicializarListas();//listas inicializadas
/**
* @NAME: correr_path()
* @DESC: dispara ejecución de un mProc
*/
void correrPath();
/**
* @NAME: correr_path()
* @DESC: crea el PCB de un mProc , lo deja listo para ejecutar
*/
int crearPcb(char* path);
/**
* @NAME: removerEnListaPorPid()
* @DESC: recibe una lista y elimina el elemento que tenga ese pid
*/
void removerEnListaPorPid(t_list *lista, int pid);
/**
* @NAME: enviarACpu()
* @DESC: Pasa el pcb de listo a ejecutando y envía el pedido a la CPU*/
void enviarACpu(t_pcb* pcb,t_cpu* cpu);

void *planificador(void *info_proc);

int planificarRR(int quantum);

int planificarFifo();

void handle(int newsock, fd_set *set);

/**
* @NAME: agregar_CPU()
* @DESC: Agrega CPU a la lista de cpus, con PID -1 (asi identificamos las que estan libres)
*/
void agregarCPU(int cpuSocket, int pid);

/**
* @NAME: agregar_CPU()
* @DESC: Elimina CPU de lista de CPUs
*/
void eliminarCPU(int socket_cpu);


/**
* @NAME: buscar_Cpu_Libre();
* @DESC: Busca CPU libre, es decir que tenga PID == -1
*/
t_cpu* buscarCpuLibre();
#endif /* PLANIFICADOR_H_ */
