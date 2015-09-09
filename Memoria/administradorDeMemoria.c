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
#define PACKAGESIZE 1024

typedef struct
{
	int puertoEscucha;
	char* ipSwap;
	char * puertoSwap;
	int maximoMarcosPorProceso;
	int cantidadDeMarcos;
	int tamanioMarcos;
	int entradasTLB;
	int tlbHabilitada; /*Las commons no tiene un config_ge_bool.... => 0 false, 1 true*/
	int retardoMemoria;
}t_config_memoria;

typedef struct
{
	int pid;
	char * direccion;
	int offset; //TAMAÑO QUE CARGO, PARA TODOS LOS CAMPOS DE LA TLB.
}t_TLB;    //LA TLB VA A SER UN ARRAY DE N entradas.(por ahora)

typedef struct
{
	int pid;
	int marco;
	int pagina;
	int bitUso;
	int bitModificado;
}t_tablaDePaginas;


t_config_memoria  configMemoria;
t_log * logMemoria;
int clienteSwap;
t_tablaDePaginas * tablaDePaginas;

void leerConfiguracion()
{
  log_info(logMemoria,"Inicio lectura de archivo de configuración");
  t_config * configuracionMemoria = config_create("memoria.conf");
  configMemoria.puertoEscucha = config_get_int_value(configuracionMemoria,"PUERTO_ESCUCHA");
  configMemoria.ipSwap = config_get_string_value(configuracionMemoria,"IP_SWAP");
  configMemoria.puertoSwap = config_get_string_value(configuracionMemoria,"PUERTO_SWAP");
  configMemoria.maximoMarcosPorProceso  = config_get_int_value(configuracionMemoria,"MAXIMO_MARCOS_POR_PROCESO");
  configMemoria.cantidadDeMarcos = config_get_int_value(configuracionMemoria,"CANTIDAD_MARCOS");
  configMemoria.tamanioMarcos = config_get_int_value(configuracionMemoria,"TAMANIO_MARCOS");
  configMemoria.entradasTLB = config_get_int_value(configuracionMemoria,"ENTRADAS_TLB");
  configMemoria.tlbHabilitada = config_get_int_value(configuracionMemoria,"TLB_HABILITADA");
  configMemoria.retardoMemoria = config_get_int_value(configuracionMemoria,"RETARDO_MEMORIA");
  log_info(logMemoria,"%d",configMemoria.puertoEscucha);
  log_info(logMemoria,"Finalizo lectura de archivo de configuración");
}

void crearServidores()
{
	servidorMultiplexor(configMemoria.puertoEscucha);
}


char * recibidoPorLaMemoria;
char mensaje[1024];

t_TLB * generarTLB(int entradasTLB)
{
	t_TLB tlb[entradasTLB];
	int entrada = 0;
	while ( entrada < entradasTLB)
	{
		tlb[entrada].direccion = '\0';
		tlb[entrada].offset = configMemoria.tamanioMarcos;
		tlb[entrada].pid = 0;//cuando vengan los procesos, ire cambiando ese pid.
		 ++entrada;
	}
	return tlb;
}
t_TLB * tlb;

void creacionTLB(const t_config_memoria* configMemoria, t_log* logMemoria,t_TLB* tlb)
{

	if(configMemoria->tlbHabilitada == 1)
	{
		printf("La TLB esta habilitada, se procede a su creación");
		log_info(logMemoria, "Inicio creacion de la TLB");
		tlb = malloc(sizeof(t_TLB*));
	    tlb = generarTLB(configMemoria->entradasTLB);
	    log_info(logMemoria, "Finalizo existosamente la creacion de la TLB");
	}
	else
	{
		printf("La TLB no esta habilitada");
		log_warning(logMemoria,"La TLB no esta activada por configuración");
	}
}

int ConexionMemoriaSwap(t_config_memoria* configMemoria, t_log* logMemoria)
{
	int intentosFallidosDeConexionASwap = 0;
	log_trace(logMemoria,"Generando cliente al SWAP con estos parametros definidos, IP: %s, PUERTO: %d",configMemoria->ipSwap,configMemoria->puertoSwap);
	cliente(configMemoria->ipSwap, configMemoria->puertoSwap);
	int clienteSwap = clienteSeleccion();
	while (clienteSwap == -1) {
		log_error(logMemoria, "No se pudo conectar al SWAP, reintando");
		if(intentosFallidosDeConexionASwap == 5)
		{
			printf("Luego de %d intentos de conexion fallidos,se espera 10seg,para reintarConexion",intentosFallidosDeConexionASwap);
			log_info(logMemoria,"Luego de %d intentos de conexion fallidos,se espera 10seg para reintarConexion",intentosFallidosDeConexionASwap);
			sleep(10);
		}

		cliente(configMemoria->ipSwap, configMemoria->puertoSwap);
		clienteSwap = clienteSeleccion();
		intentosFallidosDeConexionASwap++;
	}
	if (clienteSwap > 0) {
		log_info(logMemoria,"La memoria se conecto satisfactoriamente al SWAP");
	}
	return clienteSwap;
}

void generarTablaDePaginas(int * memoriaReservadaDeMemPpal)
{
	int  pagina = 0;
	int *cantidadDePaginas = malloc(sizeof(int *));

	recv(clienteSwap,cantidadDePaginas,sizeof(int),0);
	int frame = 0;
	int maximaCantidadDeFrames = configMemoria.cantidadDeMarcos;
   while(pagina < *cantidadDePaginas)
   {
	   if(frame == maximaCantidadDeFrames) return;
	   else
	   {
		   tablaDePaginas->bitModificado = 1;
		   tablaDePaginas->bitUso = 1;
		   tablaDePaginas->marco = frame;//ESTO NO ME GUSTA, TODO
		   tablaDePaginas->pagina = pagina;
		   tablaDePaginas->pid = -1; //-1 me indica que la pagina no esta asignada a ningun proceso
	   }
	   frame++;
	   pagina += pagina; //pagina++
   }
}


int * memoriaReservadaDeMemPpal;
int main()
{
	remove("logMemoria.txt");//Cada vez que arranca el proceso borro el archivo de log.
	logMemoria = log_create("logMemoria.txt","Administrador de memoria",false,LOG_LEVEL_INFO);
	leerConfiguracion();
	creacionTLB(&configMemoria, logMemoria, tlb);

	log_info(logMemoria,"Comienzo de las diferentes conexiones");
	clienteSwap = ConexionMemoriaSwap(&configMemoria, logMemoria);
	int servidorCPU = servidorMultiplexor(configMemoria.puertoEscucha);
	generarTablaDePaginas(memoriaReservadaDeMemPpal);
	recibidoPorLaMemoria = datosRecibidos();

	for(;;){
		//ACA SE VA A PROCESAR TODO, DESPUES DE LA CREACION DE LAS DISTINTAS ESTRUCTURAS Y CONEXIONES
	//int envioDeMensajes = send(clienteSwap,recibidoPorLaMemoria,sizeof(recibidoPorLaMemoria),0);
	int size = PACKAGESIZE;
	int envioDeMensajes = send(clienteSwap,recibidoPorLaMemoria,PACKAGESIZE,0);
	while(envioDeMensajes == -1) envioDeMensajes = send(clienteSwap,recibidoPorLaMemoria,sizeof(recibidoPorLaMemoria),0);
	log_info(logMemoria,"%d",envioDeMensajes);

	}
    exit(0);
}


