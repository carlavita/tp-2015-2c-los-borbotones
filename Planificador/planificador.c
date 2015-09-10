/*
 * planificador.c
 *
 *  Created on: 30/8/2015
 *      Author: carlavita
 */

#include "planificador.h"


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

		//seteado con el quantum, hay que evaluar que algoritmo elegido de planificacion


		mostrar_consola(NULL);

		planificar_RR(5);
/*Hilo Consola
	pthread_create (&hilo_consola, NULL, (void *) &mostrar_consola, NULL);
*/

	//pthread_join(hilo_consola,NULL);

	pthread_join(hilo_server,NULL);
	return EXIT_SUCCESS;
}

void inicializar_listas(){

	LISTOS = list_create();
	EJECUTANDO = list_create();
	BLOQUEADOS= list_create();
	FINALIZADOS=list_create();
	lista_CPU=list_create();

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


/*
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

	 	/* while (status != 0){
	 			status = recv(socketCliente, (void*) package, PACKAGESIZE, 0);
	 			if (status != 0) printf("%s", package);

	 	}

	 	close(socketCliente);
	 	close(listenningSocket);*/
	 	//status = recv(socketCliente, (void*) package, PACKAGESIZE, 0);

	 	//int mensaje = CHECKPOINT;
	/* 	int mensaje = SALUDO;
	 	status = send(socketCliente,&mensaje , sizeof(int), 0);
	 	printf("status send inicial %d", status);
//	 	printf("Cierro conexion con cpu \n");
//	 	log_info(logger,"Cierro conexion con CPU");


}

 */
void servidor_CPU( void *ptr ){

	 printf(" estoy en el hilo servidor de CPU\n");
		 //todo crear servidor para un cliente cpu...despues multiplexamos a las distintas cpus
		 log_info(logger,"Dentro del hilo conexion a cpu");

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

	     		     if (getaddrinfo(NULL,config_planificador.puertoEscucha, &hints, &res) != 0) {
	     		         perror("getaddrinfo");
	     		         exit (1);

	     		     }

	     		     /* Create the socket */
	     		     sock = socket(res->ai_family, res->ai_socktype, res->ai_protocol);
	     		     if (sock == -1) {
	     		         perror("socket");
	     		         exit (1);
	     		     }
	     		     printf("socket de escucha %d \n", sock);

	     		     /* Permitir al socket a reusar  address */
	     		     if (setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, &reuseaddr, sizeof(int)) == -1) {
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
	     		         log_info(logger, "Escuchando conexiones \n ");

	     		         if (select(maxsock + 1, &readsocks, NULL, NULL, NULL) == -1) {
	     		             perror("select");
	     		             exit (1);
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
	     		                     newsock = accept(sock, (struct sockaddr*)&their_addr, &size);
	     		                     if (newsock == -1) {
	     		                         perror("accept");
	     		                     }
	     		                     else {
	     		                         printf("Tenemos una conexion desde %s en el puerto %d\n",
	     		                                 inet_ntoa(their_addr.sin_addr), htons(their_addr.sin_port));
	     		                         FD_SET(newsock, &socks);
	     		                         if (newsock > maxsock) {
	     		                             maxsock = newsock;
	     		                         }

	     		                    	int status = 1;		// Estructura que maneja el status de los recieve

	     		                    	servidor = newsock;
	     		                    	int mensaje = SALUDO;
	     		                    	status = send(newsock,&mensaje , sizeof(int), 0);
	     		                    	printf("status send inicial %d", status);
	     		                     }
	     		                 }
	     		                 else {

	     		                	 //gestiona la conexion de una CPU segun los pedidos
	     		                	 handle(s,&socks);

	     		                 }
	     		             }
	     		         }

	     		     }

	     		     close(sock);



}

void handle(int newsock, fd_set *set){

		printf("En el handle de planificador");

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

int planificar_RR(int quantum){

	//el otro hilo se usa como planificador(consumidor) que elije de la cola de listos y lo pasa la lista de ejecutando con el quantum

	pthread_t hiloPlanificador;

	pthread_create(&hiloPlanificador, NULL,(void *) &planificador, NULL);

	//t_pcb* pcb = list_get(LISTOS,0);

	//enviarACpu(t_pcb* pcb,t_cpu* cpu);

	pthread_join(hiloPlanificador,NULL);

	return 0;
}


void *planificador(void *info_proc){
	printf("en el hilo planificador \n");

	while (true) {

			pthread_mutex_lock(&mutex_listas);

			t_pcb* pcb_ejecucion = list_remove(LISTOS, 0);

			list_add(EJECUTANDO,pcb_ejecucion);

			pthread_mutex_unlock(&mutex_listas);

			//enviar a cpu elegida el pcb del proceso elegido
			t_cpu* cpuLibre;
			cpuLibre = buscar_Cpu_Libre();
			printf("el id de la cpu libre es: %d /n", cpuLibre->id);

			//enviarACpu(cpuLibre,pcb_ejecucion.pid);

		}

	return 0;

}

void eliminar_CPU(int socket_cpu)
{
	t_cpu* cpuNodoLista = malloc(sizeof(t_cpu));
	int cont;
	pthread_mutex_lock(&mutex_lista_cpu);
	for(cont=0;cont < list_size(lista_CPU);cont++)
	{
		cpuNodoLista = list_get(lista_CPU, cont);
		if (cpuNodoLista->socket==socket_cpu)
		{	puts("CPU eliminada");
			//Remuevo el CPU de la lista

			list_remove(lista_CPU,cont);

			free(cpuNodoLista);
		}
	}
	pthread_mutex_unlock(&mutex_lista_cpu);
}

// CPUs nuevas, agregar con PID -1
void agregar_CPU(int cpuSocket, int pid) {
	t_cpu* cpu = malloc(sizeof(t_cpu));
	cpu->socket = cpuSocket;
	cpu->pid = pid;
	pthread_mutex_lock(&mutex_lista_cpu);
	list_add(lista_CPU, cpu);
	pthread_mutex_unlock(&mutex_lista_cpu);

}
t_cpu* buscar_Cpu_Libre()
{
	// Busca por pid, las que tienen -1 son las libres
	int _is_pcb(t_cpu *p) {
			return p->pid == -1;
		}
	pthread_mutex_lock(&mutex_lista_cpu);
	t_cpu* cpu = list_find(lista_CPU,(void*) _is_pcb);
	pthread_mutex_unlock(&mutex_lista_cpu);
	return 	cpu;
}

