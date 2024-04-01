#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include <sys/socket.h>
#include <netinet/in.h>

#define MAX_CLIENTS 100
#define BUFFER_SIZE 1024
#define DEFAULT_PORT 8888

typedef struct {
    int socket;
    char username[50];
    char ip[INET_ADDRSTRLEN];
    char status[20];
} Client;

typedef struct {
    Client clients[MAX_CLIENTS];
    int client_count;
    pthread_mutex_t mutex;
} ServerInfo;

ServerInfo server;


void *handle_client(void *arg);
void handle_request(int client_socket, int option);
void register_user(int client_socket, char *username, char *ip);
void send_connected_users(int client_socket);
void change_status(int client_socket, const char *new_status);
void send_message(char *recipient_username, char *message, int sender_socket);
void send_user_info(int client_socket, char *username);
void send_response(int client_socket, int option, int code, char *message);
void broadcast_message(char *message, int sender_socket);
void send_simple_response(int client_socket, const char *message); 


void *handle_client(void *arg) {
    int client_socket = *((int *)arg);
    int option;

    if (recv(client_socket, &option, sizeof(option), 0) <= 0) {
        perror("Error receiving option from client");
        close(client_socket);
        pthread_exit(NULL);
    }

    handle_request(client_socket, option);

    close(client_socket);
    pthread_exit(NULL);
}


void handle_request(int client_socket, int option) {
    int code; 
    char message[BUFFER_SIZE];
    switch (option) {
        case 1:
            {
                char username[50], ip[INET_ADDRSTRLEN];
                if (recv(client_socket, &username, sizeof(username), 0) <= 0 || recv(client_socket, &ip, sizeof(ip), 0) <= 0) {
                    perror("Error receiving user registration data from client");
                    close(client_socket);
                    pthread_exit(NULL);
                }
                register_user(client_socket, username, ip);
            }
            break;
        case 2: 
            {
                char recipient[50], message_text[BUFFER_SIZE];
                if (recv(client_socket, recipient, sizeof(recipient), 0) <= 0 || recv(client_socket, message_text, sizeof(message_text), 0) <= 0) {
                    perror("Error receiving message data from client");
                    close(client_socket);
                    pthread_exit(NULL);
                }

                send_message(recipient, message_text, client_socket);

            }
            break;
        case 3: 
            {
                char status[20];
                if (recv(client_socket, &status, sizeof(status), 0) <= 0) {
                    perror("Error receiving status change data from client");
                    close(client_socket);
                    pthread_exit(NULL);
                }
                
                change_status(client_socket, status);
            }
            break;
        case 4:
            {
                send_connected_users(client_socket);
            }
            break;
        case 5:
            {
                char username[50];
                if (recv(client_socket, &username, sizeof(username), 0) <= 0) {
                    perror("Error receiving username from client");
                    close(client_socket);
                    pthread_exit(NULL);
                }
                send_user_info(client_socket, username);
            }
            break;
        case 6:
            {

                if (recv(client_socket, message, BUFFER_SIZE, 0) <= 0) {
                    perror("Error receiving broadcast message from client");
                    close(client_socket);
                    pthread_exit(NULL);
                }
                
                broadcast_message(message, client_socket);
            }
            break;
    
        default:
            code = 500;
            sprintf(message, "Error: Invalid option");
            send_response(client_socket, option, code, message);
            break;
    }
}

void register_user(int client_socket, char *username, char *ip_address) {
    int code;
    char message[BUFFER_SIZE];

    pthread_mutex_lock(&server.mutex);

    int i;
    for (i = 0; i < server.client_count; i++) {
        if (strcmp(server.clients[i].username, username) == 0) {
            code = 500; 
            sprintf(message, "Error: User '%s' already exists", username);
            send_response(client_socket, 1, code, message);
            pthread_mutex_unlock(&server.mutex);
            return;
        }
    }

    strcpy(server.clients[server.client_count].username, username);
    strcpy(server.clients[server.client_count].ip, ip_address);
    strcpy(server.clients[server.client_count].status, "ACTIVO");
    server.clients[server.client_count].socket = client_socket;
    server.client_count++;

    code = 200;
    sprintf(message, "User '%s' registered successfully", username);
    send_response(client_socket, 1, code, message);

    pthread_mutex_unlock(&server.mutex);
}

void send_connected_users(int client_socket) {
    int count = server.client_count;
    send(client_socket, &count, sizeof(int), 0);
    for (int i = 0; i < server.client_count; i++) {
        send(client_socket, server.clients[i].username, sizeof(server.clients[i].username), 0);
    }
}

void send_simple_response(int client_socket, const char *message) {
    send(client_socket, message, strlen(message) + 1, 0); 
}

void change_status(int client_socket, const char *new_status) {
    pthread_mutex_lock(&server.mutex);
    for (int i = 0; i < server.client_count; i++) {
        if (server.clients[i].socket == client_socket) {
            strcpy(server.clients[i].status, new_status);
            send_simple_response(client_socket, "Estado actualizado con éxito.");
            break;
        }
    }
    pthread_mutex_unlock(&server.mutex);
}


void send_message(char *recipient_username, char *message, int sender_socket) {
    pthread_mutex_lock(&server.mutex);

    for (int i = 0; i < server.client_count; i++) {
        if (strcmp(server.clients[i].username, recipient_username) == 0) {
            send(server.clients[i].socket, message, strlen(message), 0);
            break;
        }
    }

    pthread_mutex_unlock(&server.mutex);
}


void send_user_info(int client_socket, char *username) {
    pthread_mutex_lock(&server.mutex);
    int user_found = 0; 

    for (int i = 0; i < server.client_count; i++) {
        if (strcmp(server.clients[i].username, username) == 0) {
            user_found = 1; 
            send(client_socket, &user_found, sizeof(int), 0);
            
            send(client_socket, &server.clients[i].username, sizeof(server.clients[i].username), 0);
            send(client_socket, &server.clients[i].ip, sizeof(server.clients[i].ip), 0);
            send(client_socket, &server.clients[i].status, sizeof(server.clients[i].status), 0);
            break;
        }
    }

    if (!user_found) {
        send(client_socket, &user_found, sizeof(int), 0);  
    }

    pthread_mutex_unlock(&server.mutex);
}

void send_response(int client_socket, int option, int code, char *message) {
    send(client_socket, &option, sizeof(int), 0);
    send(client_socket, &code, sizeof(int), 0);
    send(client_socket, message, strlen(message), 0);
}

void broadcast_message(char *message, int sender_socket) {
    pthread_mutex_lock(&server.mutex);

    printf("Mensaje broadcast recibido: %s\n", message);

    for (int i = 0; i < server.client_count; i++) {
        if (server.clients[i].socket != sender_socket) {
            send(server.clients[i].socket, message, strlen(message), 0);
        }
    }

    pthread_mutex_unlock(&server.mutex);
}


int main(int argc, char *argv[]) {
    int server_socket, client_socket, port;
    struct sockaddr_in server_addr, client_addr;
    socklen_t client_len = sizeof(client_addr);
    pthread_t tid;

    if (argc < 2) {
        port = DEFAULT_PORT;
    } else {
        port = atoi(argv[1]);
    }

    if ((server_socket = socket(AF_INET, SOCK_STREAM, 0)) == 0) {
        perror("Socket creation failed");
        exit(EXIT_FAILURE);
    }

    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(port);

    if (bind(server_socket, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
        perror("Bind failed");
        exit(EXIT_FAILURE);
    }

    if (listen(server_socket, 5) < 0) {
        perror("Listen failed");
        exit(EXIT_FAILURE);
    }

    printf("Server listening on port %d...\n", port);

    server.client_count = 0;
    pthread_mutex_init(&server.mutex, NULL);

    while (1) {
        if ((client_socket = accept(server_socket, (struct sockaddr *)&client_addr, &client_len)) < 0) {
            perror("Accept failed");
            exit(EXIT_FAILURE);
        }

        if (pthread_create(&tid, NULL, handle_client, &client_socket) != 0) {
            perror("Thread creation failed");
            exit(EXIT_FAILURE);
        }
    }

    close(server_socket);
    pthread_mutex_destroy(&server.mutex);

    return 0;
}
