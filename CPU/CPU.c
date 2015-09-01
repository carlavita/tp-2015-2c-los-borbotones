/*
 * CPU.c
 *
 *  Created on: 1/9/2015
 *      Author: Fernando Sanchez
 */


#include <stdlib.h>
#include <stdio.h>
#include <commons/log.h>
#include <commons/string.h>
#include <commons/config.h>
#include <commons/txt.h>
#include "CPU.h"



void  LeerArchivoConfiguracion()
{
	t_config* cfgCPU = malloc(sizeof(t_config));
	log_info(logCPU,"Leyendo Archivo de Configuracion");
	cfgCPU = config_create(PATH_CONFIG);
	configuracionCPU.IPPlanificador = config_get_string_value(cfgCPU,"IP_PLANIFICADOR");
	configuracionCPU.PuertoPlanificador = config_get_int_value(cfgCPU,"PUERTO_PLANIFICADOR");
	configuracionCPU.IPMemoria = config_get_string_value(cfgCPU,"IP_MEMORIA");
	configuracionCPU.PuertoMemoria = config_get_int_value(cfgCPU,"PUERTO_MEMORIA");
	configuracionCPU.CantidadHilos = config_get_int_value(cfgCPU,"CANTIDAD_HILOS");
	configuracionCPU.Retardo = config_get_int_value(cfgCPU,"RETARDO");
	log_info (logCPU,"%d",configuracionCPU.PuertoPlanificador);
	log_info(logCPU,"Archivo de Configuracion Leido correctamente");

	}



int main ()  {
	remove(PATH_LOG);
	logCPU = log_create(PATH_LOG,"CPU",true,LOG_LEVEL_INFO);
	log_info(logCPU,"Inicio Proceso CPU");


	LeerArchivoConfiguracion();




	return 0; //CARLA AMOR Y PAZ POR MI 0 :D
}
