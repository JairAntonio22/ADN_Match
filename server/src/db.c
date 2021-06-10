#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "db.h"

DB* create_db(char *name) {
    DB *db = (DB*) malloc(sizeof(DB));

    db->name = strdup(name);
    db->size = 0;
    db->tables = (Table**) calloc(sizeof(Table*), 500);

    return db;
}

Error delete_db(DB *db) {
    if (db == NULL) {
        return NullPtrError;
    }

    free(db->name);

    for (int i = 0; i < db->size; i++) {
        free(db->tables[i]);
    }

    free(db->tables);

    return SuccessOperation;
}

DB* load_db(char *filename) {
    FILE *file = fopen(filename, "r");

    if (file == NULL) {
        return NULL;
    }

    char *name = strdup(filename);
    name[strchr(name, '.') - name] = '\0';

    DB *db = create_db(name);
    free(name);

    char *tablename = (char*) calloc(sizeof(char), 50);

    while (!feof(file)) {
        fscanf(file, "%s\n", tablename);
        db->tables[db->size] = load_table(tablename);
        db->size += db->tables[db->size] == NULL ? 0 : 1;
    }

    free(tablename);

    return db;
}

Error save_db(DB *db) {
    if (db == NULL) {
        return NullPtrError;
    }

    char *filename = (char*) calloc(sizeof(char), 50);
    strcpy(filename, db->name);
    strcat(filename, ".txt");

    FILE *file = fopen(filename, "w");

    for (int i = 0; i < db->size; i++) {
        strcpy(filename, db->tables[i]->name);
        strcat(filename, ".csv");

        fprintf(file, "%s\n", filename);
        save_table(db->tables[i]);
    }

    fclose(file);
    free(filename);
    
    return SuccessOperation;
}

Error add_table(DB *db, char *name, int ncols, char **cols) {
    if (db == NULL) {
        return NullPtrError;
    }

    for (int i = 0; i < db->size; i++) {
        if (strcmp(name, db->tables[i]->name) == 0) {
            return TableCollision;
        }
    }

    db->tables[db->size] = create_table(name, ncols, cols);
    db->size++;

    return SuccessOperation;
}

Table* get_table(DB *db, char *name) {
    if (db == NULL) {
        return NULL;
    }

    int index = 0;

    while (index < db->size && strcmp(db->tables[index]->name, name) != 0) {
        index++;
    }

    return index == db->size ? NULL : db->tables[index];
}

Table* select_db(DB *db, char *name, int ncols, char **cols, char **where, Mode mode) {
    if (db == NULL) {
        return NULL;
    }

    Table *table = get_table(db, name);

    if (table == NULL) {
        return NULL;
    }

    switch (mode) {
    case select_all:
        return select_table(table, table->cols, table->data[0], NULL);

    case select_where:
        return select_table(table, table->cols, table->data[0], where);

    case select_cols:
        return select_table(table, ncols, cols, NULL);

    case select_cols_where:
        return select_table(table, ncols, cols, where);

    default:
        return NULL;
    }
}

Table* join_db(DB *db, char *name1, char *name2, char **cols) {
    if (db == NULL) {
        return NULL;
    }

    Table *table1 = get_table(db, name1);
    Table *table2 = get_table(db, name2);

    return table1 == NULL || table2 == NULL
        ? NULL 
        : join_table(table1, table2, cols);
}

Error insert_db(DB *db, char *name, char **row) {
    if (db == NULL) {
        return NullPtrError;
    }

    Table *table = get_table(db, name);

    return table == NULL ? TableNotFound : insert_table(table, row);
}
