#ifndef TABLE_H
#define TABLE_H

#include "errorMsg.h"

typedef struct Table {
    char *name;
    int rows;
    int cols;
    char ***data;
} Table;

// Regresa NULL si el tamaño de cols es menor a ncols
Table* create_table(char *name, int ncols, char **cols);

// Regresa NullPtrError si la tabla que se mandó es nula
// Regresa SuccessOperation si la operación es exitosa
Error delete_table(Table *table);

// Regresa NULL si no se encuentra el archivo de la tabla
Table* load_table(char *filename);

// Regresa NullPtrError si la tabla que se mandó es nula
// Regresa SuccessOperation si la operación es exitosa
Error save_table(Table *table);

// Regresa NULL 
//      Si la tabla que se mandó es nula
//      Si where es nulo y ncols es menor o igual a 0
Table* select_table(Table *table, int ncols, char **cols, char **where);

// Regresa NULL
//      Si las tablas que se mandaron son nulas
//      Si no existen las columnas en cols
Table* join_table(Table *table1, Table *table2, char **cols);

// Regresa NullPtrError si la tabla que se mandó es nula
// Regresa SuccessOperation si la operación es exitosa
Error insert_table(Table *table, char **row);

// Regresa NULL si la tabla que se manó es nula
char* encode_table(Table *table);

void print_table(Table *tabel);

#endif
