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


#define PATH_CONFIG "Planificador.config"
#define PATH_LOG "LOGPlanificador.config"
// constantes para algoritmos
#define FIFO 1
#define RR 2
typedef struct
{
	int puertoEscucha;
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
