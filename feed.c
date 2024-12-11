#include "utils.h"

// Función para enviar el pedido al manager
int sendManager(Request *request) {
    int fd = open(MAN_PIPE, O_WRONLY);
    if (fd == -1) {
        printf("[ERROR] Failed to open manager pipe.\n");
        return 0;
    }

    if (write(fd, request, sizeof(*request)) == -1) {
        printf("[ERROR] Failed to send validation request.\n");
        close(fd);
        return 0;
    }
    close(fd);
    return 1;  // Pedido enviado correctamente
}

// Función para leer la respuesta del manager
int readManager(int *response, const char *user_fifo) {
    int fd_feed = open(user_fifo, O_RDONLY);
    if (fd_feed == -1) {
        printf("[ERROR] Failed to open user pipe.\n");
        return 0;
    }

    if (read(fd_feed, response, sizeof(*response)) == -1) {
        printf("[ERROR] Failed to read validation response!\n");
        close(fd_feed);
        return 0;
    }
    close(fd_feed);
    return 1;  // Respuesta leida correctamente
}

// Función para enviar el nombre de usuario al proceso manager para su validación
int sendUser(char *username, const char *user_fifo) {
    Request request;
    int response;  

    // Preparar la estructura de solicitud con el pid y el nombre de usuario
    request.pid = getpid();
    strncpy(request.username, username, sizeof(request.username) - 1);
    request.username[sizeof(request.username) - 1] = '\0';
    request.type = 2;
    snprintf(request.FIFO, sizeof(request.FIFO), FEED_PIPE, request.pid);

    // Enviar la solicitud al proceso manager
    if (!sendManager(&request)) {
        return 0;  
    }

    // Esperar la respuesta del proceso manager
    if (!readManager(&response, user_fifo)) {
        return 0;  
    }

    // Validar la respuesta recibida
    if (response == 0) {
        printf("[ERROR] Username not allowed!\n");
        return 0;  
    }

    return 1;  // Usuario validado correctamente
}

// Suscribirse a un tema
void subscribe(char *topic) {
    Request request = {
        .pid = getpid(),
        .type = 0
    };

    // Copiar el nombre del tema en la estructura, asegurando la terminación nula
    strncpy(request.topic, topic, sizeof(request.topic) - 1);
    request.topic[sizeof(request.topic) - 1] = '\0';

    // Abrir el pipe del manager en modo escritura no bloqueante
    int fd = open(MAN_PIPE, O_WRONLY | O_NONBLOCK);
    if (fd < 0) {
        perror("[ERROR] Failed to open manager pipe");
        return;
    }

    // Preparar el buffer para enviar los datos
    const char *buffer = (const char *)&request;
    ssize_t remaining = sizeof(request);

    // Enviar los datos asegurando que se escriban completamente
    while (remaining > 0) {
        ssize_t written = write(fd, buffer, remaining);
        if (written < 0) {
            perror("[ERROR] Failed to send subscription request");
            close(fd); // Cerrar el descriptor en caso de error
            return;
        }
        buffer += written;   // Avanzar el puntero del buffer
        remaining -= written; // Reducir los bytes restantes
    }

    close(fd);
    printf("Successfully subscribed to topic: %s\n", topic);
}

// Cancelar suscripción de un tema
void unsubscribe(char *topic) {
    Request request = {
        .pid = getpid(),
        .type = 1
    };

    // Copiar el nombre del tema en la estructura, asegurando la terminación nula
    strncpy(request.topic, topic, sizeof(request.topic) - 1);
    request.topic[sizeof(request.topic) - 1] = '\0';

    // Abrir el pipe del manager en modo escritura no bloqueante
    int fd = open(MAN_PIPE, O_WRONLY | O_NONBLOCK);
    if (fd < 0) {
        perror("[ERROR] Failed to open manager pipe");
        return;
    }

    // Preparar el buffer para enviar los datos
    const char *buffer = (const char *)&request;
    ssize_t remaining = sizeof(request);

    // Enviar los datos asegurando que se escriban completamente
    while (remaining > 0) {
        ssize_t written = write(fd, buffer, remaining);
        if (written < 0) {
            perror("[ERROR] Failed to send unsubscription request");
            close(fd); // Cerrar el descriptor en caso de error
            return;
        }
        buffer += written;   // Avanzar el puntero del buffer
        remaining -= written; // Reducir los bytes restantes
    }

    close(fd);
    printf("Successfully unsubscribed from topic: %s\n", topic);
}

// Enviar un mensaje a un tema
void sendMessage(char *topic, int duration, char *message, char *username) {
    Message sendMsg;

    // Copiar los valores asegurando que las cadenas estén terminadas en nulo
    strncpy(sendMsg.topic, topic, sizeof(sendMsg.topic) - 1);
    sendMsg.topic[sizeof(sendMsg.topic) - 1] = '\0';

    strncpy(sendMsg.message, message, sizeof(sendMsg.message) - 1);
    sendMsg.message[sizeof(sendMsg.message) - 1] = '\0';

    strncpy(sendMsg.name, username, sizeof(sendMsg.name) - 1);
    sendMsg.name[sizeof(sendMsg.name) - 1] = '\0';

    sendMsg.duration = duration;
    sendMsg.pid = getpid();

    // Abrir el pipe del manager en modo escritura no bloqueante
    int fd = open(MAN_PIPE, O_WRONLY | O_NONBLOCK);
    if (fd < 0) {
        perror("[ERROR] Failed to open manager pipe");
        exit(EXIT_FAILURE);
    }

    // Preparar el buffer para enviar los datos
    const char *buffer = (const char *)&sendMsg;
    ssize_t remaining = sizeof(sendMsg);

    // Enviar los datos asegurando que se escriban completamente
    while (remaining > 0) {
        ssize_t written = write(fd, buffer, remaining);
        if (written < 0) {
            perror("[ERROR] Failed to send message");
            close(fd); // Cerrar el descriptor en caso de error
            exit(EXIT_FAILURE);
        }
        buffer += written;   // Avanzar el puntero del buffer
        remaining -= written; // Reducir los bytes restantes
    }

    close(fd);
}

// Salir de la sesión
void endSession(const char *user_fifo) {
    Request request = { .pid = getpid(), .type = 3 };

    // Abrir el pipe del manager en modo escritura no bloqueante
    int fd = open(MAN_PIPE, O_WRONLY | O_NONBLOCK);
    if (fd < 0) {
        printf("[ERROR] Could not open manager pipe.\n");
        return;
    }

    // Enviar el mensaje de salida asegurando que todos los bytes se escriban
    ssize_t remaining = sizeof(request);
    const char *buffer = (const char *)&request;

    while (remaining > 0) {
        ssize_t written = write(fd, buffer, remaining);
        if (written < 0) {
            printf("[ERROR] Error sending exit request.\n");
            break;
        }
        buffer += written;   // Avanzar el puntero del buffer
        remaining -= written; // Reducir los bytes restantes
    }

    close(fd);

    // Eliminar el pipe del usuario
    if (unlink(user_fifo) == 0) {
        printf("User pipe for the session has been removed successfully.\n");
    } else {
        printf("[WARNING] Could not remove PIPE: %s\n", user_fifo);
    }

    printf("Session ended successfully\n");
    exit(1);
}

int main(int argc, char *argv[]) {
    if (argc != 2) {
        printf("[ERROR] Invalid number of arguments.\n");
        printf("Format: %s <username>\n", argv[0]);
        return 1;
    }

    char *username = argv[1];
    char user_fifo[256];
    snprintf(user_fifo, sizeof(user_fifo), FEED_PIPE, getpid());

    // Crear el FIFO para el cliente
    if (mkfifo(user_fifo, 0666) == -1 && errno != EEXIST) {
        printf("[ERROR] Failed to create user pipe.\n");
        return 1;
    }

    printf("Welcome, %s!\n", username);
    printf("Enter the command you want to execute: \n");

    // Enviar datos del usuario al manager
    if (!sendUser(username, user_fifo)) {
        unlink(user_fifo);
        return 1;
    }

    // Abrir el pipe del usuario
    int fd_feed = open(user_fifo, O_RDONLY | O_NONBLOCK);
    if (fd_feed == -1) {
        printf("[ERROR] Failed to open user pipe.\n");
        return 1;
    }

    char input[256];
    Message msg;

    while (1) { 
        // Inicializar el conjunto de descriptores para `select`
        fd_set read_fds;
        FD_ZERO(&read_fds);
        FD_SET(0, &read_fds); // Entrada estándar
        FD_SET(fd_feed, &read_fds); // Añadir pipe del usuario

        int max_fd = fd_feed > 0 ? fd_feed : 0;
        if (select(max_fd + 1, &read_fds, NULL, NULL, NULL) == -1) {
            perror("[ERROR] Select failed.\n");
            break;
        }

        if (FD_ISSET(0, &read_fds)) {
            fgets(input, sizeof(input), stdin);
            input[strcspn(input, "\n")] = 0;

            char command[20];
            sscanf(input, "%s", command);

            switch (command[0]) {
                case 's':
                    if (strncmp(command, "subscribe", 9) == 0) {
                        char topic[MAX_TOPIC_LENGTH];
                        sscanf(input + 10, "%s", topic);
                        subscribe(topic);
                    } else {
                        printf("[ERROR] Unknown command! Did you mean subscribe?\n");
                    }
                    break;
                case 'u':
                    if (strncmp(command, "unsubscribe", 11) == 0) {
                        char topic[MAX_TOPIC_LENGTH];
                        sscanf(input + 12, "%s", topic);
                        unsubscribe(topic);
                    } else {
                        printf("[ERROR] Unknown command! Did you mean unsubscribe?\n");
                    }
                    break;
                case 'm':
                    if (strncmp(command, "msg", 3) == 0) {
                        char topic[MAX_TOPIC_LENGTH];
                        int duration;
                        char message[MAX_MSG_SIZE];
                        sscanf(input + 4, "%s %d %[^\n]", topic, &duration, message);
                        sendMessage(topic, duration, message, username);
                    } else {
                        printf("[ERROR] Unknown command! Did you mean msg? \n");
                    }
                    break;
                case 'e':
                    if (strcmp(command, "exit") == 0) {
                        endSession(user_fifo);
                        break;
                    } else {
                        printf("[ERROR] Unknown command! Did you mean exit?\n");
                    }
                    break;
                default:
                    printf("[ERROR] Unknown command!Valid commands are: subscribe, unsubscribe, msg, exit. \n");
                    break;
            }
        }

        if (FD_ISSET(fd_feed, &read_fds)) {
            int size = read(fd_feed, &msg, sizeof(msg));
            if (size > 0)
            {
                if (msg.closed == 1) {
                    printf("Manager terminated!! Exiting...\n");
                    fflush(stdout);
                    unlink(user_fifo);
                    break;
                }
                if (msg.blocked == 1) {
                    printf("%s", msg.message);
                    fflush(stdout);
                } else {
                    printf("<%s> %s - %s \n", msg.topic, msg.name, msg.message);
                    fflush(stdout);
                }
            }
        }
    }

    close(fd_feed);
    return 0;
}
