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
pthread_mutex_t mutex_listos;

void levantarConfiguracion();/*Levanta parametros de configuración*/
void mostrar_consola( void *ptr );/*Fucnionalidad de consola*/
void espera_enter ();/*Espera enter en la consola*/
void servidor_CPU( void *ptr );//servidor de cpus
void inicializar_listas();//listas inicializadas
void correr_path();
int crear_pcb(char* path);
#endif /* PLANIFICADOR_H_ */
