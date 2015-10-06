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

	creacionHilos(logMemoria);
	pthread_join(*hiloSigUsr1, NULL);
	log_info(logMemoria, "termino hiloSigUsr1");
	pthread_join(*hiloSigUsr2, NULL);
	log_info(logMemoria, "Termino hiloSigUsr2");
	log_info(logMemoria, "Comienzo de las diferentes conexiones");
	tablaDePaginas = list_create();
	estructuraAlgoritmos = list_create();
	generarEstructuraAdministrativaPIDFrame();
	creacionTLB(&configMemoria, logMemoria, tlb);
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
		tlb[entrada].pagina = -1;
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
	log_info(logMemoria, "INICIO TABLAS DE PAGINAS");
	int pagina = 0;
	t_tablaDePaginas * entrada = malloc(sizeof(t_tablaDePaginas));
	while (pagina < cantidadDePaginas) {
		printf("PID   PAGINA   BitUso   BitModificado\n");
		entrada->bitModificado = 1;
		entrada->bitUso = 1;
		entrada->pagina = pagina;
		entrada->pid = pid;
		entrada->contenido = NULL;
		printf(" %d      %d       %d            %d\n",entrada->pid,entrada->pagina,entrada->bitUso,entrada->bitModificado);
		fflush(stdout);
		list_add(tablaDePaginas, entrada);
		pagina++;
	}
	sleep(15);
	log_info(logMemoria, "FIN TABLAS DE PAGINAS");
}
void generarCantidadDeFramesAsignadosAlProceso(int pid, int cantidadDePaginas) {

	int frame = listaDePidFrames->elements_count;

	t_pidFrame * estructuraPidFrame = malloc(sizeof(t_pidFrame*));
	while (frame < cantidadDePaginas) {

		if (configMemoria.maximoMarcosPorProceso > frame) {
			estructuraPidFrame->frameAsignado = frame;
			estructuraPidFrame->pid = pid;
			list_add(listaDePidFrames, estructuraPidFrame);
			log_info(logMemoria, "frame asignado: %d al pid:%d", frame, pid);
		}
		frame++;
	}
	free(estructuraPidFrame);
}
void generarEstructuraAdministrativaPidFrame(int pid, int paginas) {
	log_info(logMemoria, "INICIO ESTRUCTURA ADMINISTRATIVA, FRAMES-PID");
	log_info(logMemoria, "Cantidad maxima de frames por proceso: %d",
			configMemoria.maximoMarcosPorProceso);
	log_info(logMemoria, "Paginas del proceso: %d", paginas);
	if (configMemoria.maximoMarcosPorProceso < paginas) {
		generarCantidadDeFramesAsignadosAlProceso(pid, paginas);
	} else {
		generarCantidadDeFramesAsignadosAlProceso(pid,
				configMemoria.maximoMarcosPorProceso);
	}
	log_info(logMemoria, "FIN ESTRUCTURA ADMINISTRATIVA, FRAMES-PID");
}
void enviarIniciarSwap(int cliente, t_iniciarPID *estructuraCPU,
	t_mensajeHeader mensajeHeaderSwap, int servidor, t_log* logMemoria) {
	int statusFin;


	log_info(logMemoria, "Envio pagina al SWAP, PAGINA N°:%d",
			estructuraCPU->paginas);
	log_info(logMemoria, "Envio pid al SWAP, PID:%d", estructuraCPU->pid);

	int status = serializarEstructura(INICIAR, (void *) estructuraCPU,
			sizeof(t_iniciarPID), cliente);

	log_info(logMemoria, "SWAP recibio la pagina de forma correcta");
	recv(cliente, &mensajeHeaderSwap, sizeof(t_mensajeHeader), 0);
	log_info(logMemoria, "Se recibio este codigo de error: %d",
			mensajeHeaderSwap.idmensaje);
	if (mensajeHeaderSwap.idmensaje == OK) {
		log_info(logMemoria, "Se proceso correctamente el mensaje");

		statusFin = serializarEstructura(FINALIZAPROCOK,NULL,0,servidor);
	} else if (mensajeHeaderSwap.idmensaje == ERROR) {

		log_error(logMemoria,
				"Proceso %d rechazado por falta de espacio en SWAP\n",
				estructuraCPU->pid);

		statusFin = serializarEstructura(PROCFALLA,NULL,0,servidor);

	}
	fflush(stdout);
}

void generarEstructuraParaAlgoritmos(t_list* framesAsignados)
{
	t_estructuraAlgoritmoReemplazoPaginas * armadoEstruct = malloc(sizeof(t_estructuraAlgoritmoReemplazoPaginas));
	int cantidadDeFrames = framesAsignados->elements_count;
    while(cantidadDeFrames > 0)
    {
    	armadoEstruct=	framesAsignados->head->data;
    	list_add(estructuraAlgoritmos,armadoEstruct);
    	cantidadDeFrames--;

    	free(armadoEstruct);
    }
}

void procesamientoDeMensajes(int cliente, int servidor) {
	t_iniciarPID *estructuraCPU = malloc(sizeof(t_iniciarPID));
	int tamanioDatosSwap;
	char mensajeRecibido[PACKAGESIZE];
	t_finalizarPID *finalizarCPU = malloc (sizeof(t_finalizarPID) );
	t_iniciarPID estructuraSwap;
	int tamanioLeido;
	char * contenidoLeido;
	t_leer estructuraLeerSwap;
	int * memoriaReservadaDeMemPpal = malloc(sizeof(configMemoria.cantidadDeMarcos * configMemoria.tamanioMarcos));
	int statusMensajeRecibidoDeLaCPU, mensaje2, pidRecibido; //MENSAJES QUE SE USAN EN EL PASAMANOS, POR AHORA SE LLAMAN ASI, DESPUES LOS VOY A CAMBIAR.
	t_mensajeHeader mensajeHeader, mensajeHeaderSwap;
	statusMensajeRecibidoDeLaCPU = recv(servidor, &mensajeHeader,
			sizeof(t_mensajeHeader), 0);
	printf("mensaje recibido: %d", mensajeHeader.idmensaje);
	fflush(stdout);

	if (statusMensajeRecibidoDeLaCPU < 0)
		log_info(logMemoria, "Error al recibir de la CPU");
	else {

		switch (mensajeHeader.idmensaje) {
		case INICIAR:

			recv(servidor, estructuraCPU, sizeof(t_iniciarPID), 0);
			generarTablaDePaginas(memoriaReservadaDeMemPpal, estructuraCPU->pid,
					estructuraCPU->paginas);
			generarEstructuraAdministrativaPidFrame(estructuraCPU->pid,
					estructuraCPU->paginas);
			bool FramesAsignados(t_pidFrame* pidFrame)
			{
				return pidFrame->pid == estructuraCPU->pid;
			}

	//		generarEstructuraParaAlgoritmos(list_find(listaDePidFrames,(void*) FramesAsignados));
		//	recv(servidor, estructuraCPU, sizeof(t_iniciarPID), 0);
			log_info(logMemoria,
					"Proceso mProc creado, PID: %d, Cantidad de paginas asignadas: %d",
					estructuraCPU->pid, estructuraCPU->paginas);
			log_info(logMemoria,
					"Inicio del aviso al proceso SWAp del comando INICIAR");
			enviarIniciarSwap(cliente, estructuraCPU, mensajeHeaderSwap,
					servidor, logMemoria);
			log_info(logMemoria,
					"Fin del aviso al proceso SWAp del comando INICIAR");
			free(estructuraCPU);
			break;
		case LEER: {
			log_info(logMemoria, "Solicitud de lectura recibida");
			log_info(logMemoria, "2do checkpoint: Se envia directo al swap");
			mensajeHeaderSwap.idmensaje = LEER;
			send(cliente, &mensajeHeaderSwap, sizeof(t_mensajeHeader), 0);
			recv(servidor, &estructuraLeerSwap, sizeof(t_leer), 0);
			send(cliente, &estructuraLeerSwap, sizeof(t_leer), 0);
			recv(cliente, &tamanioLeido, sizeof(int), 0);
			printf("tamaño: %d \n", tamanioLeido);

			char * contenidoLeido = malloc(tamanioLeido + 1);
			contenidoLeido[tamanioLeido] = '\0';
			recv(cliente, contenidoLeido, sizeof(tamanioLeido), 0);
			printf("Contenido: %s", contenidoLeido);
			fflush(stdout);
			send(servidor, &tamanioLeido, sizeof(int), 0);

			//send(servidor, contenidoLeido, sizeof(tamanioLeido), 0);

			send(servidor, contenidoLeido, sizeof(tamanioLeido) + 1, 0);


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
			log_info(logMemoria, "FINALIZAR!");

			//recv(servidor, finalizarCPU, sizeof(t_finalizarPID), 0);
			printf("FINALIZAR PID: %d\n", finalizarCPU->pid);

			//recv(servidor, &finalizarCPU, sizeof(t_finalizarPID), 0);
			recv(servidor, finalizarCPU, sizeof(t_finalizarPID), 0);

			printf("FINALIZAR PID: %d\n", finalizarCPU->pid);

			fflush(stdout);


			status = serializarEstructura(FINALIZAR,(void *)finalizarCPU,sizeof(t_finalizarPID),cliente);
			recv(cliente, &mensajeHeaderSwap, sizeof(t_mensajeHeader), 0);
			int statusFin;
			if (mensajeHeaderSwap.idmensaje == OK)

				statusFin = serializarEstructura(mensajeHeaderSwap.idmensaje,NULL,0,servidor);
			else

				statusFin = serializarEstructura(mensajeHeaderSwap.idmensaje,NULL,0,servidor);
				//BORRAR TODAS LAS ESTRUCTURAS ADMINISTRATIVAS PARA ESE mProc.


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
