/*
 * socket.h
 *
 *  Created on: 29/8/2015
 *      Author: utnso
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <unistd.h>

#ifndef SOCKET_H_
#define SOCKET_H_

void conexion_modo_cliente(int socket, struct sockaddr_in* socketInfo, int puerto, in_addr_t dir);
int cliente(char* IP, int PUERTO);
int servidor(int PUERTO);
int servidorMultiplexor(int PUERTO);
char * datosRecibidos();
int clienteSeleccion();

#endif /* SOCKET_H_ */
