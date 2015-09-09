/*
 * planificador.c
 *
 *  Created on: 30/8/2015
 *      Author: carlavita
 */

#include "planificador.h"
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <unistd.h>

int main(void) {
//	pthread_t hilo_consola;
	pthread_t hilo_server;

	/*Log*/
	remove(PATH_LOG);
	logger = log_create(PATH_LOG,"Planificador",true,LOG_LEVEL_INFO);
	log_info(logger,"Iniciando proceso planificador");

/*Archivo de configuración*/

	log_info(logger,"Leyendo Archivo de Configuracion");
	levantarConfiguracion();
	log_info(logger,"Archivo de Configuracion Leido correctamente");
        inicializar_listas();
	/*Hilo Server Planificador*/
	//todo sincronizar hilos consola y conexion cpu
		pthread_create (&hilo_server, NULL, (void *) &servidor_CPU, NULL);

		mostrar_consola(NULL);
/*Hilo Consola
	pthread_create (&hilo_consola, NULL, (void *) &mostrar_consola, NULL);
*/

	//pthread_join(hilo_consola,NULL);

	pthread_join(hilo_server,NULL);
	return EXIT_SUCCESS;
}

void inicializar_listas(){

	NUEVOS = list_create();
	LISTOS = list_create();
	EJECUTANDO = list_create();;
	BLOQUEADOS= list_create();;
	FINALIZADOS=list_create();;

}
void levantarConfiguracion(){

	log_info(logger,"Lectura de variables del archivo de configuracion");
	  t_config * CONFIG = config_create(PATH_CONFIG);

	  if	(CONFIG != NULL){
	  config_planificador.puertoEscucha = malloc(PUERTO_SIZE);
	  config_planificador.puertoEscucha = config_get_string_value(CONFIG,"PUERTO_ESCUCHA");

	  config_planificador.algoritmo = config_get_int_value(CONFIG,"ALGORITMO");
	  config_planificador.quantum = config_get_int_value(CONFIG,"QUANTUM");

	  printf(" puerto %s \n",config_planificador.puertoEscucha );
	  printf(" algoritmo %d\n",config_planificador.algoritmo );
	  printf(" quantum %d\n",config_planificador.quantum );
	  }else{
		  printf("estupido config, se cree tan importante");
	  }

}

void mostrar_consola( void *ptr ){
	int option = -1;
	while(option != 0){
//todo: descomentar todos clear		system("clear");

		puts("Consola - Planificador \n \n"
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
//todo
			correr_path();
			printf("soy el planificador, recibí el mensaje correr path por consola! Envi a CPU\n");
						int mensaje = CHECKPOINT;
						printf("socket : %d \n", servidor);
						int status = send(servidor,&mensaje,sizeof(int),0);
						printf("estado de envio : %d \n", status);
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

		default:
		 printf("Comando inválido\n");
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
	 	servidor = socketCliente;
	 	//char package[PACKAGESIZE];
	 	int status = 1;		// Estructura que manejea el status de los recieve.

	 	printf("CPU conectado. Esperando mensajes:\n");
	 	log_info(logger,"CPU conectado");

	 	/*while (status != 0){
	 			status = recv(socketCliente, (void*) package, PACKAGESIZE, 0);
	 			if (status != 0) printf("%s", package);

	 	}

	 	close(socketCliente);
	 	close(listenningSocket);*/
	 	//status = recv(socketCliente, (void*) package, PACKAGESIZE, 0);

	 	//int mensaje = CHECKPOINT;
	 	int mensaje = SALUDO;
	 	status = send(socketCliente,&mensaje , sizeof(int), 0);
	 	printf("status send inicial %d", status);
	 	printf("Cierro conexion con cpu \n");
	 	log_info(logger,"Cierro conexion con CPU");


}

void correr_path(){
	char path[PATH_SIZE];
	printf("Ingrese el PATH del mCod a correr \n");
	  scanf("%s", path);
	printf("PATH del mCod a correr %s \n",path);

	int pid = crear_pcb(path);
}

int crear_pcb(char* path){

	t_pcb* pcb  = malloc(sizeof(t_pcb)) ;
	    pthread_mutex_lock(&mutex_listas);
	    PID++;
		pcb->pid = PID;
		pcb->proxInst = 0; //Inicializa
		strcpy(pcb->pathProc, path);

		printf("PID mProc: %d \n",pcb->pid);
		printf("Proxima instruccion mProc: %d \n",pcb->proxInst);
		printf("Path mProc: %s \n",pcb->pathProc);
/*Lo agrega a la lista de listos*/
		list_add(LISTOS,pcb);
		printf("PID %d sumado a la cola de ready\n",pcb->pid );
		pthread_mutex_unlock(&mutex_listas);

return pcb->pid;
}


int planificar_Fifo(){

	t_pcb* pcb = list_get(LISTOS,0);
	return pcb->pid;
}
void enviarACpu(t_pcb* pcb,t_cpu* cpu)
{
// Elimina el PID de la cola de listos
	removerEnListaPorPid(LISTOS,pcb->pid);
//Pasa  a lista de ejecución
	pthread_mutex_lock(&mutex_listas);
	list_add(EJECUTANDO,pcb);
	pthread_mutex_unlock(&mutex_listas);


	//cpu->pid = pcb->pid; //todo, ver si me sirve que la cpu tenga el pid

	// todo enviarPcb(cpu->socket,pcb );
}

void removerEnListaPorPid(t_list *lista, int pid) {
	int _is_pcb(t_pcb *p) {
		return p->pid == pid;
	}
	pthread_mutex_lock(&mutex_listas);
	list_remove_by_condition(lista, (void*) _is_pcb);
	pthread_mutex_unlock(&mutex_listas);
}

t_pcb* buscarEnListaPorPID(t_list* lista, int pid) {
	// Busca por pid
	int _is_pcb(t_pcb *p) {
		return p->pid == pid;
	}
	t_pcb* pcb = list_find(lista, (void*) _is_pcb);

		return pcb;

}



