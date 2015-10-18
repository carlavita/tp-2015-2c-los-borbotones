/*
 *
 *  Created on: 30/8/2015
 *      Author: carlavita
 */

#include "planificador.h"
// includes de iblioteca compartida
#include <protocolo.h>
#include <socket.h>

int main(void) {
	pthread_t hiloConsola;
	pthread_t hiloServer;
	pthread_t hiloIO;

	/*Log*/
	remove(PATHLOG);
	logger = log_create(PATHLOG, "Planificador", true, LOG_LEVEL_INFO);
	pthread_mutex_lock(&mutexLog);
	log_info(logger, "Iniciando proceso planificador");
	log_info(logger, "Leyendo Archivo de Configuracion");
	pthread_mutex_unlock(&mutexLog);
	/*Archivo de configuración*/

	levantarConfiguracion();
	pthread_mutex_lock(&mutexLog);
	log_info(logger, "Archivo de Configuracion Leido correctamente");
	pthread_mutex_unlock(&mutexLog);

	/*Inicialicacion*/

	inicializarListas();
	inicializarSemaforos();
	/*Hilo Server Planificador*/

	pthread_create(&hiloServer, NULL, (void *) &servidorCPU, NULL);

	/*Hilo consola Planificador*/
	pthread_create(&hiloConsola, NULL, (void *) &mostrarConsola, NULL);

	pthread_create(&hiloIO, NULL, (void *) &procesarEntradasSalidas,
	NULL);

	planificar(configPlanificador.quantum);

	pthread_join(hiloConsola, NULL);
	pthread_join(hiloServer, NULL);

	return EXIT_SUCCESS;
}

void inicializarListas() {

	LISTOS = list_create();
	EJECUTANDO = list_create();
	BLOQUEADOS = list_create();
	FINALIZADOS = list_create();
	listaCPU = list_create();
	IO = list_create();
	pthread_mutex_lock(&mutexLog);
	log_info(logger, "Se inicializaron las Listas de planificacion \n");
	pthread_mutex_unlock(&mutexLog);

}

void inicializarSemaforos() {
	sem_init(&semaforoListos, 0, 0); //Semaforo productor consumidor de prcesos listos
	sem_init(&semaforoCPU, 0, 0);
	sem_init(&semaforoIO, 0, 0);
	pthread_mutex_init(&mutexListaCpu, NULL);
	pthread_mutex_init(&mutexListas, NULL);
	pthread_mutex_init(&mutexLog, NULL);
	pthread_mutex_lock(&mutexLog);
	log_info(logger, "Se inicializaron los semaforos \n");
	pthread_mutex_unlock(&mutexLog);

}

void destruirSemaforosYmutex() {
	sem_destroy(&semaforoListos);
	sem_destroy(&semaforoCPU);
	pthread_mutex_destroy(&mutexListaCpu);
	pthread_mutex_destroy(&mutexListas);
	pthread_mutex_destroy(&mutexLog);
}

void levantarConfiguracion() {
	pthread_mutex_lock(&mutexLog);
	log_info(logger, "Lectura de variables del archivo de configuracion");
	pthread_mutex_unlock(&mutexLog);

	t_config * CONFIG = config_create(PATH_CONFIG);

	if (CONFIG != NULL) {
		configPlanificador.puertoEscucha = malloc(PUERTO_SIZE);
		configPlanificador.puertoEscucha = config_get_string_value(CONFIG,
				"PUERTO_ESCUCHA");

		configPlanificador.algoritmo = config_get_int_value(CONFIG,
				"ALGORITMO");
		configPlanificador.quantum = config_get_int_value(CONFIG, "QUANTUM");
		configPlanificador.pathmCod = malloc(PATH_SIZE);
		configPlanificador.pathmCod = config_get_string_value(CONFIG,
				"PATHMCOD");

		printf(" puerto %s \n", configPlanificador.puertoEscucha);
		printf(" algoritmo %d\n", configPlanificador.algoritmo);
		printf(" quantum %d\n", configPlanificador.quantum);
		printf(" path de los mcod %s \n", configPlanificador.pathmCod);

	} else {
		printf("estupido config, se cree tan importante");
	}

}

void mostrarConsola(void *ptr) {

	int option = -1;
	while (option != 0) {
//todo: descomentar todos clear		system("clear");

		puts("Consola - Planificador \n \n"
				"1  - Correr Path \n"
				"2  - Finalizar PID\n"
				"3  - ps \n"
				"4  - CPU\n"
				"0  - Terminar\n");

		puts("Ingrese una opción\n");
		scanf("%d", &option);

		switch (option) {
		case 1:
//todo:descomentar			system("clear");

			correrPath();
			/*printf(
			 "soy el planificador, recibí el mensaje correr path por consola! Envi a CPU\n");
			 int mensaje = CORRERPATH;
			 printf("socket : %d \n", ServidorP);
			 int status = send(ServidorP, &mensaje, sizeof(int), 0);
			 printf("estado de envio : %d \n", status);*/
			esperaEnter();
			break;

		case 2:
			//todo:descomentar			system("clear");
			finalizarPid();
			esperaEnter();
			break;
		case 3:
			//todo:descomentar			system("clear");

			ejecutarPS();
			esperaEnter();
			break;
		case 4:
			//todo:descomentar			system("clear");
			ejecutarCPU();
			esperaEnter();
			break;

		case 0:
			pthread_mutex_lock(&mutexLog);
			log_info(logger, "* Apaga Consola * \n");
			pthread_mutex_unlock(&mutexLog);

			puts("Gracias por venir, vuelva pronto! :)");
			// todo: destruir semaforos?
			esperaEnter();
			break;

		default:
			printf("Comando inválido\n");
			esperaEnter();
			break;
		}

	}

}

void esperaEnter() {
	printf("Presione ENTER para continuar \n");
	char enter = '\0';
	enter = getchar();
	while (((enter = getchar()) != '\n'))
		;
}

void servidorCPU(void *ptr) {

	printf(" estoy en el hilo servidor de CPU\n");

	pthread_mutex_lock(&mutexLog);
	log_info(logger, "Dentro del hilo conexion a cpu");
	pthread_mutex_unlock(&mutexLog);

	int sock; //socket asociado a CPU
	fd_set socks;
	fd_set readsocks;
	int maxsock;
	int reuseaddr = 1; /* True */
	struct addrinfo hints, *res;

	/*Inicializa fd set*/
	//		fd_set temp;
	/* Genera socket y multiplexa conexiones con select*/

	/* Get the address info */
	memset(&hints, 0, sizeof hints);
	hints.ai_family = AF_INET;
	hints.ai_socktype = SOCK_STREAM;

	if (getaddrinfo(NULL, configPlanificador.puertoEscucha, &hints, &res)
			!= 0) {

		perror("getaddrinfo");
		//exit(1);
		//	return 1;al

	}

	/* Create the socket */
	sock = socket(res->ai_family, res->ai_socktype, res->ai_protocol);
	if (sock == -1) {
		perror("socket");
		exit(1);
	}
	printf("socket de escucha %d \n", sock);

	/* Permitir al socket a reusar  address */
	if (setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, &reuseaddr, sizeof(int))
			== -1) {
		perror("setsockopt");
		exit(1);
	}

	/* Bind to the address */
	if (bind(sock, res->ai_addr, res->ai_addrlen) == -1) {
		perror("bind");
		exit(1);
	}

	freeaddrinfo(res);

	/* Listen */
	if (listen(sock, BACKLOG) == -1) {
		perror("listen");
		exit(1);
	}

	/* Set up the fd_set */
	FD_ZERO(&socks);
	FD_SET(sock, &socks);
	maxsock = sock;

	/* Main loop */
	while (1) {

		unsigned int s;
		readsocks = socks;

		printf("Escucha pedidos de CPU \n");
		pthread_mutex_lock(&mutexLog);
		log_info(logger, "Escuchando conexiones \n ");
		pthread_mutex_unlock(&mutexLog);

		if (select(maxsock + 1, &readsocks, NULL, NULL, NULL) == -1) {
			perror("select");
			exit(1);
		}
		for (s = 0; s <= maxsock; s++) {
			if (FD_ISSET(s, &readsocks)) {
				printf("el socket %d esta listo \n", s);
				if (s == sock) {
					/* New connection */
					printf("nueva conexion en socket %d \n", s);
					int newsock;
					struct sockaddr_in their_addr;
					size_t size = sizeof(struct sockaddr_in);
					newsock = accept(sock, (struct sockaddr*) &their_addr,
							&size);
					if (newsock == -1) {
						perror("accept");
					} else {
						printf(
								"Tenemos una conexion desde %s en el puerto %d\n",
								inet_ntoa(their_addr.sin_addr),
								htons(their_addr.sin_port));
						FD_SET(newsock, &socks);
						if (newsock > maxsock) {
							maxsock = newsock;
						}
						//			t_saludoCPU saludoCPU;
						int status = 1;	// Estructura que maneja el status de los recieve
						//una vez detectada la conexion de una cpu se la agrega a la lista de cpus y se la coloca como libre -1
						status = serializarEstructura(SALUDO,
								(void *) PATH_MCODE, sizeof(PATH_MCODE) + 1,
								newsock);
						int cpu;
						status = recv(newsock, &cpu, sizeof(int), 0);
						//	t_mensajeHeader header;
						//					status = recv(newsock, &header, sizeof(t_mensajeHeader), 0);
						/*					status = recv(newsock, &saludoCPU, sizeof(t_saludoCPU), 0);

						 agregarCPU(newsock, -1, saludoCPU.cpuID);
						 ServidorP = newsock;
						 /*status = serializarEstructura(SALUDO,
						 (void *) PATH_MCODE, sizeof(PATH_MCODE) + 1,
						 newsock);*/
						//int mensaje = SALUDO;
//						status = send(newsock, &mensaje, sizeof(int), 0);
						//agregarCPU(newsock, -1, 1);
						printf("agrega CPU: %d\n", cpu);
						agregarCPU(newsock, -1, cpu);
						printf("status send inicial %d \n", status);
						//		printf("Se conectó la CPU con ID %d \n", saludoCPU.cpuID);
						//status = send(newsock, &PATH_MCODE, sizeof(PATH_MCODE), 0);
						printf("SE ENVÍA PATH , %s STATUS %d \n", PATH_MCODE,
								status);

					}
				} else {

					//gestiona la conexion de una CPU segun los pedidos
					handle(s, &socks);

				}
			}
		}

	}

	close(sock);

}

void handle(int newsock, fd_set *set) {
	t_finalizarPID rtaP;
	//t_saludoCPU saludoCPU;

	printf("En el handle de planificador \n");
	//recepcion de msj del cpu segun protocolo
	int status = 1;

	t_mensajeHeader mjeHeader;
	status = recv(newsock, &mjeHeader, sizeof(t_mensajeHeader), 0);
	int codigoMsj = mjeHeader.idmensaje;

	if (status) {
		switch (codigoMsj) {

		case FINALIZAPROCOK:

			log_info(logger,
					"El proceso finalizo correctamente su ejecucion \n");

			t_finalizarPID rtaProc;
			recv(newsock, &(rtaProc), sizeof(t_finalizarPID), 0);
			printf(" con id: %d \n", rtaProc.pid);
			printf(" de la cpu: %d \n", rtaProc.idCPU);

			//actualizarPcb();

			//actualizar el estado de la cola de finalizados

			pthread_mutex_lock(&mutexListas); //todo revisar porque bloquea al proceso
			int lalala = list_size(EJECUTANDO);
			log_info(logger, "Al finalizar hay %d procesos ejecutando\n ",
					lalala);

			t_pcb *pcb = buscarEnListaPorPID(EJECUTANDO, rtaProc.pid);

			pcb->status = FINALIZADOOK;
			list_add(FINALIZADOS, pcb);
			removerEnListaPorPid(EJECUTANDO, rtaProc.pid);
			pthread_mutex_unlock(&mutexListas);

			liberarCPU(rtaProc.idCPU);

			break;
		case PROCFALLA:

			recv(newsock, &(rtaP), sizeof(t_finalizarPID), 0);
			pthread_mutex_lock(&mutexListas); //todo revisar porque bloquea al proceso

			t_pcb *pcb1 = buscarEnListaPorPID(EJECUTANDO, rtaP.pid);

			pcb1->status = FINALIZADOERROR;
			list_add(FINALIZADOS, pcb1);
			removerEnListaPorPid(EJECUTANDO, rtaP.pid);
			pthread_mutex_unlock(&mutexListas);

			//funcion que pone a la cpu libre nuevamente
			liberarCPU(rtaP.idCPU);

			log_info(logger,
					"Proceso rechazado por falta de espacio en SWAP, PID : %d, CPU: %d \n",
					rtaP.pid, rtaP.idCPU);
			//borrarEstructurasDelProc();

			break;
		case PROCIO:

			ejecutarIO(newsock);
//todo recuperar instrucciones
			break;
		case FINDEQUANTUM:

			pthread_mutex_lock(&mutexLog);
			log_info(logger, "Fin de quantum CPU "); //todo, agregar id cpu
			pthread_mutex_unlock(&mutexLog);
			t_finalizarPID finQuantum;
			recv(newsock, &(finQuantum), sizeof(t_finalizarPID), 0);
			printf(" con id: %d \n", finQuantum.pid);
			printf(" de la cpu: %d \n", finQuantum.idCPU);

			//actualizar el estado de la cola de listos

			pthread_mutex_lock(&mutexListas); //todo revisar porque bloquea al proceso
			//int lalala = list_size(EJECUTANDO);

			t_pcb *pcbProc = buscarEnListaPorPID(EJECUTANDO, finQuantum.pid);

			pcbProc->status = LISTO;
			if (pcbProc->cantidadLineas == pcbProc->proxInst) {
				log_info(logger,
						"no se actualiza prox instruccion porque  se ejecuto antes el finalizar por consola \n");
			} else {

				pcbProc->proxInst = pcbProc->proxInst + finQuantum.instrucciones ;
				log_info(logger, " actualiza prox instrucción: %d\n", pcb->proxInst);
			}
			list_add(LISTOS, pcbProc);
			removerEnListaPorPid(EJECUTANDO, finQuantum.pid);
			pthread_mutex_unlock(&mutexListas);
			sem_post(&semaforoListos);
			liberarCPU(finQuantum.idCPU);

			break;
		case FINDERAFAGA:

			printf("el proceso finalizo correctamente su rafaga \n");
			pthread_mutex_lock(&mutexLog);
			log_info(logger, "Fin de rafaga del proceso: "); //todo, agregar id proceso
			pthread_mutex_unlock(&mutexLog);
			// todo liberar cpu y sem_post(&semaforoCPU);
			break;
		case SALUDO: {

			//int status = recv(newsock, &saludoCPU, sizeof(t_saludoCPU), 0);

			//agregarCPU(newsock, -1, saludoCPU.cpuID);
//			log_info(logger, "se conectó la CPU con ID %d \n", saludoCPU.cpuID);
		}
			break;
		default:
			printf("codigo no reconocido\n");
			break;

		}
	}

	if (!status) {
		// conexión cerrada
		printf("la conexion del socket: %d termino \n", newsock);
		pthread_mutex_lock(&mutexLog);
		log_info(logger, "Cierro conexion con CPU");
		pthread_mutex_unlock(&mutexLog);
		close(newsock); // ¡Hasta luego!
		FD_CLR(newsock, set); // eliminar del conjunto maestro
	}

}

void correrPath() {
	char path[PATH_SIZE];
	printf("Ingrese el PATH del mCod a correr \n");
	scanf("%s", path);
	printf("PATH del mCod a correr %s \n", path);

	int pid = crearPcb(path);

	printf("Creado mProc %d \n", pid);
	fflush(stdout);
}

int crearPcb(char* path) {

	t_pcb* pcb = malloc(sizeof(t_pcb));

	PID++;
	pcb->pid = PID;
	pcb->proxInst = 0; //Inicializa en 0 es la primer instruccion
	pcb->status = LISTO;
	strcpy(pcb->pathProc, path);

	pcb->cantidadLineas = obtenerCantidadLineasPath(path);
	if (configPlanificador.algoritmo == FIFO) {
		pcb->quantum = -1;
	} else if (configPlanificador.algoritmo == RR) {
		pcb->quantum = configPlanificador.quantum;
	}
	printf("PID mProc: %d \n", pcb->pid);
	printf("Proxima instruccion mProc: %d \n", pcb->proxInst);
	printf("Path mProc: %s \n", pcb->pathProc);
	printf("Path mProc: %d \n", pcb->cantidadLineas);
	printf("Quantum mProc: %d \n", pcb->quantum);
	/*Lo agrega a la lista de listos*/
	pthread_mutex_lock(&mutexListas);
	list_add(LISTOS, pcb);
	//t_pcb *mProc = list_get(LISTOS, 0);
	pthread_mutex_unlock(&mutexListas);

	pthread_mutex_lock(&mutexLog);
	log_info(logger, "PID creado %d , path %s \n", pcb->pid, pcb->pathProc);
	pthread_mutex_unlock(&mutexLog);

	sem_post(&semaforoListos); //Habilita al planificador
	sem_getvalue(&semaforoListos, &val); // valor del contador del semáforo
	printf(
			"Soy el semaforo despues de habilitar a planificador con el valor: %d\n",
			val);

	return pcb->pid;
}

t_pcb* planificarFifo() {

	sem_wait(&semaforoListos); // Consumir cuando haya procesos listos
	sem_wait(&semaforoCPU); //consumir cuando haya cpus libres
	pthread_mutex_lock(&mutexListas);
	t_pcb* pcb = list_remove(LISTOS, 0);
	pthread_mutex_unlock(&mutexListas);
	printf("eliminó el pid : % d de listos \n", pcb->pid);
	pcb->status = EJECUTA;
	pthread_mutex_lock(&mutexListas);
	list_add(EJECUTANDO, pcb);
	pthread_mutex_unlock(&mutexListas);
	printf("agrego el pid : % d a ejecutando \n", pcb->pid);

	sem_getvalue(&semaforoListos, &val); // valor del contador del semáforo
	printf(
			"Soy el semaforo despues de planificar un proceso  con el valor: %d\n",
			val);

	printf("termino la planificacion con fifo de un proceso \n");

	return pcb;

}

void enviarACpu(t_pcb* pcb, t_cpu* cpu) {
	int status = serializarEstructura(EJECUTARPROC, (void *) pcb, sizeof(t_pcb),
			cpu->socket);
	//t_mensajeHeader msjEjecutar;
	//msjEjecutar.idmensaje = EJECUTARPROC;
	//send(cpu->socket, &msjEjecutar, sizeof(int), 0);
	log_info(logger, "Envio de pedido de ejecucion PID %d a la cpu libre %d \n",
			pcb->pid, cpu->id);
//	send(cpu->socket, pcb, sizeof(t_pcb), 0);
	if (status >= 0) {
		log_info(logger,
				"Envio exitoso de contexto PID %d a la cpu libre %d \n",
				pcb->pid, cpu->id);
	}
}

void removerEnListaPorPid(t_list *lista, int pid) {
	int _is_pcb(t_pcb *p) {
		return p->pid == pid;
	}
	//pthread_mutex_lock(&mutexListas);
	list_remove_by_condition(lista, (void*) _is_pcb);
	//pthread_mutex_unlock(&mutexListas);

	//cuidado q esa funcion retorna el valor q remueve (void*)
}

t_pcb* buscarEnListaPorPID(t_list* lista, int pid) {
	// Busca por pid
	int _is_pcb(t_pcb *p) {
		return p->pid == pid;
	}
	t_pcb* pcb = list_find(lista, (void*) _is_pcb);

	return pcb;

}

int planificar(int quantum) {

	//este hilo se usa como planificador(consumidor) que elije de la cola de listos y lo pasa la lista de ejecutando con el quantum

	pthread_t hiloPlanificador;

	pthread_create(&hiloPlanificador, NULL, (void *) &planificador, NULL);

	pthread_join(hiloPlanificador, NULL);

	return 0;
}

void *planificador(void *info_proc) {
	t_pcb* pcb = malloc(sizeof(t_pcb));
	int valCPU;
	printf("en el hilo planificador \n");

	while (true) {

// el planificar lo saca de la cola de listos y lo pasa a ejecutando
		pcb = planificarFifo();

		sem_getvalue(&semaforoCPU, &valCPU); // valor del contador del semáforo
		printf("antes del wait, semaforo en : %d \n", valCPU);

		//enviar a cpu elegida el pcb del proceso elegido
		t_cpu* cpuLibre;
		cpuLibre = buscarCpuLibre();
		if (cpuLibre != NULL) {
			printf("el id de la cpu libre es: %d \n", cpuLibre->id);

			cpuLibre->pid = pcb->pid;

		}

		enviarACpu(pcb, cpuLibre);

	}
	free(pcb);

	return 0;

}

void ejecutarIO(int socketCPU) {
	//recepciono de la cpu el msj con el Tiempo(en segundos) que hay que hacer el IO
	//t_rtaIO mjeIO;
	t_io* infoIO = malloc(sizeof(t_io));
	recv(socketCPU, infoIO, sizeof(t_io), 0);

	pthread_mutex_lock(&mutexLog);

	log_info(logger,
			"el proceso esta realizando su entrada-salida, PID: %d, Tiempo: %d segundos , CPU %d \n ",
			infoIO->pid, infoIO->tiempoIO, infoIO->idCPU);
	log_info(logger, "La ultima ráfaga fue de %d instrucciones \n",
			infoIO->instrucciones);
	pthread_mutex_unlock(&mutexLog);

	pthread_mutex_lock(&mutexListas);
	//busco el proc por si pid, se borra de ejecutados y lo mando a bloqueados
	//todo falta actualizar el puntero a proxima instruccion cuando lo recibe de cpu
	t_pcb *pcb = buscarEnListaPorPID(EJECUTANDO, infoIO->pid);
	if (pcb->cantidadLineas == pcb->proxInst) {
		log_info(logger,
				"no se actualiza prox instruccion porque  se ejecuto antes el finalizar por consola \n");
	} else {

		pcb->proxInst = pcb->proxInst + infoIO->instrucciones;
		log_info(logger, " actualiza prox instrucción: %d\n", pcb->proxInst);
	}
	pcb->status = BLOQUEADO;

	list_add(BLOQUEADOS, pcb);

	removerEnListaPorPid(EJECUTANDO, infoIO->pid);

	list_add(IO, infoIO);
	pthread_mutex_unlock(&mutexListas);
	liberarCPU(infoIO->idCPU);
	sem_post(&semaforoIO);	// Habilita al hilo de entrada salida.

	//liberarCPU(infoIO->idCPU);
	free(infoIO);
}

void eliminarCPU(int socket_cpu) {
	t_cpu* cpuNodoLista = malloc(sizeof(t_cpu));
	int cont;
	pthread_mutex_lock(&mutexListaCpu);
	for (cont = 0; cont < list_size(listaCPU); cont++) {
		cpuNodoLista = list_get(listaCPU, cont);
		if (cpuNodoLista->socket == socket_cpu) {
			puts("CPU eliminada \n");
			//Remuevo el CPU de la lista

			list_remove(listaCPU, cont);

			free(cpuNodoLista);
		}
	}
	pthread_mutex_unlock(&mutexListaCpu);
}

// CPUs nuevas, agregar con PID -1
void agregarCPU(int cpuSocket, int pid, int idCPU) {
	t_cpu* cpu = malloc(sizeof(t_cpu));
	int valCPU; //valor semaforo
	cpu->socket = cpuSocket;
	cpu->pid = pid;
	//CPUID++;
	//cpu->id = CPUID;
	cpu->id = idCPU;
	cpu->porcentajeUso = 0; // inicial

	pthread_mutex_lock(&mutexListaCpu);
	list_add(listaCPU, cpu);
	pthread_mutex_unlock(&mutexListaCpu);
	sem_post(&semaforoCPU); //Habilita una CPU libre
	sem_getvalue(&semaforoCPU, &valCPU); // valor del contador del semáforo
	printf("Agrego CPU, semaforo en : %d \n", valCPU);

}

t_cpu* buscarCpuLibre() {
	// Busca por pid, las que tienen -1 son las libres
	int _is_pcb(t_cpu *p) {
		return p->pid == -1;
	}
	pthread_mutex_lock(&mutexListaCpu);
	t_cpu* cpu = list_find(listaCPU, (void*) _is_pcb);
	pthread_mutex_unlock(&mutexListaCpu);

	return cpu;
}
void ejecutarPS() {

	pthread_mutex_lock(&mutexListas);
// Crea una lista con todos los procesos.
	t_list* PROCESOS = list_create();
	int cantidadListos = list_size(LISTOS);
	int cantidadEjecutando = list_size(EJECUTANDO);
	int cantidadBloqueados = list_size(BLOQUEADOS);
	int cantidadFinalizados = list_size(FINALIZADOS);

	printf("Cantidad de listos : %d \n;  ", cantidadListos);
	printf("Cantidad de ejecutando: %d \n ", cantidadEjecutando);
	printf("Cantidad de bloqueados: %d \n ", cantidadBloqueados);
	printf("Cantidad de procesos finalizados/finalizando: %d \n ",
			cantidadFinalizados);

	list_add_all(PROCESOS, LISTOS);
	list_add_all(PROCESOS, EJECUTANDO);
	list_add_all(PROCESOS, BLOQUEADOS);
	list_add_all(PROCESOS, FINALIZADOS);
	pthread_mutex_unlock(&mutexListas);

	ordernarPorPID(PROCESOS);
	printf("---  Status de Procesos mProc    --- \n \n");
	int indexLista = 0;

	t_pcb *mProc = list_get(PROCESOS, indexLista);
	//	t_pcb *mProc = list_get(LISTOS, indexLista);

	while (mProc != NULL) {
		switch (mProc->status) {
		case LISTO:
			printf("mProc %d: %s  ->  %s \n", mProc->pid, mProc->pathProc,
					"Listo \n");
			//printf("%s",mProc->pathProc);

			break;
		case EJECUTA:
			printf("mProc %d: %s  ->  %s \n", mProc->pid, mProc->pathProc,
					"Ejecutando \n");
			//printf("%s",mProc->pathProc);
			break;
		case BLOQUEADO:
			printf("mProc %d: %s  ->  %s \n", mProc->pid, mProc->pathProc,
					"Bloqueado \n");
			//printf("%s",mProc->pathProc);
			break;
		case FINALIZADOOK:
			printf("mProc %d: %s  ->  %s \n", mProc->pid, mProc->pathProc,
					"Finalizado Sin errores \n");
			//printf("%s",mProc->pathProc);
			break;
		case FINALIZADOERROR:
			printf("mProc %d: %s  ->  %s \n", mProc->pid, mProc->pathProc,
					"Finalizado Con errores \n");
			//printf("%s",mProc->pathProc);
			break;

		default:
			break;
		}
//			printf("mProc %d: %s  ->  %d \n", mProc->pid, mProc->pathProc, mProc->status);

		indexLista++;
		mProc = list_get(PROCESOS, indexLista);
	}

}

void ordernarPorPID(t_list* lista) {
	bool _pidMayor(t_pcb *proceso, t_pcb *procesoMenorPID) {
		return proceso->pid < procesoMenorPID->pid;
	}

	list_sort(lista, (void*) _pidMayor);
}
void ordernarPorIDCPU(t_list* lista) {
	bool _idMayor(t_cpu *cpu, t_cpu *cpuMenorID) {
		return cpu->id < cpuMenorID->id;
	}

	list_sort(lista, (void*) _idMayor);
}
void ejecutarCPU() {
	pthread_mutex_lock(&mutexListaCpu);
	ordernarPorIDCPU(listaCPU);
	printf("---  Cache 13 . Reporte de CPUs    --- \n \n");

	int indexLista = 0;

	t_cpu *cpu = list_get(listaCPU, indexLista);

	while (cpu != NULL) {

		printf("CPU %d:  %d  %\n", cpu->id, cpu->porcentajeUso);
		//todo pasar el int y que muestre el nombre del estado. loguear

		indexLista++;
		cpu = list_get(listaCPU, indexLista);
	}

	pthread_mutex_unlock(&mutexListaCpu);

}

void finalizarPid() {

	int idProc;
	printf("Ingrese el ID del mCod a finalizar \n");
	scanf("%d", &idProc);
	printf("el proceso con id: %d del mCod finalizará \n", idProc);

	//el puntero de instrucciones tiene que posicionarse en la ultima del programa mCod en ejecucion
	pthread_mutex_lock(&mutexListas);

	t_pcb* pcbProc;
	pcbProc = buscarEnListaPorPID(EJECUTANDO, idProc);
	int ultimaInst;
	ultimaInst = pcbProc->cantidadLineas;
	pcbProc->proxInst = ultimaInst;

	pthread_mutex_unlock(&mutexListas);

	//todo mandar a cpu??
}

int obtenerCantidadLineasPath(char* path) {
	char *path_absoluto = string_new();

	string_append(&path_absoluto, PATH_MCODE);
	string_append(&path_absoluto, path);

	//FILE* mCod = fopen(path, "r");
	FILE* mCod = fopen(path_absoluto, "r");
	int caracter, lineasPath = 0;

	do {
		caracter = fgetc(mCod);
		if (caracter == '\n')
			lineasPath++;
	} while (caracter != EOF);

// La ultima linea no termina con \n
// pero no va a haber otra despues de la ultima
	if (caracter != '\n' && lineasPath != 0)
		lineasPath++;

	fclose(mCod);

	return lineasPath;
}

void *procesarEntradasSalidas(void *info_proc) {
	t_io* mjeIO = malloc(sizeof(t_io));
	// hilo consumidor de entrada salida
	t_pcb* pcb = malloc(sizeof(t_pcb));
	while (true) {
		sem_wait(&semaforoIO);
		pthread_mutex_lock(&mutexListas);

		mjeIO = list_remove(IO, 0);

		sleep(mjeIO->tiempoIO); //HAY QUE ENCOLAR LOS BLOQUEOS
		pcb = list_remove(BLOQUEADOS, 0);
		pcb->status = LISTO;
		list_add(LISTOS, pcb);
		pthread_mutex_unlock(&mutexListas);
		//Habilita el semaforo del consumidor de listos
		sem_post(&semaforoListos);
	}
	free(mjeIO);
	free(pcb);

}

void liberarCPU(int idCPU) {

	int _is_cpu(t_cpu *p) {
		return p->id == idCPU;
	}
	pthread_mutex_lock(&mutexListaCpu);
	t_cpu* cpu = list_find(listaCPU, (void*) _is_cpu);
	cpu->pid = -1; //actualizo su pid para que quede libre la cpu
	pthread_mutex_unlock(&mutexListaCpu);
	sem_post(&semaforoCPU); //habilito al semaforo de cpu libres

}

/*
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
 */

