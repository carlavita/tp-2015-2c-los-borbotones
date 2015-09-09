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
#include <commons/log.h>
#include <commons/string.h>
#include <commons/config.h>
#include <commons/txt.h>
#include <commons/collections/list.h>
#include <string.h>



#define PATH_CONFIG "Planificador.config"
//#define PATH_CONFIG "/home/utnso/workspace/Planificador/Planificador.config"

#define PATH_LOG "LOGPlanificador.config"
#define PATH_SIZE 256

//constantes de conexion
#define PUERTO "6008"
#define BACKLOG 5			// Define cuantas conexiones vamos a mantener pendientes al mismo tiempo
#define PACKAGESIZE 1024	// Define cual va a ser el size maximo del paquete a enviar
#define PUERTO_SIZE 7 //(MAXIMO 6 MAS EL FIN DE CADENA)

#define SALUDO 0
#define CHECKPOINT 1  //mensaje de prueba todo al protocolo
// constantes para algoritmos
#define FIFO 1
#define RR 2
typedef struct
{
	char* puertoEscucha;
	int algoritmo;
	int quantum;

}t_config_planificador;

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
}t_cpu;



typedef struct
{
	int codMje;

}t_mensaje_header;

int PID = 0; // Para numerar los procesos
int servidor = 0;//todo socket cpu, solo por pruebas
t_config_planificador config_planificador;
t_log *logger;

/*Listas de planificacion*/

t_list* NUEVOS;
t_list* LISTOS;
t_list* EJECUTANDO;
t_list* BLOQUEADOS;
t_list* FINALIZADOS;

// Semáforo para listas de pcbs
pthread_mutex_t mutex_listas;
/**
* @NAME: levantarConfiguracion()
* @DESC:Levanta parametros de configuracion
*/
void levantarConfiguracion();/*Levanta parametros de configuración*/
/**
* @NAME: mostrar_consola()
* @DESC: Muestra consola para planificador
*/
void mostrar_consola( void *ptr );/*Fucnionalidad de consola*/
/**
* @NAME: espera_enter()
* @DESC: Espera enter para continuar (Limpia y vuelve a mostrar opciones de consola)
*/
void espera_enter ();
/**
* @NAME: servidor_CPU()
* @DESC: Función para hilo de servidor
*/
void servidor_CPU( void *ptr );//servidor de cpus
/**
* @NAME: inicializar_listas()
* @DESC: Crea las listas del planificador
*/
void inicializar_listas();//listas inicializadas
/**
* @NAME: correr_path()
* @DESC: dispara ejecución de un mProc
*/
void correr_path();
/**
* @NAME: correr_path()
* @DESC: crea el PCB de un mProc , lo deja listo para ejecutar
*/
int crear_pcb(char* path);
/**
* @NAME: removerEnListaPorPid()
* @DESC: recibe una lista y elimina el elemento que tenga ese pid
*/
void removerEnListaPorPid(t_list *lista, int pid);
/**
* @NAME: enviarACpu()
* @DESC: Pasa el pcb de listo a ejecutando y envía el pedido a la CPU*/
void enviarACpu(t_pcb* pcb,t_cpu* cpu);

#endif /* PLANIFICADOR_H_ */
