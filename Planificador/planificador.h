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
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <unistd.h>


#define PATH_CONFIG "Planificador.config"
#define PATH_LOG "LOGPlanificador.config"

//constantes de conexion
#define PUERTO "6008"
#define BACKLOG 5			// Define cuantas conexiones vamos a mantener pendientes al mismo tiempo
#define PACKAGESIZE 1024	// Define cual va a ser el size maximo del paquete a enviar

// constantes para algoritmos
#define FIFO 1
#define RR 2
typedef struct
{
	const char* puertoEscucha;
	int algoritmo;
	int quantum;

}t_config_planificador;


t_config_planificador config_planificador;
t_log *logger;

void levantarConfiguracion();/*Levanta parametros de configuración*/
void mostrar_consola( void *ptr );/*Fucnionalidad de consola*/
void espera_enter ();/*Espera enter en la consola*/
void servidor_CPU( void *ptr );//servidor de cpus


#endif /* PLANIFICADOR_H_ */
