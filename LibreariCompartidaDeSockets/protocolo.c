/*
 * protocolo.c
 *
 *  Created on: 12/9/2015
 *      Author: utnso
 */

#include "protocolo.h"
void sendACK(int socket) {
	t_mensajeHeader ack;
	ack.idmensaje = ACK;
	send(socket, &ack, sizeof(t_mensajeHeader), 0);
}

void recvACK(int socket) {
	t_mensajeHeader ack;
	ack.idmensaje = ACK;
	recv(socket, &ack, sizeof(t_mensajeHeader), 0);
}

/*int serializarEstructura(int id,  void *estructura, int size, int socketDestino) {
	t_mensajeHeader header;
	header.idmensaje = id;
	header.size = size;
	char *paquete = malloc(8 + size);
    memcpy (paquete, &(header.idmensaje), sizeof(int));
    memcpy (paquete+sizeof(int), &(header.size), sizeof(int));
    memcpy (paquete+sizeof(t_mensajeHeader), estructura, size);

    int status = send (socketDestino, paquete, sizeof(t_mensajeHeader)+size,0);
    free(paquete);
    return status;
}*/
int recibirEstructura(int sock, t_mensajeHeader header, void * estructura){
	/*Recibe header*/
	int status = recv(sock, &header, sizeof(t_mensajeHeader),0);
	/*Con el payload recibe el resto de la estructura*/
	if(status >= 0){
		status = recv(sock, estructura, header.size,0);
	}
	return status;
}
int serializarEstructura(int id,  void *estructura, int size, int socketDestino) {
	t_mensajeHeader header;
	header.idmensaje = id;
	header.size = size;
	char *paquete = malloc(8 + size);
    memcpy (paquete, &header, sizeof(t_mensajeHeader));

    // si tiene una estructura ademas del header, la apendea
    if(size> 0) {
    memcpy (paquete+sizeof(t_mensajeHeader), estructura, size);
    }
    int status = send (socketDestino, paquete, sizeof(t_mensajeHeader)+size,0);
    free(paquete);
    return status;
}

