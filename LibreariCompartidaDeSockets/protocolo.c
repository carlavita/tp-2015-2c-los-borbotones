/*
 * protocolo.c
 *
 *  Created on: 12/9/2015
 *      Author: utnso
 */

#include "protocolo.h"
void sendACK(int socket)
{
	t_mensajeHeader ack;
	ack.idmensaje = ACK;
	send(socket,&ack,sizeof(t_mensajeHeader),0);
}

void recvACK(int socket)
{
	t_mensajeHeader ack;
	ack.idmensaje = ACK;
	recv(socket,&ack,sizeof(t_mensajeHeader),0);
}

