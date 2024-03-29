#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>

#define BUFFER_SIZE 1024

int main(int argc, char *argv[]) {
    if (argc != 4) {
        fprintf(stderr, "Uso: %s <nombredeusuario> <IPdelservidor> <puertodelservidor>\n", argv[0]);
        exit(1);
    }

    char *username = argv[1];
    char *ip = argv[2];
    int port = atoi(argv[3]);

    int sock;
    struct sockaddr_in addr;
    char buffer[BUFFER_SIZE];
    char message[BUFFER_SIZE];

    sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock < 0) {
        perror("[-]Socket error");
        exit(1);
    }
    
    printf("[+]TCP server socket created.\n");

    memset(&addr, '\0', sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);
    addr.sin_addr.s_addr = inet_addr(ip);

    if (connect(sock, (struct sockaddr*)&addr, sizeof(addr)) < 0) {
        perror("Connection failed");
        exit(1);
    }
    printf("Connected to the server.\n");
    
    int option = 1; 
    send(sock, &option, sizeof(int), 0);
    send(sock, username, strlen(username), 0);

    char ip_address[INET_ADDRSTRLEN];
    inet_ntop(AF_INET, &(addr.sin_addr), ip_address, INET_ADDRSTRLEN);
    send(sock, ip_address, strlen(ip_address), 0);

    bzero(buffer, BUFFER_SIZE);
    recv(sock, buffer, BUFFER_SIZE, 0);
    printf("Server: %s\n", buffer);

    while (1) {
        printf("\nOpciones disponibles:\n");
        printf("1. Chatear con todos los usuarios\n");
        printf("2. Enviar y recibir mensajes directos, privados\n");
        printf("3. Cambiar de estado\n");
        printf("4. Listar los usuarios conectados\n");
        printf("5. Obtener información de un usuario\n");
        printf("6. Ayuda\n");
        printf("7. Salir\n");
        printf("Elige una opción: ");
        scanf("%d", &option);
        getchar();

        switch (option) {
            case 1:
                printf("Enter message to broadcast ('quit' to exit): ");
                fgets(buffer, BUFFER_SIZE, stdin);
                buffer[strcspn(buffer, "\n")] = 0;  // Remueve el salto de línea

                if (strcmp(buffer, "quit") == 0) {
                    break;  
                }

                send(sock, &option, sizeof(int), 0);
                send(sock, buffer, strlen(buffer), 0);
            break;
            case 2:
                    option = 4;  // verificar
                    send(sock, &option, sizeof(int), 0); 
    
                    char recipient_username[50];
                    printf("Enter recipient username: ");
                    fgets(recipient_username, 50, stdin);
                    recipient_username[strcspn(recipient_username, "\n")] = '\0'; 
                    send(sock, recipient_username, strlen(recipient_username), 0); // envia el nombre de usuario del destinatario

                    printf("Enter message to send: ");
                    fgets(message, BUFFER_SIZE, stdin);
                    message[strcspn(message, "\n")] = '\0'; 
                    send(sock, message, strlen(message), 0); // envia el mensaje privado
                break;
                case 3:
                    printf("Elige un nuevo estado:\n");
                    printf("1. ACTIVO\n");
                    printf("2. OCUPADO\n");
                    printf("3. INACTIVO\n");
                    printf("Ingresa el número de tu nuevo estado: ");

                    int choice;
                    scanf("%d", &choice);
                    while (getchar() != '\n');  // Limpia el buffer de entrada

                    char *new_status;
                    switch (choice) {
                        case 1:
                            new_status = "ACTIVO";
                            break;
                        case 2:
                            new_status = "OCUPADO";
                            break;
                        case 3:
                            new_status = "INACTIVO";
                            break;
                        default:
                            printf("Opción no válida. Selecciona un estado válido.\n");
                            return;
                    }

                    option = 3;  //VERIFICAR
                    send(sock, &option, sizeof(int), 0);  

                    send(sock, new_status, strlen(new_status) + 1, 0);  

                break;
            case 4:
                option = 4;  //VERIFICAR
                send(sock, &option, sizeof(int), 0);

                    int num_users;
                    recv(sock, &num_users, sizeof(int), 0);

                    // Recibe y muestra la lista de usuarios conectados
                    printf("Connected users:\n");
                    for (int i = 0; i < num_users; i++) {
                        char username[50] = {0};
                        recv(sock, username, sizeof(username), 0);
                        printf("- %s\n", username);
                    }
                break;
            case 5:
                    option = 5;  

                    send(sock, &option, sizeof(int), 0);

                    char target_username[50];
                    printf("Enter username to get information: ");
                    fgets(target_username, 50, stdin);
                    target_username[strcspn(target_username, "\n")] = '\0';
                    send(sock, target_username, strlen(target_username), 0);

                    int user_found;
                    recv(sock, &user_found, sizeof(int), 0);  // Recibimos el indicador de usuario encontrado o no

                    if (user_found) {
                        // Si se encontró el usuario, recibimos la información
                        char username[50] = {0};
                        char ip[INET_ADDRSTRLEN] = {0};
                        char status[20] = {0};

                        recv(sock, &username, sizeof(username), 0);
                        recv(sock, &ip, sizeof(ip), 0);
                        recv(sock, &status, sizeof(status), 0);

                        // recibido
                        printf("User info:\n");
                        printf("- Username: %s\n", username);
                        printf("- IP: %s\n", ip);
                        printf("- Status: %s\n", status);
                    } else {
                        printf("User not found.\n");
                    }

                break;
            case 6:
                    printf("\nComandos disponibles:\n");
                    printf("1. Chatear con todos los usuarios\n");
                    printf("2. Enviar y recibir mensajes directos, privados\n");
                    printf("3. Cambiar de estado\n");
                    printf("4. Listar los usuarios conectados\n");
                    printf("5. Obtener información de un usuario\n");
                    printf("6. Ayuda\n");
                    printf("7. Salir\n");
                break;
            case 7:
                printf("Saliendo...\n");
                close(sock);
                return 0;
            default:
                printf("Opción no válida. Por favor, intenta de nuevo.\n");
        }
    }

    return 0;
}