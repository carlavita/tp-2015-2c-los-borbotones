/*
 * planificador.c
 *
 *  Created on: 30/8/2015
 *      Author: carlavita
 */

#include "planificador.h"


int main(void) {
	pthread_t hilo_consola;
	pthread_t hilo_server;

	/*Log*/
	remove(PATH_LOG);
	logger = log_create(PATH_LOG,"Planificador",true,LOG_LEVEL_INFO);
	log_info(logger,"Iniciando proceso planificador");

/*Archivo de configuración*/

	log_info(logger,"Leyendo Archivo de Configuracion");
	levantarConfiguracion();
	log_info(logger,"Archivo de Configuracion Leido correctamente");

/*Hilo Consola*/
	pthread_create (&hilo_consola, NULL, (void *) &mostrar_consola, NULL);

/*Hilo Server Planificador*/
//todo sincronizar hilos consola y conexion cpu
	pthread_create (&hilo_server, NULL, (void *) &servidor_CPU, NULL);

	pthread_join(hilo_consola,NULL);

	pthread_join(hilo_server,NULL);
	return EXIT_SUCCESS;
}

void levantarConfiguracion(){

	log_info(logger,"Lectura de variables del archivo de configuracion");
	  t_config * CONFIG = config_create(PATH_CONFIG);
	  config_planificador.puertoEscucha = config_get_string_value(CONFIG,"PUERTO_ESCUCHA");
	  config_planificador.algoritmo = config_get_int_value(CONFIG,"ALGORITMO");
	  config_planificador.quantum = config_get_int_value(CONFIG,"QUANTUM");

	  printf(" puerto %s \n",config_planificador.puertoEscucha );
	  printf(" algoritmo %d\n",config_planificador.algoritmo );
	  printf(" quantum %d\n",config_planificador.quantum );

}

void mostrar_consola( void *ptr ){
	int option = -1;
	while(option != 0){
//todo: descomentar todos clear		system("clear");

		puts("Consola - Filesystem \n \n"
			 "1  - Correr Path \n"
			 "2  - Finalizar PID\n"
			 "3  - ps \n"
			 "4  - CPU\n"
			 "0  - Terminar\n");

		puts("Ingrese una opción\n");
		scanf("%d",&option);


		switch(option){
		case 1:
//todo:descomentar			system("clear");
//todo            correr_path();
			espera_enter();
			break;

		case 2:
			//todo:descomentar			system("clear");
//todo			finalizar_pid();
			espera_enter();
			break;
		case 3:
			//todo:descomentar			system("clear");
//todo			ps();
			espera_enter();
			break;
		case 4:
			//todo:descomentar			system("clear");
//todo			cpu();
			espera_enter();
			break;


		case 0:
			log_info(logger, "* Apaga Consola * \n");
			puts("Gracias por venir, vuelva pronto! :)");
			espera_enter();
		break;
		}

	}

}

void espera_enter ()
       	{
       			printf("Presione ENTER para continuar \n");
       			char enter='\0';
       			enter = getchar();
       			while(((enter = getchar()) != '\n'));
       	}


void servidor_CPU( void *ptr ){

	 printf(" estoy en el hilo servidor de CPU\n");
	 //todo crear servidor para un cliente cpu...despues multiplexamos a las distintas cpus
	 log_info(logger,"Dentro del hilo conexion a cpu");

	 	struct addrinfo hints;
	 	struct addrinfo *serverInfo;

	 	memset(&hints, 0, sizeof(hints));
	 	hints.ai_family = AF_UNSPEC;		// No importa si uso IPv4 o IPv6
	 	hints.ai_flags = AI_PASSIVE;		// Asigna el address del localhost: 127.0.0.1
	 	hints.ai_socktype = SOCK_STREAM;	// Indica que usaremos el protocolo TCP

	 	//getaddrinfo(NULL, PUERTO, &hints, &serverInfo); // Notar que le pasamos NULL como IP, ya que le indicamos que use localhost en AI_PASSIVE
	 	//dejo la misma ip de la maquina porque el planificador y la cpu son la misma pc--sino cambiar por ip_planificador
	 	getaddrinfo(NULL,config_planificador.puertoEscucha, &hints, &serverInfo);

	 	int listenningSocket;
	 	listenningSocket = socket(serverInfo->ai_family, serverInfo->ai_socktype, serverInfo->ai_protocol);

	 	bind(listenningSocket,serverInfo->ai_addr, serverInfo->ai_addrlen);
	 	freeaddrinfo(serverInfo); // Ya no lo vamos a necesitar

	 	listen(listenningSocket, BACKLOG);// IMPORTANTE: listen() es una syscall BLOQUEANTE.

	 	struct sockaddr_in addr;			// Esta estructura contendra los datos de la conexion del cliente. IP, puerto, etc.
	 	socklen_t addrlen = sizeof(addr);

	 	int socketCliente = accept(listenningSocket, (struct sockaddr *) &addr, &addrlen);
	 	char package[PACKAGESIZE];
	 	int status = 1;		// Estructura que manejea el status de los recieve.

	 	printf("CPU conectado. Esperando mensajes:\n");
	 	log_info(logger,"CPU conectado");

	 	while (status != 0){
	 			status = recv(socketCliente, (void*) package, PACKAGESIZE, 0);
	 			if (status != 0) printf("%s", package);

	 	}

	 	close(socketCliente);
	 	close(listenningSocket);
	 	printf("Cierro conexion con cpu \n");
	 	log_info(logger,"Cierro conexion con CPU");


}

