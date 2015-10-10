/*
 * CPU.c
 *
 *  Created on: 1/9/2015
 *      Author: Fernando Sanchez
 */

#include "CPU.h"

int main() {
	pthread_t hiloCPU;

	remove(PATH_LOG);
	logCPU = log_create(PATH_LOG, "CPU", true, LOG_LEVEL_INFO);
	log_info(logCPU, "Inicio Proceso CPU");

	LeerArchivoConfiguracion();
	inicializarSemaforosCPU();

	serverMemoria = conexion_con_memoria();
	Conexion_con_planificador();
	//todo crear un hilo por cada cantidad que indica en config identificando a cada id de cpu
//	cpuID = pthread_create(&hiloCPU,NULL,ejecucion,NULL);//todo revisar porque el idCPU no puede ser el valor de retorno de creacion del hilo
	printf("el id de la CPU es:%d \n", cpuID);
	//  pthread_join(hiloCPU,NULL);

	return 0; //CARLA AMOR Y PAZ POR MI 0 :D
}

void LeerArchivoConfiguracion() {

	t_config* cfgCPU = malloc(sizeof(t_config));
	pthread_mutex_lock(&mutexLogueo);
	log_info(logCPU, "Leyendo Archivo de Configuracion");
	pthread_mutex_unlock(&mutexLogueo);
	cfgCPU = config_create(PATH_CONFIG);
	configuracionCPU.IPPlanificador = config_get_string_value(cfgCPU,
			"IP_PLANIFICADOR");
	configuracionCPU.PuertoPlanificador = config_get_string_value(cfgCPU,
			"PUERTO_PLANIFICADOR");
	configuracionCPU.IPMemoria = config_get_string_value(cfgCPU, "IP_MEMORIA");
	configuracionCPU.PuertoMemoria = config_get_string_value(cfgCPU,
			"PUERTO_MEMORIA");
	configuracionCPU.CantidadHilos = config_get_int_value(cfgCPU,
			"CANTIDAD_HILOS");
	configuracionCPU.Retardo = config_get_int_value(cfgCPU, "RETARDO");
	pthread_mutex_lock(&mutexLogueo);
	log_info(logCPU, "%s", configuracionCPU.PuertoPlanificador);
	log_info(logCPU, "Archivo de Configuracion Leido correctamente");
	pthread_mutex_unlock(&mutexLogueo);

}

void inicializarSemaforosCPU() {

	pthread_mutex_init(&mutexLogueo, NULL);

	pthread_mutex_lock(&mutexLogueo);
	log_info(logCPU, "Se inicializaron los semaforos \n");
	pthread_mutex_unlock(&mutexLogueo);

}

int conexion_con_memoria() {

	printf("Conectando a memoria \n");
	pthread_mutex_lock(&mutexLogueo);
	log_info(logCPU, "Conectando a memoria\n");
	pthread_mutex_unlock(&mutexLogueo);

	struct addrinfo hints;
	struct addrinfo *serverInfo;

	memset(&hints, 0, sizeof(hints));
	hints.ai_family = AF_UNSPEC; // Permite que la maquina se encargue de verificar si usamos IPv4 o IPv6
	hints.ai_socktype = SOCK_STREAM;	// Indica que usaremos el protocolo TCP

	// conectando a memoria

	getaddrinfo(configuracionCPU.IPMemoria, configuracionCPU.PuertoMemoria,
			&hints, &serverInfo);
	int serverSocketMemoria;
	serverSocketMemoria = socket(serverInfo->ai_family, serverInfo->ai_socktype,
			serverInfo->ai_protocol);
	printf("socket memoria %d \n", serverSocketMemoria);
	connect(serverSocketMemoria, serverInfo->ai_addr, serverInfo->ai_addrlen);
	freeaddrinfo(serverInfo);	// No lo necesitamos mas

	return serverSocketMemoria;
}

void Conexion_con_planificador() {

	printf("Conectando a planificador \n");
	pthread_mutex_lock(&mutexLogueo);
	log_info(logCPU, "Conectando a planificador");
	pthread_mutex_unlock(&mutexLogueo);

	struct addrinfo hints;
	struct addrinfo *serverInfo;

	memset(&hints, 0, sizeof(hints));
	hints.ai_family = AF_UNSPEC;// Permite que la maquina se encargue de verificar si usamos IPv4 o IPv6
	hints.ai_socktype = SOCK_STREAM;	// Indica que usaremos el protocolo TCP

	getaddrinfo(configuracionCPU.IPPlanificador,
			configuracionCPU.PuertoPlanificador, &hints, &serverInfo);

	serverSocket = socket(serverInfo->ai_family, serverInfo->ai_socktype,
			serverInfo->ai_protocol);
	printf("socket %d \n", serverSocket);
	connect(serverSocket, serverInfo->ai_addr, serverInfo->ai_addrlen);
	freeaddrinfo(serverInfo);	// No lo necesitamos mas

	int enviar = 1;
	char message[PACKAGESIZE];
	//t_mensajeHeader mensaje;
	t_mensajeHeader * mensaje = malloc(sizeof(t_mensajeHeader));
	char * estructuraCabecera = malloc(sizeof(t_mensajeHeader));

	printf(
			"Conectado al servidor. Bienvenido al sistema, ya puede enviar mensajes. Escriba 'exit' para salir\n");
	while (enviar) {
		printf("recibir\n");
		//int status = recv(serverSocket, &mensaje, sizeof(mensaje), 0);
		int status = recv(serverSocket, estructuraCabecera,
				sizeof(t_mensajeHeader), 0);
		//if (errno == ECONNREFUSED) enviar = 0;
		//printf("Error! %s\n", strerror(errno));
		mensaje = (t_mensajeHeader *) estructuraCabecera;
		if (status > 0) {

			switch (mensaje->idmensaje) {

			/*case CORRERPATH:
			 //todo revisar si va este msj sino borrar este case
			 printf(
			 "recibido el mensaje correr path desde el planificador\n");
			 printf("reenvío mensaje a memoria\n");
			 strcpy(message, "Correr path\n");

			 break;*/
			case SALUDO:
				//recv(serverSocket, &PATH, sizeof(PATH), 0);
				recv(serverSocket, &PATH, mensaje->size, 0);
				printf("recibido el mensaje saludo de planificador \n");
				pthread_mutex_lock(&mutexLogueo);
				log_info(logCPU, "se recibe el msj de saludo de planificador:");
				log_info(logCPU, "PATH MCOD: %s", PATH);

				pthread_mutex_unlock(&mutexLogueo);

				break;
			case EJECUTARPROC:
				printf(
						"recibido el mensaje de ejecutar proceso de planificador\n");
				pthread_mutex_lock(&mutexLogueo);
				log_info(logCPU,
						"se recibe el msj de ejecucion de un proceso:");
				pthread_mutex_unlock(&mutexLogueo);

				t_pcb pcbProc;

				recv(serverSocket, &pcbProc, sizeof(t_pcb), 0);
				printf("recibido el contexto del proceso con su id %d \n",
						pcbProc.pid);
				printf("recibido el contexto del proceso con su path %s \n",
						pcbProc.pathProc);

				pthread_mutex_lock(&mutexLogueo);
				log_info(logCPU, "ejecutando el proceso con id:%d",
						pcbProc.pid);
				pthread_mutex_unlock(&mutexLogueo);

				//todo verificar que struct usar para enviar instruccion-rta
				//todo desarrollar funcion que acumula las rtas por instruccion-rta
				/*		t_list* listaEjecucion;//lista local por cada proceso que se ejecuta
				 listaEjecucion = ejecutarmProc(pcbProc);*/
				parsermCod(pcbProc.pathProc, pcbProc.pid, pcbProc.proxInst);

				/*todo estas rtas van dentro de una funcion segun la ejecucion por linea de mproc*/

				//rtas al planificador en base a lo que se manda a ejecutar del proceso
				//FINDERAFAGA->parametros->pid, idcpu, mensaje de cada instruccion hecha
				/* t_mensajeHeader mjeFR;
				 mjeFR.idmensaje = FINDERAFAGA;
				 status = send(serverSocket, &(mjeFR.idmensaje), sizeof(t_mensajeHeader), 0);
				 printf("envio de fin de rafaga del proceso con id: %d ", pcbProc.pid); */

				//todo armar struct comun a planif y cpu de envio de fin de quantum
				//todo rtas a memoria
				//fin de quantum->parametros->pid, cpuid,mensaje de cada instruccion hecha y el tiempo consumido
				/*t_mensajeHeader mjeFQ;
				 mjeFQ.idmensaje = FINDEQUANTUM;
				 status = send(serverSocket, &(mjeFQ.idmensaje), sizeof(t_mensajeHeader), 0);
				 printf("envio de fin de quantum del proceso con id: %d ", pcbProc.pid);*/

				//todo armar struct comun a planif y cpu de envio de fin de quantum
				//todo rtas a memoria
				//PROCIO->parametros->pid,Tiempo, todo no falta el id de cpu?
				/*t_mensajeHeader mjeIO;
				 mjeIO.idmensaje = PROCIO;
				 status = send(serverSocket, &(mjeIO.idmensaje), sizeof(t_mensajeHeader), 0);
				 printf("envio de entrada-salida del proceso con id: %d ", pcbProc.pid);

				 t_io rtaIO;
				 rtaIO.pid = pcbProc.pid;
				 rtaIO.tiempoIO = 4; //todo seteado para probar-->proviene del parseo del mproc
				 status = send(serverSocket, &(rtaIO), sizeof(t_io), 0);
				 printf("y tiempo: %d \n",rtaIO.tiempoIO);*/
				//todo rtas a memoria
				break;

			default:
				printf("error al recibir\n");
				pthread_mutex_lock(&mutexLogueo);
				log_info(logCPU, "erro al recibir por codigo no reconocido");
				pthread_mutex_unlock(&mutexLogueo);

				enviar = 0;

			}

		}
	}
	free(estructuraCabecera);
	close(serverSocket);
	printf("Conexion a planificador cerrada \n");
	pthread_mutex_lock(&mutexLogueo);
	log_info(logCPU, "Conexion a planificador cerrada");
	pthread_mutex_unlock(&mutexLogueo);

}

//funcion que recibe el pcb del mProc parsea el mismo y todo devuelve una lista formada por instruccion resultado
t_list* ejecutarmProc(t_pcb pcbProc) {

	printf("en la funcion ejecutar proceso  \n");
	parsermCod(pcbProc.pathProc, pcbProc.pid, pcbProc.proxInst);
	printf("termino la funcion ejecutar proceso  \n");
	t_list* listaRtasEjecucion;
	listaRtasEjecucion = list_create();

	//esta lista de carga todas las rtas de cada linea de ejecucion del proceso

	return listaRtasEjecucion;

}

int busquedaPosicionCaracter(int posicion, char *listaDeArchivos,
		char valorABuscar) {

	if (listaDeArchivos[posicion] == '\0')
		return -1;
	else if (listaDeArchivos[posicion] == valorABuscar)
		return posicion;
	else
		return busquedaPosicionCaracter(posicion + 1, listaDeArchivos,
				valorABuscar);
}

char *parsearLinea(char * lineaLeida) {
	int posicion = busquedaPosicionCaracter(0, lineaLeida, ';');
	char lineaParseada[100] = "";
	strncpy(lineaParseada, &lineaLeida[0], posicion);
	//printf("Linea leida1: %s\n", lineaParseada);
	return lineaParseada;
}

//todo revisar
/*void *ejecucion (void *ptr){
 FILE *fd;
 fd = fopen("/home/utnso/codigo/test.cod","r");
 iniciar(3,12);
 escribir(3,"HOLA",12);
 leer(3,12);
 finalizar(12);
 fclose(fd);
 return 0;
 }*/

void iniciar(int paginas, int mProcID) {
	printf("mProc %d - Iniciado \n", mProcID);
	//t_mensajeHeader inicia;
	t_finalizarPID *mensajeFinalizar = malloc(sizeof(t_finalizarPID));
	t_iniciarPID *mensajeIniciar = malloc(sizeof(t_iniciarPID));
	t_mensajeHeader mensajeCpu;
	mensajeIniciar->paginas = paginas;
	mensajeIniciar->pid = mProcID;
	//inicia.idmensaje = INICIAR;
	//send(serverMemoria, &(inicia.idmensaje), sizeof(t_mensajeHeader), 0);

	//send(serverMemoria, &mensajeIniciar, sizeof(t_iniciarPID), 0);
	int status = serializarEstructura(INICIAR, (void *) mensajeIniciar,
			sizeof(t_iniciarPID), serverMemoria);

	recv(serverMemoria, &mensajeCpu, sizeof(t_mensajeHeader), 0);
	if (mensajeCpu.idmensaje == FINALIZAPROCOK) {
		log_info(logCPU, "El proceso %d iniciado correctamente correctamente\n",
				mProcID);
		sleep(configuracionCPU.Retardo);
	}
	if (mensajeCpu.idmensaje == PROCFALLA) {
		mensajeFinalizar->pid = mProcID;
		mensajeFinalizar->idCPU = cpuID;
		/* send(serverSocket,&mensajeCpu.idmensaje,sizeof(t_mensajeHeader),0);
		 send(serverSocket,&mensajeFinalizar.pid,sizeof(t_finalizarPID),0);*/
		log_info(logCPU, "proceso %d rechazado por falta de espacio en SWAP\n",
				mProcID);
		status = serializarEstructura(mensajeCpu.idmensaje,
				(void *) mensajeFinalizar, sizeof(t_finalizarPID),
				serverSocket);
		fseek(fid, -1, SEEK_END);	//TERMINO EL ARCHIVO!!!! NO SACAR!!!!!
	}
	free(mensajeIniciar);
	free(mensajeFinalizar);
}

void escribir(int pagina, char *texto, int mProcID) {
	t_escribir * escribir= malloc(sizeof(t_escribir));
	escribir->pid = mProcID;
	escribir->pagina = pagina;
	escribir->contenido = texto;

	serializarEstructura(ESCRIBIR,escribir,sizeof(escribir),serverMemoria);
	/*printf("mProc %d - Pagina %d escrita:%s \n", mProcID, pagina, texto);
	log_info(logCPU, "mProc %d - Pagina %d escrita:%s \n", mProcID, pagina,
			texto);*/

	//todo msj de rta con memoria

	sleep(configuracionCPU.Retardo);

}

void leer(int pagina, int mProcID) {
	//t_mensajeHeader inicia;
	t_leer *mensajeLeer = malloc(sizeof(t_leer));
	int tamanio;
	char * contenido;

	//inicia.idmensaje = LEER;
	printf("mProc %d - Pagina %d a leer, envio a memoria \n", mProcID, pagina);
	log_info(logCPU, "mProc %d - Pagina %d a leer:%s, envio a memoria \n",
			mProcID, pagina);

//	int status = send(serverMemoria, &(inicia.idmensaje),
	//		sizeof(t_mensajeHeader), 0);
	//if (status > 0) {

	mensajeLeer->pid = mProcID;
	mensajeLeer->pagina = pagina;
	//status = send(serverMemoria, &mensajeLeer, sizeof(t_leer), 0);
	int status = serializarEstructura(LEER, (void *) mensajeLeer,
			sizeof(t_leer), serverMemoria);

	recv(serverMemoria, &tamanio, sizeof(int), 0);
	//t_mensajeHeader header;
	//recv(serverMemoria, &header, sizeof(t_mensajeHeader), 0);
	//tamanio = header.size;
	contenido = malloc(tamanio + 1);	//+1 por fin de cadena
	recv(serverMemoria, contenido, sizeof(tamanio) + 1, 0);
	printf("CONTENIDO:%s \n", contenido);
	log_info(logCPU, "El contenido es: %s \n", contenido);
	fflush(stdout);
	//}

	sleep(configuracionCPU.Retardo);
	free(mensajeLeer);
}
void procesaIO(int pid, int tiempo, int cpu, int instrucciones) {
	//	envía mensaje de IO a planificador.
	t_io *infoIO = malloc(sizeof(t_io));

	infoIO->pid = pid;
	infoIO->tiempoIO = tiempo;
	infoIO->idCPU = cpu;
	infoIO->instrucciones = instrucciones;
	log_info(logCPU, "Entrada Salida- PID: %d, tiempo: %d, CPU: %d \n",
			infoIO->pid, infoIO->tiempoIO, infoIO->idCPU);
	int status = serializarEstructura(PROCIO, (void *) infoIO, sizeof(t_io),
			serverSocket);

	log_info(logCPU, "Status envío IO: %d \n", status);
	free(infoIO);
	//todo enviar las sentencias ejecutadas hasta ahora

}
void finalizar(int mProcID, int instrucciones) {

	//t_mensajeHeader header;
	t_finalizarPID *mensajeFinalizar = malloc(sizeof(t_finalizarPID));

	//header.idmensaje = FINALIZAR;
	printf("mProc %d - Finalizado \n", mProcID);
	/*	int status = send(serverMemoria, &header.idmensaje,
	 sizeof(t_mensajeHeader), 0);
	 if (status > 0) {*/
	//	recvACK(serverMemoria);
	mensajeFinalizar->pid = mProcID;
	mensajeFinalizar->instrucciones = instrucciones;
	/*
	 status = send(serverMemoria, &mensajeFinalizar, sizeof(t_finalizarPID),0);
	 }*/
	int status = serializarEstructura(FINALIZAR, (void *) mensajeFinalizar,
			sizeof(t_finalizarPID), serverMemoria);
	t_mensajeHeader rta;
	recv(serverMemoria, &rta, sizeof(t_mensajeHeader), 0);

	sleep(configuracionCPU.Retardo);

	/*rta.idmensaje = FINALIZAPROCOK;
	 status = send(serverSocket, &(rta.idmensaje), sizeof(t_mensajeHeader), 0);
	 printf("envio finalizar ok del proceso con id: %d ", mProcID);*/

	t_finalizarPID *rtaFin = malloc(sizeof(t_finalizarPID));
	rtaFin->pid = mProcID;
	rtaFin->idCPU = cpuID;
	//status = send(serverSocket, &(rtaFin), sizeof(t_finalizarPID), 0);
	status = serializarEstructura(FINALIZAPROCOK, (void *) rtaFin,
			sizeof(t_finalizarPID), serverSocket);
	printf("de la cpu con id: %d \n", rtaFin->idCPU);
	log_info(logCPU, "mProc %d - Finalizado, de la cpu con id: %d\n", mProcID,
			rtaFin->idCPU);
	free(mensajeFinalizar);
	free(rtaFin);
}

void parsermCod(char *path, int pid, int lineaInicial) {
	int i = 0;
	int contadorEjecutadas = 0;
	int seguir = 1;
	char *path_absoluto = string_new();
	log_info (logCPU,"Inicia parseo desde linea incial: %d", lineaInicial);
	string_append(&path_absoluto, PATH);
	string_append(&path_absoluto, path);

	//if ((fid = fopen(path, "r")) == NULL) {
	if ((fid = fopen(path_absoluto, "r")) == NULL) {
		printf("Error al abrir el archivo \n");
	} else {

		char *p;

		char string[100];

		while (!feof(fid) && seguir) //Recorre el archivo
		{
			//printf(" I: %d, linea inicial: %d \n", i , lineaInicial);
			i++;
			fgets(string, 100, fid);

			p = strtok(string, ";");
			if (i >= lineaInicial) {
			/*	i++;
				fgets(string, 100, fid);

				p = strtok(string, ";");*/

				if (p != NULL) {
					char *string = string_new();
					string_append(&string, p);
					char** substrings = string_split(string, " ");

					if (esIniciar(substrings[0])) {
						contadorEjecutadas++;
						printf("comando iniciar, parametro %d \n",
								atoi(substrings[1]));
						iniciar(atoi(substrings[1]), pid);
						free(substrings[0]);
						free(substrings[1]);
						free(substrings);
					}
					if (esLeer(substrings[0])) {
						contadorEjecutadas++;
						printf("comando leer, parametro %d \n",
								atoi(substrings[1]));
						leer(atoi(substrings[1]), pid);
						free(substrings[0]);
						free(substrings[1]);
						free(substrings);
					}
					if (esEscribir(substrings[0])) {
						contadorEjecutadas++;
						printf("comando Escribir, parametros %d  %s \n",
								atoi(substrings[1]), substrings[2]);
						escribir(atoi(substrings[1]), substrings[2], pid);
						free(substrings[0]);
						free(substrings[1]);
						free(substrings[2]);
						free(substrings);
					}
					if (esIO(substrings[0])) {
						contadorEjecutadas++;
						printf("comando entrada salida, parametro %d \n",
								atoi(substrings[1]));
						//todo cuando haya n hilos pasar el id que corresponde
						procesaIO(pid, atoi(substrings[1]), cpuID,
								contadorEjecutadas);
						free(substrings[0]);
						free(substrings[1]);
						free(substrings);

						//todo hay que salir de aca... exit? pongo break por ahora
						seguir = 0; //para que salga del while
					}
					if (esFinalizar(substrings[0])) {
						contadorEjecutadas++;
						printf("comando Finalizar no tiene parametros \n");
						finalizar(pid, contadorEjecutadas);
						free(substrings[0]);
						free(substrings);
					}

				}
			}


		}
		fclose(fid);

	}
}

bool esLeer(char* linea) {
	return string_starts_with(linea, TOKENLEER);
}
bool esEscribir(char* linea) {
	return string_starts_with(linea, TOKENESCRIBIR);
}

bool esIniciar(char* linea) {
	return string_starts_with(linea, TOKENINICIAR);
}
bool esIO(char* linea) {
	return string_starts_with(linea, TOKENIO);
}
bool esFinalizar(char* linea) {
	return string_starts_with(linea, TOKENFINALIZAR);
}
