/*
 * protocolo.c
 *
 *  Created on: 25/5/2015
 *      Author: utnso
 */
#include "protocolo.h"
#include <sys/socket.h>

void sendACK(int socket)
{
	t_mensaje_header ack;
	ack.idmensaje = ACK;
	ack.idmodulo = 0;
	send(socket,&ack,sizeof(t_mensaje_header),0);
}

