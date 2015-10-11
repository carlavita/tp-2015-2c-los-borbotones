/*
 * protocolo.h
 *
 *  Created on: 12/9/2015
 *      Author: utnso
 */

#ifndef PROTOCOLO_H_
#define PROTOCOLO_H_

#define ACK 0
#define NAK 1

#define SALUDO 2
#define CORRERPATH 3

//mensajes del Planificador a CPU
#define EJECUTARPROC 4

//mensajes del CPU a Planificador
#define FINALIZAPROCOK 5
#define PROCFALLA 6
#define FINDERAFAGA 7
#define FINDEQUANTUM 8

//MENSAJES de operaciones
#define PROCIO 9
#define INICIAR 10
#define FINALIZAR 11
#define LEER 12
#define ESCRIBIR 13


#define OK 14
#define ERROR 15

#define RESULTADOLECTURA 16

//estados de linea de mCod



#include <sys/types.h>
#include <sys/socket.h>
#include <string.h> // memcpy
#include <stdlib.h> //malloc /realloc

//estructura de header
typedef struct __attribute__((packed))
{
	//int idmodulo;
	int idmensaje;
	int size; // payload
}t_mensajeHeader;
/* Estructura para comando correrMensaje*/

typedef struct __attribute__((packed))
{
	int pid;
	char pathProc[256];
	int proxInst;
	int quantum;
	int cantidadLineas;
	int status;
}t_pcb;

typedef struct __attribute__((packed))
{
	int pid;
	int idCPU;
	int instrucciones;//Cantidad de instrucciones
}t_finalizarPID;

typedef struct __attribute__((packed))
{
	int pid;
	int paginas;
}t_iniciarPID;

typedef struct __attribute__((packed))
{
	int pid;
	int pagina;
}t_leer;


typedef struct __attribute__((packed))
{
	int pid;
	int tiempoIO;
	int idCPU;
	int instrucciones; //Cantidad de instrucciones ejecutadas
} t_io;

typedef struct __attribute__((packed))
{
	int instruccion;
	char resultado[256];
} t_instruccion;

typedef struct __attribute__((packed))
{
	int pid;
	int pagina;
	char contenidoPagina[256];
} t_escribir;



// Funciones
void sendACK(int socket);
void recvACK(int socket);
//int serializarEstructura(int id , void * estructura, int size , int socketDestino);
int recibirEstructura(int sock, t_mensajeHeader header, void * estructura);
int serializarEstructura(int id,  void *estructura, int size, int socketDestino);
#endif /* PROTOCOLO_H_ */
