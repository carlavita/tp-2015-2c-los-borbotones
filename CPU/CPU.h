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

char PATH[256];

#define MAXTHREADS 15

typedef struct __attribute__((packed))
{
	int id;
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


typedef struct __attribute__((packed))
{
	int instruccion;
	char resultado[MAXRTA];

}t_rtaEjecucion;


t_config_ProcesoCPU configuracionCPU;
t_log * logCPU;
//static int serverMemoria = 0;//server para memoria
//int cpuID = 1;
//static int serverSocket; //socket de conexion con planificador
//semaforos
pthread_mutex_t mutexLogueo;//Mutex para archivo de logueo

//OJO!!!!!!
FILE* fid;


//funciones de la cpu


void* thread_func(void* cpu);

void LeerArchivoConfiguracion();
void Conexion_con_planificador(int cpu);

//int Conexion_con_planificador();

int conexion_con_memoria();
int busquedaPosicionCaracter (int posicion,char *listaDeArchivos, char valorABuscar);
char *parsearLinea(char * lineaLeida);
t_list* ejecutarmProc(int cpu,t_pcb pcbProc,int serverSocket,int serverMemoria);
void inicializarSemaforosCPU();


void *ejecucion (void *ptr);
void iniciar (int cpu, int paginas, int mProcID,int serverSocket,int serverMemoria);
void escribir (int pagina, char *texto, int mProcID,int serverSocket,int serverMemoria);
void leer (int pagina, int mProcID,int serverSocket,int serverMemoria);
void finalizar(int cpu, int mProcID, int instrucciones,int serverSocket,int serverMemoria);
void procesaIO(int pid, int tiempo, int cpu, int instrucciones,int serverSocket,int serverMemoria);



//Funciones para parser
bool esLeer(char* linea);
bool esIniciar(char* linea);
bool esFinalizar(char* linea);
bool esEscribir(char* linea);
bool esIO(char* linea);
// Abre el path y parsea el contenido desde la linea indicada

//void parsermCod(int cpu,char *path, int pid, int lineaInicial,int serverSocket,int serverMemoria);
void parsermCod(int cpu,char *path, int pid, int lineaInicial,int serverSocket,int serverMemoria, int quantum);

#endif /* CPU_H_ */
