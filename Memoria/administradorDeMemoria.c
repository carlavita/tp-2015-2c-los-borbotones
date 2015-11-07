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
#include <signal.h>

pthread_mutex_t BLOQUEAR;
pthread_mutex_t BORRAR;
/*SEMAFOROS*/
pthread_mutex_t mutexFrameSinUsar;
pthread_mutex_t mutexIniciar;
pthread_mutex_t mutexLeer;
pthread_mutex_t mutexEscribir;
pthread_mutex_t mutexFinalizar;
pthread_mutex_t mutexFrame;
pthread_mutex_t mutexSenhal;
pthread_mutex_t mutexTLB;
/********************/

int ultimoFrameAsignado;
int senhal;

void * inicioHiloSigUsr1() {
	pthread_mutex_lock(&mutexTLB);
	list_destroy(tlb);
	list_create(tlb);
	pthread_mutex_unlock(&mutexTLB);
	return NULL;
}
void * inicioHiloSigUsr2() {
	//LIMPIAR TODAS LAS TABLAS DE PAGINAS(cada tabla de paginas de cada proceso)
	//pasarlas al LOG
	return NULL;
}

void atenderSeaniales(int senhal) {

	pid_t pidProcesoClonado;
	switch (senhal) {
	case SIGUSR1:
		pthread_create(hiloSigUsr1, NULL, inicioHiloSigUsr1, NULL);
		pthread_join(*hiloSigUsr1, NULL);
		break;
	case SIGUSR2:
		pthread_create(hiloSigUsr2, NULL, inicioHiloSigUsr2, NULL);
		pthread_join(*hiloSigUsr2, NULL);
		raise(SIGALRM);
		break;
	case SIGPOLL:
		pidProcesoClonado = fork();
		if (pidProcesoClonado == 0) //PROCESO HIJO
			RealizarVolcadoMemoriaLog();
		else
			return;
		break;
	}
}

void crearListas() {
	tablaDePaginas = list_create();
	estructuraAlgoritmos = list_create();
	tlb = list_create();
	listaDePidFrames = list_create();
	estructurasPorProceso = list_create();
}

int main() {
	/*SEÑALES*/
	signal(SIGUSR1, atenderSeaniales);
	signal(SIGUSR2, atenderSeaniales);
	signal(SIGPOLL, atenderSeaniales);
	/*SEÑALES*/
	remove("logMemoria.txt"); //Cada vez que arranca el proceso borro el archivo de log.A
	logMemoria = log_create("logMemoria.txt", "Administrador de memoria", true,
			LOG_LEVEL_INFO);
	leerConfiguracion();
	ultimoFrameAsignado = 0; //TODO

	log_info(logMemoria, "Comienzo de las diferentes conexiones");
	crearListas();
	creacionTLB(&configMemoria, logMemoria);
	clienteSwap = ConexionMemoriaSwap(&configMemoria, logMemoria);

	int servidorCPU = servidorMultiplexorCPU(configMemoria.puertoEscucha);

	exit(0);
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
void generarTLB(int entradasTLB) {

	int entrada = 0;
	while (entrada < entradasTLB) {
		t_TLB * estructTLB = malloc(sizeof(t_TLB));
		estructTLB->pagina = -1;
		estructTLB->pid = -1;
		//cuando vengan los procesos, ire cambiando ese pid.
		++entrada;
		list_add(tlb, estructTLB);
		free(estructTLB);
	}
}
void creacionTLB(const t_config_memoria* configMemoria, t_log* logMemoria) {
	if (configMemoria->tlbHabilitada == 1) {
		log_info(logMemoria,
				"La TLB esta habilitada, se procede a su creación");
		log_info(logMemoria, "Inicio creacion de la TLB");
		generarTLB(configMemoria->entradasTLB);
		log_info(logMemoria, "Finalizo existosamente la creacion de la TLB");
	} else {
		log_info(logMemoria, "La TLB no esta habilitada");
		log_info(logMemoria, "La TLB no esta activada por configuración");
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
		log_info(logMemoria, "No se pudo conectar al SWAP, reintando");
		if (intentosFallidosDeConexionASwap == 5) {
			log_info(logMemoria,
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
	t_estructurasDelProceso estructuraDelProceso;
	log_info(logMemoria, "INICIO TABLAS DE PAGINAS");
	int pagina = 0;

	while (pagina < cantidadDePaginas) {
		t_tablaDePaginas * entrada = malloc(sizeof(t_tablaDePaginas));
		entrada->bitModificado = 1;
		entrada->bitUso = 0;
		entrada->bitValidez = 1;
		entrada->presencia = 0;
		entrada->pagina = pagina;
		entrada->pid = pid;
		entrada->contenido = NULL;
		entrada->marco = -1;
		list_add(tablaDePaginas, entrada);
		pagina++;
	}
	estructuraDelProceso.pid = pid;
	estructuraDelProceso.estructura = tablaDePaginas;
	list_add(estructurasPorProceso, &estructuraDelProceso);
	log_info(logMemoria, "FIN TABLAS DE PAGINAS");
}

void agregarAEstructuraGeneral(void * estructurasDelProceso, int pid) {
	log_info(logMemoria, "AGREGANDO ESTRUCTURAS AL PID: %d", pid);
	t_estructurasDelProceso * ep = malloc(sizeof(t_estructurasDelProceso));
	ep->pid = pid;
	ep->estructura = estructurasDelProceso;
	list_add(estructurasPorProceso, ep);
}

void AsignarFrameAlProceso(int pid, int cantidadDePaginas) {
	t_pidFrame * estructuraPidFrame = malloc(sizeof(t_pidFrame));

	estructuraPidFrame->frameAsignado = ultimoFrameAsignado;
	estructuraPidFrame->pid = pid;
	estructuraPidFrame->frameUsado = 0; //0 SIN USAR, 1 USADO.

	list_add(listaDePidFrames, estructuraPidFrame);

	log_info(logMemoria, "frame asignado: %d al pid:%d", ultimoFrameAsignado,
			pid);

	if (list_size(listaDePidFrames) <= configMemoria.maximoMarcosPorProceso) {
		agregarAEstructuraGeneral(listaDePidFrames, pid);
	}

	//pthread_mutex_unlock(&mutexFrame);
}

void generarEstructuraAdministrativaPidFrame(int pid, int paginas) {
	log_info(logMemoria, "INICIO ESTRUCTURA ADMINISTRATIVA, FRAMES-PID");
	log_info(logMemoria, "Cantidad maxima de frames por proceso: %d",
			configMemoria.maximoMarcosPorProceso);
	log_info(logMemoria, "Paginas del proceso: %d", paginas);
	AsignarFrameAlProceso(pid, paginas);

	log_info(logMemoria, "FIN ESTRUCTURA ADMINISTRATIVA, FRAMES-PID");
}
void enviarIniciarSwap(int cliente, t_iniciarPID *estructuraCPU,
		t_mensajeHeader mensajeHeaderSwap, int servidor, t_log* logMemoria) {

	log_info(logMemoria, "Envio pagina al SWAP, PAGINA N°:%d",
			estructuraCPU->paginas);
	log_info(logMemoria, "Envio pid al SWAP, PID:%d", estructuraCPU->pid);

	serializarEstructura(INICIAR, (void *) estructuraCPU, sizeof(t_iniciarPID),
			cliente);

	log_info(logMemoria, "SWAP recibio la pagina de forma correcta");
	recv(cliente, &mensajeHeaderSwap, sizeof(t_mensajeHeader), 0);
	log_info(logMemoria, "Se recibio este codigo de error: %d",
			mensajeHeaderSwap.idmensaje);
	if (mensajeHeaderSwap.idmensaje == OK) {
		log_info(logMemoria, "Se proceso correctamente el mensaje");

		serializarEstructura(FINALIZAPROCOK, NULL, 0, servidor);
	} else if (mensajeHeaderSwap.idmensaje == ERROR) {

		log_info(logMemoria,
				"Proceso %d rechazado por falta de espacio en SWAP\n",
				estructuraCPU->pid);

		serializarEstructura(PROCFALLA, NULL, 0, servidor);

	}
}

int buscarEnLaTLB( pid, pagina) {
	log_info(logMemoria, "INICIO BUSQUEDA DE PAGINA EN TLB");
	bool buscarPagina(t_TLB * buscarTLB) {
		return buscarTLB->pid == pid && buscarTLB->pagina == pagina;
	}
	int cantidad = list_count_satisfying(tlb, (void*) buscarPagina);
	if (cantidad > 0) {
		log_info(logMemoria, "PAGINA ENCONTRADA EN LA TLB");
		return 1;
	} else {
		log_info(logMemoria, "PAGINA NO ENCONTRADA EN LA TLB");
		return -1;
	}
}
/*int busquedaPIDEnLista(int PID) {
 int posicion = 0;
 t_tablaProcesos* proceso;
 proceso = list_get(listaProcesos, posicion);
 while (proceso->pid != PID) {
 posicion++;
 proceso = list_get(listaProcesos, posicion);
 }
 return posicion;
 }*/
int busquedaPIDEnLista(int PID, int pagina) {
	int posicion = 0;
	t_tablaDePaginas* pag = list_get(tablaDePaginas, posicion);
	/*if (pag != NULL) {
	 posicion = pagina;
	 return pagina;
	 }*/
	while (pag->pagina != pagina || pag->pid != PID) {

		posicion++;
		pag = list_get(tablaDePaginas, posicion);
	}
	if (pag->pagina == pagina && pag->pid == PID) {
		// todo	&& pag->bitValidez == 1){
		//	&& pag->presencia == 1){
		log_info(logMemoria, "POSICION ENCONTRADA : %d \n", posicion);
		//fflush(stdout);

		return posicion;
	} else {
		log_info(logMemoria, "Pagina no encontrada \n");

		return -1;
	}

}

int busquedaFRAMESinUsar(int PID) {
	int posicion = -1;
	t_pidFrame* pframe = malloc(sizeof(t_pidFrame));
	pframe = list_get(listaDePidFrames, posicion);
	while (pframe->frameUsado == 0) {
		if (pframe->pid == PID) {
			if (posicion == list_size(listaDePidFrames))
				return -1;
			posicion++;
			pframe = list_get(listaDePidFrames, posicion);
		}
	}
	free(pframe);

	return posicion;
}

int buscarEnTablaDePaginas( pid, pagina) {
	t_tablaDePaginas * entrada = malloc(sizeof(t_tablaDePaginas));

	log_info(logMemoria, "INICIO BUSQUEDA EN TABLA DE PAGINAS");

	t_list * paginasEncontradas = list_create();
	int posicion = busquedaPIDEnLista(pid, pagina);
	if (posicion >= 0) { //encontro la pagina, es valida y esta presente
		entrada = list_get(tablaDePaginas, posicion);
		list_add(paginasEncontradas, entrada);
	}
	if (list_size(paginasEncontradas) >= 1) {

		log_info(logMemoria, "PAGINA ENCONTRADA EN TABLA DE PAGINAS");
		if (entrada->bitValidez == 1 && entrada->presencia == 1) {
			return entrada->marco;
		} else
			return -1; // si la pagina para el pid NO es unica retorno falso

	} else
		return -1;

}

char * buscarContenidoEnTablaDePaginas(int pid, int pagina) {
	log_info(logMemoria,
			"OBTENIENDO CONTENIDO DE LA PAGINA,fue encontrada en la TLB o en la TABLA DE PAGINAS");
	char * contenido;
	t_tablaDePaginas * estructContenidoPaginas = malloc(
			sizeof(t_tablaDePaginas));
	estructContenidoPaginas = list_get(tablaDePaginas,
			busquedaPIDEnLista(pid, pagina));
	contenido = estructContenidoPaginas->contenido;
	free(estructContenidoPaginas);
	log_info(logMemoria, "CONTENIDO ENCONTRADO: %s", contenido);
	return contenido;
}

void buscarContenidoPagina(int socketSwap,int pid, int pagina, int socketCPU) {
//ANTES DE USAR ESTA FUNCION SE VALIDA QUE EXISTA EN LA TLB, o en la TABLA DE PAGINAS, y despues se busca el contenido en la TABLA de paginas
//porque si esta en la TLB, o en la tabla de paginas, TENGO EL CONTENIDO EN EL ADMINISTRADOR DE MEMORIA, sino debo pedirselo al swap

	char* contenidoADevolver = buscarContenidoEnTablaDePaginas(pid, pagina);
	int tamanio = strlen(contenidoADevolver);
	if (strlen(contenidoADevolver) == 0){
		contenidoADevolver = pedirContenidoAlSwap(socketSwap, pid, pagina,
				socketCPU);
	}
	else {
		send(socketCPU, &tamanio, sizeof(tamanio), 0);
		send(socketCPU, contenidoADevolver, tamanio, 0);
	}
	sleep(5);
}

char * pedirContenidoAlSwap(int cliente, int pid, int pagina, int servidor) {
	//log_info(logMemoria, "INICIO PEDIDO AL SWAP");
	int tamanioLeido;
	t_leer * estructuraLeerSwap = malloc(sizeof(t_leer));
	estructuraLeerSwap->pid = pid;
	estructuraLeerSwap->pagina = pagina;
	serializarEstructura(LEER, estructuraLeerSwap, sizeof(t_leer), cliente);
	recv(cliente, &tamanioLeido, sizeof(int), 0);

	char* contenidoLeido = malloc(tamanioLeido);

	recv(cliente, contenidoLeido, tamanioLeido, 0);

	log_info(logMemoria, "Contenido: %s", contenidoLeido);
	//log_info(logMemoria, "FIN PEDIDO AL SWAP");
	contenidoLeido[tamanioLeido] = '\0';
	//TODO agregar contenido

	send(servidor, &tamanioLeido, sizeof(tamanioLeido), 0);
	send(servidor, contenidoLeido, tamanioLeido, 0);

	free(estructuraLeerSwap);
	free(contenidoLeido);
	return contenidoLeido;
}

void ActualizarFrame(t_tablaDePaginas* paginaAAsignar, int pid) {
	int posicion = busquedaFRAMESinUsar(pid);
	t_pidFrame * pidAAsignar = malloc(sizeof(t_pidFrame));
	pidAAsignar = list_get(listaDePidFrames, posicion);
	paginaAAsignar->marco = pidAAsignar->frameAsignado;

	pidAAsignar->frameUsado = 1;
	list_replace(listaDePidFrames, posicion, pidAAsignar);
}

void AsignarContenidoALaPagina(int pid, int pagina,
		char * contenidoPedidoAlSwap) {
	bool framesPorPid() {
		int i = 0;
		int cantidadF = 0;
		while (i < ultimoFrameAsignado) {
			t_pidFrame * pf = list_get(listaDePidFrames, i);
			if (pf->pid == pid)
				cantidadF++;
			i++;
		}
		return cantidadF > 0;
	}
	t_tablaDePaginas * paginaAAsignar = malloc(sizeof(t_tablaDePaginas));
	int posicion = busquedaPIDEnLista(pid, pagina);
	paginaAAsignar = list_get(tablaDePaginas, posicion);
	paginaAAsignar->pid = pid;
	paginaAAsignar->contenido = contenidoPedidoAlSwap;
	paginaAAsignar->pagina = pagina;
	paginaAAsignar->bitModificado = 1;
	paginaAAsignar->bitUso = 1;
	paginaAAsignar->bitValidez = 1;
	paginaAAsignar->presencia = 1;

	int cf = CantidadDeFrames(pid);
	if (cf <= configMemoria.maximoMarcosPorProceso - 1) {
		paginaAAsignar->marco = ultimoFrameAsignado;
		ultimoFrameAsignado++;
	} else {
		log_info(logMemoria,
				"EJECUTAR ALGORITMO DE REEMPLAZO DE PAGINAS \0 ACTUALIZACION DE FRAMES \0 ACTUALIZACION DE TLB \n");

		//paginaAAsignar->marco = 0;//algoritmoFIFO(pid);
		pthread_mutex_lock(&BLOQUEAR);
		paginaAAsignar->marco = algoritmoFIFO(pid);
		pthread_mutex_unlock(&BLOQUEAR);

		//sleep(configMemoria.retardoMemoria);
	}
	list_replace(tablaDePaginas, paginaAAsignar->pagina, paginaAAsignar);
	/*log_info(logMemoria, "PID    PAGINA    FRAME\n;");
	 log_info(logMemoria, "%d        %d        %d", paginaAAsignar->pid,
	 paginaAAsignar->pagina, paginaAAsignar->marco);*/
}

void leerPagina(t_leer estructuraLeerSwap, int socketSwap, int socketCPU,
		t_mensajeHeader mensajeHeaderSwap) {
	int resultadoBusquedaTablaPaginas;
	int resultadoBusquedaTLB;
	int pid = estructuraLeerSwap.pid;
	int pagina = estructuraLeerSwap.pagina;
//	generarEstructuraAdministrativaPidFrame(pid, pagina);
	switch (configMemoria.tlbHabilitada) {
	case 1:
		resultadoBusquedaTLB = buscarEnLaTLB(pid, pagina);
		if (resultadoBusquedaTLB == 0) //CASO VERDADERO
				{
			buscarContenidoPagina(socketSwap,pid, pagina, socketCPU);
		} else {
			int resultadoBusquedaTP = buscarEnTablaDePaginas(pid, pagina);
			if (resultadoBusquedaTP == 1) {
				buscarContenidoPagina(socketSwap,pid, pagina, socketCPU);
			} else {
				if (list_size(busquedaListaFramesPorPid(pid))
						<= configMemoria.maximoMarcosPorProceso) {
					AsignarFrameAlProceso(pid, pagina);
				}
				char * contenidoPedidoAlSwap = pedirContenidoAlSwap(socketSwap,
						pid, pagina, socketCPU);
				AsignarContenidoALaPagina(pid, pagina, contenidoPedidoAlSwap);
			}
			break;
			case 0:
			resultadoBusquedaTablaPaginas = buscarEnTablaDePaginas(pid, pagina);
			if (resultadoBusquedaTablaPaginas == 0) {
				buscarContenidoPagina(socketSwap,pid, pagina, socketCPU);
			} else {
				//TODO funcion para que cuent

				if (list_size(busquedaListaFramesPorPid(pid))
						<= configMemoria.maximoMarcosPorProceso) { //TODO funcion para que cuent
					AsignarFrameAlProceso(pid, pagina);
				}
				char * contenidoPedidoAlSwap = pedirContenidoAlSwap(socketSwap,
						pid, pagina, socketCPU);
				AsignarContenidoALaPagina(pid, pagina, contenidoPedidoAlSwap);

				break;
			}

		}
	}
}

void BorrarEstructuras(int PID) {
	//list_remove_and_destroy_by_condition()
	int posicion = 0;

	while (posicion < list_size(listaDePidFrames)) {
		t_pidFrame * pidFrame = list_get(listaDePidFrames, posicion);

		if (pidFrame->pid == PID) {
			list_remove(listaDePidFrames, posicion);
			//posicion++;
		} else {
			posicion++;
		}

	}
	posicion = 0;
	while (posicion < list_size(tablaDePaginas)) {
		t_tablaDePaginas * pidFrame = list_get(tablaDePaginas, posicion);

		if (pidFrame->pid == PID) {
			list_remove(tablaDePaginas, posicion);
			posicion++;
		} else {
			posicion++;
		}

	}
	ultimoFrameAsignado = 0;
}

//CREATE BY MARTIN
t_list * busquedaListaFramesPorPid(int pid) {
	t_list * listaFramesPid = list_create();
	t_pidFrame * pidFrame;
	int contador = 0;
	pidFrame = list_get(listaDePidFrames, contador);
	while (contador < list_size(listaDePidFrames)) {
		if (pidFrame->pid == pid) {
			list_add(listaFramesPid, pidFrame);
		}
		contador++;
	}

	return listaFramesPid;
}

void procesamientoDeMensajes(int clienteSWAP, int servidorCPU) {
	t_iniciarPID *estructuraCPU = malloc(sizeof(t_iniciarPID));

	t_finalizarPID *finalizarCPU = malloc(sizeof(t_finalizarPID));

	t_leer estructuraLeerSwap;

	t_escribir *estructuraEscribirSwap = malloc(sizeof(t_escribir));

	int * memoriaReservadaDeMemPpal =
			malloc(
					sizeof(configMemoria.cantidadDeMarcos
							* configMemoria.tamanioMarcos));
	int statusMensajeRecibidoDeLaCPU; //MENSAJES QUE SE USAN EN EL PASAMANOS, POR AHORA SE LLAMAN ASI, DESPUES LOS VOY A CAMBIAR.
	t_mensajeHeader mensajeHeader, mensajeHeaderSwap;
	statusMensajeRecibidoDeLaCPU = recv(servidorCPU, &mensajeHeader,
			sizeof(t_mensajeHeader), 0);

	if (statusMensajeRecibidoDeLaCPU < 0)
		log_info(logMemoria, "Error al recibir de la CPU");

	else {

		switch (mensajeHeader.idmensaje) {
		case INICIAR:
			pthread_mutex_lock(&mutexIniciar);
			recv(servidorCPU, estructuraCPU, sizeof(t_iniciarPID), 0);
			generarTablaDePaginas(memoriaReservadaDeMemPpal, estructuraCPU->pid,
					estructuraCPU->paginas);

			bool FramesAsignados(t_pidFrame* pidFrame) {
				return pidFrame->pid == estructuraCPU->pid;
			}
			log_info(logMemoria,
					"Proceso mProc creado, PID: %d, Cantidad de paginas asignadas: %d",
					estructuraCPU->pid, estructuraCPU->paginas);
			log_info(logMemoria,
					"Inicio del aviso al proceso SWAp del comando INICIAR");
			enviarIniciarSwap(clienteSWAP, estructuraCPU, mensajeHeaderSwap,
					servidorCPU, logMemoria);
			log_info(logMemoria,
					"Fin del aviso al proceso SWAp del comando INICIAR");
			free(estructuraCPU);
			pthread_mutex_unlock(&mutexIniciar);
			break;
		case LEER: {
			pthread_mutex_lock(&mutexLeer);

			log_info(logMemoria, "Solicitud de lectura recibida");
			log_info(logMemoria, "2do checkpoint: Se envia directo al swap");
			mensajeHeaderSwap.idmensaje = LEER;

			recv(servidorCPU, &estructuraLeerSwap, sizeof(t_leer), 0);
			leerPagina(estructuraLeerSwap, clienteSWAP, servidorCPU,
					mensajeHeaderSwap);

			log_info(logMemoria, "Finalizo comando LEER");
			pthread_mutex_unlock(&mutexLeer);
		}
			break;
		case ESCRIBIR:
			pthread_mutex_lock(&mutexEscribir);
			log_info(logMemoria, "Solicitud de escritura recbidia");
			log_info(logMemoria, "2do chekcpoint NO APLICA");

			recv(servidorCPU, estructuraEscribirSwap, sizeof(t_escribir), 0);

			serializarEstructura(ESCRIBIR, (void *) estructuraEscribirSwap,
					sizeof(t_escribir), clienteSWAP);
			recv(clienteSWAP, &mensajeHeaderSwap, sizeof(t_mensajeHeader), 0);

			pthread_mutex_unlock(&mutexEscribir);

			serializarEstructura(mensajeHeaderSwap.idmensaje,
			NULL, 0, servidorCPU);
			free(estructuraEscribirSwap);

			break;
		case FINALIZAR:
			pthread_mutex_lock(&mutexFinalizar);
			log_info(logMemoria, "FINALIZAR!");

			recv(servidorCPU, finalizarCPU, sizeof(t_finalizarPID), 0);

			BorrarEstructuras(finalizarCPU->pid);
			log_info(logMemoria, "FINALIZAR PID: %d\n", finalizarCPU->pid);

			fflush(stdout);

			serializarEstructura(FINALIZAR, (void *) finalizarCPU,
					sizeof(t_finalizarPID), clienteSWAP);
			recv(clienteSWAP, &mensajeHeaderSwap, sizeof(t_mensajeHeader), 0);

			if (mensajeHeaderSwap.idmensaje == OK)

				serializarEstructura(mensajeHeaderSwap.idmensaje,
				NULL, 0, servidorCPU);
			else

				serializarEstructura(mensajeHeaderSwap.idmensaje,
				NULL, 0, servidorCPU);

			//TODO BORRAR TODAS LAS ESTRUCTURAS ADMINISTRATIVAS PARA ESE mProc.
			pthread_mutex_unlock(&mutexFinalizar);
			break;
		default:
			log_info(logMemoria, "mensaje N°: %d", mensajeHeaderSwap.idmensaje);
			log_info(logMemoria, "Mensaje que no es del SWAP N°:%d",
					mensajeHeader.idmensaje);
			log_info(logMemoria, "Mensaje incorrecto");
		}
	}

}

int ServidorCPU(int PUERTO) {
	int socketservidor;
	int yes = 1;
	struct sockaddr_in serveraddr;

	if ((socketservidor = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
		perror("Error en la creacion del Socket");
		return -1;
	}
	log_info(logMemoria, "Socket Servidor Creado\n");

	if (setsockopt(socketservidor, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int))
			== -1) {
		perror("Error en la definicion del protocolo de comunicacion");
		return -1;
	}
	log_info(logMemoria, "Protocolo de Comunicacion definido correctamente\n");

	serveraddr.sin_family = AF_INET;
	serveraddr.sin_addr.s_addr = INADDR_ANY;
	serveraddr.sin_port = htons(PUERTO);
	memset(&(serveraddr.sin_zero), '\0', 8);

	if (bind(socketservidor, (struct sockaddr *) &serveraddr,
			sizeof(serveraddr)) == -1) {
		perror("Error en el bind");
		return -1;
	}

	if (listen(socketservidor, 10) == -1) {
		perror("Error en el Listen");
		return -1;
	}
	log_info(logMemoria, "Socket Escuchando\n");

	return socketservidor;
}

void RealizarVolcadoMemoriaLog() {

}

int servidorMultiplexorCPU(int PUERTO) {
	fd_set master;
	fd_set read_fds;
	struct sockaddr_in clientaddr;
	int fdmax;
	int newfd;
	int i;
	FD_ZERO(&master);
	FD_ZERO(&read_fds);

	int socketservidor = Servidor(PUERTO);

	FD_SET(socketservidor, &master);
	fdmax = socketservidor;
	for (;;) {
		read_fds = master;
		if (select(fdmax + 1, &read_fds, NULL, NULL, NULL) == -1) {
			perror("Error en el Select");
			exit(1);
		}
		log_info(logMemoria, "Select Activado\n");

		for (i = 0; i <= fdmax; i++) {
			//		signal(senhal,atenderSeaniales);
			if (FD_ISSET(i, &read_fds)) {
				if (i == socketservidor) {
					socklen_t addrlen = sizeof(clientaddr);
					if ((newfd = accept(socketservidor,
							(struct sockaddr *) &clientaddr, &addrlen)) == -1) {
						perror("Error en el Accept");
					} else {
						log_info(logMemoria, "Socket Aceptado\n");

						FD_SET(newfd, &master);
						if (newfd > fdmax) {
							fdmax = newfd;
						}
						log_info(logMemoria,
								"Nueva conexion de %s en el socket %d\n",
								inet_ntoa(clientaddr.sin_addr), newfd);

					}
				} else {

					procesamientoDeMensajes(clienteSwap, i);
				}
			}
		}
	}
	close(i);

	exit(0);		//el descriptor del cliente seleccionado
}
/*FUNCIONES DE ALGORITMO FIFO!*/
/*hay que devolver el frame*/
/*FUNCIONES DE ALGORITMO FIFO!*/

/*hay que devolver el frame*/

int algoritmoFIFO(int pid) {
	return llamar(pid);

}
int CantidadDeFrames(int pid) {
	int cantidadDeFrames = 0;
	int posicion = 0;
	t_list * pf = list_create();
	t_list * listaADestruir = list_create();

	list_add_all(pf, listaDePidFrames);

	while (posicion < configMemoria.maximoMarcosPorProceso) {
		t_pidFrame * pfr = malloc(sizeof(t_pidFrame));

		pfr = list_get(pf, posicion);
		list_add(listaADestruir, pfr);
		if (pfr != NULL)
			cantidadDeFrames++;
		posicion++;
	}
	list_destroy(pf);

	return cantidadDeFrames;
}

//int colaParaReemplazo(int frameAReemplazar, int cantidadDeFrames, int pid);

int llamar(int pid) {
	//pthread_mutex_lock(&BLOQUEAR);
	int i;
	int cantidadDeFramesDelPid = CantidadDeFrames(pid);

	for (i = 0; i < cantidadDeFramesDelPid; i++) {
		if (i > 5) {
			//pthread_mutex_unlock(&BLOQUEAR);
			return ultimoFrameAsignado;
		}
		//	pthread_mutex_unlock(&BLOQUEAR);
		return colaParaReemplazo(i, cantidadDeFramesDelPid, pid);	//i:frame

	}
	//pthread_mutex_unlock(&BLOQUEAR);
	return ultimoFrameAsignado;
}

int busquedaPIDFrame(int PID) {
	int posicion = 0;
	t_pidFrame * pidFrame = list_get(listaDePidFrames, posicion);

	while (pidFrame->pid != PID) {
		posicion++;
		pidFrame = list_get(listaDePidFrames, posicion);
	}
	if (pidFrame->pid == PID) {
		return posicion;
	} else {
		return -1;
	}
}

int ObtenerPrimerFrame(int pid) {
	/*	int i = 0;

	 t_list * lpidframe = list_create();

	 t_list * listaADestruir = list_create();

	 t_pidFrame * pf = malloc(sizeof(t_pidFrame));


	 list_add(listaADestruir, pf);

	 while (list_size(lpf2) > list_size(listaADestruir)) {
	 if (pf->pid == pid) {
	 list_add(lpidframe, list_remove(lpf2, i));
	 list_add(listaADestruir, pf);
	 }
	 i++;
	 pf = list_get(lpf2, i);
	 }
	 free(pf);
	 return lpidframe;*/
	return busquedaPIDFrame(pid);

}

int colaParaReemplazo(int frameAReemplazar, int cantidadDeFrames, int pid) {
	int posicion;
	t_pidFrame * pf = malloc(sizeof(t_pidFrame));
	t_pidFrame * pidframe = malloc(sizeof(t_pidFrame));
	posicion = ObtenerPrimerFrame(pid);
	int i = 0;

	while (i < list_size(listaDePidFrames)) {
		t_pidFrame* lp = malloc(sizeof(t_pidFrame));
		lp = list_get(listaDePidFrames, i);
		log_warning(logMemoria, "FRAME: %d", lp->frameAsignado);
		i++;
	}

	pf = list_remove(listaDePidFrames, posicion);

	pidframe->frameAsignado = pf->frameAsignado;
	pidframe->frameUsado = 1;
	pidframe->pid = pid;

	list_add(listaDePidFrames, pidframe);

	log_warning(logMemoria, "Frame asignado: %d, FRAME GRABADO: %d",
			pf->frameAsignado, pidframe->frameAsignado);
	int frameADevolver = pf->frameAsignado;
	return frameADevolver;
}
