#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "errorMsg.h"
#include "table.h"

#define bool int
#define true 1
#define false 0

Table* create_table(char *name, int ncols, char **cols) {
    Table *table = (Table*) malloc(sizeof(Table));

    table->name = strdup(name);
    table->rows = 1;
    table->cols = ncols;
    table->data = (char***) calloc(sizeof(char**), 500);
    table->data[0] = (char**) calloc(sizeof(char*), ncols);

    for (int i = 0; i < ncols; i++) {
        if (cols[i] == NULL) {
            return NULL;
        }

        table->data[0][i] = strdup(cols[i]);
    }

    for (int i = 0; i < ncols - 1; i++) {
        char count = '0';

        for (int j = i + 1; j < ncols; j++) {
            if (strcmp(table->data[0][i], table->data[0][j]) != 0) {
                continue;
            }

            int len = strlen(table->data[0][j]);
            table->data[0][j] = (char*) realloc(table->data[0][j], len + 2);
            table->data[0][j][len] = count;
            table->data[0][j][len + 1] = '\0';
            count++;
        }
    }

    return table;
}

Error delete_table(Table *table) {
    if (table == NULL) {
        return NullPtrError;
    }

    free(table->name);

    for (int i = 0; i < table->rows; i++) {
        for (int j = 0; j < table->cols; j++) {
            free(table->data[i][j]);
        }

        free(table->data[i]);
    }
    
    free(table->data);
    free(table);

    return SuccessOperation;
}

Table* load_table(char *filename) {
    FILE *file = fopen(filename, "r");

    if (file == NULL) {
        return NULL;
    }

    char *word = (char*) calloc(sizeof(char), 500);
    int size = 0;

    char ***data = (char***) calloc(sizeof(char**), 500);
    int nrows = 0;
    int ncols = 0;

    data[0] = (char**) calloc(sizeof(char*), 500);
    int ccol = 0;

    for (char c = fgetc(file); c != EOF; c = fgetc(file)) {
        switch (c) {
        case ',':
            word[size] = '\0';
            data[nrows][ccol] = strdup(word);
            size = 0;
            ccol++;
            break;

        case '\n':
            word[size] = '\0';
            data[nrows][ccol] = strdup(word);
            size = 0;
            ncols = ccol > ncols ? ccol : ncols;
            ccol = 0;
            nrows++;
            data[nrows] = (char**) calloc(sizeof(char*), 500);
            break;

        default:
            word[size] = c;
            size++;
        }
    }

    fclose(file);
    free(word);

    char *name = strdup(filename);
    name[strchr(name, '.') - name] = '\0';

    Table *table = create_table(name, ncols + 1, data[0]);
    free(name);

    for (int i = 1; i < nrows; i++) {
        insert_table(table, data[i]);

        for (int j = 0; j < ncols + 1; j++) {
            free(data[i][j]);
        }
        
        free(data[i]);
    }

    free(data);
    return table;
}

Error save_table(Table *table) {
    if (table == NULL) {
        return NullPtrError;
    }

    char *filename = (char*) calloc(sizeof(char), strlen(table->name) + 5);
    strcpy(filename, table->name);
    strcat(filename, ".csv");
    
    FILE *file = fopen(filename, "w");
    
    for (int i = 0; i < table->rows; i++) {
        for (int j = 0; j < table->cols - 1; j++) {
            fprintf(file, "%s,", table->data[i][j]);
        }

        fprintf(file, "%s\n", table->data[i][table->cols - 1]);
    }

    free(filename);
    fclose(file);

    return SuccessOperation;
}

Table* select_table(Table *table, int ncols, char **cols, char **where) {
    if (table == NULL) {
        return NULL;
    }

    if (ncols <= 0 && where == NULL) {
        return NULL;
    }

    bool valid = false;

    for (int i = 0; i < ncols; i++) {
        for (int j = 0; j < table->cols; j++) {
            if (strcmp(cols[i], table->data[0][j]) == 0) {
                valid = true;
            }
        }

        if (!valid) {
            return NULL;
        }
    }

    if (where != NULL) {
        valid = false;

        for (int j = 0; j < table->cols; j++) {
            if (strcmp(where[0], table->data[0][j]) == 0) {
                valid = true;
            }
        }

        if (!valid) {
            return NULL;
        }
    }


    bool *mask = (bool*) calloc(sizeof(bool), table->cols);
    int *map = (int*) calloc(sizeof(bool), table->cols);
    int w_id;

    for (int i = 0; i < table->cols; i++) {
        for (int j = 0; j < ncols; j++) {
            if (strcmp(table->data[0][i], cols[j]) == 0) {
                mask[i] = true;
                map[i] = j;
            }

        }

        if (where != NULL && strcmp(table->data[0][i], where[0]) == 0) {
            w_id = i;
        }
    }

    char ***data = (char***) calloc(sizeof(char**), table->rows);
    int rows = 1;

    for (int i = 1; i < table->rows; i++) {
        if (where != NULL && strcmp(table->data[i][w_id], where[1]) != 0) {
            continue;
        }

        data[rows] = (char**) calloc(sizeof(char*), ncols);

        for (int j = 0; j < table->cols; j++) {
            if (mask[j]) {
                data[rows][map[j]] = strdup(table->data[i][j]);
            }
        }
        
        rows++;
    }

    char *name = (char*) calloc(sizeof(char), strlen(table->name) + 10);
    strcpy(name, table->name);
    strcat(name, " selected");

    Table *result = create_table(name, ncols, cols);

    for (int i = 1; i < rows; i++) {
        insert_table(result, data[i]);

        for (int j = 0; j < ncols; j++) {
            free(data[i][j]);
        }

        free(data[i]);
    }

    free(data);

    return result;
}


Table* join_table(Table *table1, Table *table2, char **cols) {
    if (table1 == NULL || table2 == NULL) {
        return NULL;
    }

    int ncols = table1->cols + table2->cols;

    char ***data = (char***) calloc(sizeof(char**), 500);

    int key1 = -1;
    int key2 = -1;

    /* Primer renglón */ {
        data[0] = (char**) calloc(sizeof(char*), ncols);

        int i = 0;

        while (i < table1->cols) {
            if (strcmp(table1->data[0][i], cols[0]) == 0) {
                key1 = i;
            }

            data[0][i] = strdup(table1->data[0][i]);
            i++;
        }

        while (i - table1->cols < table2->cols) {
            if (strcmp(table2->data[0][i - table1->cols], cols[1]) == 0) {
                key2 = i - table1->cols;
            }
            
            data[0][i] = strdup(table2->data[0][i - table1->cols]);
            i++;
        }
    }

    if (key1 == -1 || key2 == -1) {
        return NULL;
    }

    int row = 1;

    for (int i = 1; i < table1->rows; i++) {
        for (int j = 1; j < table2->rows; j++) {
            data[row] = (char**) calloc(sizeof(char*), ncols);

            if (strcmp(table1->data[i][key1], table2->data[j][key2]) != 0) {
                continue;
            }

            int k = 0;

            while (k < table1->cols) {
                data[row][k] = strdup(table1->data[i][k]);
                k++;
            }

            while (k - table1->cols < table2->cols) {
                data[row][k] = strdup(table2->data[j][k - table1->cols]);
                k++;
            }

            row++;
        }
    }

    int name_len = strlen(table1->name) + 3 + strlen(table2->name);
    char *name = (char*) calloc(sizeof(char), name_len);

    strcpy(name, table1->name);
    strcat(name, " join ");
    strcat(name, table2->name);

    Table *table = create_table(name, ncols, data[0]);
    free(name);

    for (int i = 1; i < row; i++) {
        insert_table(table, data[i]);

        for (int j = 0; j < ncols; j++) {
            free(data[i][j]);
        }
        
        free(data[i]);
    }

    free(data);
    return table;
}

Error insert_table(Table *table, char **row) {
    if (table == NULL) {
        return NullPtrError;
    }

    table->data[table->rows] = (char**) calloc(sizeof(char*), table->cols);

    for (int i = 0; i < table->cols; i++) {
        if (row[i] == NULL) {
            return MissingColums;
        }

        table->data[table->rows][i] = strdup(row[i]);
    }

    table->rows++;

    return SuccessOperation;
}

char* encode_table(Table *table) {
    if (table == NULL) {
        return NULL;
    }

    int **widths = (int**) calloc(sizeof(int*), table->rows);
    int *maxwidths = (int*) calloc(sizeof(int), table->cols);

    for (int i = 0; i < table->rows; i++) {
        widths[i] = (int*) calloc(sizeof(int), table->cols);

        for (int j = 0; j < table->cols; j++) {
            widths[i][j] = strlen(table->data[i][j]);
            maxwidths[j] = widths[i][j] > maxwidths[j] 
                ? widths[i][j] 
                : maxwidths[j];
        }
    }

    int space = 4;

    char *msg = (char*) calloc(sizeof(char), 2000);
    char separator[] = ",";

    for (int i = 0; i < table->rows; i++) {
        for (int j = 0; j < table->cols - 1; j++) {
            strcat(msg, table->data[i][j]);

            for (int k = 0; k < maxwidths[j] - widths[i][j] + space; k++) {
                strcat(msg, " ");
            }

            strcat(msg, separator);
        }

        strcat(msg, table->data[i][table->cols - 1]);
        strcat(msg, "\n");

        free(widths[i]);
    }

    free(maxwidths);
    free(widths);

    return msg;
}

void print_table(Table *table) {
    if (table == NULL) {
        return;
    }

    int **widths = (int**) calloc(sizeof(int*), table->rows);
    int *maxwidths = (int*) calloc(sizeof(int), table->cols);

    for (int i = 0; i < table->rows; i++) {
        widths[i] = (int*) calloc(sizeof(int), table->cols);

        for (int j = 0; j < table->cols; j++) {
            widths[i][j] = strlen(table->data[i][j]);
            maxwidths[j] = widths[i][j] > maxwidths[j] 
                ? widths[i][j] 
                : maxwidths[j];
        }
    }

    printf("Table: %s\n", table->name);
    int space = 4;

    for (int i = 0; i < table->rows; i++) {
        printf("%i", i);

        for (int j = 0; j < space; j++) {
            printf(" ");
        }

        for (int j = 0; j < table->cols; j++) {
            printf("%s", table->data[i][j]);

            for (int k = 0; k < maxwidths[j] - widths[i][j] + space; k++) {
                printf(" ");
            }
        }
        
        printf("\n");
        free(widths[i]);
    }

    free(maxwidths);
    free(widths);
}
