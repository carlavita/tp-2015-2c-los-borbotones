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
#include <commons/collections/list.h>
#include <pthread.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <unistd.h>

// lIBRERIA COMPARTIDA
#include <protocolo.h>

#define PACKAGESIZE 1024	// Define cual va a ser el size maximo del paquete a enviar
#define MAXRTA 256

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
	char* PuertoPlanificador;
	char* IPMemoria ;
	char* PuertoMemoria;
	int CantidadHilos;
	int Retardo;
} t_config_ProcesoCPU;


typedef struct __attribute__((packed))
{
	int instruccion;
	char resultado[MAXRTA];

}t_rtaEjecucion;

typedef struct {
	int pid;
	int tiempoIO;
} t_io;


t_config_ProcesoCPU configuracionCPU;
t_log * logCPU;
int serverMemoria = 0;
int cpuID = 1;//seteado para probar-->despues pasar a N cpus creados cos sus respectivos hilos

//semaforos
pthread_mutex_t mutexLogueo;//Mutex para archivo de logueo

//funciones de la cpu

void LeerArchivoConfiguracion();
void Conexion_con_planificador();
int conexion_con_memoria();
int busquedaPosicionCaracter (int posicion,char *listaDeArchivos, char valorABuscar);
char *parsearLinea(char * lineaLeida);
t_list* ejecutarmProc(t_pcb pcbProc);
void inicializarSemaforosCPU();


void *ejecucion (void *ptr);
void iniciar (int paginas, int mProcID);
void escribir (int pagina, char *texto, int mProcID);
void leer (int pagina, int mProcID);
void finalizar (int mProcID);



//Funciones para parser
bool esLeer(char* linea);
bool esIniciar(char* linea);
bool esFinalizar(char* linea);
bool esEscribir(char* linea);
bool esIO(char* linea);
void parsermCod(char *path);// Abre el path y parsea el contenido completo
#endif /* CPU_H_ */
