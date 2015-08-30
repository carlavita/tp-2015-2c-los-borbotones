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

char * buffer ;
char * datosRecibidosPorElServidor;

void conexion_modo_cliente(int socket, struct sockaddr_in* socketInfo, int puerto, in_addr_t dir) {
char  * datosRecibidos = malloc(sizeof(char*));

			(*socketInfo).sin_family = AF_INET; //configuracion del socket destino que es el que se comunicara con mi socketParaKernel
			(*socketInfo).sin_addr.s_addr = dir; //vinculo la ip con el socket destino
			(*socketInfo).sin_port = htons(puerto); //vinculo el puerto con el socket destino
				int clienteConectado = connect(socket, (struct sockaddr *) socketInfo, sizeof(*socketInfo));


											send(clienteConectado,"Esperando datos para el Fs desde este servidor",sizeof("Esperando datos para el Fs desde este servidor"),0);

						printf("%s",datosRecibidos);

}

int cliente(char* IP, int PUERTO){
	//struct addrinfo hints;
	struct addrinfo *serverInfo;
	serverInfo = malloc(sizeof(struct addrinfo *));

	(*serverInfo).ai_family = AF_UNSPEC;
	//(*serverInfo).ai_socktype = SOCK_STREAM;
	(*serverInfo).ai_protocol = 0;
			int serverSocket = (int)malloc(sizeof(serverInfo));

	 serverSocket = socket(serverInfo->ai_family, serverInfo->ai_socktype, serverInfo->ai_protocol);

	connect(serverSocket, serverInfo->ai_addr, serverInfo->ai_addrlen);
	freeaddrinfo(serverInfo);
	printf("Conectado al servidor. Bienvenido al sistema, ya puede enviar mensajes.\n");
	return serverSocket;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

// EJEMPLO PARA CLIENTE
/*
int main (){
	int serverSocket = cliente("127.0.0.1","6667");
	int enviar = 1;
	char message[1024];

	while(enviar){
		fgets(message, 1024, stdin);
		if (!strcmp(message,"exit\n")) enviar = 0;
		if (enviar) send(serverSocket, message, strlen(message) + 1, 0);
	}
	close(serverSocket);
	return 0;
}
*/

//////////////////////////////////////////////////////////////////////////////////////////////////

// FUNCION SERVIDOR, te devuelve el socket por donde esta escuchando. Si devuelve -1, se produjo un error

int servidor(int PUERTO)
{
	int socketservidor;
	int yes = 1;
	struct sockaddr_in serveraddr;

	if((socketservidor = socket(AF_INET, SOCK_STREAM, 0)) == -1)
	{
		perror("Error en la creacion del Socket");
		return -1;
	}
	printf("Socket Servidor Creado\n");

	if(setsockopt(socketservidor, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int)) == -1)
	{
		perror("Error en la definicion del protocolo de comunicacion");
		return -1;
	}
	printf("Protocolo de Comunicacion definido correctamente\n");

	serveraddr.sin_family = AF_INET;
	serveraddr.sin_addr.s_addr = INADDR_ANY;
	serveraddr.sin_port = htons(PUERTO);
	memset(&(serveraddr.sin_zero), '\0', 8);

	if(bind(socketservidor, (struct sockaddr *)&serveraddr, sizeof(serveraddr)) == -1)
	{
		perror("Error en el bind");
		return -1;
	}

	if(listen(socketservidor, 10) == -1)
	{
		perror("Error en el Listen");
		return -1;
	}
	printf("Socket Escuchando\n");

	return socketservidor;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

int servidorMultiplexor(int PUERTO){
	fd_set master;
	fd_set read_fds;
	struct sockaddr_in clientaddr;
	int fdmax;
	int newfd;
	char paquete[1024];
	int nbytes;
	int i;
	FD_ZERO(&master);
	FD_ZERO(&read_fds);

	int socketservidor = servidor(PUERTO);

	FD_SET(socketservidor, &master);
	fdmax = socketservidor;
	for(;;)
		{
			read_fds = master;
			if(select(fdmax+1, &read_fds, NULL, NULL, NULL) == -1)
			{
				perror("Error en el Select");
				exit(1);
			}
			printf("Select Activado\n");

			for(i = 0; i <= fdmax; i++)
			{
				if(FD_ISSET(i, &read_fds))
				{
					if(i == socketservidor)
					{
						socklen_t addrlen = sizeof(clientaddr);
						if((newfd = accept(socketservidor, (struct sockaddr *)&clientaddr, &addrlen)) == -1)
						{
							perror("Error en el Accept");
						}
						else
						{
							printf("Socket Aceptado\n");

							FD_SET(newfd, &master);
							if(newfd > fdmax)
							{
								fdmax = newfd;
							}
							printf("Nueva conexion de %s en el socket %d\n", inet_ntoa(clientaddr.sin_addr), newfd);

							if (1){
								break;
							}
						}
					}
					else
					{
						if((nbytes = recv(i, paquete, sizeof(paquete), 0)) <= 0)
						{
							if(nbytes == 0)
								printf("Socket %d conexion caida\n", i);
							else
								perror("Error recv");
							close(i);
							FD_CLR(i, &master);
						}
						else
						{
							printf("El socket %i escribio: %s con tamanio: %i \n", i, paquete, nbytes); // A partir de aca se recibe el dato y se comienza a utilizar
							strncpy(buffer,paquete,sizeof(paquete));
						}
					}
				}
			}
		}
	exit(0);//el descriptor del cliente seleccionado
}

char * datosRecibidos()
{
	return buffer;
}

int main()
{
	int servidor = servidorMultiplexor(5000);
	return servidor;
}
