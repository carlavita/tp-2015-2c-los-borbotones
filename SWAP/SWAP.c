/*
 * SWAP.C
 *
 *  Created on: 30/8/2015
 *      Author: Martin Fleichman
 */


#include <stdlib.h>
#include <stdio.h>
#include <commons/log.h>
#include <commons/string.h>
#include <commons/config.h>
#include <commons/txt.h>
#include "ConfigSWAP.h"


t_log * logueo;
t_config_ProcesoSWAP* configuracionSWAP;



void * LeerArchivoConfiguracion()
{
	t_config* cfgSWAP = malloc(sizeof(t_config));


	cfgSWAP = config_create("/home/utnso/tp-2015-2c-los-borbotones/SWAP/archivoConfig.conf");
	configuracionSWAP.PuertoEscucha = config_get_int_value(cfgSWAP,"PUERTO_ESCUCHA");
	configuracionSWAP.NombreSwap = config_get_string_value(cfgSWAP,"NOMBRE_SWAP");
	configuracionSWAP.CantidadPaginas = config_get_int_value(cfgSWAP,"CANTIDAD_PAGINAS");
	configuracionSWAP.TamanioPagina = config_get_int_value(cfgSWAP,"TAMANIO_PAGINA");
	configuracionSWAP.RetardoCompactacion = config_get_int_value(cfgSWAP,"RETARDO_COMPACTACION");

	return NULL;
}



int main ()  {
	remove("ArchivoLogueoSWAP.txt");
	logueo = log_create("ArchivoLogueoSWAP.txt","SWAP",true,LOG_LEVEL_INFO);
	log_info(logueo,"Inicio Proceso SWAP");

	log_info(logueo,"Leyendo Archivo de Configuracion");
	LeerArchivoConfiguracion();
	log_info(logueo,"Archivo de Configuracion Leido correctamente");



	return 0; //CARLA AMOR Y PAZ POR MI 0 :D
}
