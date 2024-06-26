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
#define MAX_MESSAGES 500 

typedef struct {
    int socket;
    char username[50];
    char ip[INET_ADDRSTRLEN];
    char status[20];
} Client; 

typedef struct {
    char messages[MAX_MESSAGES][BUFFER_SIZE];
    int message_count;
} ChatHistory;

typedef struct {
    Client clients[MAX_CLIENTS];
    int client_count;
    pthread_mutex_t mutex;
    ChatHistory chat_history;
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
void send_group_chat_history(int client_socket);


void *handle_client(void *arg) {
    int client_socket = *((int *)arg);
    int option;

    while (1) {
        if (recv(client_socket, &option, sizeof(option), 0) <= 0) {
            perror("Error receiving option from client");
            break;
        }

        handle_request(client_socket, option);
    }

    // close(client_socket);
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

                int sender_index = -1;
                for (int i = 0; i < server.client_count; i++) {
                    if (server.clients[i].socket == client_socket) {
                        sender_index = i;
                        break;
                    }
                }

                if (sender_index == -1) {
                    perror("Error: Sender not found");
                    close(client_socket);
                    pthread_exit(NULL);
                }

                send_message(recipient, message_text, client_socket);

                printf("Mensaje privado recibido de %s: %s\n", server.clients[sender_index].username, message_text);
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
        case 7:
            send_group_chat_history(client_socket);
            break;
        case 8:
            printf("Cliente desconectado.\n");
            close(client_socket); 
            pthread_exit(NULL); 
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

    printf("Nuevo cliente conectado: %s\n", username);

    pthread_mutex_unlock(&server.mutex);
}


void send_connected_users(int client_socket) {
    int count = server.client_count;
    send(client_socket, &count, sizeof(int), 0);
    for (int i = 0; i < server.client_count; i++) {
        char user_info[100];
        sprintf(user_info, "%s %s", server.clients[i].username, server.clients[i].ip);
        send(client_socket, user_info, sizeof(user_info),0);
    }
}

void send_simple_response(int client_socket, const char *message) {
    send(client_socket, message, strlen(message) + 1, 0); 
}

void *listen_for_messages(void *sock) {
    int server_socket = *((int *)sock);
    char message[BUFFER_SIZE];

    while(1) {
        int read = recv(server_socket, message, BUFFER_SIZE, 0);
        if (read > 0) {
            printf("%s\n", message);
        } else if (read == 0) {
            puts("Disconnected from the server.");
            break;
        } else {
            // else
        }
    }

    return NULL;
}

void change_status(int client_socket, const char *new_status) {
    pthread_mutex_lock(&server.mutex);
    char username[50];
    for (int i = 0; i < server.client_count; i++) {
        if (server.clients[i].socket == client_socket) {
            strcpy(username, server.clients[i].username);
            strcpy(server.clients[i].status, new_status);
            send_simple_response(client_socket, "Estado actualizado con éxito.");

            break;
        }
    }
    pthread_mutex_unlock(&server.mutex);

    printf("El usuario %s cambió de status a %s\n", username, new_status);
}



void send_message(char *recipient_username, char *message, int sender_socket) {
    pthread_mutex_lock(&server.mutex);

    int recipient_found = 0;
    for (int i = 0; i < server.client_count; i++) {
        if (strcmp(server.clients[i].username, recipient_username) == 0) {
            send(server.clients[i].socket, message, strlen(message), 0);
            recipient_found = 1;
            break;
        }
    }

    if (recipient_found) {
        printf("Mensaje privado enviado a %s: %s\n", recipient_username, message);
    } else {
        printf("El usuario %s no está conectado.\n", recipient_username);
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

void send_group_chat_history(int client_socket) {
    pthread_mutex_lock(&server.mutex);

    for (int i = 0; i < server.chat_history.message_count; i++) {
        send(client_socket, server.chat_history.messages[i], strlen(server.chat_history.messages[i]), 0);
    }

    const char *end_marker = "END_OF_HISTORY";
    send(client_socket, end_marker, strlen(end_marker), 0);

    pthread_mutex_unlock(&server.mutex);
}

void broadcast_message(char *message, int sender_socket) {
    pthread_mutex_lock(&server.mutex);

    char sender_username[50];
    int sender_index = -1;
    for (int i = 0; i < server.client_count; i++) {
        if (server.clients[i].socket == sender_socket) {
            sender_index = i;
            strcpy(sender_username, server.clients[i].username);
            break;
        }
    }

    char formatted_message[BUFFER_SIZE];
    snprintf(formatted_message, BUFFER_SIZE, "%s: %s\n", sender_username, message); // Agregar salto de línea

    if (server.chat_history.message_count < MAX_MESSAGES) {
        strcpy(server.chat_history.messages[server.chat_history.message_count], formatted_message);
        server.chat_history.message_count++;
    }

    for (int i = 0; i < server.client_count; i++) {
        send(server.clients[i].socket, formatted_message, strlen(formatted_message), 0);
    }

    if (server.chat_history.message_count >= MAX_MESSAGES) {
        server.chat_history.message_count = 0;
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
    server.chat_history.message_count = 0;

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
