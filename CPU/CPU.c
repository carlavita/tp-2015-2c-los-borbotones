/*
 * CPU.c
 *
 *  Created on: 1/9/2015
 *      Author: Fernando Sanchez
 */

#include "CPU.h"

int main() {

	pthread_t threads[MAXTHREADS];
	int i;

	remove(PATH_LOG);
	logCPU = log_create(PATH_LOG, "CPU", true, LOG_LEVEL_INFO);
	log_info(logCPU, "Inicio Proceso CPU");

	LeerArchivoConfiguracion();
	inicializarSemaforosCPU();

	int id = 1;	//para que el id de cpu comience en 1
	for (i = 0; i < configuracionCPU.CantidadHilos; i++) {

		t_envio *envio = malloc(sizeof(t_envio));
		envio->id = id + i;
		pthread_create(&threads[i], NULL, thread_func, (void*) envio);
	}

	for (i = 0; i < configuracionCPU.CantidadHilos; i++)
		pthread_join(threads[i], NULL);

	return 0; //CARLA AMOR Y PAZ POR MI 0 :D
}

void* thread_func(void *envio) {

	pthread_t hiloPorc;
	t_envio* num;
	num = (t_envio*) envio;
	printf("dentro del hilo de cpu con id:%d \n", num->id);

	//creacion de hilo calculador de porcentaje de uso de cpu
	pthread_create(&hiloPorc, NULL, (void *) &calcularPorc, NULL);
	Conexion_con_planificador(num->id);

	pthread_join(hiloPorc, NULL);
	pthread_exit(NULL);

}

void calcularPorc(void *ptr){

	printf("Calculando porcentaje de uso de cpu %lf \n", porcentajeCPU);

	//aca un while con sleep de 60 seg para actualizar el porcentaje
	/*while (1){
		sleep(60);
		//todo realizar el porcentaje de uso del último minuto de cada cpu
	}*/

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

void Conexion_con_planificador(int cpu) {

	int serverMemoria = conexion_con_memoria();
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

	int serverSocket = socket(serverInfo->ai_family, serverInfo->ai_socktype,
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
		if  (status <= 0) enviar = 0;
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
				send(serverSocket, &cpu, sizeof(int),0);

				break;
			case EJECUTARPROC:
				printf("recibido el mensaje de ejecutar proceso de planificador\n");
				pthread_mutex_lock(&mutexLogueo);
				log_info(logCPU,"se recibe el msj de ejecucion de un proceso:");
				pthread_mutex_unlock(&mutexLogueo);

				t_pcb pcbProc;

				recv(serverSocket, &pcbProc, sizeof(t_pcb), 0);
				printf("recibido el contexto del proceso con su id %d \n",pcbProc.pid);
				printf("recibido el contexto del proceso con su path %s \n",
						pcbProc.pathProc);

				pthread_mutex_lock(&mutexLogueo);
				log_info(logCPU, "ejecutando el proceso con id:%d",pcbProc.pid);
				pthread_mutex_unlock(&mutexLogueo);

				//todo verificar que struct usar para enviar instruccion-rta
				//todo desarrollar funcion que acumula las rtas por instruccion-rta
				/*		t_list* listaEjecucion;//lista local por cada proceso que se ejecuta
				 listaEjecucion = ejecutarmProc(pcbProc);*/
				parsermCod(cpu, pcbProc.pathProc, pcbProc.pid, pcbProc.proxInst,
						serverSocket, serverMemoria, pcbProc.quantum);

				break;
			case COMANDOCPU:

				printf("recibido el comando cpu de planificador \n");
				//todo enviar a planificador la valor del porcentaje de uso de cpu

				break;
			default:
				printf("error al recibir\n");
				pthread_mutex_lock(&mutexLogueo);
				log_info(logCPU, "error al recibir por codigo no reconocido");
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
t_list* ejecutarmProc(int cpu, t_pcb pcbProc, int serverSocket,
		int serverMemoria) {

	printf("en la funcion ejecutar proceso  \n");
	parsermCod(cpu, pcbProc.pathProc, pcbProc.pid, pcbProc.proxInst,
			serverSocket, serverMemoria, pcbProc.quantum);
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

char *iniciar(int cpu,int paginas, int mProcID,int serverSocket,int serverMemoria) {

	printf("mProc %d - Iniciado \n", mProcID);

	char *comienzo = string_new();//cadena donde devuelvo el resultado de la instruccion
	char *id = string_itoa(mProcID);
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
		log_info(logCPU, "El proceso %d inició su ejecucion correctamente\n",
				mProcID);
		sleep(configuracionCPU.Retardo);

		string_append(&comienzo,"mProc-");
		/*char id[12];
		sprintf(id,"%d",mProcID);*/
		string_append(&comienzo,id);
		string_append(&comienzo,"-Iniciado");
	}
	if (mensajeCpu.idmensaje == PROCFALLA) {
		mensajeFinalizar->pid = mProcID;
		mensajeFinalizar->idCPU = cpu;
		/* send(serverSocket,&mensajeCpu.idmensaje,sizeof(t_mensajeHeader),0);
		 send(serverSocket,&mensajeFinalizar.pid,sizeof(t_finalizarPID),0);*/
		log_info(logCPU, "proceso %d rechazado por falta de espacio en SWAP\n",
				mProcID);
		status = serializarEstructura(mensajeCpu.idmensaje,
				(void *) mensajeFinalizar, sizeof(t_finalizarPID),
				serverSocket);

		string_append(&comienzo,"mProc-");
		string_append(&comienzo,id);
		string_append(&comienzo,"-Fallo");

		fseek(fid, -1, SEEK_END);	//TERMINO EL ARCHIVO!!!! NO SACAR!!!!!
	}
	free(mensajeIniciar);
	free(mensajeFinalizar);

	return comienzo;
}

//void escribir(int pagina, char *texto, int mProcID, int serverSocket, int serverMemoria) {
char *escribir(int pagina, char *texto, int mProcID, int serverSocket, int serverMemoria) {

	printf("mProc %d - Pagina %d escrita:%s \n", mProcID, pagina, texto);
	t_escribir *mensajeEscribir = malloc(sizeof(t_escribir));
	//int tamanio;
	//char * contenido;

	//inicia.idmensaje = LEER;
	printf("mProc %d - Pagina %d a escribir con contenido %s, envio a memoria \n",
			mProcID, pagina, texto);

	char *comienzo = string_new();//cadena donde devuelvo el resultado de la instruccion
	char *id = string_itoa(mProcID);
	char *pag = string_itoa(pagina);

//	int status = send(serverMemoria, &(inicia.idmensaje),
	//		sizeof(t_mensajeHeader), 0);
	//if (status > 0) {

	mensajeEscribir->pid = mProcID;
	mensajeEscribir->pagina = pagina;
	strcpy(mensajeEscribir->contenidoPagina, texto);
	//status = send(serverMemoria, &mensajeLeer, sizeof(t_leer), 0);
	int status = serializarEstructura(ESCRIBIR, (void *) mensajeEscribir,
			sizeof(t_escribir), serverMemoria);
    free(mensajeEscribir);
	t_mensajeHeader reta;
	recv(serverMemoria, &reta, sizeof(t_mensajeHeader), 0);
	printf("La respuesta fue %d", reta.idmensaje);
	log_info(logCPU, "mProc %d - Pagina %d escrita:%s \n", mProcID, pagina,
			texto);
	//t_mensajeHeader rta;
	//recv(serverMemoria, &rta, sizeof(t_mensajeHeader), 0);
	//printf("La respuesta es %d", rta.idmensaje);

	//todo msj de rta con memoria

	sleep(configuracionCPU.Retardo);

	string_append(&comienzo,"mProc-");
	string_append(&comienzo,id);
	string_append(&comienzo,"-Pagina-");
	string_append(&comienzo,pag);
	string_append(&comienzo,"-escrita:");
	string_append(&comienzo,texto);

	return comienzo;

}

char *leer(int pagina, int mProcID, int serverSocket, int serverMemoria) {

	//t_mensajeHeader inicia;
	t_leer *mensajeLeer = malloc(sizeof(t_leer));
	int tamanio;
	char * contenido;
	char *comienzo = string_new();//cadena donde devuelvo el resultado de la instruccion
	char *id = string_itoa(mProcID);
	char *pag = string_itoa(pagina);

	//inicia.idmensaje = LEER;
	printf("mProc %d - Pagina %d a leer, envio a memoria \n", mProcID, pagina);
	log_info(logCPU, "mProc %d - Pagina %d a leer, envio a memoria \n", mProcID,
			pagina);

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
//	contenido = malloc(tamanio + 1);	//+1 por fin de cadena
	printf("tamanio recibido %d\n ", tamanio);
	contenido = malloc(tamanio);
//	recv(serverMemoria, contenido, sizeof(tamanio), 0);
	recv(serverMemoria, contenido, tamanio, 0);

//	recv(serverMemoria, contenido, sizeof(tamanio), 0);

	//contenido[tamanio-1] = '\0';
	//printf("CONTENIDO:%s \n", contenido);
	log_info(logCPU, "El contenido es: %s \n", contenido);

	//}

	sleep(configuracionCPU.Retardo);

	string_append(&comienzo,"mProc-");
	string_append(&comienzo,id);
	string_append(&comienzo,"-Pagina-");
	string_append(&comienzo,pag);
	string_append(&comienzo,"-Leida:");
	string_append(&comienzo,contenido);

	//free(contenido);
	return comienzo;
}

//void procesaIO(int pid, int tiempo, int cpu, int instrucciones, int serverSocket, int serverMemoria) {

char *procesaIO(int pid, int tiempo, int cpu, int instrucciones,
			int serverSocket, int serverMemoria) {

	char *comienzo = string_new();//cadena donde devuelvo el resultado de la instruccion
	char *id = string_itoa(pid);
	char *time = string_itoa(tiempo);

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

	string_append(&comienzo,"mProc-");
	string_append(&comienzo,id);
	string_append(&comienzo,"-en entrada salida de tiempo-");
	string_append(&comienzo,time);

	return comienzo;

}


char *finalizar(int cpu, int mProcID, int instrucciones, int serverSocket,int serverMemoria) {

	char *comienzo = string_new();//cadena donde devuelvo el resultado de la instruccion
	char *id = string_itoa(mProcID);
	//t_mensajeHeader header;
	t_finalizarPID *mensajeFinalizar = malloc(sizeof(t_finalizarPID));

	//header.idmensaje = FINALIZAR;
	printf("mProc %d - Finalizado \n", mProcID);
	/*	int status = send(serverMemoria, &header.idmensaje,
	 sizeof(t_mensajeHeader), 0);
	 if (status > 0) {*/
	//	recvACK(serverMemoria);
	mensajeFinalizar->pid = mProcID;
	mensajeFinalizar->idCPU = cpu;
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
	rtaFin->idCPU = cpu;
	rtaFin->instrucciones = instrucciones;
	//status = send(serverSocket, &(rtaFin), sizeof(t_finalizarPID), 0);
	status = serializarEstructura(FINALIZAPROCOK, (void *) rtaFin,
			sizeof(t_finalizarPID), serverSocket);
	printf("de la cpu con id: %d \n", rtaFin->idCPU);
	log_info(logCPU, "mProc %d - Finalizado, de la cpu con id: %d\n", mProcID,
			rtaFin->idCPU);

	string_append(&comienzo,"mProc-");
	string_append(&comienzo,id);
	string_append(&comienzo,"-Finalizado");

	free(mensajeFinalizar);
	free(rtaFin);

	return comienzo;

}

void finalizarQuantum(int cpu, int mProcID, int instrucciones, int serverSocket) {

	t_finalizarPID *finQuantum = malloc(sizeof(t_finalizarPID));
	printf("mProc %d - Fin de quantum \n", mProcID);
	finQuantum->pid = mProcID;
	finQuantum->idCPU = cpu;
	finQuantum->instrucciones = instrucciones;

	int status = serializarEstructura(FINDEQUANTUM, (void *) finQuantum,
			sizeof(t_finalizarPID), serverSocket);
	log_info(logCPU, "mProc %d - fin de quantum , CPU id: %d\n", mProcID,
			finQuantum->idCPU);
	free(finQuantum);
}

time_t obtenerTiempoActual(void){

	time_t timer;
	time(&timer);
	return timer;
}

double diferenciaEnSegundos (time_t inicio, time_t fin){

	double segundos;
	segundos = difftime(fin,inicio);
	printf ("%.f  segundos en\n", segundos);
	return segundos;
}

void parsermCod(int cpu, char *path, int pid, int lineaInicial, int serverSocket, int serverMemoria, int quantum) {

	time_t valori;
	valori = obtenerTiempoActual();
	char *resultado = string_new();//instrucciones concatenadas con / a devolver al planificador con los resultados
	int i = 0;
	int contadorEjecutadas = 0;
	int seguir = 1;
	char *path_absoluto = string_new();
	log_info(logCPU, "Inicia parseo desde linea incial: %d", lineaInicial);
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
			// chequeo de quantum
			if (quantum > 0  // es RR
						&& quantum == contadorEjecutadas) {

					finalizarQuantum(cpu, pid, contadorEjecutadas, serverSocket);
					//envio a planificador las lineas ejecutadas hasta el quantum
					int tamanio;
					tamanio = strlen(resultado)+1;//por el fin de cadena
					send(serverSocket,&tamanio, sizeof(int), 0);
					send(serverSocket,resultado, tamanio, 0);

					break;
			}
			//printf(" I: %d, linea inicial: %d \n", i , lineaInicial);
			i++;
			fgets(string, 100, fid);

			p = strtok(string, ";");
			if (i > lineaInicial) {

				if (p != NULL) {
					char *string = string_new();
					string_append(&string, p);
					char** substrings = string_split(string, " ");

					if (esIniciar(substrings[0])) {
						contadorEjecutadas++;
						printf("comando iniciar, parametro %d \n",atoi(substrings[1]));
						//iniciar(cpu, atoi(substrings[1]), pid, serverSocket,serverMemoria);

						char *inicio = iniciar(cpu, atoi(substrings[1]), pid,serverSocket,serverMemoria);

						string_append(&resultado,inicio);
						string_append(&resultado,SEPARADORINSTRUCCION);
						printf("el resultado de iniciar es %s \n",resultado);

						free(substrings[0]);
						free(substrings[1]);
						free(substrings);
						continue;
					}
					if (esLeer(substrings[0])) {
						contadorEjecutadas++;
						printf("comando leer, parametro %d \n",
								atoi(substrings[1]));
						//leer(atoi(substrings[1]), pid, serverSocket,serverMemoria);
						char *lectura = leer(atoi(substrings[1]), pid, serverSocket,serverMemoria);

						string_append(&resultado,lectura);
						string_append(&resultado,SEPARADORINSTRUCCION);
						printf("el resultado de leer es %s \n",resultado);

						free(substrings[0]);
						free(substrings[1]);
						free(substrings);
						continue;
					}
					if (esEscribir(substrings[0])) {
						contadorEjecutadas++;
						printf("comando Escribir, parametros %d  %s \n",
								atoi(substrings[1]), substrings[2]);
						//escribir(atoi(substrings[1]), substrings[2], pid,serverSocket, serverMemoria);
						char *escritura = escribir(atoi(substrings[1]), substrings[2], pid,serverSocket, serverMemoria);
						string_append(&resultado,escritura);
						string_append(&resultado,SEPARADORINSTRUCCION);
						printf("el resultado de leer es %s \n",resultado);

						free(substrings[0]);
						free(substrings[1]);
						free(substrings[2]);
						free(substrings);
						continue;
					}
					if (esIO(substrings[0])) {
						contadorEjecutadas++;
						printf("comando entrada salida, parametro %d \n",
								atoi(substrings[1]));

						//procesaIO(pid, atoi(substrings[1]), cpu,contadorEjecutadas, serverSocket,serverMemoria);
						char *entradaSalida = procesaIO(pid, atoi(substrings[1]), cpu,contadorEjecutadas, serverSocket,serverMemoria);
						string_append(&resultado,entradaSalida);
						string_append(&resultado,SEPARADORINSTRUCCION);
						printf("el resultado de leer es %s \n",resultado);

						int tamanioIO;
						tamanioIO = strlen(resultado)+1;//por el fin de cadena
						send(serverSocket,&tamanioIO, sizeof(int), 0);
						send(serverSocket,resultado, tamanioIO, 0);

						free(substrings[0]);
						free(substrings[1]);
						free(substrings);

						seguir = 0; //para que salga del while
						continue;
					}
					if (esFinalizar(substrings[0])) {
						contadorEjecutadas++;
						printf("comando Finalizar no tiene parametros \n");
					//	finalizar(cpu, pid, contadorEjecutadas, serverSocket,serverMemoria);
						char *fin = finalizar(cpu, pid, contadorEjecutadas, serverSocket,serverMemoria);
						string_append(&resultado,fin);
						string_append(&resultado,SEPARADORINSTRUCCION);
						printf("el resultado de leer es %s \n",resultado);

						int tamanioFin;
						tamanioFin = strlen(resultado)+1;//por el fin de cadena
						send(serverSocket,&tamanioFin, sizeof(int), 0);
						send(serverSocket,resultado, tamanioFin, 0);

						//todo pasar al hilo calculador
						//por finalizar proceso
						time_t valorf;
						valorf = obtenerTiempoActual();
						double diferencia = diferenciaEnSegundos(valori,valorf);

						//devuelve la cantidad de lineas ejecutadas cada 60 seg
						double total=(60*contadorEjecutadas)/diferencia;
						double porcentaje = (total* 100) / 60 ;
						printf("el porcentaje de uso de cpu es %lf \n",porcentaje);
						//hasta aca pasar al hilo

						free(substrings[0]);
						free(substrings);

						seguir = 0;
						continue;
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
