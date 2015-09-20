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
#include <pthread.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <unistd.h>

// lIBRERIA COMPARTIDA
#include <protocolo.h>

#define PACKAGESIZE 1024	// Define cual va a ser el size maximo del paquete a enviar

#define PATH_CONFIG "archivoConfig.conf"
#define PATH_LOG "ArchivoLogueoCPU.txt"

#define TOKENINICIAR "iniciar"
#define TOKENLEER "leer"
#define TOKENESCRIBIR "escribir"
#define TOKENFINALIZAR "finalizar"
#define TOKENIO "entrada-salida"


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
int busquedaPosicionCaracter (int posicion,char *listaDeArchivos, char valorABuscar);
char *parsearLinea(char * lineaLeida);

//Funciones para parser
bool esLeer(char* linea);
bool esIniciar(char* linea);
bool esFinalizar(char* linea);
bool esEscribir(char* linea);
bool esIO(char* linea);
void parsermCod(char *path);// Abre el path y parsea el contenido completo
#endif /* CPU_H_ */
