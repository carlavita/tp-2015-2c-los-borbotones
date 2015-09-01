/*
 * CPU.h
 *
 *  Created on: 1/9/2015
 *      Author: utnso
 */

#ifndef CPU_H_
#define CPU_H_

#define PATH_CONFIG "archivoConfig.conf"
#define PATH_LOG "ArchivoLogueoCPU.txt"


typedef struct
{
	char* IPPlanificador;
	int PuertoPlanificador;
	char* IPMemoria ;
	int PuertoMemoria;
	int CantidadHilos;
	int Retardo;
} t_config_ProcesoCPU;

t_config_ProcesoCPU configuracionCPU;
t_log * logCPU;

#endif /* CPU_H_ */
