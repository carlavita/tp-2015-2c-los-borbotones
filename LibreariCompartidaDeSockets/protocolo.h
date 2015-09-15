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



#include <sys/types.h>
#include <sys/socket.h>
//estructura de header
typedef struct __attribute__((packed))
{
	//int idmodulo;
	int idmensaje;

}t_mensajeHeader;
/* Estructura para comando correrMensaje*/
  typedef struct __attribute__((packed))
{
	int pid;
	char pathProc[256];
	int proxInst;
	int quantum;
}t_pcb;

typedef struct __attribute__((packed))
{
	int pid;
}t_finalizarPID;


// Funciones
void sendACK(int socket);

#endif /* PROTOCOLO_H_ */
