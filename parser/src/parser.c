/*
 ============================================================================
 Name        : parser.c
 Author      : CVITA
 Version     :
 Copyright   : Your copyright notice
 Description : Hello World in C, Ansi-style
 ============================================================================
 */

#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <commons/string.h>

#define INICIAR "iniciar"
#define LEER "leer"
#define ESCRIBIR "escribir"
#define FINALIZAR "finalizar"
#define IO "entrada-salida"

bool _esLeer(char* linea) {
	return string_starts_with(linea, LEER);
}
bool _esEscribir(char* linea) {
	return string_starts_with(linea, ESCRIBIR);
}

bool _esIniciar(char* linea) {
	return string_starts_with(linea, INICIAR);
}
bool _esIO(char* linea) {
	return string_starts_with(linea, IO);
}
bool _esFinalizar(char* linea) {
	return string_starts_with(linea, FINALIZAR);
}
int main(void) {
	/*
	 int c;

	 FILE *input_file;

	 input_file = fopen("/home/utnso/workspace/Planificador/mCod/simple.cod", "r");

	 if (input_file == 0)
	 {
	 //fopen returns 0, the NULL pointer, on failure
	 perror("Canot open input file\n");
	 exit(-1);
	 }
	 else
	 {
	 int found_word = 0;

	 while ((c =fgetc(input_file)) != EOF )
	 {
	 //if it's an alpha, convert it to lower case
	 if (isalpha(c))
	 {
	 found_word = 1;
	 c = tolower(c);
	 putchar(c);
	 }
	 else {
	 if (found_word) {
	 putchar('\n');
	 found_word=0;
	 }
	 }

	 }
	 }

	 fclose(input_file);

	 printf("\n");
	 */
	int pid;
	float idle, busy, user;

	FILE* fid;
	fpos_t pos;
	int pos_init = 0;

//	while (1)
	//{
	// try to open the file
	if ((fid = fopen("simple.cod", "r")) == NULL) {
		sleep(1); // sleep for a little while, and try again
		//continue;
	}

	int i = 0;
	int line_index = 0;
	char *p;
	char *line;
	char *buffer;

	char comando[100];
	char parametro1[100];
	char parametro2[100];
	char coma[100];
	char string[100];
	/*  while(fgets(string, 100, fid)) {
	 printf("%s\n", string);
	 }*/
	while (!feof(fid)) //loop to read the file
	{
		//fgets(buffer,"%s",fid);
		fgets(string, 100, fid);
		//p = strtok(line,";");
		p = strtok(string, ";");
		i = 0;

		//while(p!=NULL)
		if (p != NULL) {
			char *string = string_new();
			string_append(&string, p);
			char** substrings = string_split(string, " ");

			//if( substrings[0] == "iniciar"){
			if (_esIniciar(substrings[0])) {
				printf("comando iniciar, parametro %d \n", atoi(substrings[1]));
				free(substrings[0]);
				free(substrings[1]);
				free(substrings);
			}
			if (_esLeer(substrings[0])) {
				printf("comando leer, parametro %d \n", atoi(substrings[1]));
				free(substrings[0]);
				free(substrings[1]);
				free(substrings);
			}
			if (_esEscribir(substrings[0])) {
				printf("comando Escribir, parametros %d  %s \n",
						atoi(substrings[1]), substrings[2]);
				/*
				free(substrings[0]);
				free(substrings[1]);
				free(substrings[2]);
				free(substrings);
			*/}
			if (_esIO(substrings[0])) {
				printf("comando entrada salida, parametro %d \n",
						atoi(substrings[1]));
				free(substrings[0]);
				free(substrings[1]);
				free(substrings);
			}
			if (_esFinalizar(substrings[0])) {
							printf("comando Finalizar no tiene parametros \n");
							free(substrings[0]);
							free(substrings);
						}
		//	i++;

		}

		line_index++;
	} //end of file-reading loop.}
	fclose(fid);
	int var;
	for (var = 0; var < line_index; ++var) {

		printf("comando: %s", comando[line_index]);
		printf("parametro: %s", parametro1[line_index]);
		printf("parametro2: %s", parametro2[line_index]);
		printf("comando: %s", coma[line_index]);
	}

	//}
	return EXIT_SUCCESS;
}
