#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <unistd.h>
 
// Server IP
#define SERVER_IP "127.0.0.1"
#define SERVER_LOGIN_REPLY_BUFFER_SIZE 8
// HTTP port
#define PORT 3000

// Buffer sizes
#define INPUT_BUFFER_SIZE 256
// Username buffer size
#define USERNAME_BUFFER_SIZE 26
// Password buffer size
#define PASSWORD_BUFFER_SIZE 26
// Request size
#define REQUEST_BUFFER_SIZE 513
// Server reply size
#define REPLY_BUFFER_SIZE 2001
// Statement buffer size
#define STATEMENT_BUFFER_SIZE 550

#define LOGIN_MAX_TRIES 3

#define OK_INIT 0
#define OK_LOGIN 0
#define OK_LOGOUT 0
#define OK_INSERT 0
#define OK_SELECT 0
#define OK_JOIN 0
#define OK_REQUEST_SENT 0
#define OK_REPLY_RECEIVED 0
#define OK_RESULT_TABLE_PARSED 0
#define ERROR_SOCKET_NOT_CREATED 1
#define ERROR_SERVER_NOT_CONNECTED 2
#define ERROR_REQUEST_NOT_SENT 3
#define ERROR_REPLY_NOT_RECEIVED 4
#define ERROR_LOGIN_MAX_TRIES 5
#define ERROR_LOGOUT 6
#define ERROR_RESULT_TABLE_ROW_NOT_PARSED 7
#define ERROR_RESULT_TABLE_COL_NOT_PARSED 8
#define ERROR_INSERT 9

#define DEFAULT 'd'
#define QUIT 'q'
#define SELECT 's'
#define INSERT 'i'
#define JOIN 'j'

#define SELECT_ALL '1'
#define SELECT_ALL_WHERE ' 2'
#define SELECT_ALL_COLS ' 3'
#define SELECT_COLS_WHERE '4'

typedef enum {false, true} bool;

int init();
int send_request();
int receive_reply();
void print_reply();
int print_result_table(char*);
void finish();
int login();
int logout();
int insert_db();
int select_all();
int select_all_where();
int select_all_cols();
int select_cols_where();
void select_db();
int join_db();
void clear_reply_buffer();

// Server socket descriptor
int socket_desc;
// Server info struct
struct sockaddr_in server;
// Username
char username[USERNAME_BUFFER_SIZE];
// Password
char password[PASSWORD_BUFFER_SIZE];
// Input buffer
char input_buffer[INPUT_BUFFER_SIZE];
// Reply buffer to receive data from server
char reply_buffer[REPLY_BUFFER_SIZE];
// Request buffer to send data to server
char request_buffer[REQUEST_BUFFER_SIZE];
// Buffer to confirm user statement
char statement[STATEMENT_BUFFER_SIZE];
// Global flag to check connection status
bool connected = false;

char choice[2] = "d";
// bool login_status;

int main(void) 
{
    // 1. Init: create socket and connect to server
    switch (init())
    {
    case OK_INIT:
        printf("Connected\n");
        break;
    case ERROR_SOCKET_NOT_CREATED:
        printf("Could not create socket\n");
        return 1;
        break;
    case ERROR_SERVER_NOT_CONNECTED:
        printf("Could not connect to server\n");
        return 1;
        break;
    default:
        break;
    }

    // 2. Login
    switch (login())
    {
    case ERROR_REQUEST_NOT_SENT:
        printf("Could not send request to server\n");
        break;
    case ERROR_REPLY_NOT_RECEIVED:
        printf("Could not receive reply from server\n");
        break;
    case ERROR_LOGIN_MAX_TRIES:
        printf("You have used all attempts. Try again later.\n");
        break;
    case OK_LOGIN:
        printf("Login successful\n");
    default:
        break;
    }

    do
    {
        // Display options
        printf("\t%c - SELECT\n", SELECT);
        printf("\t%c - INSERT\n", INSERT);
        printf("\t%c - JOIN\n", JOIN);
        printf("\t%c - QUIT\n", QUIT);
        printf("Instruction: ");

        // Read choice from console
        fgets(input_buffer, INPUT_BUFFER_SIZE, stdin);

        printf("\n");
        
        // Replace newline with termination character
        input_buffer[1] = '\0';
        strcpy(choice, input_buffer);

        switch (choice[0])
        {
        case SELECT:
            select_db();
            break;
        case INSERT:
            insert_db();
            break;
        case JOIN:
            join_db();
            break;
        default:
            break;
        }

    } while (choice[0] != QUIT);

    logout();
    printf("You have logged out\n");
    
    // Close socket
    finish();
    printf("Socket closed\n");
    return 0;
}

int init()
{
    clear_reply_buffer();

    // 1. Create new socket using IPv4 and TCP/IP
    if ((socket_desc = socket(AF_INET, SOCK_STREAM, 0)) == -1)
        return ERROR_SOCKET_NOT_CREATED;

    // 2. Define server values: address family, address and port
    server.sin_family = AF_INET;
    server.sin_port = htons(PORT);
    server.sin_addr.s_addr = inet_addr(SERVER_IP);

    // 3. Connect socket to server
    if (connect(socket_desc, (struct sockaddr*) &server, sizeof(server)) < 0)
        return ERROR_SERVER_NOT_CONNECTED;

    // 4. Connection successful
    connected = true;
    return OK_INIT;
}

int send_request()
{
    if (connected == false)
        return ERROR_SERVER_NOT_CONNECTED;

    // Message buffer to send data to server
    char* message = "GET / HTTP/1.1\r\n\r\n";

    if (send(socket_desc, message, strlen(message), 0) < 0)
        return ERROR_REQUEST_NOT_SENT;

    return OK_REQUEST_SENT;
}

int receive_reply()
{
    if (connected == false)
        return ERROR_SERVER_NOT_CONNECTED;

    if (recv(socket_desc, reply_buffer, REPLY_BUFFER_SIZE, 0) < 0)
        return ERROR_REPLY_NOT_RECEIVED;

    return OK_REPLY_RECEIVED;
}

void print_reply()
{
    for (int i = 0; i < REPLY_BUFFER_SIZE; i++)
        printf("%c", reply_buffer[i]);
        
    printf("\n");
    //fclose(fptr);
}

int print_result_table(char* reply)
{
    //if (connected == false)
      //  return ERROR_SERVER_NOT_CONNECTED;

    // Reply
    // PRUEBA: asi debe lucir el mensaje de respuesta del servidor
    //char* reply = "1,2,3\n4,5,6\n7,8,9";
    // Row and column delimiters
    char* delim_row = "\n";
    char* delim_col = ",";

    // Pointers to receive the beginning of each row and column
    char *row = NULL;
    char *col = NULL;
    // Pointers to maintain context between strtok_r() simultaenous calls
    char *saveptr1 = NULL;
    char *saveptr2 = NULL;

    // Copy reply to buffer to avoid reference issues after call to strtok_r()
    char* copy = malloc(strlen(reply) + 1);
    strcpy(copy, reply);

    // Parse all rows and all columns per row from message
    // First call to strok_r for all rows: point to message, next calls will point to NULL
    row = strtok_r(copy, delim_row, &saveptr1);

    // Error: token not found
    if (row == NULL)
        return ERROR_RESULT_TABLE_ROW_NOT_PARSED;

    // First call to strok_r for all cols in current row: point to row, next calls will point to NULL
    col = strtok_r(row, delim_col, &saveptr2);

    // Error: token not found
    if (col == NULL)
        return ERROR_RESULT_TABLE_COL_NOT_PARSED;
    
    // Print first col
    printf("%s", col);
    
    // Print all remaining cols in current row
    while ((col = strtok_r(NULL, delim_col, &saveptr2)) != NULL)
        printf("%s", col);
    printf("\n");

    // Parse remaining rows in message
    while ((row = strtok_r(NULL, delim_row, &saveptr1)) != NULL)
    {   
        col = strtok_r(row, delim_col, &saveptr2);
        if (col == NULL)
            return ERROR_RESULT_TABLE_COL_NOT_PARSED;

        // Print first col
        printf("%s", col);

        // Print all remaining cols in current row
        while ((col = strtok_r(NULL, delim_col, &saveptr2)) != NULL)
            printf("%s", col);
        printf("\n");
    }

    free(copy);
    return OK_RESULT_TABLE_PARSED;

}

void finish()
{
    close(socket_desc);
}

int login()
{
    clear_reply_buffer();

    // Set max number of login attempts
    int max_tries = LOGIN_MAX_TRIES;
    // Buffer to receive reply from server

    printf("Please login to continue\n");

    // Try login for max number of attempts
    do
    {
        // Read username from console
        printf("Username: ");
        fgets(username, USERNAME_BUFFER_SIZE, stdin);
        // Replace newline with termination character
        username[strlen(username) - 1] = '\0';
        
        // Read password from console
        printf("Password: ");
        fgets(password, PASSWORD_BUFFER_SIZE, stdin);
        // Replace newline with termination character
        password[strlen(password) - 1] = '\0';

        // Join username and password in a comma-separated request to send to server
        strcpy(request_buffer, "login,");
        strcat(request_buffer, username);
        strcat(request_buffer, ",");
        strcat(request_buffer, password);

        // Send request to server
        if (send(socket_desc, request_buffer, strlen(request_buffer), 0) < 0)
            return ERROR_REQUEST_NOT_SENT;

        // Receive reply from server
        if (recv(socket_desc, reply_buffer, REPLY_BUFFER_SIZE, 0) < 0)
            return ERROR_REPLY_NOT_RECEIVED;

        // Check if login was successful
        if (strcmp(reply_buffer, "success") == 0)
            return OK_LOGIN; // Success
        
        // Failure: try again
        printf("\nUser and password do not match. Try again.\n");
        max_tries--;
    } while (max_tries > 0);
    
    // User exceeded max number of attempts
    return ERROR_LOGIN_MAX_TRIES;
}

int logout()
{
    clear_reply_buffer();

    strcpy(request_buffer, "logout");

    // Send request to server
    if (send(socket_desc, request_buffer, strlen(request_buffer), 0) < 0)
        return ERROR_REQUEST_NOT_SENT;

    return OK_LOGOUT;
}

int insert_db()
{
    clear_reply_buffer();
    
    // Read insert statement until user confirms their input
    do
    {
        // Append request type to request
        strcpy(request_buffer, "insert,");
        strcpy(statement, "INSERT INTO ");

        // Read table name
        printf("Enter table name: ");
        fgets(input_buffer, INPUT_BUFFER_SIZE, stdin);
        // Replace newline at the end with termination character
        input_buffer[strlen(input_buffer) - 1] = '\0';

        // Append table name to request
        strcat(statement, input_buffer);
        strcat(request_buffer, input_buffer);

        strcat(statement, " VALUES(");

        // Read values until user enters empty line
        printf("Enter values one per line (enter empty line to finish):\n");

        do
        {
            fgets(input_buffer, INPUT_BUFFER_SIZE, stdin);
            // Replace newline at the end with termination character
            input_buffer[strlen(input_buffer) - 1] = '\0';

            // Append non-empty values to request
            if (input_buffer[0] != '\0')
            {
                strcat(request_buffer, ",");
                strcat(request_buffer, input_buffer);
                strcat(statement, input_buffer);
                strcat(statement, ", ");
            }
            
        } while (input_buffer[0] != '\0');
        
        // Final statement formating
        statement[strlen(statement) - 2] = ')';
        statement[strlen(statement) - 1] = ';';

        printf("%s\n", statement);

        printf("Do you confirm the execution of this statement? (y/n)");
        // Get user confirmation
        fgets(input_buffer, INPUT_BUFFER_SIZE, stdin);
        // Replace newline at the end with termination character
        input_buffer[strlen(input_buffer) - 1] = '\0';
    } while (tolower(input_buffer[0]) != 'y');

    // Statement confirmed

    // Send request to server
    if (send(socket_desc, request_buffer, strlen(request_buffer), 0) < 0)
        return ERROR_REQUEST_NOT_SENT;

    // Receive reply from server
    if (recv(socket_desc, reply_buffer, REPLY_BUFFER_SIZE, 0) < 0)
        return ERROR_REPLY_NOT_RECEIVED;

    print_result_table(reply_buffer);
    printf("\n");
    
    return OK_INSERT; // Success
}

void select_db()
{
    clear_reply_buffer();

    printf("1 All columns from all entries\n");
    printf("2 All columns from entries that meet a condition\n");
    printf("3 Certain columns from all entries\n");
    printf("4 Certain columns from entries that meet a condition\n");
    printf("Choose type of select operation: ");

    fgets(input_buffer, INPUT_BUFFER_SIZE, stdin);
    // Replace newline at the end with termination character
    input_buffer[strlen(input_buffer) - 1] = '\0';

    switch (input_buffer[0])
    {
    // Select all columns from all entries
    case '1':
        select_all();
        break;
    // Select all columns from entries that meet a condition
    case '2':
        select_all_where();
        break;
    // Select certain columns from all entries
    case '3':
        select_all_cols();
        break;
    // Select certain columns from entries that meet a condition
    case '4':
        select_cols_where();
        break;
    default:
        break;
    }
}

int select_all()
{
    do
    {
        // Append request type to request
        strcpy(request_buffer, "select_all,");
        strcpy(statement, "SELECT * FROM ");

        // Read table name
        printf("Enter table name: ");
        fgets(input_buffer, INPUT_BUFFER_SIZE, stdin);
        // Replace newline at the end with termination character
        input_buffer[strlen(input_buffer) - 1] = '\0';

        // Append table name to request
        strcat(statement, input_buffer);
        strcat(statement, ";");
        strcat(request_buffer, input_buffer);

        printf("%s\n", statement);

        printf("Do you confirm the execution of this statement? (y/n)");
        // Get user confirmation
        fgets(input_buffer, INPUT_BUFFER_SIZE, stdin);
        // Replace newline at the end with termination character
        input_buffer[strlen(input_buffer) - 1] = '\0';
    } while (tolower(input_buffer[0]) != 'y');


    // Send request to server
    if (send(socket_desc, request_buffer, strlen(request_buffer), 0) < 0)
        return ERROR_REQUEST_NOT_SENT;

    // Receive reply from server
    if (recv(socket_desc, reply_buffer, REPLY_BUFFER_SIZE, 0) < 0)
        return ERROR_REPLY_NOT_RECEIVED;

    print_result_table(reply_buffer);
    printf("\n");
    
    return OK_SELECT;
}

int select_all_where()
{
    do
    {
        // Append request type to request
        strcpy(request_buffer, "select_all_where,");
        strcpy(statement, "SELECT * FROM ");

        // Read table name
        printf("Enter table name: ");
        fgets(input_buffer, INPUT_BUFFER_SIZE, stdin);
        // Replace newline at the end with termination character
        input_buffer[strlen(input_buffer) - 1] = '\0';

        // Append table name to request
        strcat(statement, input_buffer);
        strcat(statement, " WHERE ");
        strcat(request_buffer, input_buffer);

        // Read column
        printf("Enter column: ");
        fgets(input_buffer, INPUT_BUFFER_SIZE, stdin);
        // Replace newline at the end with termination character
        input_buffer[strlen(input_buffer) - 1] = '\0';

        strcat(statement, input_buffer);
        strcat(statement, " = ");
        strcat(request_buffer, ",");
        strcat(request_buffer, input_buffer);

        // Read required value
        printf("Enter required value: ");
        fgets(input_buffer, INPUT_BUFFER_SIZE, stdin);
        // Replace newline at the end with termination character
        input_buffer[strlen(input_buffer) - 1] = '\0';

        strcat(statement, input_buffer);
        strcat(statement, ";");
        strcat(request_buffer, ",");
        strcat(request_buffer, input_buffer);

        printf("%s\n", statement);

        printf("Do you confirm the execution of this statement? (y/n)");
        // Get user confirmation
        fgets(input_buffer, INPUT_BUFFER_SIZE, stdin);
        // Replace newline at the end with termination character
        input_buffer[strlen(input_buffer) - 1] = '\0';
    } while (tolower(input_buffer[0]) != 'y');

    // Send request to server
    if (send(socket_desc, request_buffer, strlen(request_buffer), 0) < 0)
        return ERROR_REQUEST_NOT_SENT;

    // Receive reply from server
    if (recv(socket_desc, reply_buffer, REPLY_BUFFER_SIZE, 0) < 0)
        return ERROR_REPLY_NOT_RECEIVED;

    print_result_table(reply_buffer);
    printf("\n");

    return OK_SELECT;
}

int select_all_cols()
{
    char* table;
    
    // Read insert statement until user confirms their input
    do
    {
        // Append request type to request
        strcpy(request_buffer, "select_all_cols,");
        strcpy(statement, "SELECT ");

        // Read table name
        printf("Enter table name: ");
        fgets(input_buffer, INPUT_BUFFER_SIZE, stdin);
        // Replace newline at the end with termination character
        input_buffer[strlen(input_buffer) - 1] = '\0';
        
        strcat(request_buffer, input_buffer);

        table = malloc(strlen(input_buffer));
        strcpy(table, input_buffer);

        // Read values until user enters empty line
        printf("Enter columns one per line (enter empty line to finish):\n");
        do
        {
            fgets(input_buffer, INPUT_BUFFER_SIZE, stdin);
            // Replace newline at the end with termination character
            input_buffer[strlen(input_buffer) - 1] = '\0';

            // Append non-empty values to request
            if (input_buffer[0] != '\0')
            {
                strcat(request_buffer, ",");
                strcat(request_buffer, input_buffer);
                strcat(statement, input_buffer);
                strcat(statement, ", ");
            }
            
        } while (input_buffer[0] != '\0');

        statement[strlen(statement) - 2] = ' ';
        statement[strlen(statement) - 1] = '\0';
        strcat(statement, "FROM ");

        // Append table name to request
        strcat(statement, table);
        
        // Final statement formating
        strcat(statement, ";");

        printf("%s\n", statement);

        printf("Do you confirm the execution of this statement? (y/n)");
        // Get user confirmation
        fgets(input_buffer, INPUT_BUFFER_SIZE, stdin);
        // Replace newline at the end with termination character
        input_buffer[strlen(input_buffer) - 1] = '\0';

        free(table);
    } while (tolower(input_buffer[0]) != 'y');

    // Send request to server
    if (send(socket_desc, request_buffer, strlen(request_buffer), 0) < 0)
        return ERROR_REQUEST_NOT_SENT;

    // Receive reply from server
    if (recv(socket_desc, reply_buffer, REPLY_BUFFER_SIZE, 0) < 0)
        return ERROR_REPLY_NOT_RECEIVED;

    print_result_table(reply_buffer);

    printf("\n");
    return OK_SELECT;
}

int select_cols_where()
{
    char* table;
    char* column;
    char* value;
    
    // Read insert statement until user confirms their input
    do
    {
        // Append request type to request
        strcpy(request_buffer, "select_cols_where,");
        strcpy(statement, "SELECT ");

        // Read table name
        printf("Enter table name: ");
        fgets(input_buffer, INPUT_BUFFER_SIZE, stdin);
        // Replace newline at the end with termination character
        input_buffer[strlen(input_buffer) - 1] = '\0';

        strcat(request_buffer, input_buffer);
        table = malloc(strlen(input_buffer));
        strcpy(table, input_buffer);

        // Read column
        printf("Enter column: ");
        fgets(input_buffer, INPUT_BUFFER_SIZE, stdin);
        // Replace newline at the end with termination character
        input_buffer[strlen(input_buffer) - 1] = '\0';

        strcat(request_buffer, ",");
        strcat(request_buffer, input_buffer);
        column = malloc(strlen(input_buffer));
        strcpy(column, input_buffer);

        // Read required value
        printf("Enter required value: ");
        fgets(input_buffer, INPUT_BUFFER_SIZE, stdin);
        // Replace newline at the end with termination character
        input_buffer[strlen(input_buffer) - 1] = '\0';

        strcat(request_buffer, ",");
        strcat(request_buffer, input_buffer);
        value = malloc(strlen(input_buffer));
        strcpy(value, input_buffer);

        // Read values until user enters empty line
        printf("Enter columns one per line (enter empty line to finish):\n");
        do
        {
            fgets(input_buffer, INPUT_BUFFER_SIZE, stdin);
            // Replace newline at the end with termination character
            input_buffer[strlen(input_buffer) - 1] = '\0';

            // Append non-empty values to request
            if (input_buffer[0] != '\0')
            {
                strcat(request_buffer, ",");
                strcat(request_buffer, input_buffer);
                strcat(statement, input_buffer);
                strcat(statement, ", ");
            }
            
        } while (input_buffer[0] != '\0');

        statement[strlen(statement) - 2] = ' ';
        statement[strlen(statement) - 1] = '\0';

        strcat(statement, "FROM ");

        // Append table name to request
        strcat(statement, table);

        strcat(statement, " WHERE ");
        strcat(statement, column);
        strcat(statement, " = ");
        strcat(statement, value);

        // Final statement formating
        strcat(statement, ";");

        printf("%s\n", statement);

        printf("Do you confirm the execution of this statement? (y/n)");
        // Get user confirmation
        fgets(input_buffer, INPUT_BUFFER_SIZE, stdin);
        // Replace newline at the end with termination character
        input_buffer[strlen(input_buffer) - 1] = '\0';

        free(table);
        free(column);
        free(value);
    } while (tolower(input_buffer[0]) != 'y');

    // Send request to server
    if (send(socket_desc, request_buffer, strlen(request_buffer), 0) < 0)
        return ERROR_REQUEST_NOT_SENT;

    // Receive reply from server
    if (recv(socket_desc, reply_buffer, REPLY_BUFFER_SIZE, 0) < 0)
        return ERROR_REPLY_NOT_RECEIVED;

    print_result_table(reply_buffer);

    printf("\n");
    return OK_SELECT;
}

int join_db()
{
    clear_reply_buffer();
    
    char* table1;
    char* table2;

    // Read join statement until user confirms their input
    do
    {
        // Append request type to request
        strcpy(request_buffer, "join,");
        strcpy(statement, "SELECT * FROM ");

        // Read table 1 name
        printf("Enter table 1 name: ");
        fgets(input_buffer, INPUT_BUFFER_SIZE, stdin);
        // Replace newline at the end with termination character
        input_buffer[strlen(input_buffer) - 1] = '\0';

        // Append table 1 name to request
        strcat(statement, input_buffer);
        strcat(request_buffer, input_buffer);

        // Store table 1 name for later use
        table1 = malloc(strlen(input_buffer));
        strcpy(table1, input_buffer);

        // Read table 2 name
        printf("Enter table 2 name: ");
        fgets(input_buffer, INPUT_BUFFER_SIZE, stdin);
        // Replace newline at the end with termination character
        input_buffer[strlen(input_buffer) - 1] = '\0';
        
        strcat(statement, " JOIN ");

        // Append table 2 name to request
        strcat(statement, input_buffer);
        strcat(request_buffer, ",");
        strcat(request_buffer, input_buffer);

        // Store table 2 name for later use
        table2 = malloc(strlen(input_buffer));
        strcpy(table2, input_buffer);

        // Read link column 1
        printf("Enter link column from table 1: ");
        fgets(input_buffer, INPUT_BUFFER_SIZE, stdin);
        // Replace newline at the end with termination character
        input_buffer[strlen(input_buffer) - 1] = '\0';

        strcat(statement, " ON ");
        strcat(statement, table1);
        strcat(statement, ".");

        // Append link column 1 to request
        strcat(statement, input_buffer);
        strcat(request_buffer, ",");
        strcat(request_buffer, input_buffer);

        strcat(statement, " = ");

        // Read link column 2
        printf("Enter link column from table 2: ");
        fgets(input_buffer, INPUT_BUFFER_SIZE, stdin);
        // Replace newline at the end with termination character
        input_buffer[strlen(input_buffer) - 1] = '\0';

        strcat(statement, table2);
        strcat(statement, ".");

        // Append link column 1 to request
        strcat(statement, input_buffer);
        strcat(request_buffer, ",");
        strcat(request_buffer, input_buffer);

        strcat(statement, ";");

        // PRUEBA
        printf("%s\n", statement);

        printf("Do you confirm the execution of this statement? (y/n)");
        // Get user confirmation
        fgets(input_buffer, INPUT_BUFFER_SIZE, stdin);
        // Replace newline at the end with termination character
        input_buffer[strlen(input_buffer) - 1] = '\0';

        free(table1);
        free(table2);
    } while (tolower(input_buffer[0]) != 'y');

    // Send request to server
    if (send(socket_desc, request_buffer, strlen(request_buffer), 0) < 0)
        return ERROR_REQUEST_NOT_SENT;

    // Receive reply from server
    if (recv(socket_desc, reply_buffer, REPLY_BUFFER_SIZE, 0) < 0)
        return ERROR_REPLY_NOT_RECEIVED;

    print_result_table(reply_buffer);

    printf("\n");
    return OK_JOIN;
}

void clear_reply_buffer()
{
    for (int i = 0; i < REPLY_BUFFER_SIZE; i++)
        reply_buffer[i] = '\0';
}