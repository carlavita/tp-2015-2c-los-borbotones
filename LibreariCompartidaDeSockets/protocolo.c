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
	send(socket,&ack,sizeof(t_mensaje_header),0);
}


