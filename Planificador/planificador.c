/*
 * planificador.c
 *
 *  Created on: 30/8/2015
 *      Author: carlavita
 */

#include "planificador.h"


int main(void) {
//	pthread_t hilo_consola;
	pthread_t hiloServer;

	/*Log*/
	remove(PATHLOG);
	logger = log_create(PATHLOG,"Planificador",true,LOG_LEVEL_INFO);
	log_info(logger,"Iniciando proceso planificador");

/*Archivo de configuración*/

	log_info(logger,"Leyendo Archivo de Configuracion");
	levantarConfiguracion();
	log_info(logger,"Archivo de Configuracion Leido correctamente");

	inicializarListas();
	/*Hilo Server Planificador*/
	//todo sincronizar hilos consola y conexion cpu
	pthread_create (&hiloServer, NULL, (void *) &servidorCPU, NULL);

	//seteado con el quantum, hay que evaluar que algoritmo elegido de planificacion


	mostrarConsola(NULL);

	planificar(configPlanificador.quantum);
/*Hilo Consola
	pthread_create (&hilo_consola, NULL, (void *) &mostrar_consola, NULL);
*/

	//pthread_join(hilo_consola,NULL);

	pthread_join(hiloServer,NULL);
	return EXIT_SUCCESS;
}

void inicializarListas(){

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
		  configPlanificador.puertoEscucha = malloc(PUERTO_SIZE);
		  configPlanificador.puertoEscucha = config_get_string_value(CONFIG,"PUERTO_ESCUCHA");

		  configPlanificador.algoritmo = config_get_int_value(CONFIG,"ALGORITMO");
		  configPlanificador.quantum = config_get_int_value(CONFIG,"QUANTUM");

	  printf(" puerto %s \n",configPlanificador.puertoEscucha );
	  printf(" algoritmo %d\n",configPlanificador.algoritmo );
	  printf(" quantum %d\n",configPlanificador.quantum );
	  }else{
		  printf("estupido config, se cree tan importante");
	  }

}

void mostrarConsola( void *ptr ){
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
			correrPath();
			printf("soy el planificador, recibí el mensaje correr path por consola! Envi a CPU\n");
						int mensaje = CHECKPOINT;
						printf("socket : %d \n", Servidor);
						int status = send(Servidor,&mensaje,sizeof(int),0);
						printf("estado de envio : %d \n", status);
			esperaEnter();
			break;

		case 2:
			//todo:descomentar			system("clear");
//todo			finalizar_pid();
			esperaEnter();
			break;
		case 3:
			//todo:descomentar			system("clear");
//todo			ps();
			esperaEnter();
			break;
		case 4:
			//todo:descomentar			system("clear");
//todo			cpu();
			esperaEnter();
			break;


		case 0:
			log_info(logger, "* Apaga Consola * \n");
			puts("Gracias por venir, vuelva pronto! :)");
			esperaEnter();
		break;

		default:
		 printf("Comando inválido\n");
		 esperaEnter();
		 break;
		}

	}

}

void esperaEnter ()
       	{
       			printf("Presione ENTER para continuar \n");
       			char enter='\0';
       			enter = getchar();
       			while(((enter = getchar()) != '\n'));
       	}


void servidorCPU( void *ptr ){

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

	     		     if (getaddrinfo(NULL,configPlanificador.puertoEscucha, &hints, &res) != 0) {
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

	     		                    	Servidor = newsock;
	     		                    	int mensaje = SALUDO;
	     		                    	status = send(newsock,&mensaje , sizeof(int), 0);
	     		                    	printf("status send inicial %d \n", status);
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


		printf("En el handle de planificador \n");
		//recepcion de msj del cpu segun protocolo
		int status = 1;

		t_mensajeHeader mje_header;
		status = recv(newsock, &mje_header, sizeof(t_mensajeHeader), 0);
		int codigoMsj = mje_header.codMje;

		if (status){
				switch ( codigoMsj ) {

					case FINALIZAPROCOK:

						printf("el proceso finalizo correctamente su rafaga\n");
						//actualizarPcb();

						break;
					case PROCFALLA:

						printf("el proceso falló en su ejecucion\n");
						//borrarEstructurasDelProc();

						break;
					case PROCIO:

						printf("el proceso esta realizando su entrada-salida\n");
						ejecutarIO(newsock);
						break;
					default:
						printf("codigo no reconocido\n");
						break;

				}
		}

		if(!status){
				// conexión cerrada
				printf("la conexion del socket: %d termino \n", newsock);
				log_info(logger,"Cierro conexion con CPU");
				close(newsock); // ¡Hasta luego!
				FD_CLR(newsock, set); // eliminar del conjunto maestro
			}


}

void correrPath(){
	char path[PATH_SIZE];
	printf("Ingrese el PATH del mCod a correr \n");
	  scanf("%s", path);
	printf("PATH del mCod a correr %s \n",path);

	int pid = crearPcb(path);

	printf("Creado mProc %d", pid);
}

int crearPcb(char* path){

	t_pcb* pcb  = malloc(sizeof(t_pcb)) ;
	    pthread_mutex_lock(&mutex_listas);
	    PID++;
		pcb->pid = PID;
		pcb->proxInst = 0; //Inicializa en 0 es la primer instruccion
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


t_pcb* planificarFifo(){
	pthread_mutex_lock(&mutex_listas);
	t_pcb* pcb = list_remove(LISTOS,0);
	list_add(EJECUTANDO, pcb);
	pthread_mutex_unlock(&mutex_listas);
	return pcb;
}

void enviarACpu(t_pcb* pcb,t_cpu* cpu)
{
/*// Elimina el PID de la cola de listos
	removerEnListaPorPid(LISTOS,pcb->pid);
//Pasa  a lista de ejecución
	pthread_mutex_lock(&mutex_listas);
	list_add(EJECUTANDO,pcb);
	pthread_mutex_unlock(&mutex_listas);*/


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

int planificar(int quantum){

	//el otro hilo se usa como planificador(consumidor) que elije de la cola de listos y lo pasa la lista de ejecutando con el quantum

	pthread_t hiloPlanificador;

	pthread_create(&hiloPlanificador, NULL,(void *) &planificador, NULL);

	//t_pcb* pcb = list_get(LISTOS,0);

	//enviarACpu(t_pcb* pcb,t_cpu* cpu);

	pthread_join(hiloPlanificador,NULL);

	return 0;
}


void *planificador(void *info_proc){
	t_pcb* pcb ;
	printf("en el hilo planificador \n");

	while (true) {

// el planificar lo saca de la cola de listos y lo pasa a ejecutando
				pcb = planificarFifo();


			//enviar a cpu elegida el pcb del proceso elegido
			t_cpu* cpuLibre;
			cpuLibre = buscarCpuLibre();
			printf("el id de la cpu libre es: %d /n", cpuLibre->id);

			//enviarACpu(cpuLibre,pcb.pid);

		}

	return 0;

}

void ejecutarIO(int socketCPU){

	//recepciono de la pcu el msj con el Tiempo(en segundos) que hay que hacer el IO
	t_rtaIO mjeIO;
	recv(socketCPU, &mjeIO, sizeof(t_rtaIO), 0);

	//busco el proc por si pid, se borra de ejecutados y lo mando a bloqueados
	removerEnListaPorPid(EJECUTANDO,mjeIO.pid);

	//por T segundos
	sleep(mjeIO.tiempo);

	//Ahora enviarlo a la cola de listos



}

void eliminarCPU(int socket_cpu)
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
void agregarCPU(int cpuSocket, int pid) {
	t_cpu* cpu = malloc(sizeof(t_cpu));
	cpu->socket = cpuSocket;
	cpu->pid = pid;
	pthread_mutex_lock(&mutex_lista_cpu);
	list_add(lista_CPU, cpu);
	pthread_mutex_unlock(&mutex_lista_cpu);

}

t_cpu* buscarCpuLibre()
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

