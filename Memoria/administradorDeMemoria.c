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

/******SEMAFOROS*****/
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
	//pasarlas al log
	return NULL;
}

void RealizarVolcadoMemorialog() {

}

void atenderSeaniales(int senial) {

	pid_t pidProcesoClonado;
	switch (senial) {
	case SIGUSR1:
		pthread_create(hiloSigUsr1, NULL, inicioHiloSigUsr1, NULL);
		break;
	case SIGUSR2:
		pthread_create(hiloSigUsr2, NULL, inicioHiloSigUsr2, NULL);
		break;
	case SIGPOLL:
		pidProcesoClonado = fork();
		if (pidProcesoClonado == 0) //PROCESO HIJO
			RealizarVolcadoMemorialog();
		else
			return;
		break;
	}
}

void crearListas() {
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

	remove("logMemoria.txt"); //Cada vez que arranca el proceso borro el archivo de log.
	logMemoria = log_create("logMemoria.txt", "Administrador de memoria", true,
			LOG_LEVEL_INFO);
	leerConfiguracion();
	ultimoFrameAsignado = 0; //TODO
	memoriaReservadaDeMemPpal =
			malloc(
					sizeof(configMemoria.cantidadDeMarcos
							* configMemoria.tamanioMarcos));

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
		estructTLB->frame = -1;

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

void generarTablaDePaginas(char* memoriaReservadaDeMemPpal, int pid,
		int cantidadDePaginas, t_list * tablaDePaginas) {
	log_info(logMemoria, "INICIO TABLAS DE PAGINAS");
	int pagina = 0;

	while (pagina < cantidadDePaginas) {
		t_tablaDePaginas * entrada = malloc(sizeof(t_tablaDePaginas));
		entrada->bitModificado = 0;
		entrada->bitUso = 0;
		entrada->pagina = pagina;
		entrada->pid = pid;
		entrada->direccion = NULL;
		entrada->marco = -1;
		entrada->presencia = 0;
		list_add(tablaDePaginas, entrada);
		pagina++;
		t_estructurasDelProceso * estructuraDelProceso = malloc(
				sizeof(t_estructurasDelProceso));

		estructuraDelProceso->pid = pid;
		estructuraDelProceso->estructura = entrada;
		list_add(estructurasPorProceso, estructuraDelProceso);
		//free(entrada);
		//free(estructuraDelProceso);

	}

	log_info(logMemoria, "FIN TABLAS DE PAGINAS");
}

/*void agregarAEstructuraGeneral(void * estructurasDelProceso, int pid) {
 log_info(logMemoria, "AGREGANDO ESTRUCTURAS AL PID: %d", pid);
 t_estructurasDelProceso * ep = malloc(sizeof(t_estructurasDelProceso));
 ep->pid = pid;
 ep->estructura = estructurasDelProceso;
 list_add(estructurasPorProceso, ep);
 }*/

void AsignarFrameAlProceso(int pid, int cantidadDePaginas) {
	t_pidFrame * estructuraPidFrame = malloc(sizeof(t_pidFrame));

	estructuraPidFrame->frameAsignado = ultimoFrameAsignado;
	estructuraPidFrame->pid = pid;
	estructuraPidFrame->frameUsado = 0; //0 SIN USAR, 1 USADO.
	list_add(listaDePidFrames, estructuraPidFrame);
	log_info(logMemoria, "frame asignado: %d al pid:%d",
			ultimoFrameAsignado - 1, pid);

	//agregarAEstructuraGeneral(listaDePidFrames, pid);

	pthread_mutex_unlock(&mutexFrame);
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
	t_TLB * TLB = malloc(sizeof(t_TLB));
	int cantidad = list_count_satisfying(tlb, (void*) buscarPagina);

	if (cantidad > 0) {
		log_info(logMemoria, "PAGINA ENCONTRADA EN LA TLB");
		TLB = list_find(tlb, (void *) buscarPagina);
		int frame = TLB->frame;
		free(TLB);
		return frame;
	} else {
		log_info(logMemoria, "PAGINA NO ENCONTRADA EN LA TLB");
		free(TLB);
		return -1;
	}
}

int busquedaPidPaginaEnLista(int PID, int pagina, t_list * tablaDePaginas) {
	int posicion = 0;
	t_tablaDePaginas* pag = malloc(sizeof(t_tablaDePaginas));
	if (list_size(tablaDePaginas) == 1)
		return posicion;
	pag = list_get(tablaDePaginas, posicion);

	while (pag->pagina != pagina) {
		posicion++;
		pag = list_get(tablaDePaginas, posicion);
	}
	log_info(logMemoria, "POSICION ENCONTRADA : %d", posicion);
	free(pag);
	return posicion;
}

int busquedaFRAMESinUsar(int PID) {
	int posicion = 0;
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
	if (pframe->frameUsado == 0)
		return pframe->frameAsignado;
	else
		return -1;
}

int buscarEnTablaDePaginas(int pid, int pagina, t_list * tablaDePaginas,
		int posicion) {
	t_tablaDePaginas * tp = malloc(sizeof(t_tablaDePaginas));
	log_info(logMemoria, "INICIO BUSQUEDA EN TABLA DE PAGINAS");
	bool verificarBitValidezYPresencia(t_list * tablaDePaginasEncontradas) {
		int i = 0;

		while (i <= list_size(tablaDePaginas)) {
			tp = list_get(tablaDePaginasEncontradas, i);
			if (tp->pid == pid && tp->pagina)
				break; //corta el ciclo
			i++;

		}
		return tp->bitValidez == 0 && tp->presencia == 1
				&& string_is_empty(tp->direccion);
	}

	t_list * ltp = list_create();
	t_tablaDePaginas * tp1 = malloc(sizeof(t_tablaDePaginas));
	tp1 = list_get(estructurasPorProceso, posicion);

	list_add(ltp, tp1);

	if (list_size(ltp) >= 1) {
		return tp->marco;
	} else {
		return -1;
	}
}

char * buscarContenidoEnMemoriaPPal(int pid, int pagina, t_list *tablaDePaginas) {
	char * contenido = malloc(sizeof(list_size(tablaDePaginas)));
	t_tablaDePaginas * estructContenidoPaginas = malloc(
			sizeof(t_tablaDePaginas));
	memcpy(contenido,
			memoriaReservadaDeMemPpal
					+ (estructContenidoPaginas->marco
							* configMemoria.tamanioMarcos),
			configMemoria.tamanioMarcos);

	free(estructContenidoPaginas);

	return contenido;
}

void buscarContenidoPagina(int pid, int pagina, int socketCPU,
		t_list* tablaDePaginas) {
	t_contenido* contenidoCPU = malloc(sizeof(t_contenido));
	char* contenidoADevolver = buscarContenidoEnMemoriaPPal(pid, pagina,
			tablaDePaginas);

	contenidoADevolver[strlen(contenidoADevolver) + 1] = '\0';

	contenidoCPU->tamanio = strlen(contenidoADevolver);
	contenidoCPU->contenido = contenidoADevolver;

	serializarEstructura(LEER, contenidoCPU, sizeof(contenidoCPU), socketCPU);
}

char * pedirContenidoAlSwap(int cliente, int pid, int pagina, int servidor) {
	log_info(logMemoria, "INICIO PEDIDO AL SWAP");
	int tamanioLeido;
	t_leer * estructuraLeerSwap = malloc(sizeof(t_leer));
	estructuraLeerSwap->pid = pid;
	estructuraLeerSwap->pagina = pagina;
	serializarEstructura(LEER, estructuraLeerSwap, sizeof(t_leer), cliente);
	recv(cliente, &tamanioLeido, sizeof(int), 0);
	log_info(logMemoria, "tamaño: %d \n", tamanioLeido);

	char* contenidoLeido = malloc(tamanioLeido);

	recv(cliente, contenidoLeido, tamanioLeido, 0);

	log_info(logMemoria, "Contenido: %s", contenidoLeido);
	contenidoLeido[tamanioLeido] = '\0';

	send(servidor, &tamanioLeido, sizeof(tamanioLeido), 0);
	send(servidor, contenidoLeido, tamanioLeido, 0);

	return contenidoLeido;
}

void ActualizarFrame(t_tablaDePaginas* paginaAAsignar, int pid,
		char * contenido, int frame) {
	t_pidFrame * pidAAsignar = malloc(sizeof(t_pidFrame));
	pidAAsignar = list_get(listaDePidFrames, frame);
	paginaAAsignar->marco = frame;
	pidAAsignar->pid = pid;
	pidAAsignar->frameUsado = 1;
	list_replace(listaDePidFrames, frame, pidAAsignar);
//TODO FALTA ACUTALIZAR LA ENTRADA DE LA TLB->fifo
	memcpy(memoriaReservadaDeMemPpal + frame * configMemoria.tamanioMarcos,
			contenido, configMemoria.tamanioMarcos);

}

void AsignarContenidoALaPagina(t_tablaDePaginas * tablaDePaginas, int pid,
		int pagina, char * contenidoPedidoAlSwap) {
	t_tablaDePaginas * paginaAAsignar = malloc(sizeof(t_tablaDePaginas));
	//paginaAAsignar = list_get(tablaDePaginas, pagina);
	paginaAAsignar->pid = pid;

	memcpy(&memoriaReservadaDeMemPpal[strlen(contenidoPedidoAlSwap) + 1],
			contenidoPedidoAlSwap, strlen(contenidoPedidoAlSwap));

	paginaAAsignar->pagina = pagina;
	paginaAAsignar->bitModificado = 1;
	paginaAAsignar->bitUso = 1;
	paginaAAsignar->bitValidez = 1;
	paginaAAsignar->presencia = 1;

	if (CantidadDeFrames(pid) <= configMemoria.maximoMarcosPorProceso) {
		ActualizarFrame(paginaAAsignar, pid, contenidoPedidoAlSwap,
				ultimoFrameAsignado);

	} else {
		ActualizarFrame(paginaAAsignar, pid, contenidoPedidoAlSwap,
				algoritmoFIFO(pid));
		sleep(configMemoria.retardoMemoria);
	}
	//list_replace(tablaDePaginas,pagina, paginaAAsignar);
}

t_tablaDePaginas * buscarTP(int pid) {
	int posicion = pid - 1;
	t_tablaDePaginas * tp = malloc(sizeof(t_tablaDePaginas));
	tp = list_get(estructurasPorProceso, posicion);
	return tp;

}

void leerPagina(t_leer estructuraLeerSwap, int socketSwap, int socketCPU,
		t_mensajeHeader mensajeHeaderSwap, t_tablaDePaginas * listaPaginas,
		int posicion) {
	int resultadoBusquedaTablaPaginas;
	int resultadoBusquedaTLB;
	int pid = estructuraLeerSwap.pid;
	int pagina = estructuraLeerSwap.pagina;
	t_list * ltp = list_create();
	list_add(ltp, listaPaginas);
	switch (configMemoria.tlbHabilitada) {
	case 1:
		resultadoBusquedaTLB = buscarEnLaTLB(pid, pagina);
		if (resultadoBusquedaTLB >= 0) //CASO VERDADERO
				{
			buscarContenidoPagina(pid, pagina, socketCPU, ltp);
		} else {
			int resultadoBusquedaTP = buscarEnTablaDePaginas(pid, pagina, ltp,
					posicion); //todo
			if (resultadoBusquedaTP >= 0) {
				buscarContenidoPagina(pid, pagina, socketCPU, ltp);
			} else {
				AsignarFrameAlProceso(pid, pagina);
				char * contenidoPedidoAlSwap = pedirContenidoAlSwap(socketSwap,
						pid, pagina, socketCPU);
				t_tablaDePaginas * paginaAsignarContenido = malloc(
						sizeof(t_tablaDePaginas));
				paginaAsignarContenido = list_get(ltp, pagina);
				AsignarContenidoALaPagina(paginaAsignarContenido, pid, pagina,
						contenidoPedidoAlSwap);
			}
		}
		break;
	case 0:
		resultadoBusquedaTablaPaginas = buscarEnTablaDePaginas(pid, pagina, ltp,
				posicion);
		if (resultadoBusquedaTablaPaginas == 0)
			buscarContenidoPagina(pid, pagina, socketCPU, ltp);
		else {
			AsignarFrameAlProceso(pid, pagina);
			char * contenidoPedidoAlSwap = pedirContenidoAlSwap(socketSwap, pid,
					pagina, socketCPU);
			t_tablaDePaginas * paginaAsignarContenido = malloc(
					sizeof(t_tablaDePaginas));
			paginaAsignarContenido = list_get(ltp, pagina);
			AsignarContenidoALaPagina(paginaAsignarContenido, pid, pagina,
					contenidoPedidoAlSwap);
		}
		break;
	}
}

void procesamientoDeMensajes(int clienteSWAP, int servidorCPU) {
	t_iniciarPID *estructuraCPU;
	t_finalizarPID *finalizarCPU;
	t_leer estructuraLeerSwap;
	t_escribir *estructuraEscribirSwap;
	t_list * tablaDePaginas;
	t_mensajeHeader mensajeHeader, mensajeHeaderSwap;

	int statusMensajeRecibidoDeLaCPU; //MENSAJES QUE SE USAN EN EL PASAMANOS, POR AHORA SE LLAMAN ASI, DESPUES LOS VOY A CAMBIAR.

	statusMensajeRecibidoDeLaCPU = recv(servidorCPU, &mensajeHeader,
			sizeof(t_mensajeHeader), 0);

	if (statusMensajeRecibidoDeLaCPU < 0)
		log_info(logMemoria, "Error al recibir de la CPU");

	else {

		switch (mensajeHeader.idmensaje) {
		case INICIAR:
			pthread_mutex_lock(&mutexIniciar);
			estructuraCPU = malloc(sizeof(t_iniciarPID));
			recv(servidorCPU, estructuraCPU, sizeof(t_iniciarPID), 0);
			tablaDePaginas = list_create();
			generarTablaDePaginas(memoriaReservadaDeMemPpal, estructuraCPU->pid,
					estructuraCPU->paginas, tablaDePaginas);
//			agregarAEstructuraGeneral(tablaDePaginas, estructuraCPU->pid);

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

			//estructurasPorProceso
			int pos = buscarPaginasProceso(estructuraLeerSwap.pid,
					estructuraLeerSwap.pagina);
			t_estructurasDelProceso * listaPaginas = list_get(
					estructurasPorProceso, pos);
			leerPagina(estructuraLeerSwap, clienteSWAP, servidorCPU,
					mensajeHeaderSwap, listaPaginas->estructura, pos);

			log_info(logMemoria, "Finalizo comando LEER");
			pthread_mutex_unlock(&mutexLeer);
		}
			break;
		case ESCRIBIR:
			pthread_mutex_lock(&mutexEscribir);
			log_info(logMemoria, "Solicitud de escritura recbidia");
			log_info(logMemoria, "2do chekcpoint NO APLICA");
			estructuraEscribirSwap = malloc(sizeof(estructuraEscribirSwap));
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
			finalizarCPU = malloc(sizeof(t_finalizarPID));
			recv(servidorCPU, finalizarCPU, sizeof(t_finalizarPID), 0);

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

			//finalizar(
			//BORRAR TODAS LAS ESTRUCTURAS ADMINISTRATIVAS PARA ESE mProc.
			pthread_mutex_unlock(&mutexFinalizar);
			break;
		default:
			printf("HOLA");
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
pthread_mutex_t BLOQUEAR;
int algoritmoFIFO(int pid) {
	return llamar(pid);

}
int CantidadDeFrames(int pid) {
	int cantidadDeFrames = 0;
	int posicion = 0;
	t_list * pf = list_create();
	t_list * listaADestruir = list_create();

	list_add_all(pf, listaDePidFrames);

	while (list_size(pf) != list_size(listaADestruir)) {
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

int colaParaReemplazo(int frameAReemplazar, int cantidadDeFrames, int pid);

int llamar(int pid) {
	pthread_mutex_lock(&BLOQUEAR);
	int fs = ultimoFrameAsignado;
	int i;
	for (i = 0; i < fs; i++) {
		t_pidFrame * frame = list_get(listaDePidFrames, i);
		frame->frameUsado = -1;
	}
	int cantidadDeFramesDelPid = CantidadDeFrames(pid);

	for (i = 0; i < cantidadDeFramesDelPid; i++) {
		if (i > 5) {
			pthread_mutex_unlock(&BLOQUEAR);
			return ultimoFrameAsignado;
		}
		pthread_mutex_unlock(&BLOQUEAR);
		return colaParaReemplazo(i, cantidadDeFramesDelPid, pid);	//i:frame

	}
	pthread_mutex_unlock(&BLOQUEAR);
	return ultimoFrameAsignado;
}

t_list * ObtenerPrimerFrame(int pid) {
	int i = 0;
	t_list * lpf = list_create();
	t_list * listaADestruir = list_create();
	t_pidFrame * pf = malloc(sizeof(t_pidFrame));
	t_list *lpf2 = list_create();
	list_add_all(lpf2, listaDePidFrames);
	pf = list_get(lpf2, i);
	list_add(listaADestruir, pf);
	while (list_size(lpf2) > list_size(listaADestruir)) {
		if (pf->pid == pid) {
			list_add(lpf, list_remove(lpf2, i));
			list_add(listaADestruir, pf);
		}
		i++;
		pf = list_get(lpf2, i);
		/*if (i > 50) {
		 log_error(logMemoria,
		 "Despues de 50 intentos, devuelvo el primer elemento de la lista!");
		 pf = list_get(lpf2, 0);
		 list_add(lpf, list_remove(lpf2,i));
		 break;
		 }*/
	}
	free(pf);
	return lpf;
}

int r = -1;		//DEBE SER GLOBAL!
int colaParaReemplazo(int frameAReemplazar, int cantidadDeFrames, int pid) {
	int fs = cantidadDeFrames;
	t_list * lpf = list_create();
	t_pidFrame * pf = malloc(sizeof(t_pidFrame));

	lpf = ObtenerPrimerFrame(pid);

	pf = list_get(lpf, 0);
	if (pf->frameAsignado == frameAReemplazar)
		return frameAReemplazar;

	if (r == fs - 1)
		r = 0;
	else if (r == -1)
		r = 0;
	else if (r > fs + 1)
		r = 0;

	log_error(logMemoria, "VALOR DE R: %d", r);

	pf = list_get(lpf, 0);
	pf->frameAsignado = frameAReemplazar;
	pf->frameUsado = 1;
	list_add(listaDePidFrames, pf);
	free(pf);
	return pf->frameAsignado;
}

int buscarPaginasProceso(int pid, int pagina) {
	t_estructurasDelProceso * edP;
	/*	bool estructProc (t_estructurasDelProceso * ep)
	 {
	 return ep->pid == pid;
	 }
	 t_list * l = list_create();
	 list_add(l,list_find(estructurasPorProceso, (void*)estructProc));

	 estructura = list_remove(l,0);
	 return estructura->estructura;*/
	int posicion = 0;
	printf("CANTIDAD DE ESTRUCTURA: %d",list_size(estructurasPorProceso));
	fflush(stdout);
	edP = list_get(estructurasPorProceso, posicion);
	while ((edP->pid != pid) && (edP->estructura->pagina != pagina)
			&& (posicion < list_size(estructurasPorProceso))) {
		posicion++;
		edP = list_get(estructurasPorProceso, posicion);
	}
	free(edP);
	return posicion;

}
