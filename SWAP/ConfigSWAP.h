/*
 * ConfigSWAP.h
 *
 *  Created on: 30/8/2015
 *      Author: utnso
 */

#ifndef CONFIGSWAP_H_
#define CONFIGSWAP_H_

typedef
struct
{
	int PuertoEscucha;
	char * NombreSwap;
	int CantidadPaginas ;
	int TamanioPagina;
	int RetardoCompactacion;

} t_config_ProcesoSWAP;

#endif /* CONFIGSWAP_H_ */
