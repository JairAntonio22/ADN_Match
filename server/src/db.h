#ifndef DB_H
#define DB_H

#include "table.h"

typedef struct DB {
    char *name;
    int size;
    Table **tables;
} DB;

DB* create_db(char *name);

// Regresa SuccessOperation si la operación es exitosa
// Regresa NullPtrError si la db que se mandó es nula
Error delete_db(DB* db);

// Regresa NULL si el archivo de tablas no existe
DB* load_db(char *filename);

// Regresa SuccessOperation si la operación es exitosa
// Regresa NullPtrError si la db que se mandó es nula
Error save_db(DB *db);

// Regresa SuccessOperation la operación es exitosa
// Regresa NullPtrError si la db que se mandó es nula
// Regresa TableCollision si existe ya una tabla usando el nombre
Error add_table(DB *db, char *name, int ncols, char **cols);

typedef enum Mode {
    select_all = 1,
    select_where,
    select_cols,
    select_cols_where
} Mode;

// Regresa NULL
//      Si la db que se mandó es nula
//      Si la tabla no existe en la db
//      Si where es nulo y ncols es menor o igual a 0
Table* select_db(DB *db, char *name, int ncols, char **cols, char **where, Mode mode);

// Regresa NULL
//      Si alguna de las dos tablas no existe
//      Si no existen las columnoas en cols
Table* join_db(DB *db, char *name1, char *name2, char **cols);

// Regresa SuccessOperation si la operación es exitosa
// Regresa NullPtrError si la db que se mandó es nula
// Regresa TableNotFound si no se encuentra la tabla
Error insert_db(DB *db, char *name, char **row);

#endif
