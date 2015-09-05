/*
 * CPU.h
 *
 *  Created on: 1/9/2015
 *      Author: utnso
 */

#ifndef CPU_H_
#define CPU_H_


#include <stdlib.h>
#include <stdio.h>
#include <commons/log.h>
#include <commons/string.h>
#include <commons/config.h>
#include <commons/txt.h>

#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <unistd.h>

#define PACKAGESIZE 1024	// Define cual va a ser el size maximo del paquete a enviar

#define PATH_CONFIG "archivoConfig.conf"
#define PATH_LOG "ArchivoLogueoCPU.txt"


typedef struct
{
	char* IPPlanificador;
	//int PuertoPlanificador;
	char* PuertoPlanificador;
	char* IPMemoria ;
	char* PuertoMemoria;
	int CantidadHilos;
	int Retardo;
} t_config_ProcesoCPU;

t_config_ProcesoCPU configuracionCPU;
t_log * logCPU;
int serverMemoria = 0;


//funciones de la cpu

void LeerArchivoConfiguracion();
void Conexion_con_planificador();
int conexion_con_memoria();

#endif /* CPU_H_ */
