/*
 * protocolo.h
 *
 *  Created on: 25/5/2015
 *      Author: utnso
 */

#ifndef PROTOCOLO_H_
#define PROTOCOLO_H_

// Definir mensajes posibles
#define ACK 1
#define PERSONA 2

typedef struct __attribute__((packed))
{
	int idmodulo;
	int idmensaje;

}t_mensaje_header;


typedef struct __attribute__((packed))
{
	char nombre[50];
	char apellido[50];

}t_mensaje_estructura;

void sendACK(int socket);


#endif /* PROTOCOLO_H_ */
