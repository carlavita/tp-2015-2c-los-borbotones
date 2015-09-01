/*
 * planificador.c
 *
 *  Created on: 30/8/2015
 *      Author: carlavita
 */

//#include <stdio.h>
//#include <stdlib.h>
#include "planificador.h"


int main(void) {
	pthread_t hilo_consola;
//todo	pthread_t hilo_server;
	pthread_t hilo_server;
	/*Log*/
	remove(PATH_LOG);
	logger = log_create(PATH_LOG,"Planificador",true,LOG_LEVEL_INFO);
	log_info(logger,"Iniciando proceso planificador");

/*Archivo de configuración*/

	log_info(logger,"Leyendo Archivo de Configuracion");
	levantarConfiguracion();
	log_info(logger,"Archivo de Configuracion Leido correctamente");

/*Hilo Consola*/
	pthread_create (&hilo_consola, NULL, (void *) &mostrar_consola, NULL);

/*Hilo Server Planificador*/

	pthread_create (&hilo_server, NULL, (void *) &servidor_CPU, NULL);

	pthread_join(hilo_consola,NULL);
	//todo pthread_join(hilo_servidor);
	pthread_join(hilo_server,NULL);
	return EXIT_SUCCESS;
}

void levantarConfiguracion(){
	  log_info(logger,"Lectura de variables del archivo de configuracion");
	  t_config * CONFIG = config_create(PATH_CONFIG);
	  config_planificador.puertoEscucha = config_get_int_value(CONFIG,"PUERTO_ESCUCHA");
	  config_planificador.algoritmo = config_get_int_value(CONFIG,"ALGORITMO");
	  config_planificador.quantum = config_get_int_value(CONFIG,"QUANTUM");

	  printf(" puerto %d \n",config_planificador.puertoEscucha );
	  printf(" algoritmo %d\n",config_planificador.algoritmo );
	  printf(" quantum %d\n",config_planificador.quantum );
}

void mostrar_consola( void *ptr ){
	int option = -1;
	while(option != 0){
//todo: descomentar todos clear		system("clear");

		puts("Consola - Filesystem \n \n"
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
//todo            correr_path();
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
		}

	}

}

void espera_enter ()
       	{
       			printf("Presione ENTER para continuar");
       			char enter='\0';
       			enter = getchar();
       			while(((enter = getchar()) != '\n'));
       	}


void servidor_CPU( void *ptr ){

	 printf(" estoy en el hilo servidor de CPU\n");
	 //todo crear servidor para un cliente cpu...despues multiplexamos a las distintas cpus
}
