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
#include <semaphore.h>
// includes de iblioteca compartida
#include <protocolo.h>
#include <socket.h>


#define PATH_CONFIG "Planificador.config"
//#define PATH_CONFIG "/home/utnso/workspace/Planificador/Planificador.config"

#define PATHLOG "LOGPlanificador.config"
#define PATH_SIZE 256

//constantes de conexion
#define PUERTO "6008"
#define BACKLOG 5			// Define cuantas conexiones vamos a mantener pendientes al mismo tiempo
#define PACKAGESIZE 1024	// Define cual va a ser el size maximo del paquete a enviar
#define PUERTO_SIZE 7 //(MAXIMO 6 MAS EL FIN DE CADENA)


/*

#define SALUDO 0
#define CHECKPOINT 1

//mensajes del Planificador a CPU
#define EJECUTARPROC 2

//mensajes del CPU a Planificador
#define FINALIZAPROCOK 3
#define PROCFALLA 4
#define PROCIO 5
#define FINDERAFAGA 6
#define FINDEQUANTUM 7
*/


// constantes para algoritmos
#define FIFO 1
#define RR 2

// constantes STATUS DE PROCESOS
#define LISTO 1
#define EJECUTA 2
#define BLOQUEADO 3
#define FINALIZADO 4
typedef struct
{
	char* puertoEscucha;
	int algoritmo;
	int quantum;

}t_configPlanificador;

/*
typedef struct
{
	int pid;
	char pathProc[256];
	int proxInst;
	int quantum;
}t_pcb;
*/

typedef struct
{
	//int estado;//(0 libre, 1 ocupada)
	int id;
	int socket;
	int pid;
	int porcentajeUso;
}t_cpu;



/*typedef struct
{
	int pid;//revisar si es necesario este campo
	char* pathProc;
	int proxInst;
	int quantum;

}t_contextoEjecucion;*/

typedef struct
{
	int pid;
	int tiempo;

}t_rtaIO;

int val; //variable para saber el valor del semaforoListos

int PID = 0; // Para numerar los procesos
int CPUID = 0;
int ServidorP = 0;//todo socket cpu, solo por pruebas
t_configPlanificador configPlanificador;
t_log *logger;

/*Listas de planificacion*/

t_list* LISTOS;
t_list* EJECUTANDO;
t_list* BLOQUEADOS;
t_list* FINALIZADOS;
t_list* listaCPU;

//Mutex
pthread_mutex_t mutexListas; //Listas
pthread_mutex_t mutexListaCpu; //Lista de cpus

// Semáforos

sem_t semaforoListos;

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
/**
* @NAME: planificador()
* @DESC: Funcion para hilo de planificador*/
void *planificador(void *info_proc);
/**
* @NAME: planificar()
* @DESC: Planifica de acuerdo al algoritmo, el quantum -1 indica FIFO*/
int planificar(int quantum);

/**
* @NAME: planificarFifo()
* @DESC: Planifica  FIFO, devuelve el pcb (contexto de ejecucion). Lo saca de listos lo pasa a ejecutando
* vamos a usar el mismo para RR porque es un fifo y el fin de quantum lo infomra la CPU*/
t_pcb* planificarFifo();
/**
* @NAME: handle(newsock, set)
* @DESC: Manejo de mensajes recibidos en el select*/

void handle(int newsock, fd_set *set);
/**
* @NAME: ejecutarIO()
* @DESC: ejecuta entrada salida, discpositivo unico, encola.*/
void ejecutarIO(int socketCPU);

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
/**
* @NAME: ejecutarPS()
* @DESC: Muestra los procesos por pantalla
*/
void ejecutarPS();//
/**
* @NAME: ordernarPorPID()
* @DESC: ordena una lista de pcbs ascendente por PID.
*/
void ordernarPorPID(t_list* lista);

/**
* @NAME: ejecutarCPU()
* @DESC: Muestra los porcentajes de uso de cada CPU.
*/
void ejecutarCPU();

/**
* @NAME: obtenerCantidadLineasPath()
* @DESC: Para saber que linea es la ultima, obtiene la cantidad de lineas del archivo mCod
* la funcion retorna el int con el numero de lineas
*/
int obtenerCantidadLineasPath(char* path);
