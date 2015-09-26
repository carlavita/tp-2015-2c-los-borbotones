/*
 * CPU.c
 *
 *  Created on: 1/9/2015
 *      Author: Fernando Sanchez
 */


#include "CPU.h"



int main ()  {
	pthread_t hiloCPU;
	//int cpuID

	remove(PATH_LOG);
	logCPU = log_create(PATH_LOG,"CPU",true,LOG_LEVEL_INFO);
	log_info(logCPU,"Inicio Proceso CPU");

	LeerArchivoConfiguracion();
	inicializarSemaforosCPU();

	serverMemoria = conexion_con_memoria();
	Conexion_con_planificador();
//	cpuID = pthread_create(&hiloCPU,NULL,ejecucion,NULL);//todo revisar porque el idCPU no puede ser el valor de retorno de creacion del hilo
	printf("el id de la CPU es:%d \n",cpuID);
  //  pthread_join(hiloCPU,NULL);

    return 0; //CARLA AMOR Y PAZ POR MI 0 :D
}


void  LeerArchivoConfiguracion(){

	t_config* cfgCPU = malloc(sizeof(t_config));
	pthread_mutex_lock(&mutexLogueo);
	log_info(logCPU,"Leyendo Archivo de Configuracion");
	pthread_mutex_unlock(&mutexLogueo);
	cfgCPU = config_create(PATH_CONFIG);
	configuracionCPU.IPPlanificador = config_get_string_value(cfgCPU,"IP_PLANIFICADOR");
	configuracionCPU.PuertoPlanificador = config_get_string_value(cfgCPU,"PUERTO_PLANIFICADOR");
	configuracionCPU.IPMemoria = config_get_string_value(cfgCPU,"IP_MEMORIA");
	configuracionCPU.PuertoMemoria = config_get_string_value(cfgCPU,"PUERTO_MEMORIA");
	configuracionCPU.CantidadHilos = config_get_int_value(cfgCPU,"CANTIDAD_HILOS");
	configuracionCPU.Retardo = config_get_int_value(cfgCPU,"RETARDO");
	pthread_mutex_lock(&mutexLogueo);
	log_info (logCPU,"%s",configuracionCPU.PuertoPlanificador);
	log_info(logCPU,"Archivo de Configuracion Leido correctamente");
	pthread_mutex_unlock(&mutexLogueo);

}

void inicializarSemaforosCPU(){

	pthread_mutex_init(&mutexLogueo, NULL);

	pthread_mutex_lock(&mutexLogueo);
	log_info(logCPU, "Se inicializaron los semaforos \n");
	pthread_mutex_unlock(&mutexLogueo);

}

int conexion_con_memoria(){

	printf("Conectando a memoria \n");
	pthread_mutex_lock(&mutexLogueo);
	log_info(logCPU,"Conectando a memoria\n");
	pthread_mutex_unlock(&mutexLogueo);

		struct addrinfo hints;
		struct addrinfo *serverInfo;

		memset(&hints, 0, sizeof(hints));
		hints.ai_family = AF_UNSPEC;		// Permite que la maquina se encargue de verificar si usamos IPv4 o IPv6
		hints.ai_socktype = SOCK_STREAM;	// Indica que usaremos el protocolo TCP


	// conectando a memoria

		getaddrinfo(configuracionCPU.IPMemoria, configuracionCPU.PuertoMemoria, &hints, &serverInfo);
		int serverSocketMemoria;
		serverSocketMemoria = socket(serverInfo->ai_family, serverInfo->ai_socktype, serverInfo->ai_protocol);
		printf("socket memoria %d \n", serverSocketMemoria);
		connect(serverSocketMemoria, serverInfo->ai_addr, serverInfo->ai_addrlen);
		freeaddrinfo(serverInfo);	// No lo necesitamos mas

	return serverSocketMemoria;
}


void Conexion_con_planificador(){

		printf("Conectando a planificador \n");
		pthread_mutex_lock(&mutexLogueo);
		log_info(logCPU,"Conectando a planificador");
		pthread_mutex_unlock(&mutexLogueo);

		struct addrinfo hints;
		struct addrinfo *serverInfo;

		memset(&hints, 0, sizeof(hints));
		hints.ai_family = AF_UNSPEC;		// Permite que la maquina se encargue de verificar si usamos IPv4 o IPv6
		hints.ai_socktype = SOCK_STREAM;	// Indica que usaremos el protocolo TCP


		getaddrinfo(configuracionCPU.IPPlanificador, configuracionCPU.PuertoPlanificador, &hints, &serverInfo);
		int serverSocket;
		serverSocket = socket(serverInfo->ai_family, serverInfo->ai_socktype, serverInfo->ai_protocol);
		printf("socket %d \n", serverSocket);
		connect(serverSocket, serverInfo->ai_addr, serverInfo->ai_addrlen);
		freeaddrinfo(serverInfo);	// No lo necesitamos mas

		int enviar = 1;
		char message[PACKAGESIZE];
		t_mensajeHeader mensaje;
		printf("Conectado al servidor. Bienvenido al sistema, ya puede enviar mensajes. Escriba 'exit' para salir\n");
		while (enviar){
			printf("recibir\n");
		int status = recv(serverSocket, &mensaje, sizeof(mensaje), 0);
			if (status > 0){

				switch ( mensaje.idmensaje) {

					case CORRERPATH:

						printf("recibido el mensaje correr path desde el planificador\n");
						printf("reenvío mensaje a memoria\n");
						strcpy(message,"Correr path\n");
						/*SACAR*/
						t_mensajeHeader inicia;
						inicia.idmensaje = INICIAR;
						status = send(serverMemoria, &(inicia.idmensaje), sizeof(t_mensajeHeader), 0);
						/*SACAR*/
						//status = send(serverMemoria, message, strlen(message) + 1, 0);

						break;
					case SALUDO:
						printf("recibido el mensaje saludo de planificador\n");

						break;
					case EJECUTARPROC:
						printf("recibido el mensaje de ejecutar proceso de planificador\n");

						t_pcb pcbProc;
						recv(serverSocket, &pcbProc, sizeof(t_pcb), 0);
						printf("recibido el contexto del proceso de planificador con su id %d \n",pcbProc.pid);
						printf("recibido el contexto del proceso de planificador con su path %s \n",pcbProc.pathProc);

				/*		t_list* listaEjecucion;//lista local por cada proceso que se ejecuta
						listaEjecucion = ejecutarmProc(pcbProc);*/
						parsermCod(pcbProc.pathProc);

						//todo prueba borrar!
						t_mensajeHeader inicia1;
						inicia.idmensaje = INICIAR;
						status = send(serverMemoria, &(inicia1.idmensaje), sizeof(t_mensajeHeader), 0);

						/*todo estas rtas van dentro de una funcion segun la ejecucion por linea de mproc*/

						//rtas al planificador en base a lo que se manda a ejecutar del proceso
						//FINDERAFAGA->parametros->pid, idcpu, mensaje de cada instruccion hecha


						//fin de quantum->parametros->pid, cpuid,mensaje de cada instruccion hecha y el tiempo consumido


						//PROCIO->parametros->pid,Tiempo, todo no falta el id de cpu?
						t_mensajeHeader mjeIO;
						mjeIO.idmensaje = PROCIO;
						status = send(serverSocket, &(mjeIO.idmensaje), sizeof(t_mensajeHeader), 0);
						printf("envio de entrada-salida del proceso con id: %d ", pcbProc.pid);

						t_io rtaIO;
						rtaIO.pid = pcbProc.pid;
						rtaIO.tiempoIO = 4; //todo seteado para probar-->proviene del parseo del mproc
						status = send(serverSocket, &(rtaIO), sizeof(t_io), 0);
						printf("y tiempo: %d \n",rtaIO.tiempoIO);
						//todo rtas a memoria


						//terminarconfalla->parametros->pid, cpuid


						//finalizarprocok->parametros->pid, cpuid

						//todo logica de finalizar proceso ok
						/*t_mensajeHeader rta;
						rta.idmensaje = FINALIZAPROCOK;
						status = send(serverSocket, &(rta.idmensaje), sizeof(t_mensajeHeader), 0);
						printf("envio finalizar ok del proceso con id: %d ", pcbProc.pid);

						t_finalizarPID rtaFin;
						rtaFin.pid = pcbProc.pid;
						rtaFin.idCPU = cpuID;
						status = send(serverSocket, &(rtaFin), sizeof(t_finalizarPID), 0);
						printf("de la cpu con id: %d \n",rtaFin.idCPU);*/

						//todo rtas a memoria

						//finalizarprocfalla->parametros-> pid, cpuid

					/*	t_mensajeHeader rtaE;
						rtaE.idmensaje = PROCFALLA;
						status = send(serverSocket, &(rtaE.idmensaje), sizeof(t_mensajeHeader), 0);
						printf("envio falla del proceso con id: %d ", pcbProc.pid);

						t_finalizarPID rtaF;
						rtaF.pid = pcbProc.pid;
						rtaF.idCPU = cpuID;
						status = send(serverSocket, &(rtaF), sizeof(t_finalizarPID), 0);
						printf("de la cpu con id: %d \n", rtaF.idCPU);*/
						//todo rtas a memoria

						break;

					default:
						printf("error al recibir\n");
						enviar = 0;

					}


			}
		}

		close(serverSocket);
		printf("Conexion a planificador cerrada \n");
		pthread_mutex_lock(&mutexLogueo);
		log_info(logCPU,"Conexion a planificador cerrada");
		pthread_mutex_unlock(&mutexLogueo);

}


//funcion que recibe el pcb del mProc parsea el mismo y todo devuelve una lista formada por instruccion resultado
t_list* ejecutarmProc(t_pcb pcbProc){

	printf("en la funcion ejecutar proceso  \n");
	parsermCod(pcbProc.pathProc);
	printf("termino la funcion ejecutar proceso  \n");
	t_list* listaRtasEjecucion;
	listaRtasEjecucion = list_create();

	//esta lista de carga todas las rtas de cada linea de ejecucion del proceso

	return listaRtasEjecucion;

}


int busquedaPosicionCaracter (int posicion,char *listaDeArchivos, char valorABuscar){

if (listaDeArchivos[posicion]== '\0')
	return -1; else if (listaDeArchivos[posicion]== valorABuscar)
		return posicion;
	else
	return busquedaPosicionCaracter(posicion+1,listaDeArchivos,valorABuscar);
}

char *parsearLinea(char * lineaLeida){
	int posicion = busquedaPosicionCaracter (0,lineaLeida,';');
	char lineaParseada[100] = "";
	strncpy(lineaParseada,&lineaLeida[0],posicion);
	//printf("Linea leida1: %s\n", lineaParseada);
	return lineaParseada;
}

//todo revisar
void *ejecucion (void *ptr){
	FILE *fd;
	fd = fopen("/home/utnso/codigo/test.cod","r");
	iniciar(3,12);
	escribir(3,"HOLA",12);
	leer(3,12);
	finalizar(12);
	fclose(fd);
	return 0;
}


void iniciar (int paginas, int mProcID){
	printf("mProc %d - Iniciado \n", mProcID);
	sleep(1);
}


void escribir (int pagina, char *texto, int mProcID){
	printf("mProc %d - Pagina %d escrita: HOLA \n", mProcID, pagina);
	sleep(5);
}


void leer (int pagina, int mProcID){
	printf("mProc %d - Pagina %d leida: HOLA \n", mProcID, pagina);
	sleep(5);
}


void finalizar (int mProcID){
	printf("mProc %d - Finalizado \n", mProcID);
	sleep(1);
}


void parsermCod(char *path){
	FILE* fid;
	if ((fid = fopen(path, "r")) == NULL) {
		printf("Error al abrir el archivo \n");
	}else{

	char *p;

	char string[100];

	while (!feof(fid)) //Recorre el archivo
	{
		fgets(string, 100, fid);

		p = strtok(string, ";");


		if (p != NULL) {
			char *string = string_new();
			string_append(&string, p);
			char** substrings = string_split(string, " ");

			if (esIniciar(substrings[0])) {
				printf("comando iniciar, parametro %d \n", atoi(substrings[1]));
				free(substrings[0]);
				free(substrings[1]);
				free(substrings);
			}
			if (esLeer(substrings[0])) {
				printf("comando leer, parametro %d \n", atoi(substrings[1]));
				free(substrings[0]);
				free(substrings[1]);
				free(substrings);
			}
			if (esEscribir(substrings[0])) {
				printf("comando Escribir, parametros %d  %s \n",
						atoi(substrings[1]), substrings[2]);
				/*
				free(substrings[0]);
				free(substrings[1]);
				free(substrings[2]);
				free(substrings);
			*/}
			if (esIO(substrings[0])) {
				printf("comando entrada salida, parametro %d \n",
						atoi(substrings[1]));
				free(substrings[0]);
				free(substrings[1]);
				free(substrings);
			}
			if (esFinalizar(substrings[0])) {
							printf("comando Finalizar no tiene parametros \n");
							free(substrings[0]);
							free(substrings);
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
