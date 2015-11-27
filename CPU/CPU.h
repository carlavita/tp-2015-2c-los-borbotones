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
#include <errno.h>
#include <time.h>       /* time_t, struct tm, difftime, time, mktime */

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

#define SEPARADORINSTRUCCION "/"

char PATH[256];

#define MAXTHREADS 15

typedef struct __attribute__((packed))
{
	int id;
	double porcentajeCPU;
	time_t valori;
	time_t valorf;
	int contadorEjecutadas;
	int serverSocket;
}t_envio;

typedef struct
{
	char* IPPlanificador;
	char* PuertoPlanificador;
	char* IPMemoria ;
	char* PuertoMemoria;
	int CantidadHilos;
	int Retardo;
} t_config_ProcesoCPU;


typedef struct {
	//int estado;//(0 libre, 1 ocupada)
	int id;
	int socket;
	int pid;
	double porcentajeUso;
} t_cpu;

typedef struct __attribute__((packed))
{
	int instruccion;
	char resultado[MAXRTA];

}t_rtaEjecucion;


t_config_ProcesoCPU configuracionCPU;
t_log * logCPU;


//semaforos
pthread_mutex_t mutexLogueo;//Mutex para archivo de logueo

//funciones de la cpu

void calcularPorcentaje(void *ptr);
void* thread_func(void* cpu);

void LeerArchivoConfiguracion();
void Conexion_con_planificador(t_envio *param);


int conexion_con_memoria();
int busquedaPosicionCaracter (int posicion,char *listaDeArchivos, char valorABuscar);
char *parsearLinea(char * lineaLeida);
void inicializarSemaforosCPU();


void *ejecucion (void *ptr);
char *iniciar (int cpu,FILE * fid, int paginas, int mProcID,int serverSocket,int serverMemoria);
char *escribir (int pagina, char *texto, int mProcID,int serverSocket,int serverMemoria);
char *leer (int pagina, int mProcID,int serverSocket,int serverMemoria);
char *finalizar(int cpu, int mProcID, int instrucciones,int serverSocket,int serverMemoria);
char *procesaIO(int pid, int tiempo, int cpu, int instrucciones,int serverSocket,int serverMemoria);


//Funciones para parser
bool esLeer(char* linea);
bool esIniciar(char* linea);
bool esFinalizar(char* linea);
bool esEscribir(char* linea);
bool esIO(char* linea);

// Abre el path y parsea el contenido desde la linea indicada
void parsermCod(t_envio *param,char *path, int pid, int lineaInicial,int serverSocket,int serverMemoria, int quantum);

#endif /* CPU_H_ */
