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
#include <commons/string.h>
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
#include <errno.h>

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
pthread_mutex_t mutexFrames;
pthread_mutex_t senialUsr2;
pthread_mutex_t senialUsr1;
pthread_mutex_t mutexsenial;
pthread_mutex_t MEMORIAPPAL;
/********************/

int ultimoFrameAsignado;
int busquedaPIDFramePorFrame(int frame) {

	int posicion = 0;
	t_pidFrame * pidFrame = list_get(listaDePidFrames, posicion);

	while (pidFrame->frameAsignado != frame) {
		posicion++;
		if (posicion == list_size(listaDePidFrames))
			break;
		pidFrame = list_get(listaDePidFrames, posicion);
	}
	if (pidFrame->frameAsignado == frame) {
		return posicion;
	} else {
		return -1;
	}
}

void inicioHiloSigUsr1() {
	log_info(logMemoria, "HILO SIGUSR1");
	int i = 1;
	while (i == 1) {
		pthread_mutex_lock(&senialUsr1);
		log_info(logMemoria, "FLUSH de TLB");
		list_destroy(tlb);
		tlb = list_create();
		pthread_mutex_unlock(&mutexsenial);
	}

}

struct sigaction estructuraSignal;
int buscarPaginaEnTP(int frame) {
	int posicion = 0;
	t_tablaDePaginas * tp = list_get(tablaDePaginas, posicion);
	while (posicion < list_size(tablaDePaginas)) {
		tp = list_get(tablaDePaginas, posicion);
		if (tp->marco == frame) {
			return tp->pagina;
		} else {
			posicion++;
		}
	}
	return -1;
}
void borrarMemoria() {
	t_escribir * escribir = malloc(sizeof(t_escribir));
	int posicion = 0;
	t_frames * frame = list_get(frames, posicion);
	char * contenido = malloc(configMemoria.tamanioMarcos);
	while (posicion < list_size(frames)) {
		frame = list_get(frames, posicion);
		int pagina = buscarPaginaEnTP(frame->frame);
		t_pidFrame * pid = list_get(listaDePidFrames,
				busquedaPIDFramePorFrame(frame->frame));

		if (frame->ocupado == OCUPADO) {
			pthread_mutex_lock(&MEMORIAPPAL);
			memcpy(contenido,
					memoriaReservadaDeMemPpal
							+ (frame->frame * configMemoria.tamanioMarcos),
					configMemoria.tamanioMarcos);
			contenido[configMemoria.tamanioMarcos] = '\0';
			pthread_mutex_unlock(&MEMORIAPPAL);
			escribir->pid = pid->pid;
			escribir->pagina = pagina;
			strncpy(escribir->contenidoPagina, contenido, strlen(contenido));

			serializarEstructura(ESCRIBIR, escribir, sizeof(escribir),
					clienteSwap);

		}
		posicion++;
	}
}

void inicioHiloSigUsr2() {
	log_info(logMemoria, "HILO SIGUSR2");
	int i = 1;
	while (i == 1) {
		pthread_mutex_lock(&senialUsr2);
		log_info(logMemoria, "FLUSH de la tabla de paginas");
		borrarMemoria();

		list_destroy(tablaDePaginas);
		tablaDePaginas = list_create(tablaDePaginas);
		pthread_mutex_unlock(&mutexsenial);
	}
}

void atenderSeniales(int senial) {

	pid_t pidProcesoClonado;
	switch (senial) {
	case SIGUSR1:
		pthread_mutex_unlock(&senialUsr1);
		pthread_mutex_lock(&mutexsenial);
		break;
	case SIGUSR2:
		pthread_mutex_unlock(&senialUsr2);
		pthread_mutex_lock(&mutexsenial);
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

void initSeniales() {
	//sigemptyset(&estructuraSignal.sa_mask);
	estructuraSignal.sa_handler = atenderSeniales;
	estructuraSignal.sa_flags = SA_RESTART;
	sigfillset(&estructuraSignal.sa_mask);

	sigaction(SIGUSR1, &estructuraSignal, NULL);
	sigaction(SIGUSR2, &estructuraSignal, NULL);
	sigaction(SIGPOLL, &estructuraSignal, NULL);
}

void crearListas() {
	tablaDePaginas = list_create();
	estructuraAlgoritmos = list_create();
	tlb = list_create();
	listaDePidFrames = list_create();
	estructurasPorProceso = list_create();
	frames = list_create();

}

int main() {
	/*DEJAR ESTOS SEMAFOROS ACA!!!!!!!! NO BORRAR!!!!!!!!*/
	pthread_mutex_lock(&senialUsr1);
	pthread_mutex_lock(&senialUsr2);
	/*DEJAR ESTOS SEMAFOROS ACA!!!!!!!! NO BORRAR!!!!!!!!*/

	/*SEÑALES*/

	initSeniales();

	remove("logMemoria2.txt"); //Cada vez que arranca el proceso borro el archivo de log.A
	logMemoria = log_create("logMemoria2.txt", "Administrador de memoria", true,
			LOG_LEVEL_INFO);
	leerConfiguracion();

	memoriaReservadaDeMemPpal = malloc(
			configMemoria.cantidadDeMarcos * configMemoria.tamanioMarcos);
	memset(memoriaReservadaDeMemPpal, '\0',
			configMemoria.cantidadDeMarcos * configMemoria.tamanioMarcos);
	log_info(logMemoria, "Comienzo de las diferentes conexiones");
	crearListas();
	inicializarFrames();

	//creacionTLB(&configMemoria, logMemoria);

	pthread_create(&hiloTasaTLB, NULL, (void *) &calcularTasaAciertos, NULL);
	pthread_create(&hiloSigUsr1, NULL, (void*) inicioHiloSigUsr1, NULL);
	pthread_create(&hiloSigUsr2, NULL, (void*) inicioHiloSigUsr2, NULL);

	clienteSwap = ConexionMemoriaSwap(&configMemoria, logMemoria);

	int servidorCPU = servidorMultiplexorCPU(configMemoria.puertoEscucha);
	log_destroy(logMemoria);
	free(memoriaReservadaDeMemPpal);
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
	configMemoria.algoritmoReemplazo = config_get_int_value(
			configuracionMemoria, "ALGORITMO_REEMPLAZO");
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
		estructTLB->frame = -1;
		++entrada;
		list_add(tlb, estructTLB);
		free(estructTLB);
	}
}

void creacionTLB() {
	if (configMemoria.tlbHabilitada == 1) {
		log_info(logMemoria,
				"La TLB esta habilitada, se procede a su creación");
		log_info(logMemoria, "Inicio creacion de la TLB");
		generarTLB(configMemoria.entradasTLB);
		log_info(logMemoria, "Finalizo existosamente la creacion de la TLB");
	} else {
		log_info(logMemoria, "La TLB no esta habilitada");
		log_info(logMemoria, "La TLB no esta activada por configuración");
	}
}
int ConexionMemoriaSwap(t_config_memoria* configMemoria, t_log* logMemoria) {
	int intentosFallidosDeConexionASwap = 0;
	log_info(logMemoria,
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
			usleep(10);
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
void generarTablaDePaginas(char * memoriaReservadaDeMemPpal, int pid,
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
		entrada->direccion = NULL;
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

int AsignarFrameAlProceso(int pid, int cantidadDePaginas, int bit) {
	//time_t t = time(NULL);
	int frameLibre = seleccionarFrameLibre();
	if (frameLibre != -1) {
		t_pidFrame * estructuraPidFrame = malloc(sizeof(t_pidFrame));

		//estructuraPidFrame->frameAsignado = seleccionarFrameLibre();
		estructuraPidFrame->frameAsignado = frameLibre;
		estructuraPidFrame->pid = pid;
		//estructuraPidFrame->frameUsado =0; //0 SIN USAR, 1 USADO.
		//estructuraPidFrame->frameModificado = bit; //0 NECESARIO PARA ALGORITMO CLOCK TODO REVISAR: LO ACTUALIZA?cual usa?
		//estructuraPidFrame->puntero = 0; //NECESARIO PARA SABER DONDE CONTINUAR EN EL ALGORITMO CLOCK
		//estructuraPidFrame->ultimaReferencia = t;
		list_add(listaDePidFrames, estructuraPidFrame);

		log_info(logMemoria, "frame asignado: %d al pid:%d",
				estructuraPidFrame->frameAsignado, pid);

		if (list_size(busquedaListaFramesPorPid(pid))
				< configMemoria.maximoMarcosPorProceso) {
			agregarAEstructuraGeneral(listaDePidFrames, pid);
		}
		return estructuraPidFrame->frameAsignado;
	}

	else {
		return frameLibre;
	}

}

void enviarIniciarSwap(int cliente, t_iniciarPID *estructuraCPU,
		t_mensajeHeader mensajeHeaderSwap, int servidor, t_log* logMemoria) {
	if (list_size(listaDePidFrames) == configMemoria.cantidadDeMarcos) {
		//RECHAZO EL PROCESO
		serializarEstructura(PROCFALLA, NULL, 0, servidor); //ENVIO ERROR A LA CPU

		/*			t_finalizarPID  fp;
		 fp.pid = estructuraCPU->pid;
		 fp.idCPU = 0;
		 fp.instrucciones = 0;
		 serializarEstructura(FINALIZAR,)*/
		return;
	}

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
	int posicion = busquedaPIDEnListaTLB(pid, pagina);
	if (posicion >= 0) {
		t_TLB * entradaTLB;// = malloc(sizeof(t_TLB));
		entradaTLB = list_get(tlb, posicion);
		int frame = entradaTLB->frame;
		//free(entradaTLB);
		accesosTLB++;
		aciertosTLB++;
		log_info(logMemoria, "PAGINA ENCONTRADA EN LA TLB - FRAME = %d\n",
				frame);
		return frame;
	} else {
		accesosTLB++;
		log_info(logMemoria, "PAGINA NO ENCONTRADA EN LA TLB\n");
		return -1;
	}
}

int busquedaPIDEnLista(int PID, int pagina) {
	int posicion = 0;
	if (list_size(tablaDePaginas) == 0)
		return -1;
	t_tablaDePaginas* pag = list_get(tablaDePaginas, posicion);
	while ((pag->pagina != pagina || pag->pid != PID)) {

		posicion++;
		if (posicion == list_size(tablaDePaginas))
			break;
		pag = list_get(tablaDePaginas, posicion);

	}

	//TODO- REVISAR: VALE LA PENA ESTA VALIDACION?? EN LA DE PAGINAS VA A ESTAR SIEMPRE, POR AHORA LO COMENTO
	if (pag->pagina == pagina && pag->pid == PID) {
		return posicion;
	} else {
		log_info(logMemoria, "Pagina no encontrada \n");

		return -1;
	}
}
int busquedaPIDEnListaTLB(int PID, int pagina) {
	int posicion = 0;
	if (list_size(tlb) == 0)
		return -1;
	t_TLB* pag = list_get(tlb, posicion);
	while ((pag->pagina != pagina || pag->pid != PID)) {

		posicion++;
		if (posicion == list_size(tlb)){
			return -1;
		break;
		}
		pag = list_get(tlb, posicion);

	}

	//TODO- REVISAR: VALE LA PENA ESTA VALIDACION?? EN LA DE PAGINAS VA A ESTAR SIEMPRE, POR AHORA LO COMENTO
	if (pag->pagina == pagina && pag->pid == PID) {
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

int buscarEnTablaDePaginas(int pid, int pagina) {
	t_tablaDePaginas * entrada = malloc(sizeof(t_tablaDePaginas));

	log_info(logMemoria, "INICIO BUSQUEDA EN TABLA DE PAGINAS");

	t_list * paginasEncontradas = list_create();
	int posicion = busquedaPIDEnLista(pid, pagina);
	if (posicion >= 0) { //encontro la pagina, es valida y esta presente
		entrada = list_get(tablaDePaginas, posicion);
		list_add(paginasEncontradas, entrada);
	}
	if (list_size(paginasEncontradas) >= 1) {

		if (entrada->bitValidez == 1 && entrada->presencia == 1) {
			log_info(logMemoria,
					"PAGINA PRESENTE EN TABLA DE PAGINAS - FRAME = %d \n",
					entrada->marco);
			return entrada->marco;
		} else
			log_info(logMemoria, "PAGINA NO PRESENTE EN TABLA DE PAGINAS \n");
		return -1; // si la pagina para el pid NO es unica retorno falso

	} else
		log_info(logMemoria, "PAGINA NO ENCONTRADA EN TABLA DE PAGINAS \n");
	return -1;
}

char * pedirContenidoAlSwap(int cliente, int pid, int pagina, int servidor) {
	log_info(logMemoria, "INICIO PEDIDO AL SWAP");
	int tamanioLeido;
	t_leer * estructuraLeerSwap = malloc(sizeof(t_leer));
	estructuraLeerSwap->pid = pid;
	estructuraLeerSwap->pagina = pagina;
	serializarEstructura(LEER, estructuraLeerSwap, sizeof(t_leer), cliente);
	recv(cliente, &tamanioLeido, sizeof(int), 0);

	char* contenidoLeido = malloc(tamanioLeido);

	recv(cliente, contenidoLeido, tamanioLeido, 0);

	log_info(logMemoria, "Contenido: %s", contenidoLeido);
	log_info(logMemoria, "FIN PEDIDO AL SWAP");
	send(servidor, &tamanioLeido, sizeof(tamanioLeido), 0);
	send(servidor, contenidoLeido, tamanioLeido, 0);

	free(estructuraLeerSwap);
	return contenidoLeido;
}

void ActualizarFrame(t_tablaDePaginas* paginaAAsignar, int pid) {
	time_t t = time(NULL);
	int posicion = busquedaFRAMESinUsar(pid); //todo revisar, esa funcion me hace ruido
	t_pidFrame * pidAAsignar = malloc(sizeof(t_pidFrame));
	pidAAsignar = list_get(listaDePidFrames, posicion);
	paginaAAsignar->marco = pidAAsignar->frameAsignado;

	pidAAsignar->frameUsado = 1;
	pidAAsignar->ultimaReferencia = t;
	//list_replace(listaDePidFrames, posicion, pidAAsignar);
}

void FifoTLB(int pid, int pagina, int frame) {

	t_TLB * estructTlb = malloc(sizeof(t_TLB));

	estructTlb = list_remove(tlb, 0);
	estructTlb->frame = frame;
	estructTlb->pagina = pagina;
	estructTlb->pid = pid;
	list_add(tlb, estructTlb);

}

void AsignarEnTlb(int pid, int pagina, int frame) {

	bool buscarPagina(t_TLB * buscarTLB) {
		return buscarTLB->pid == pid && buscarTLB->pagina == pagina;
	}
	int cantidad = -1;
	//cantidad = list_count_satisfying(tlb, (void*) buscarPagina);
	//if (cantidad <= 0){
	int posicion = busquedaPIDEnListaTLB(pid, pagina);
	if (posicion < 0) {
		if (list_size(tlb) < configMemoria.entradasTLB) //La TLB es unica para todos los procesos
				{
			t_TLB * estructTlb = malloc(sizeof(t_TLB));
			estructTlb->frame = frame;
			estructTlb->pagina = pagina;
			estructTlb->pid = pid;
			list_add(tlb, estructTlb);
		} else {
			FifoTLB(pid, pagina, frame);
		}
	}
}
int busquedaPIDFrame(int pid);
void AsignarContenidoALaPagina(int pid, int pagina,
		char * contenidoPedidoAlSwap, int marco, int bitModificado) {

	t_escribir * escribir = malloc(sizeof(t_escribir));
	memset(&escribir->contenidoPagina, 0, sizeof(escribir->contenidoPagina));
	t_tablaDePaginas * paginaAAsignar = malloc(sizeof(t_tablaDePaginas));
	int ejecucionAlgoritmo = 0;
	int posicion = busquedaPIDEnLista(pid, pagina);
	if (posicion != -1) {
		paginaAAsignar = list_get(tablaDePaginas, posicion);
		paginaAAsignar->pid = pid;

		paginaAAsignar->pagina = pagina;
		paginaAAsignar->bitModificado = bitModificado;

		paginaAAsignar->bitUso = 1;
		paginaAAsignar->bitValidez = 1;
		paginaAAsignar->presencia = 1;

		if (CantidadDeFrames(pid) <= configMemoria.maximoMarcosPorProceso
				&& marco != -1) { //menor o igual porque si tenia libre ya lo asigno afuera
			log_info(logMemoria, " no requiere algoritmo de reemplazo \n");
			paginaAAsignar->marco = marco;
		} else {
			pthread_mutex_lock(&BLOQUEAR);
			log_info(logMemoria, "ejecuta algoritmo de reemplazo \n");
			log_info("Cantidad actual de frames asignados al proceso %d  : %d",
					CantidadDeFrames(pid));
			ejecucionAlgoritmo = 1;
			paginaAAsignar->marco = ejecutarAlgoritmo(pid);
			imprimirBit(pid);
			pthread_mutex_unlock(&BLOQUEAR);

			usleep(configMemoria.retardoMemoria);

			//TODO
			char * contenido = malloc(strlen(contenidoPedidoAlSwap));
			strncpy(contenido, contenidoPedidoAlSwap,
					strlen(contenidoPedidoAlSwap));
			pthread_mutex_lock(&MEMORIAPPAL);
			memcpy(
					memoriaReservadaDeMemPpal
							+ (paginaAAsignar->marco
									* configMemoria.tamanioMarcos), contenido,
					configMemoria.tamanioMarcos);
			pthread_mutex_unlock(&MEMORIAPPAL);

			//paginaAAsignar->direccion = strdup(memoriaReservadaDeMemPpal);
			//todo revisar esta asignacion//>reemplazarla por la de arriba si falla
			paginaAAsignar->direccion = memoriaReservadaDeMemPpal;

			usleep(configMemoria.retardoMemoria);
		}
		time_t t = time(NULL);
		int ppf = busquedaPIDFramePorFrame(paginaAAsignar->marco);
		if (ppf != -1) {
			t_pidFrame * pf = list_get(listaDePidFrames, ppf);
			pf->ultimaReferencia = t;
			pf->frameModificado = bitModificado;

			if (ejecucionAlgoritmo == 1) {
				pf->puntero = 1;
			} else {
				//pf->frameModificado = bitModificado;
				pf->puntero = 0;
			}

			pf->frameUsado = 1;
		} else if (list_size(busquedaListaFramesPorPid(pid))
				< configMemoria.maximoMarcosPorProceso) {
			t_pidFrame *pf = malloc(sizeof(t_pidFrame));
			pf->frameAsignado = paginaAAsignar->marco;
			pf->pid = pid;
			pf->puntero = 0;
			pf->ultimaReferencia = t;
			pf->frameModificado = bitModificado;
			pf->frameUsado = 1;
			list_add(listaDePidFrames, pf);

		}

		AsignarEnTlb(pid, pagina, paginaAAsignar->marco);

		escribir->pagina = pagina;
		escribir->pid = pid;
		//TODO
		strncpy(escribir->contenidoPagina, contenidoPedidoAlSwap,
				strlen(contenidoPedidoAlSwap) + 1);
		escribirContenido(escribir, paginaAAsignar->marco, 4);
		//list_replace(tablaDePaginas, posicion, paginaAAsignar); //TODO hay que buscar la pagina del proceso, para reemplazar esa posicion y no cualquiera
	}

	free(escribir);
}

void leerPagina(t_leer estructuraLeerSwap, int socketSwap, int socketCPU,
		t_mensajeHeader mensajeHeaderSwap) {

	int resultadoBusquedaTablaPaginas;
	int resultadoBusquedaTLB;
	int pid = estructuraLeerSwap.pid;
	int pagina = estructuraLeerSwap.pagina;
	int marco = -1;

	//Actualiza accesos
	accesos[pid]++;
	log_info(logMemoria, "PROCESO %d - LEER PAGINA %d ", pid, pagina);
	switch (configMemoria.tlbHabilitada) {
	case 1:
		resultadoBusquedaTLB = buscarEnLaTLB(pid, pagina);
		if (resultadoBusquedaTLB >= 0) //CASO VERDADERO
				{
			leerFrame(resultadoBusquedaTLB, pid, pagina, socketCPU, 1);
		} else {
			int resultadoBusquedaTP = buscarEnTablaDePaginas(pid, pagina);
			if (resultadoBusquedaTP >= 0) {
				leerFrame(resultadoBusquedaTP, pid, pagina, socketCPU, 0);
			} else {
				//Actualiza Fallos
				fallos[pid]++;
				if (list_size(busquedaListaFramesPorPid(pid))
						< configMemoria.maximoMarcosPorProceso) {
					marco = -1;
					marco = AsignarFrameAlProceso(pid, pagina, 0);
				}
				char * contenidoPedidoAlSwap = pedirContenidoAlSwap(socketSwap,
						pid, pagina, socketCPU);
				AsignarContenidoALaPagina(pid, pagina, contenidoPedidoAlSwap,
						marco, NOMODIFICADO);
			}
			break;
			case 0:
			resultadoBusquedaTablaPaginas = buscarEnTablaDePaginas(pid, pagina);
			if (resultadoBusquedaTablaPaginas >= 0) {
				leerFrame(resultadoBusquedaTablaPaginas, pid, pagina, socketCPU,
						0);
			} else {
				//Actualiza Fallos
				fallos[pid]++;
				if (list_size(busquedaListaFramesPorPid(pid))
						< configMemoria.maximoMarcosPorProceso) { //TODO funcion para que cuent
					marco = -1;
					marco = AsignarFrameAlProceso(pid, pagina, 0);
				}
				char * contenidoPedidoAlSwap = pedirContenidoAlSwap(socketSwap,
						pid, pagina, socketCPU);
				AsignarContenidoALaPagina(pid, pagina, contenidoPedidoAlSwap,
						marco, NOMODIFICADO);

				break;
			}

		}
	}
}

void BorrarEstructuras(int PID) {
	int posicion = 0;

	while (posicion < list_size(listaDePidFrames)) {
		t_pidFrame * pidFrame = list_get(listaDePidFrames, posicion);
		liberarFrame(pidFrame->frameAsignado);
		if (pidFrame->pid == PID) {
			list_remove(listaDePidFrames, posicion);
		} else {
			posicion++;
		}

	}
	posicion = 0;
	while (posicion < list_size(tablaDePaginas)) {
		t_tablaDePaginas * pidFrame = list_get(tablaDePaginas, posicion);

		if (pidFrame->pid == PID) {
			list_remove(tablaDePaginas, posicion);

		} else {
			posicion++;
		}

	}
}

//CREATE BY MARTIN
t_list * busquedaListaFramesPorPid(int pid) {
	t_list * listaFramesPid = list_create();
	t_pidFrame * pidFrame;
	int contador = 0;
	pidFrame = list_get(listaDePidFrames, contador);
	while (contador < list_size(listaDePidFrames)) {
		if (pidFrame->pid == pid) {
			//	log_info(logMemoria,"FRAME:%d,MODIFICADO: %d, USO: %d \n",pidFrame->frameAsignado, pidFrame->frameModificado,pidFrame->frameUsado);
			list_add(listaFramesPid, pidFrame);
		}
		contador++;
		pidFrame = list_get(listaDePidFrames, contador);
	}

	return listaFramesPid;
}
void imprimirBit(int pid) {
	//t_list * listaFramesPid = list_create();
	t_pidFrame * pidFrame;
	int contador = 0;
	pidFrame = list_get(listaDePidFrames, contador);
	while (contador < list_size(listaDePidFrames)) {
		if (pidFrame->pid == pid) {
			log_info(logMemoria,
					"FRAME:%d,MODIFICADO: %d, USO: %d , PUNTERO %d\n",
					pidFrame->frameAsignado, pidFrame->frameModificado,
					pidFrame->frameUsado, pidFrame->puntero);
			//list_add(listaFramesPid, pidFrame);
		}
		contador++;
		pidFrame = list_get(listaDePidFrames, contador);
	}

}
void procesamientoDeMensajes(int clienteSWAP, int servidorCPU) {
	t_iniciarPID *estructuraCPU = malloc(sizeof(t_iniciarPID));

	t_finalizarPID *finalizarCPU = malloc(sizeof(t_finalizarPID));

	t_leer estructuraLeerSwap;

	t_escribir *estructuraEscribirSwap = malloc(sizeof(t_escribir));

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
			recv(servidorCPU, estructuraEscribirSwap, sizeof(t_escribir), 0);
			escribir(estructuraEscribirSwap, clienteSWAP);

			serializarEstructura(OK, NULL, 0, servidorCPU);
			free(estructuraEscribirSwap);
			pthread_mutex_unlock(&mutexEscribir);
			break;
		case FINALIZAR:
			pthread_mutex_lock(&mutexFinalizar);
			log_info(logMemoria, "FINALIZAR!");

			recv(servidorCPU, finalizarCPU, sizeof(t_finalizarPID), 0);

			log_info(logMemoria, "FINALIZAR PID: %d\n", finalizarCPU->pid);
			log_info(logMemoria, "TOTAL ACCESOS: %d - FALLOS: %d \n",
					accesos[finalizarCPU->pid], fallos[finalizarCPU->pid]);
			//fflush(stdout);

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
			BorrarEstructuras(finalizarCPU->pid);
			free(finalizarCPU);
			pthread_mutex_unlock(&mutexFinalizar);
			break;
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
//Frame por frame.
	char * frameContenido = malloc(configMemoria.tamanioMarcos + 1);
	//el +1 es para agregarlo como cadena
	log_info(logMemoria, "VOLCADO DE MEMORIA \n ");
	int i = 0;

	for (i = 0; i <= configMemoria.cantidadDeMarcos; i++) {
		pthread_mutex_lock(&MEMORIAPPAL);
		memcpy(frameContenido,
				memoriaReservadaDeMemPpal + (i * configMemoria.tamanioMarcos),
				configMemoria.tamanioMarcos);
		pthread_mutex_unlock(&MEMORIAPPAL);
		frameContenido[configMemoria.tamanioMarcos + 1] = '\0';
		log_info(logMemoria, "FRAME: %d - CONTENIDO: %s ", i, frameContenido);

	}

	free(frameContenido);
}
sigset_t senialAEnMascarar;
sigset_t senialOriginal;
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

	sigemptyset(&senialAEnMascarar);
	sigaddset(&senialAEnMascarar, SIGUSR1);
	sigaddset(&senialAEnMascarar, SIGUSR2);
	sigaddset(&senialAEnMascarar, SIGPOLL);

	sigprocmask(SIG_BLOCK, &senialAEnMascarar, &senialOriginal);
	FD_SET(socketservidor, &master);
	fdmax = socketservidor;

	for (;;) {
		read_fds = master;
		if (pselect(fdmax + 1, &read_fds, NULL, NULL, NULL, NULL) == -1) {
			perror("Error en el Select");
		}
		log_info(logMemoria, "Select Activado\n");

		for (i = 0; i <= fdmax; i++) {
			//		signal(senhal,atenderSeniales);
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

//	exit(0);		//el descriptor del cliente seleccionado
}
/*FUNCIONES DE ALGORITMO FIFO!*/

int algoritmoFIFO(int pid) {
	return llamar(pid);

}

int algoritmoClockModificado(int pid) {
	t_list * listaParaAlgoritmo = list_create();
	listaParaAlgoritmo = busquedaListaFramesPorPid(pid);
	imprimirBit(pid);
	//return ejecutarAlgoritmoClock(pid, listaParaAlgoritmo);
	return ejecutarClockModificado(listaParaAlgoritmo, pid);

}

int ejecutarClockModificado(t_list * listaAUtilizar, int pid) {
	int posicion = busquedaPosicionContinuarClock(listaAUtilizar); //// ESTA FUNCION ME DEVUELVE LA ULTIMA POSICION UTILIZADA CON EL ALGORITMO, SE DEBE IR A LA POSICION SIGUIENTE

	if (posicion + 1 == list_size(listaAUtilizar)) {
		posicion = 0;
	} else {
		posicion = posicion + 1;
	}

	int posicionFinal = posicion; // REALIZAMOS ESTA ASIGNACION PARA SABER CUANDO TERMINA LA PRIMER VUELTA DEL ALGORITMO
	setearPuntero(pid); // funcion para poner el puntero en 0 de ese pid en la listaPidFrames
	int primeraVuelta = 1; //VARIABLE UTILIZADA PARA SABER SI BUSCAR POR MODIFICADO 0 O UN 1 ( PRIMERAVUELTA = 1 SE BUSCA POR 0, PRIMERAVUELTA = 0 SE BUSCA POR 1 )

	t_pidFrame *frameAlgoritmo;
	frameAlgoritmo = list_get(listaAUtilizar, posicion);

	while (posicion < list_size(listaAUtilizar)) {

		/*
		 3 SITUACIONES POSIBLES
		 1: FRAME USADO EN 0 Y MODIFICADO EN 0 (sin poner el usado en 0)
		 2: FRAME USADO EN 0 Y MODIFICADO EN 1: (poninedo el usado en 0)
		 3; FRAME USADO EN 1
		 */

		if (frameAlgoritmo->frameUsado == 0) { //CONTROLA QUE EMPIECE CON FRAME USADO EN 0 PARA HACER TODO LO SIGUIENTE:
			if (primeraVuelta >= 1) { //se busca por condicion (0,0)
				if (frameAlgoritmo->frameModificado == 0) {
					return frameAlgoritmo->frameAsignado; //ENCONTRO LA VICTIMA, LA DEVUELVO

				}
				if (frameAlgoritmo->frameModificado == 1) {
					//BUSCO LA SIGUIENTE POSIBLE VICTIMA
					if (posicion + 1 == list_size(listaAUtilizar)) //LLEGUE AL FINAL DE LA LISTA, VUELVO AL COMIENZO
							{
						posicion = 0;
						frameAlgoritmo = list_get(listaAUtilizar, posicion);
						if (posicion == posicionFinal) //consulto si ya di una vuelta completa
								{
							if (primeraVuelta == 1) {
								primeraVuelta = 0;
							} else {
								primeraVuelta++;
							}

						}

					} else {
						posicion++;
						frameAlgoritmo = list_get(listaAUtilizar, posicion);
						if (posicion == posicionFinal) //consulto si ya di una vuelta completa
								{
							if (primeraVuelta == 1) {
								primeraVuelta = 0;
							} else {
								primeraVuelta++;
							}
						}
					}

				}

			}
			if (primeraVuelta == 0) { //se busca condicion (0,1)
				if (frameAlgoritmo->frameModificado == 1) {
					return frameAlgoritmo->frameAsignado; //ENCONTRO LA VICTIMA, LA DEVUELVO

				}
				if (frameAlgoritmo->frameModificado == 0) {
					//BUSCO LA SIGUIENTE POSIBLE VICTIMA
					if (posicion + 1 == list_size(listaAUtilizar)) //LLEGUE AL FINAL DE LA LISTA, VUELVO AL COMIENZO
							{
						posicion = 0;
						frameAlgoritmo = list_get(listaAUtilizar, posicion);
						if (posicion == posicionFinal) //consulto si ya di una vuelta completa
								{
							primeraVuelta = 1;
						}

					} else {
						posicion++;
						frameAlgoritmo = list_get(listaAUtilizar, posicion);
						if (posicion == posicionFinal) //consulto si ya di una vuelta completa
								{
							primeraVuelta = 1;
						}
					}

				}

			}

		}

		if (frameAlgoritmo->frameUsado == 1) {
			if (primeraVuelta < 1) {
				frameAlgoritmo->frameUsado = 0;
				actualizarFrameUsado(frameAlgoritmo->pid,
						frameAlgoritmo->frameAsignado);
			}

			if (posicion + 1 == list_size(listaAUtilizar)) //LLEGUE AL FINAL DE LA LISTA, VUELVO AL COMIENZO
					{
				posicion = 0;

				frameAlgoritmo = list_get(listaAUtilizar, posicion);
				if (posicion == posicionFinal) //consulto si ya di una vuelta completa
						{
					if (primeraVuelta > 0) { //ME FIJO QUE SITUACION ESTÁ Y CUAL DEBE SEGUIR

						primeraVuelta = 0;
					} else {
						primeraVuelta = 1;
					}

				}
			} else {
				posicion++;
				frameAlgoritmo = list_get(listaAUtilizar, posicion);
				if (posicion == posicionFinal) //consulto si ya di una vuelta completa
						{
					if (primeraVuelta > 0) {
						primeraVuelta = 0;
					} else {
						primeraVuelta = 1;
					}
				}
			}
		}

	}

	return frameAlgoritmo->frameAsignado;

}

void actualizarFrameUsado(int pid, int frame) {
	int posicion = 0;
	t_pidFrame * frameBusqueda;
	frameBusqueda = list_get(listaDePidFrames, posicion);

	while (posicion < list_size(listaDePidFrames)) {

		if (posicion + 1 == list_size(listaDePidFrames)) {
			break;
		}
		if (frameBusqueda->frameAsignado == frame
				&& frameBusqueda->pid == pid) {

			frameBusqueda->frameUsado = 0;
			break;
		} else {
			posicion++;
			frameBusqueda = list_get(listaDePidFrames, posicion);
		}

	}

}

int busquedaPosicionContinuarClock(t_list *listaBusqueda) {
	int posicion = 0;
	t_pidFrame * frameBusqueda;
	frameBusqueda = list_get(listaBusqueda, posicion);
	while (posicion < list_size(listaBusqueda)) {
		if (posicion + 1 == list_size(listaBusqueda)) {
			return -1;
		}
		if (frameBusqueda->puntero == 1) {

			return posicion;

		} else {
			posicion++;
			frameBusqueda = list_get(listaBusqueda, posicion);
		}

	}

	return -1;
}

void setearPuntero(int pid) {
	int posicion = 0;
	t_pidFrame * frameBusqueda;
	frameBusqueda = list_get(listaDePidFrames, posicion);

	while (posicion < list_size(listaDePidFrames)) {

		if (posicion + 1 == list_size(listaDePidFrames)) {
			break;
		}
		if (frameBusqueda->puntero == 1 && frameBusqueda->pid == pid) {

			frameBusqueda->puntero = 0;
			posicion++;
			frameBusqueda = list_get(listaDePidFrames, posicion);

		} else {
			posicion++;
			frameBusqueda = list_get(listaDePidFrames, posicion);
		}

	}
}

int CantidadDeFrames(int pid) {
	int cantidadDeFrames = 0;
	int posicion = 0;
	t_list * pf = list_create();
	t_list * listaADestruir = list_create();

	list_add_all(pf, listaDePidFrames);

	while (posicion < list_size(listaDePidFrames)) {
		t_pidFrame * pfr = malloc(sizeof(t_pidFrame));

		pfr = list_get(pf, posicion);
		list_add(listaADestruir, pfr);
		if (pfr != NULL && pfr->pid == pid) {
			cantidadDeFrames++;
			posicion++;
		} else
			posicion++;
	}

	list_destroy(pf);

	log_info(logMemoria,
			"Cantidad actual de frames asignados al proceso %d  : %d \n", pid,
			cantidadDeFrames);

	return cantidadDeFrames;
}

void escribir(t_escribir * estructuraEscribir, int socketSwap) {
	int resultadoBusquedaTLB, resultadoBusquedaTP;
	int pid = estructuraEscribir->pid;
	int pagina = estructuraEscribir->pagina;
	log_info(logMemoria, "ESCRIBIR PAGINA %d  - PROCESO %d \n", pagina, pid);
	accesos[pid]++;
	switch (configMemoria.tlbHabilitada) {
	case 1:
		resultadoBusquedaTLB = buscarEnLaTLB(pid, pagina);
		if (resultadoBusquedaTLB >= 0) //resultadoBusquedaTLB = frame
				{

			escribirContenido(estructuraEscribir, resultadoBusquedaTLB, 1);

		} else {
			int resultadoBusquedaTP = buscarEnTablaDePaginas(pid, pagina); //resultadoBusquedaTP = frame
			if (resultadoBusquedaTP >= 0) {
				escribirContenido(estructuraEscribir, resultadoBusquedaTP, 1);
			} else {
				//Actualiza Fallos
				fallos[pid]++;
				escribirContenidoSwap(estructuraEscribir, socketSwap);
			}
		}
		break;
	case 0:
		resultadoBusquedaTP = buscarEnTablaDePaginas(pid, pagina); //resultadoBusquedaTP = frame
		if (resultadoBusquedaTP >= 0) {
			escribirContenido(estructuraEscribir, resultadoBusquedaTP, 1);
		} else {
			//Actualiza Fallos
			fallos[pid]++;
			escribirContenidoSwap(estructuraEscribir, socketSwap);
			break;
		}

	}
}

void escribirContenido(t_escribir * estructEscribir, int frame, int bit) {
	int posicion = busquedaPIDEnLista(estructEscribir->pid,
			estructEscribir->pagina);

	/*char * contenido = malloc(configMemoria.tamanioMarcos + 1);
	 memcpy(contenido,estructEscribir->contenidoPagina,configMemoria.tamanioMarcos );
	 contenido[configMemoria.tamanioMarcos] = '\0';*/

	if (posicion > 0) {
		t_tablaDePaginas * tp = malloc(sizeof(t_tablaDePaginas));
		tp = list_get(tablaDePaginas, posicion);
		if (bit <= 1) {
			tp->bitModificado = bit;
		}

		if (bit <= 1) {
			time_t t = time(NULL);
			int ppf = busquedaPIDFramePorFrame(tp->marco);
			if (ppf != -1) {
				t_pidFrame * pf = list_get(listaDePidFrames, ppf);
				pf->ultimaReferencia = t;
				pf->frameAsignado = frame;
				pf->frameModificado = 1;
				pf->frameUsado = 1;

			} else if (list_size(
					busquedaListaFramesPorPid(estructEscribir->pid))
					< configMemoria.maximoMarcosPorProceso) {
				t_pidFrame *pf = malloc(sizeof(t_pidFrame));
				pf->frameAsignado = frame;
				pf->pid = estructEscribir->pid;
				pf->puntero = 0;
				pf->ultimaReferencia = t;
				pf->frameModificado = bit;
				pf->frameUsado = 1;
				list_add(listaDePidFrames, pf);
			}
		}

		//list_replace(tablaDePaginas, posicion, (void *)tp);
		//TODO BUSCAR EL FRAME PARA ESE PID Y METERLE EL MODIFICADO EN 1
		/* t_pidFrame * pidFrame;
		 pidFrame = list_get(listaDePidFrames,busquedaListaFrame(tp->pid,tp->marco));
		 if (pidFrame != -1){
		 pidFrame->frameModificado = 1;
		 }*/
		//memcpy(direccion, contenido,configMemoria.tamanioMarcos);
		int offset = frame * configMemoria.tamanioMarcos;

		strncpy(memoriaReservadaDeMemPpal + offset,
				estructEscribir->contenidoPagina, configMemoria.tamanioMarcos);
	}
}

int busquedaListaFrame(int pid, int frame) {
	t_pidFrame * pidFrame;
	int contador = 0;
	int posicion = -1;
	pidFrame = list_get(listaDePidFrames, contador);
	while (contador < list_size(listaDePidFrames)) {
		if (pidFrame->pid == pid && pidFrame->frameAsignado == frame) {
			posicion = contador;
			return posicion;
		} else
			contador++;
		list_get(listaDePidFrames, contador);
	}

	return posicion;
}

char * pedirLecturaAlSwapEscribir(int cliente, int pid, int pagina) {
	log_info(logMemoria, "INICIO PEDIDO AL SWAP");
	int tamanioLeido;
	t_leer * estructuraLeerSwap = malloc(sizeof(t_leer));
	estructuraLeerSwap->pid = pid;
	estructuraLeerSwap->pagina = pagina;
	serializarEstructura(LEER, estructuraLeerSwap, sizeof(t_leer), cliente);
	recv(cliente, &tamanioLeido, sizeof(int), 0);

	char* contenidoLeido = malloc(tamanioLeido);

	recv(cliente, contenidoLeido, tamanioLeido, 0);

	log_info(logMemoria, "Contenido: %s", contenidoLeido);
	free(estructuraLeerSwap);
	return contenidoLeido;
}

void escribirContenidoSwap(t_escribir * estructEscribir, int socketSwap) {
	int posicion = busquedaPIDEnLista(estructEscribir->pid,
			estructEscribir->pagina);
	int marco = -1;

	if (posicion >= 0) {
		t_tablaDePaginas * tp = malloc(sizeof(t_tablaDePaginas));
		tp = list_get(tablaDePaginas, posicion);
		tp->bitModificado = 1;
		tp->presencia = 1;
		tp->bitUso = 1;
		//	list_replace(tablaDePaginas, posicion, tp);

		//TODO BUSCAR EL FRAME PARA ESE PID Y METERLE EL MODIFICADO EN 1
		/*	 t_pidFrame * pidFrame;
		 pidFrame = list_get(listaDePidFrames,busquedaPIDFrame(tp->pid,tp->marco));
		 if (tp->marco != -1){

		 pidFrame->frameModificado = 1;
		 pidFrame->frameAsignado = tp->marco;
		 pidFrame->ultimaReferencia = t;
		 }*/

		char * contenido = pedirLecturaAlSwapEscribir(socketSwap,
				estructEscribir->pid, estructEscribir->pagina);
		contenido = estructEscribir->contenidoPagina;

		if (list_size(busquedaListaFramesPorPid(estructEscribir->pid))
		//	<= configMemoria.maximoMarcosPorProceso) {
				< configMemoria.maximoMarcosPorProceso) {
			marco = AsignarFrameAlProceso(estructEscribir->pid,
					estructEscribir->pagina, 1);
		}
		AsignarContenidoALaPagina(estructEscribir->pid, estructEscribir->pagina,
				contenido, marco, 1);

	}
}

int llamar(int pid) {
	int i;
	int cantidadDeFramesDelPid = CantidadDeFrames(pid);

	for (i = 0; i < cantidadDeFramesDelPid; i++) {
		if (i > 5) {
			return ultimoFrameAsignado;
		}
		return colaParaReemplazo(i, cantidadDeFramesDelPid, pid);	//i:frame
	}
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

	return busquedaPIDFrame(pid);

}

int colaParaReemplazo(int frameAReemplazar, int cantidadDeFrames, int pid) {
	int posicion;
	time_t t = time(NULL);
	t_pidFrame * pf = malloc(sizeof(t_pidFrame));
	t_pidFrame * pidframe = malloc(sizeof(t_pidFrame));
	posicion = ObtenerPrimerFrame(pid);
	int i = 0;

	while (i < list_size(listaDePidFrames)) {
		t_pidFrame* lp = malloc(sizeof(t_pidFrame));
		lp = list_get(listaDePidFrames, i);
		i++;
	}

	pf = list_remove(listaDePidFrames, posicion);

	pidframe->frameAsignado = pf->frameAsignado;
	pidframe->frameUsado = 1;
	pidframe->frameModificado = 0; //NECESARIO PARA CLOCK
	pidframe->pid = pid;
	pidframe->puntero = 0; //NECESARIO PARA SABER DONDE CONTINUAR EN ALGORITMO CLOCK
	pidframe->ultimaReferencia = t; //SE USA EN EL LRU
	list_add(listaDePidFrames, pidframe);

	log_info(logMemoria, "Frame asignado: %d, FRAME GRABADO: %d",
			pf->frameAsignado, pidframe->frameAsignado);
	int frameADevolver = pf->frameAsignado;
	return frameADevolver;
}

int algoritmoLRU(int pid) {
	t_list * listaParaAlgoritmo = list_create();
	pthread_mutex_lock(&mutexFrame);
	listaParaAlgoritmo = busquedaListaFramesPorPid(pid);
	pthread_mutex_unlock(&mutexFrame);
	int frame = ejecutarlru(pid, listaParaAlgoritmo);
//list_destroy(listaParaAlgoritmo);
	return frame;
}

int ejecutarlru(int pid, t_list * listaParaAlgoritmo) {
//time_t t = time(NULL);
//int posicion = busquedaPosicionAlgoritmoLRU(listaParaAlgoritmo); //BUSCO DESDE DONDE CONTINUAR CON EL ALGORITMO
	ordenarLista(listaParaAlgoritmo);
	int i = 0;
	printf("ORDENAMIENTO \n");
	while (i < list_size(listaParaAlgoritmo)) {
		t_pidFrame * pf = list_get(listaParaAlgoritmo, i);

		printf("Poscion: %d, FRAME: %d \n", i, pf->frameAsignado);
		fflush(stdout);
		i++;
	}

	sleep(2);

	t_pidFrame * frameAReemplazar = malloc(sizeof(t_pidFrame));
	if (list_size(listaParaAlgoritmo) > 0) {
		time_t t = time(NULL);
		frameAReemplazar = list_remove(listaParaAlgoritmo, 0);

		frameAReemplazar->frameModificado = 1;
		frameAReemplazar->ultimaReferencia = t;

		return frameAReemplazar->frameAsignado;
	} else {

		return 0;

	}
}

//void * ordenarLista(t_list * listaParaAlgoritmo) {
/*	int j = 0;
 int i = 0;
 t_pidFrame * temporal;
 t_pidFrame * primeraPosicion;
 primeraPosicion = list_get(listaParaAlgoritmo, 0);
 t_pidFrame * segundaPosicion = list_get(listaParaAlgoritmo, 1);
 while (i < list_size(listaParaAlgoritmo)) {
 while (j < list_size(listaParaAlgoritmo) - 1) {
 if (primeraPosicion->ultimaReferencia.tm_min
 > segundaPosicion->ultimaReferencia.tm_min
 || primeraPosicion->ultimaReferencia.tm_sec
 > segundaPosicion->ultimaReferencia.tm_sec) {
 temporal = list_get(listaParaAlgoritmo, j);
 primeraPosicion = list_get(listaParaAlgoritmo, j + 1);
 list_replace(listaParaAlgoritmo, j+1, primeraPosicion);
 list_replace(listaParaAlgoritmo, j, temporal);
 }
 j++;
 primeraPosicion = list_get(listaParaAlgoritmo, j);
 segundaPosicion = list_get(listaParaAlgoritmo, j + 1);

 }
 i++;
 }
 */

//}
void * ordenarLista(t_list * listaParaAlgoritmo) {
	bool _ordenamiento_porHorario(t_pidFrame* frameBusqueda,
			t_pidFrame* otroFrameBusqueda) {
		return frameBusqueda->ultimaReferencia
				< otroFrameBusqueda->ultimaReferencia;
	}
	list_sort(listaParaAlgoritmo, (void*) _ordenamiento_porHorario);
	return NULL;
}
int ejecutarAlgoritmo(int pid) {

	int frame;
	switch (configMemoria.algoritmoReemplazo) {
	case 1: //FIFO
		//return algoritmoFIFO(pid);
		frame = algoritmoFIFO(pid);
		break;
	case 2: //"LRU"
		//return algoritmoLRU(pid);
		frame = algoritmoLRU(pid);
		break;
	case 3:   //"CLOCK_MODIFICADO"
		//return algoritmoClockModificado(pid);

		frame = algoritmoClockModificado(pid);
		break;
	default:
		return -1;
	}
	verificarPaginaAReemplazar(frame);
	return frame;
}
void verificarPaginaAReemplazar(int frame) {
//Busca ese frame para ver a que pagina esta asignado
	int posicion = 0;
	t_escribir *estructuraEscribirSwap = malloc(sizeof(t_escribir));
	t_mensajeHeader mensajeHeaderSwap;
//Busca la pagina que tenga el frame
	t_tablaDePaginas* pag = list_get(tablaDePaginas, posicion);
	while (pag->marco != frame || (pag->marco == frame && pag->presencia != 1)) {

		posicion++;
		if (posicion == list_size(tablaDePaginas)) {
			posicion = -1; //termino de recorrer y no encontro
			break;
		}
		pag = list_get(tablaDePaginas, posicion);

	}
	if (posicion != -1) {
		pag = list_get(tablaDePaginas, posicion);
		pag->presencia = 0;
		pag->bitUso = 0;
		if (pag->bitModificado == MODIFICADO) {

			time_t t = time(NULL);
			int ppf = busquedaPIDFramePorFrame(pag->marco);
			if (ppf != -1) {
				t_pidFrame * pf = list_get(listaDePidFrames, ppf);
				pf->ultimaReferencia = t;
				pf->frameAsignado = frame;
				//pf->frameModificado = 0;
				pf->frameUsado = 1;
			} else if (list_size(busquedaListaFramesPorPid(pag->pid))
					< configMemoria.maximoMarcosPorProceso) {
				t_pidFrame *pf = malloc(sizeof(t_pidFrame));
				pf->frameAsignado = pag->marco;
				pf->pid = pag->pid;
				pf->puntero = 0;
				pf->ultimaReferencia = t;
				//	pf->frameModificado = 1;
				pf->frameUsado = 1;
				list_add(listaDePidFrames, pf);

			}

			//char* contenidoADevolver = buscarContenidoFrame(frame, pag->pid, pag->pagina);
			estructuraEscribirSwap->pid = pag->pid;
			estructuraEscribirSwap->pagina = pag->pagina;

			memcpy(estructuraEscribirSwap->contenidoPagina,
					buscarContenidoFrame(frame, pag->pid, pag->pagina, 0),
					configMemoria.tamanioMarcos);
			serializarEstructura(ESCRIBIR, (void *) estructuraEscribirSwap,
					sizeof(t_escribir), clienteSwap);
			recv(clienteSwap, &mensajeHeaderSwap, sizeof(t_mensajeHeader), 0);
			pag->bitModificado = NOMODIFICADO;
		}
	}
//Actualiza TLB todo
	borrarEnLaTLB(pag->pid, pag->pagina);
	free(estructuraEscribirSwap);
}
void inicializarFrames() {

	log_info(logMemoria, "Inicializa frames \n");
	int x = 0;
	while (x < configMemoria.cantidadDeMarcos) {
		agregarFrame(x);
		x++;
	}

}

void agregarFrame(int frameID) {
	t_frames * frame = malloc(sizeof(t_frames));
	frame->frame = frameID;
	frame->ocupado = LIBRE;
	list_add(frames, frame);
}

t_frames * buscarFrame(int frameId) {
	int posicion = 0;
	t_frames * frame = list_get(frames, posicion);
	while (frame->frame != frameId) {
		posicion++;
		if (posicion == list_size(frames))
			break;
		frame = list_get(frames, posicion);
	}
	if (posicion == list_size(frames))
		frame = list_get(frames, posicion - 1);
	return frame;

}
void liberarFrame(int idFrame) {

	int _is_frame(t_frames *p) {
		return p->frame == idFrame;
	}
	pthread_mutex_lock(&mutexFrames);
	t_frames* frame = buscarFrame(idFrame); //list_find(frames, (void*) _is_frame);
	frame->ocupado = LIBRE; //actualizo su pid para que quede libre la cpu
	pthread_mutex_unlock(&mutexFrames);

}
int seleccionarFrameLibre() {
// Busca el primer frame libre, y lo marca como usado. Retorna -1 si no tiene ningun frame libre
	int _is_frame(t_frames *p) {
		return p->ocupado == LIBRE;
	}
	pthread_mutex_lock(&mutexFrames);
	t_frames* libre = malloc(sizeof(t_frames));
	libre = list_find(frames, (void*) _is_frame);

	pthread_mutex_unlock(&mutexFrames);
	if (libre != NULL) {
		libre->ocupado = OCUPADO;
		log_info(logMemoria, "FRAME LIBRE : %d \n", libre->frame);
		return libre->frame;
	} else {

		return -1;
		log_info(logMemoria, "NO HAY NINGUN FRAME LIBRE : %d \n");
	}

}
void leerFrame(int frame, int pid, int pagina, int socketCPU, int tlbPresente) {
//ANTES DE USAR ESTA FUNCION SE VALIDA QUE EXISTA EN LA TLB, o en la TABLA DE PAGINAS, y despues se busca el contenido en la TABLA de paginas
//porque si esta en la TLB, o en la tabla de paginas, TENGO EL CONTENIDO EN EL ADMINISTRADOR DE MEMORIA, sino debo pedirselo al swap
	char* contenidoADevolver = buscarContenidoFrame(frame, pid, pagina,
			tlbPresente);
	t_tablaDePaginas * paginaAAsignar = malloc(sizeof(t_tablaDePaginas));
	int tamanio = configMemoria.tamanioMarcos;

//aCTUALIZA BITS PAGINA
	int posicion = busquedaPIDEnLista(pid, pagina);
	if (posicion != -1) {
		paginaAAsignar = list_get(tablaDePaginas, posicion);
		paginaAAsignar->pid = pid;
		paginaAAsignar->marco = frame;
		paginaAAsignar->pagina = pagina;
		//paginaAAsignar->bitModificado = 1;
		//paginaAAsignar->bitModificado = NOMODIFICADO; //ver que no lo llame el ecrribit todo
		paginaAAsignar->bitUso = 1;
		paginaAAsignar->bitValidez = 1;
		paginaAAsignar->presencia = 1;

		time_t t = time(NULL);
		int ppf = busquedaPIDFramePorFrame(frame);
		if (ppf != -1) {
			t_pidFrame * pf = list_get(listaDePidFrames, ppf);
			pf->ultimaReferencia = t;
			pf->frameAsignado = frame;
			//	pf->frameModificado = 0;
			pf->frameUsado = 1;
		} else if (list_size(busquedaListaFramesPorPid(pid))
				< configMemoria.maximoMarcosPorProceso) {
			t_pidFrame *pf = malloc(sizeof(t_pidFrame));
			pf->frameAsignado = frame;
			pf->pid = pid;
			pf->puntero = 0;
			pf->ultimaReferencia = t;
			pf->frameModificado = 0;
			pf->frameUsado = 1;
			list_add(listaDePidFrames, pf);

		}

	}
	send(socketCPU, &tamanio, sizeof(configMemoria.tamanioMarcos), 0);
	send(socketCPU, contenidoADevolver, tamanio, 0);

}
char * buscarContenidoFrame(int frame, int pid, int pagina, int tlbPresente) {
//	log_info(logMemoria,
//		"OBTENIENDO CONTENIDO DE LA PAGINA,fue encontrada en la TLB o en la TABLA DE PAGINAS");

//TODO
	if (tlbPresente == 0) {	//Si es 0 no esta presente en la tlb
		AsignarEnTlb(pid, pagina, frame);
	}
	char * contenido = malloc(configMemoria.tamanioMarcos + 1);
	pthread_mutex_lock(&MEMORIAPPAL);
	memcpy(contenido,
			memoriaReservadaDeMemPpal + (frame * configMemoria.tamanioMarcos),
			configMemoria.tamanioMarcos);
	pthread_mutex_unlock(&MEMORIAPPAL);
	contenido[configMemoria.tamanioMarcos] = '\0';

	return contenido;

}
void iniciarFallosYAccesos() {
	int i;
	for (i = 0; i < MAXPROCESOS; i++) {
		fallos[i] = 0;
		accesos[i] = 0;
	}

}

void calcularTasaAciertos(void *ptr) {
	double tasa = 0;
	printf("en el hilo de la tlbbbbbb\n");
	while (true) {
		sleep(60);
		if (accesosTLB != 0) {
			tasa = aciertosTLB *100 / accesosTLB;
			log_info(logMemoria, "Aciertos TLB : %d- Accesos : %d  %\n", aciertosTLB, accesosTLB);
			log_info(logMemoria, "Tasa aciertos TLB : %f  %\n", tasa);
		}
	}

}
void borrarEnLaTLB(int pid, int pagina) {
	int posicion = 0;
//	int encuentra = -1;

	bool buscarPagina(t_TLB * buscarTLB) {
		return buscarTLB->pid == pid && buscarTLB->pagina == pagina;
	}
	int cantidad = list_count_satisfying(tlb, (void*) buscarPagina);
	if (cantidad > 0) {
		//  malloc(sizeof(t_TLB));

		t_TLB * entradaTLB = list_get(tlb, posicion);

		while ((entradaTLB->pagina != pagina || entradaTLB->pid != pid)) {

			posicion++;
			if (posicion == list_size(tlb)){
				posicion = -1; // llego al finak de la lista y no encuentra
			break;
			}
			entradaTLB = list_get(tlb, posicion);

		}
		if (posicion != -1) {
			entradaTLB = list_remove(tlb, posicion);
		}
	}
}
