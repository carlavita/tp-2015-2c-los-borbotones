/*
 * administradorDeMemoria.c
 *
 *  Created on: 30/8/2015
 *      Author: Fernando Rabinovich
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <commons/config.h>
#include <commons/collections/list.h>
#include <pthread.h>
#include <string.h>
#include <stdbool.h>
#include <commons/log.h>
#include <semaphore.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <protocolo.h>
#include "administradorDeMemoria.h"

t_config_memoria configMemoria;
t_log * logMemoria;
int clienteSwap;
t_tablaDePaginas * tablaDePaginas;
t_pidFrame * tablaAdministrativaProcesoFrame; //INICIALIZAR EN EL MAIN()  !!!!!!
int * memoriaReservadaDeMemPpal;
pthread_t * hiloSigUsr1;
pthread_t * hiloSigUsr2;
t_list * listaDePidFrames;
t_TLB * tlb;
char * recibidoPorLaMemoria;
char mensaje[1024];

void * inicioHiloSigUsr1() {
	return NULL;
}
void * inicioHiloSigUsr2() {
	return NULL;
}

int main() {
	remove("logMemoria.txt"); //Cada vez que arranca el proceso borro el archivo de log.
	logMemoria = log_create("logMemoria.txt", "Administrador de memoria", true,
			LOG_LEVEL_INFO);
	leerConfiguracion();
	generarEstructuraAdministrativaPIDFrame();
	creacionTLB(&configMemoria, logMemoria, tlb);

	creacionHilos(logMemoria);
	pthread_join(*hiloSigUsr1, NULL);
	log_info(logMemoria, "termino hiloSigUsr1");
	pthread_join(*hiloSigUsr2, NULL);
	log_info(logMemoria, "Termino hiloSigUsr2");
	log_info(logMemoria, "Comienzo de las diferentes conexiones");
	clienteSwap = ConexionMemoriaSwap(&configMemoria, logMemoria);
	int servidorCPU = servidorMultiplexor(configMemoria.puertoEscucha);

	for (;;)
		procesamientoDeMensajes(clienteSwap, servidorCPU);

}

void leerConfiguracion() {
	log_info(logMemoria, "Inicio lectura de archivo de configuración");
	t_config * configuracionMemoria = config_create("memoria.conf");
	configMemoria.puertoEscucha = config_get_int_value(configuracionMemoria,
			"PUERTO_ESCUCHA");
	configMemoria.ipSwap = config_get_string_value(configuracionMemoria,
			"IP_SWAP");
	configMemoria.puertoSwap = config_get_string_value(configuracionMemoria,
			"PUERTO_SWAP");
	configMemoria.maximoMarcosPorProceso = config_get_int_value(
			configuracionMemoria, "MAXIMO_MARCOS_POR_PROCESO");
	configMemoria.cantidadDeMarcos = config_get_int_value(configuracionMemoria,
			"CANTIDAD_MARCOS");
	configMemoria.tamanioMarcos = config_get_int_value(configuracionMemoria,
			"TAMANIO_MARCOS");
	configMemoria.entradasTLB = config_get_int_value(configuracionMemoria,
			"ENTRADAS_TLB");
	configMemoria.tlbHabilitada = config_get_int_value(configuracionMemoria,
			"TLB_HABILITADA");
	configMemoria.retardoMemoria = config_get_int_value(configuracionMemoria,
			"RETARDO_MEMORIA");
	free(configuracionMemoria);
	log_info(logMemoria, "%d", configMemoria.puertoEscucha);
	log_info(logMemoria, "Finalizo lectura de archivo de configuración");
}
void crearServidores() {
	servidorMultiplexor(configMemoria.puertoEscucha);
}
t_TLB * generarTLB(int entradasTLB) {
	t_TLB tlb[entradasTLB];
	int entrada = 0;
	while (entrada < entradasTLB) {
		tlb[entrada].frame = -1;
		tlb[entrada].numeroPagina = -1;
		tlb[entrada].pid = -1; //cuando vengan los procesos, ire cambiando ese pid.
		++entrada;
	}
	return tlb;
}
void creacionTLB(const t_config_memoria* configMemoria, t_log* logMemoria,
		t_TLB* tlb) {

	if (configMemoria->tlbHabilitada == 1) {
		printf("La TLB esta habilitada, se procede a su creación");
		log_info(logMemoria, "Inicio creacion de la TLB");
		tlb = malloc(sizeof(t_TLB*));
		tlb = generarTLB(configMemoria->entradasTLB);
		log_info(logMemoria, "Finalizo existosamente la creacion de la TLB");
	} else {
		printf("La TLB no esta habilitada");
		log_warning(logMemoria, "La TLB no esta activada por configuración");
	}
}
int ConexionMemoriaSwap(t_config_memoria* configMemoria, t_log* logMemoria) {
	int intentosFallidosDeConexionASwap = 0;
	log_trace(logMemoria,
			"Generando cliente al SWAP con estos parametros definidos, IP: %s, PUERTO: %d",
			configMemoria->ipSwap, configMemoria->puertoSwap);
	cliente(configMemoria->ipSwap, configMemoria->puertoSwap);
	int clienteSwap = clienteSeleccion();
	while (clienteSwap == -1) {
		log_error(logMemoria, "No se pudo conectar al SWAP, reintando");
		if (intentosFallidosDeConexionASwap == 5) {
			printf(
					"Luego de %d intentos de conexion fallidos,se espera 10seg,para reintarConexion",
					intentosFallidosDeConexionASwap);
			log_info(logMemoria,
					"Luego de %d intentos de conexion fallidos,se espera 10seg para reintarConexion",
					intentosFallidosDeConexionASwap);
			sleep(10);
		}

		cliente(configMemoria->ipSwap, configMemoria->puertoSwap);
		clienteSwap = clienteSeleccion();
		intentosFallidosDeConexionASwap++;
	}
	if (clienteSwap > 0) {
		log_info(logMemoria,
				"La memoria se conecto satisfactoriamente al SWAP");
	}
	return clienteSwap;
}
void generarTablaDePaginas(int * memoriaReservadaDeMemPpal, int pid,
		int cantidadDePaginas) {
	int pagina = 0;

	int frame = 0;
	int maximaCantidadDeFrames = configMemoria.cantidadDeMarcos;
	while (pagina < cantidadDePaginas) {
		if (frame == maximaCantidadDeFrames)
			return;
		else {
			tablaDePaginas->bitModificado = 1;
			tablaDePaginas->bitUso = 1;
			tablaDePaginas->marco = frame; //ESTO NO ME GUSTA, TODO
			tablaDePaginas->pagina = pagina;
			tablaDePaginas->pid = -1; //-1 me indica que la pagina no esta asignada a ningun proceso
		}
		frame++;
		pagina++;
	}
}
void generarCantidadDeFramesAsignadosAlProceso(int pid, int cantidadDePaginas) {
	int frame = listaDePidFrames->elements_count - 1; //EMPIEZA EN 0
	t_pidFrame * estrucPidFrame = malloc(sizeof(t_pidFrame*));
	while (frame < cantidadDePaginas) {
		estrucPidFrame = list_find(listaDePidFrames,
				(void*) (listaDePidFrames->head->data == &frame));
		estrucPidFrame->pid = pid;
		list_add(listaDePidFrames, estrucPidFrame);
		frame++;
	}
}
void avisarAlSwap(int clienteSwap) {
	t_mensajeHeader mensajeHeader;
	mensajeHeader.idmensaje = 10;
	send(clienteSwap, &mensajeHeader, sizeof(t_mensajeHeader), 0);
}
void generarEstructurasAdministrativas(int pid, int paginas) {

	if (configMemoria.maximoMarcosPorProceso < paginas) {
		generarCantidadDeFramesAsignadosAlProceso(pid, paginas);
	} else {
		generarCantidadDeFramesAsignadosAlProceso(pid,
				configMemoria.maximoMarcosPorProceso);
	}

}

void envioFinalizarSwap(t_mensajeHeader mensajeHeaderSwap, int cliente, int pid,
		int pagina) {
	mensajeHeaderSwap.idmensaje = FINALIZAR;
	send(cliente, &mensajeHeaderSwap, sizeof(t_mensajeHeader), 0);
	//recvACK(cliente);
	send(cliente, &pid, sizeof(int), 0);
	//recvACK(cliente);
	send(cliente, &pagina, sizeof(int), 0);
	//recvACK(cliente);
	recv(cliente, &mensajeHeaderSwap, sizeof(t_mensajeHeader), 0);
}

void enviarIniciarSwap(int cliente, t_iniciarPID estructuraCPU,
		t_mensajeHeader mensajeHeaderSwap, int servidor, t_log* logMemoria) {
	t_mensajeHeader mensajeCPU;
	avisarAlSwap(cliente);
	//recvACK(cliente);

	log_info(logMemoria, "Envio pagina al SWAP, PAGINA N°:%d",
			estructuraCPU.paginas);
	log_info(logMemoria, "Envio pid al SWAP, PID:%d", estructuraCPU.pid);
	send(cliente, &estructuraCPU, sizeof(t_iniciarPID), 0);
	//recvACK(cliente);
	log_info(logMemoria, "SWAP recibio la pagina de forma correcta");
	recv(cliente, &mensajeHeaderSwap, sizeof(t_mensajeHeader), 0);
	log_info(logMemoria, "Se recibio este codigo de error: %d",
			mensajeHeaderSwap.idmensaje);
	if (mensajeHeaderSwap.idmensaje == OK) {
		log_info(logMemoria, "Se proceso correctamente el mensaje");
		mensajeCPU.idmensaje = FINALIZAPROCOK;
		send(servidor, &mensajeCPU, sizeof(t_mensajeHeader), 0);
	} else if (mensajeHeaderSwap.idmensaje == ERROR) {
		log_error(logMemoria, "Fallo envio mensaje");
		mensajeCPU.idmensaje = PROCFALLA;
		send(servidor,&mensajeCPU,sizeof(t_mensajeHeader),0);
	}
	fflush(stdout);
}

void procesamientoDeMensajes(int cliente, int servidor) {
	t_iniciarPID estructuraCPU;
	int tamanioDatosSwap;
	char mensajeRecibido[PACKAGESIZE];
	t_finalizarPID finalizarCPU;
	t_iniciarPID estructuraSwap;
	int tamanioLeido;
	char * contenidoLeido;
	t_leer estructuraLeerSwap;
	int * memoriaReservadaDeMemPpal =
			malloc(
					sizeof(configMemoria.cantidadDeMarcos
							* configMemoria.tamanioMarcos));
	int statusMensajeRecibidoDeLaCPU, mensaje2, pidRecibido; //MENSAJES QUE SE USAN EN EL PASAMANOS, POR AHORA SE LLAMAN ASI, DESPUES LOS VOY A CAMBIAR.
	t_mensajeHeader mensajeHeader, mensajeHeaderSwap;
	statusMensajeRecibidoDeLaCPU = recv(servidor, &mensajeHeader,
			sizeof(t_mensajeHeader), 0);
	printf("mensaje recibido: %d", mensajeHeader.idmensaje);
	fflush(stdout);
	//  log_error(logMemoria,"Mensaje recibido: %d",statusMensajeRecibidoDeLaCPU);

	if (statusMensajeRecibidoDeLaCPU < 0)
		log_info(logMemoria, "Error al recibir de la CPU");
	else {
		// char * m = malloc(sizeof(char *));
		//strncpy(m,mensajeRecibido,sizeof(mensajeRecibido));
		//switch ((int)m)
		switch (mensajeHeader.idmensaje) {
		case INICIAR:
			//sendACK(servidor);
			recv(servidor, &estructuraCPU, sizeof(t_iniciarPID), 0);
			//generarTablaDePaginas(memoriaReservadaDeMemPpal,pid,CantidadDePaginas);
			//generarEstructurasAdministrativas(*pid,*pagina);

			log_info(logMemoria,
					"Proceso mProc creado, PID: %d, Cantidad de paginas asignadas: %d",
					estructuraCPU.pid, estructuraCPU.paginas);
			log_info(logMemoria,
					"Inicio del aviso al proceso SWAp del comando INICIAR");
			enviarIniciarSwap(cliente, estructuraCPU, mensajeHeaderSwap,
					servidor, logMemoria);
			log_info(logMemoria,
					"Fin del aviso al proceso SWAp del comando INICIAR");
			break;
		case LEER: {
			log_info(logMemoria, "Solicitud de lectura recibida");
			log_info(logMemoria, "2do checkpoint: Se envia directo al swap");
			mensajeHeaderSwap.idmensaje = LEER;
			send(cliente, &mensajeHeaderSwap, sizeof(t_mensajeHeader), 0);
			recv(servidor,&estructuraLeerSwap,sizeof(t_leer),0);
			send(cliente,&estructuraLeerSwap,sizeof(t_leer),0);
			recv(cliente,&tamanioLeido,sizeof(int),0);
			printf("tamaño: %d \n",tamanioLeido);

			char * contenidoLeido = malloc(tamanioLeido);
			contenidoLeido[tamanioLeido] = '\0';
			recv(cliente,contenidoLeido,sizeof(tamanioLeido),0);
			printf("Contenido: %s",contenidoLeido);
			fflush(stdout);
			send(servidor,&tamanioLeido,sizeof(int),0);
			send(servidor,contenidoLeido,sizeof(tamanioLeido),0);

			log_info(logMemoria, "Finalizo comando LEER");
			free(contenidoLeido);
		}
			break;
		case ESCRIBIR:
			log_info(logMemoria, "Solicitud de escritura recbidia");
			log_info(logMemoria, "2do chekcpoint NO APLICA");
			send(cliente, 1/*&pid*/, sizeof(int), 0);
			send(cliente, 4/*pagina*/, sizeof(int), 0);
			int sizeContenido = 10;		//deberia ser lo que recibo de la CPU
			recv(cliente, &sizeContenido, sizeof(int), 0);
			recv(cliente, "HOLA"/*&contenidoEscribir*/, sizeof("HOLA"), 0);
			int status;
			recv(cliente, &status, sizeof(int), 0);	//RECIBO DEL SWAP COMO TERMINO LA OPERACION
			send(servidor, &status, sizeof(int), 0);//MANDO A LA CPU  COMO TERMINO LA OPERACION
			break;
		case FINALIZAR:
			log_info(logMemoria,"FINALIZAR!");
			recv(servidor,&finalizarCPU,sizeof(t_finalizarPID),0);
			printf("FINALIZAR PID: %d\n",finalizarCPU.pid);
			fflush(stdout);
			mensajeHeaderSwap.idmensaje = FINALIZAR;

			send(cliente,&mensajeHeaderSwap,sizeof(t_mensajeHeader),0);
			send(cliente,&finalizarCPU,sizeof(t_finalizarPID),0);

			recv(cliente,&mensajeHeaderSwap,sizeof(t_mensajeHeader),0);
		    if(mensajeHeaderSwap.idmensaje == OK)
		    	send(servidor,&mensajeHeaderSwap,sizeof(t_mensajeHeader),0);
		    else
		    	send(servidor,&mensajeHeaderSwap,sizeof(t_mensajeHeader),0);
			//BORRAR TODAS LAS ESTRUCTURAS ADMINISTRATIVAS PARA ESE mProc.
			//	envioFinalizarSwap(mensajeHeaderSwap, cliente, pid, CantidadDePaginas);
			break;
		default:
			log_error(logMemoria, "mensaje N°: %d",
					mensajeHeaderSwap.idmensaje);
			log_info(logMemoria, "Mensaje que no es del SWAP N°:%d",
					mensajeHeader.idmensaje);
			log_info(logMemoria, "Mensaje incorrecto");
		}
	}
}

void creacionHilos(t_log* logMemoria) {
	/*
	 * 	CREACION HILOS SINCRONIZADOS PARA ADMINISTRAR LAS SEÑALES sigusr1 y sigusr2
	 */
	log_info(logMemoria,
			"Inicio creación hilos para atender señales SIGUSR1 y SIGUSR2");
	hiloSigUsr1 = malloc(sizeof(pthread_t*));
	hiloSigUsr2 = malloc(sizeof(pthread_t*));
	pthread_create(hiloSigUsr1, NULL, inicioHiloSigUsr1, NULL);
	pthread_create(hiloSigUsr2, NULL, inicioHiloSigUsr2, NULL);
	free(hiloSigUsr1);
	free(hiloSigUsr2);
	log_info(logMemoria,
			"Fin creación hilos para atender señales SIGUSR1 y SIGUSR2");
}
void generarEstructuraAdministrativaPIDFrame() {
	log_info(logMemoria,
			"Inicio creacion estructura administrativa frame asignado a proceso");
	listaDePidFrames = malloc(sizeof(t_list *));/*ESTA CREACION VA A ESTAR EN EL MAIN*/
	listaDePidFrames = list_create();
	t_pidFrame *estructuraPidFrame = malloc(sizeof(t_pidFrame*));
	int frame = 0;
	while (frame < configMemoria.cantidadDeMarcos) {
		log_trace(logMemoria, "N° frame:%d", frame);
		estructuraPidFrame->frameAsignado = frame;
		estructuraPidFrame->pid = -1;		// F RAME/MARCO LIBRE
		list_add(listaDePidFrames, estructuraPidFrame);
		frame++;
	}
	log_debug(logMemoria,
			"Fin creacion estructura administrativa frame asignado a proceso");
}
