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
pthread_mutex_t mutexIniciar;
pthread_mutex_t mutexLeer;
pthread_mutex_t mutexEscribir;
pthread_mutex_t mutexFinalizar;
pthread_mutex_t mutexFrame;
pthread_mutex_t mutexSenhal;
pthread_mutex_t mutexTLB;
int ultimoFrameAsignado = 0;
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

void crearListas() {
	tablaDePaginas = list_create();
	estructuraAlgoritmos = list_create();
	tlb = list_create();
	listaDePidFrames = list_create();
	estructurasPorProceso = list_create();
}

int main() {
	remove("logMemoria.txt"); //Cada vez que arranca el proceso borro el archivo de log.
	logMemoria = log_create("logMemoria.txt", "Administrador de memoria", true,
			LOG_LEVEL_INFO);
	leerConfiguracion();

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
		printf("La TLB esta habilitada, se procede a su creación");
		log_info(logMemoria, "Inicio creacion de la TLB");
		generarTLB(configMemoria->entradasTLB);
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
	t_estructurasDelProceso estructuraDelProceso;
	log_info(logMemoria, "INICIO TABLAS DE PAGINAS");
	int pagina = 0;

	while (pagina < cantidadDePaginas) {
		t_tablaDePaginas * entrada = malloc(sizeof(t_tablaDePaginas));
		printf("PID   PAGINA   BitUso   BitModificado   FRAME\n");
		entrada->bitModificado = 1;
		entrada->bitUso = 1;
		entrada->pagina = pagina;
		entrada->pid = pid;
		entrada->contenido = NULL;
		entrada->marco = -1;
		printf(" %d      %d       %d            %d         %d\n", entrada->pid,
				entrada->pagina, entrada->bitUso, entrada->bitModificado,
				entrada->marco);
		fflush(stdout);
		list_add(tablaDePaginas, entrada);
		pagina++;
	}
	estructuraDelProceso.pid = pid;
	estructuraDelProceso.estructura = tablaDePaginas;
	list_add(estructurasPorProceso, &estructuraDelProceso);
	log_info(logMemoria, "FIN TABLAS DE PAGINAS");
}

void agregarAEstructuraGeneral(t_estructurasDelProceso* estructurasDelProceso,
		int pid) {
	estructurasDelProceso->pid = pid;
	estructurasDelProceso->estructura = listaDePidFrames;
	list_add(estructurasPorProceso, &*estructurasDelProceso);
}

void generarCantidadDeFramesAsignadosAlProceso(int pid, int cantidadDePaginas) {
	t_estructurasDelProceso estructurasDelProceso;
	bool yaSeAsignoPid(t_pidFrame * estructuraPidFrame) {
		return estructuraPidFrame->pid == pid;
	}
	pthread_mutex_lock(&mutexFrame);
	bool CantidadDePaginasDelProceso(t_tablaDePaginas * tablaDePaginas) {
		return tablaDePaginas->pid == pid;
	}
	if (list_count_satisfying(listaDePidFrames, (void*) yaSeAsignoPid)
			< configMemoria.maximoMarcosPorProceso) { //SI YA LE ASIGNE TODOS LOS frames posibles no le asigno ninguno mas

			t_pidFrame * estructuraPidFrame = malloc(sizeof(t_pidFrame));
			if ((configMemoria.cantidadDeMarcos > ultimoFrameAsignado) && (list_count_satisfying(listaDePidFrames, (void*) yaSeAsignoPid)
					< configMemoria.maximoMarcosPorProceso)) {
				estructuraPidFrame->frameAsignado = ultimoFrameAsignado;
				estructuraPidFrame->pid = pid;
				estructuraPidFrame->frameUsado = 0; //0 SIN USAR, 1 USADO.
				list_add(listaDePidFrames, estructuraPidFrame);
				log_info(logMemoria, "frame asignado: %d al pid:%d",
						ultimoFrameAsignado, pid);
				/*if (paginaAsignada
						< list_count_satisfying(tablaDePaginas,
								(void *) CantidadDePaginasDelProceso)) {
					generarEstructuraParaAlgoritmos(pid, ultimoFrameAsignado,
							paginaAsignada);*/
					ultimoFrameAsignado++;
				}


		//	paginaAsignada++;
			free(estructuraPidFrame);
		}
		agregarAEstructuraGeneral(&estructurasDelProceso, pid);
		printf("Cantidad de frames asignados: %d \n", ultimoFrameAsignado);
		fflush(stdout);
		pthread_mutex_unlock(&mutexFrame);
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

		log_error(logMemoria,
				"Proceso %d rechazado por falta de espacio en SWAP\n",
				estructuraCPU->pid);

		serializarEstructura(PROCFALLA, NULL, 0, servidor);

	}
}

void generarEstructuraParaAlgoritmos(int pid, int frameAsignado,
		int paginaAsignada) {
	log_info(logMemoria, "Inicio creacion estructura para algoritmos");
	int cantidadDeFrames = 0;

	printf("FRAME  PID  PAGINA\n");
	while (cantidadDeFrames == 0) {
		t_estructuraAlgoritmoReemplazoPaginas * armadoEstruct = malloc(
				sizeof(t_estructuraAlgoritmoReemplazoPaginas));
		armadoEstruct->frame = frameAsignado;
		armadoEstruct->pid = pid;
		armadoEstruct->pagina = paginaAsignada;
		printf("  %d   %d   %d\n", armadoEstruct->frame, armadoEstruct->pid,
				armadoEstruct->pagina);
		fflush(stdout);
		list_add(estructuraAlgoritmos, armadoEstruct);
		cantidadDeFrames++;

		free(armadoEstruct);
	}
	log_info(logMemoria, "FIN creacion estructura para algoritmos");
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

int busquedaPIDEnLista(int PID, int pagina) {
	int posicion = 0;
	t_tablaDePaginas* pag;
	pag = list_get(tablaDePaginas, posicion);
	if(pag->pagina != pagina) {
		if (pag->pid == PID) {
			if (posicion == list_size(tablaDePaginas))
				return -1;
			posicion++;
			pag = list_get(tablaDePaginas, posicion);
		}
	}
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
	return posicion;
}

int buscarEnTablaDePaginas( pid, pagina) {
	log_info(logMemoria, "INICIO BUSQUEDA EN TABLA DE PAGINAS");
	bool buscarEnPaginaEnTP(t_tablaDePaginas * tablaDePaginas) {
		return tablaDePaginas->pid == pid && tablaDePaginas->pagina == pagina;
	}
	bool verificarBitValidezYPresencia(
			t_tablaDePaginas * tablaDePaginasEncontradas) {
		return tablaDePaginasEncontradas->bitValidez == 0
				&& tablaDePaginasEncontradas->presencia == 1
				&& string_is_empty(tablaDePaginasEncontradas->contenido);
	}
	log_info(logMemoria, "FIN CLOSURE");
	t_list * paginasEncontradas = list_create();
	int posicion = busquedaPIDEnLista(pid, pagina);
	paginasEncontradas = list_get(tablaDePaginas, posicion);
	log_info(logMemoria, "Cantidad de elementos: %d",
			paginasEncontradas->elements_count);
	if ((paginasEncontradas->elements_count) == 1) {

		log_info(logMemoria, "PAGINA ENCONTRADA EN TABLA DE PAGINAS");
		return list_all_satisfy(tablaDePaginas,
				(void *) verificarBitValidezYPresencia);
	} else {

		log_info(logMemoria, "PAGINA NO ENCONTRADA EN TABLA DE PAGINAS");
		return -1; // si la pagina para el pid NO es unica retorno falso
	}
}

char * buscarContenidoEnTablaDePaginas(int pid, int pagina) {
	log_info(logMemoria,
			"OBTENIENDO CONTENIDO DE LA PAGINA,fue encontrada en la TLB o en la TABLA DE PAGINAS");
	bool buscarEnPaginaEnTP(t_tablaDePaginas * tablaDePaginas) {
		return tablaDePaginas->pid == pid && tablaDePaginas->pagina == pagina;
	}
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

void buscarContenidoPagina(int pid, int pagina, int socketCPU) {
	//ANTES DE USAR ESTA FUNCION SE VALIDA QUE EXISTA EN LA TLB, o en la TABLA DE PAGINAS, y despues se busca el contenido en la TABLA de paginas
	//porque si esta en la TLB, o en la tabla de paginas, TENGO EL CONTENIDO EN EL ADMINISTRADOR DE MEMORIA, sino debo pedirselo al swap
	t_contenido* contenidoCPU = malloc(sizeof(t_contenido));
	char* contenidoADevolver = buscarContenidoEnTablaDePaginas(pid, pagina);
	contenidoCPU->tamanio = sizeof(contenidoADevolver);
	contenidoCPU->contenido = contenidoADevolver;
	serializarEstructura(LEER, contenidoCPU, sizeof(contenidoCPU), socketCPU);
	//free(contenidoCPU);
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
	log_info(logMemoria, "FIN PEDIDO AL SWAP");
	contenidoLeido[tamanioLeido] = '\0';

	send(servidor, &tamanioLeido, sizeof(tamanioLeido), 0);
	send(servidor, contenidoLeido, tamanioLeido, 0);

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
	bool buscarEnPaginaEnTP(t_tablaDePaginas * tablaDePaginas) {
		return tablaDePaginas->pid == pid && tablaDePaginas->pagina == pagina;
	}
	generarCantidadDeFramesAsignadosAlProceso(pid, pagina);
	t_tablaDePaginas * paginaAAsignar = malloc(sizeof(t_tablaDePaginas));
	int posicion = busquedaPIDEnLista(pid, pagina);
	paginaAAsignar = list_get(tablaDePaginas, posicion);
	paginaAAsignar->contenido = contenidoPedidoAlSwap;
	paginaAAsignar->bitModificado = 1;
	paginaAAsignar->bitUso = 1;
	paginaAAsignar->bitValidez = 1;
	paginaAAsignar->presencia = 1;
	if (ultimoFrameAsignado == configMemoria.maximoMarcosPorProceso) {
		log_warning(logMemoria,
				"EJECUTAR ALGORITMO DE REEMPLAZO DE PAGINAS \0 ACTUALIZACION DE FRAMES \0 ACTUALIZACION DE TLB \n");
		algoritmoFifo(pid, paginaAAsignar->pagina);
		//paginaAAsignar->marco = -1; //ASIGNO INVALIDO POR AHORA
	} else
		paginaAAsignar->marco = ultimoFrameAsignado;

	list_add(tablaDePaginas, paginaAAsignar);

}

void leerPagina(t_leer estructuraLeerSwap, int socketSwap, int socketCPU,
		t_mensajeHeader mensajeHeaderSwap) {
	int resultadoBusquedaTablaPaginas;
	int resultadoBusquedaTLB;
	int pid = estructuraLeerSwap.pid;
	int pagina = estructuraLeerSwap.pagina;

	switch (configMemoria.tlbHabilitada) {
	case 1:
		resultadoBusquedaTLB = buscarEnLaTLB(pid, pagina);
		if (resultadoBusquedaTLB == 0) //CASO VERDADERO
				{
			buscarContenidoPagina(pid, pagina, socketCPU);
		} else {
			int resultadoBusquedaTP = buscarEnTablaDePaginas(pid, pagina);
			if (resultadoBusquedaTP == 1) {
				buscarContenidoPagina(pid, pagina, socketCPU);
			} else {
				char * contenidoPedidoAlSwap = pedirContenidoAlSwap(socketSwap,
						pid, pagina, socketCPU);
				AsignarContenidoALaPagina(pid, pagina, contenidoPedidoAlSwap);
			}
		}
		break;
	case 0:
		resultadoBusquedaTablaPaginas = buscarEnTablaDePaginas(pid, pagina);
		if (resultadoBusquedaTablaPaginas == 0) {
			buscarContenidoPagina(pid, pagina, socketCPU);
		} else {
			char * contenidoPedidoAlSwap = pedirContenidoAlSwap(socketSwap, pid,
					pagina, socketCPU);
			AsignarContenidoALaPagina(pid, pagina, contenidoPedidoAlSwap);
		}
		break;
	}
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

	printf("mensaje recibido: %d", mensajeHeader.idmensaje);
	fflush(stdout);

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

			/*generarEstructuraAdministrativaPidFrame(estructuraCPU->pid,
			 estructuraCPU->paginas);*/

			//	recv(servidor, estructuraCPU, sizeof(t_iniciarPID), 0);
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
			//send(servidor, contenidoLeido, sizeof(tamanioLeido), 0);

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

			//recv(servidor, finalizarCPU, sizeof(t_finalizarPID), 0);
			//recv(servidor, &finalizarCPU, sizeof(t_finalizarPID), 0);
			recv(servidorCPU, finalizarCPU, sizeof(t_finalizarPID), 0);

			printf("FINALIZAR PID: %d\n", finalizarCPU->pid);

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
			//BORRAR TODAS LAS ESTRUCTURAS ADMINISTRATIVAS PARA ESE mProc.
			pthread_mutex_unlock(&mutexFinalizar);
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


int ServidorCPU(int PUERTO) {
	int socketservidor;
	int yes = 1;
	struct sockaddr_in serveraddr;

	if ((socketservidor = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
		perror("Error en la creacion del Socket");
		return -1;
	}
	printf("Socket Servidor Creado\n");

	if (setsockopt(socketservidor, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int))
			== -1) {
		perror("Error en la definicion del protocolo de comunicacion");
		return -1;
	}
	printf("Protocolo de Comunicacion definido correctamente\n");

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
	printf("Socket Escuchando\n");

	return socketservidor;
}

void RealizarVolcadoMemoriaLog()
{

}

void atenderSeaniales(int senhal)
{

	pid_t pidProcesoClonado;
	switch (senhal) {
		case SIGUSR1:
			pthread_create(hiloSigUsr1,NULL,inicioHiloSigUsr1,NULL);
			pthread_join(*hiloSigUsr1,NULL);
			break;
		case SIGUSR2:
			pthread_create(hiloSigUsr2,NULL,inicioHiloSigUsr2,NULL);
			pthread_join(*hiloSigUsr2,NULL);
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
		printf("Select Activado\n");

		for (i = 0; i <= fdmax; i++) {
	//		signal(senhal,atenderSeaniales);
			if (FD_ISSET(i, &read_fds)) {
				if (i == socketservidor) {
					socklen_t addrlen = sizeof(clientaddr);
					if ((newfd = accept(socketservidor,
							(struct sockaddr *) &clientaddr, &addrlen)) == -1) {
						perror("Error en el Accept");
					} else {
						printf("Socket Aceptado\n");

						FD_SET(newfd, &master);
						if (newfd > fdmax) {
							fdmax = newfd;
						}
						printf("Nueva conexion de %s en el socket %d\n",
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
void algoritmoFifo(int pid, int paginaLeida) {
	log_info(logMemoria, "ALGORTIMO FIFO INICIADO");
	FrameSinUsar(pid, paginaLeida);
}

int pageFault = 0;
void reemplazoDePagina(int pid, int paginaRefenciada,
		t_tablaDePaginas *reemplazo) {

	bool framesDelPID(
			t_estructuraAlgoritmoReemplazoPaginas * estructuraReemplazo) {
		return estructuraReemplazo->pid == pid;
	}
	bool paginasDelPid(t_tablaDePaginas * tablaDePaginas) {
		return tablaDePaginas->pid == pid;
	}
	int cantidadDeFrames = list_count_satisfying(estructuraAlgoritmos,
			(void*) framesDelPID);
	int i;
	for (i = 0; i < cantidadDeFrames; i++) {
		reemplazo = list_get(estructuraAlgoritmos, i);
		reemplazo->marco = i;
		if (reemplazo->pagina == paginaRefenciada) {
			t_tablaDePaginas * reemplazarFrame = malloc(
					sizeof(t_tablaDePaginas));
			reemplazarFrame = list_get(tablaDePaginas, paginaRefenciada);
			reemplazarFrame->marco = i;
			list_replace(tablaDePaginas, paginaRefenciada, reemplazarFrame);
			return;
		}
	}
	if (reemplazo->marco == cantidadDeFrames - 1) {
		reemplazo->marco = 0;
	} else if (reemplazo->marco == -1) {
		reemplazo->marco = 0;
	} else {
		reemplazo->marco = reemplazo->marco + 1;
	}
	reemplazo->pagina = paginaRefenciada;
	pageFault++;
}

pthread_mutex_t mutexFrameSinUsar;
void FrameSinUsar(int pid, int paginaReferenciada) {
	pthread_mutex_lock(&mutexFrameSinUsar);
	t_tablaDePaginas * reemplazo = malloc(sizeof(t_tablaDePaginas));
	bool framesDelPID(
			t_estructuraAlgoritmoReemplazoPaginas * estructuraReemplazo) {
		return estructuraReemplazo->pid == pid;
	}
	bool paginasDelPid(t_tablaDePaginas * tablaDePaginas) {
		return tablaDePaginas->pid == pid;
	}
	int cantidadDeFrames = configMemoria.maximoMarcosPorProceso;/*list_count_satisfying(estructuraAlgoritmos,
	 (void*) framesDelPID);*/
	int i = 0;
	while (i < cantidadDeFrames) {
		reemplazo = list_get(tablaDePaginas, i);
		reemplazo->marco = i;

		i++;
	}
	i = 0;
	while (i < cantidadDeFrames) {
		reemplazoDePagina(pid, paginaReferenciada, reemplazo);
		i++;
	}
	//free(reemplazo);
	pthread_mutex_unlock(&mutexFrameSinUsar);
}
