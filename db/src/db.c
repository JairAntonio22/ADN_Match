// El servidor, deberá leer línea por línea la secuencia y buscará en que parte de la secuencia de referencia embona.
// El programa dará un reporte linea por linea, en que parte de la referencia lo encontró (15pts) 
// y qué porcentaje de la secuencia referencia fue cubierto. (20 pts)

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "seq.h"

#define BUFFERSIZE 12000000
#define LINESIZE 20000

int main(){
    char secuenciaMappear = "s_cerevisia-1.seq";
    char genomaCompleto = "S._cerevisiae_processed.txt";
    char *secuencia, linea[LINESIZE];
    //char ejemplo;
    FILE *archivoGenoma;
    FILE *archivoSecuencia;

    // Abrimos la secuencia referencia
    archivoGenoma = open(genomaCompleto,"r");

    // Recalculando tamaño de memoria de la secuencia a mappear
    secuencia = (char *)realloc(secuencia,BUFFERSIZE);

    // Leector linea por linea
    fgets(secuencia, sizeof(char)*BUFFERSIZE, archivoGenoma);

    int i = 0;
    archivoSecuencia = open(secuenciaMappear, "r");
    while (fgets(linea, sizeof(linea), archivoSecuencia) != NULL){
        linea[strlen(linea) - 1] = '\0';
        linea[strlen(linea) - 1] = '\0';
        // ejemplo[i] = strdup(linea);
        // i++;
    }
    fclose(archivoSecuencia);




    return 0;
}