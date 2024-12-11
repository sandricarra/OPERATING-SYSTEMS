#include "utils.h"

// Función para enviar el pedido al manager
int enviarPedidoManager(pedidoStruct *pedido) {
    int fd = open(MAN_PIPE, O_WRONLY);
    if (fd == -1) {
        printf("[ERROR] Failed to open manager pipe.\n");
        return 0;
    }

    if (write(fd, pedido, sizeof(*pedido)) == -1) {
        printf("Failed to send validation request.\n");
        close(fd);
        return 0;
    }
    close(fd);
    return 1;  // Pedido enviado correctamente
}

// Función para leer la respuesta del manager
int leerRespuestaManager(int *resposta,const char *user_fifo ) {
    int fd_feed = open(user_fifo, O_RDONLY);
    if (fd_feed == -1) {
        printf("[ERROR] Failed to open user pipe.\n");
        return 0;
    }

    if (read(fd_feed, resposta, sizeof(*resposta)) == -1) {
        printf("\n[ERROR] Failed to read validation response!\n");
        close(fd_feed);
        return 0;
    }
    close(fd_feed);
    return 1;  // Respuesta leída correctamente
}

// Función para enviar el nombre de usuario al proceso manager para su validación
int enviaUser(char *username, const char *user_fifo) {
    pedidoStruct pedido;
    int resposta;  // Cambiado a int en lugar de respostaStruct

    // Preparar la estructura de pedido con el pid y el nombre de usuario
    pedido.pid = getpid();
    strncpy(pedido.username, username, sizeof(pedido.username) - 1);
    pedido.username[sizeof(pedido.username) - 1] = '\0';
    pedido.tipo = 2;
    snprintf(pedido.FIFO, sizeof(pedido.FIFO), FEED_PIPE , pedido.pid);

    // Enviar el pedido al proceso manager
    if (!enviarPedidoManager(&pedido)) {
        return 0;  
    }

    // Esperar la respuesta del proceso manager
    if (!leerRespuestaManager(&resposta, user_fifo)) {
        return 0;  
    }

    // Validar la respuesta recibida
    if (resposta == 0) {
        printf("\n[ERROR] Username not allowed!\n");
        return 0;  // El nombre de usuario no está permitido
    }

    return 1;  // Usuario validado correctamente
}



// Subscribe to a topic
void commSubscribe(char *topico) {

    pedidoStruct pedido = {
        .pid = getpid(),
        .tipo = 0 
    };
    // Copiar el tópico en la estructura, asegurando la terminación nula
    strncpy(pedido.topico, topico, sizeof(pedido.topico) - 1);
    pedido.topico[sizeof(pedido.topico) - 1] = '\0';

    // Abrir el FIFO del manager en modo escritura no bloqueante
    int fd = open(MAN_PIPE, O_WRONLY | O_NONBLOCK);
    if (fd < 0) {
        // Manejar errores de apertura
        perror("[ERROR] Failed to open manager pipe");
        return;
    }

    // Preparar el buffer para enviar los datos
    const char *buffer = (const char *)&pedido;
    ssize_t remaining = sizeof(pedido);

    // Enviar los datos asegurando la escritura completa
    while (remaining > 0) {
        ssize_t written = write(fd, buffer, remaining);
        if (written < 0) {
            // Manejo de errores durante la escritura
            perror("[ERROR] Failed to send subscription request");
            close(fd); // Cerrar el descriptor en caso de error
            return;
        }
        buffer += written;  // Avanzar el puntero del buffer
        remaining -= written; // Reducir los bytes restantes
    }


    close(fd);


    printf("Successfully subscribed to topic: %s\n", topico);
}

// Unsubscribe from a topic
void commUnsubscribe(char *topico) {
 
    pedidoStruct pedido = {
        .pid = getpid(),
        .tipo = 1 
    };
    // Copiar el tópico en la estructura, asegurando la terminación nula
    strncpy(pedido.topico, topico, sizeof(pedido.topico) - 1);
    pedido.topico[sizeof(pedido.topico) - 1] = '\0';

    // Abrir el FIFO del manager en modo escritura no bloqueante
    int fd = open(MAN_PIPE, O_WRONLY | O_NONBLOCK);
    if (fd < 0) {
        // Manejar errores de apertura
        perror("[ERROR] Failed to open manager pipe");
        return;
    }

    // Preparar el buffer para enviar los datos
    const char *buffer = (const char *)&pedido;
    ssize_t remaining = sizeof(pedido);

    // Enviar los datos asegurando la escritura completa
    while (remaining > 0) {
        ssize_t written = write(fd, buffer, remaining);
        if (written < 0) {
            // Manejo de errores durante la escritura
            perror("[ERROR] Failed to send unsubscription request");
            close(fd); // Cerrar el descriptor en caso de error
            return;
        }
        buffer += written;  // Avanzar el puntero del buffer
        remaining -= written; // Reducir los bytes restantes
    }

    close(fd);


    printf("Successfully unsubscribed from topic: %s\n", topico);
}

// Comando msg.
void commMsg(char *topico, int duracao, char *menssagem, char *username) {
    // Estructura para el mensaje
    msgStruct sendMsg;

    // Copiar los valores asegurando la terminación nula de las cadenas
    strncpy(sendMsg.topico, topico, sizeof(sendMsg.topico) - 1);
    sendMsg.topico[sizeof(sendMsg.topico) - 1] = '\0';

    strncpy(sendMsg.menssagem, menssagem, sizeof(sendMsg.menssagem) - 1);
    sendMsg.menssagem[sizeof(sendMsg.menssagem) - 1] = '\0';

    strncpy(sendMsg.nome, username, sizeof(sendMsg.nome) - 1);
    sendMsg.nome[sizeof(sendMsg.nome) - 1] = '\0';

    sendMsg.duracao = duracao;
    sendMsg.pid = getpid();

    // Apertura del FIFO del manager en modo escritura no bloqueante
    int fd = open(MAN_PIPE, O_WRONLY | O_NONBLOCK);
    if (fd < 0) {
        // Mensaje de error si el pipe no pudo abrirse
        perror("[ERROR] Failed to open manager pipe");
        exit(EXIT_FAILURE); // Salida segura en caso de error
    }

    // Preparar puntero al buffer para la escritura
    const char *buffer = (const char *)&sendMsg;
    ssize_t remaining = sizeof(sendMsg);

    // Enviar los datos asegurando que se escribe todo el mensaje
    while (remaining > 0) {
        ssize_t written = write(fd, buffer, remaining);
        if (written < 0) {
            // Manejo de errores durante la escritura
            perror("[ERROR] Failed to send message");
            close(fd); // Asegurar el cierre del descriptor
            exit(EXIT_FAILURE); // Salida segura en caso de error
        }
        buffer += written;  // Avanzar el puntero del buffer
        remaining -= written; // Reducir los bytes restantes
    }

    // Cerrar el descriptor del pipe tras escribir
    close(fd);
}

// Comando de exit.
void commExit(const char *user_fifo) {
    // Estructura para el mensaje de salida
    pedidoStruct pedido = { .pid = getpid(), .tipo = 3 };

    // Apertura del pipe del manager en modo escritura no bloqueante
    int fd = open(MAN_PIPE, O_WRONLY | O_NONBLOCK);
    if (fd < 0) {
        // Mensaje de error si el pipe no pudo abrirse
        printf("[ERROR] Could not open manager pipe.\n");
        return;
    }

    // Enviar el mensaje de salida asegurando que todos los bytes se escriban
    ssize_t remaining = sizeof(pedido);
    const char *buffer = (const char *)&pedido;

    while (remaining > 0) {
        ssize_t written = write(fd, buffer, remaining);
        if (written < 0) {
            // Manejo de errores durante la escritura
            printf("[ERROR] Error sending exit request.\n");
            break;
        }
        buffer += written;   // Avanzar el puntero del buffer
        remaining -= written; // Reducir los bytes restantes
    }

    // Cerrar el pipe después de escribir
    close(fd);

    // Eliminar el FIFO del usuario
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
    if (!enviaUser(username,user_fifo)) {
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
    msgStruct msg;

    while (1) { 

        // Inicializar el conjunto de descriptores para `select`
        fd_set read_fds;
        FD_ZERO(&read_fds);
        FD_SET(0, &read_fds); // Entrada estandar
        FD_SET(fd_feed, &read_fds); // Añadir pipe del usuario

        int max_fd = fd_feed > 0 ? fd_feed : 0;
        if (select(max_fd + 1, &read_fds, NULL, NULL, NULL) == -1) {
            printf("[ERROR] Select failed.\n");
            break;
        }

        if (FD_ISSET(0, &read_fds)) {

            fgets(input, sizeof(input), stdin);
            input[strcspn(input, "\n")] = 0;

            char command[20];
            sscanf(input, "%s", command);

            switch (command[0]) {
                /*case 't':
                    if (strcmp(command, "topics") == 0) {
                        commTopicos(FEED_PIPE);
                    } else {
                        printf("[ERROR] Unknown command!\n");
                    }
                    break; -> No funciona correctamente revisar. */
                case 's':
                    if (strncmp(command, "subscribe", 9) == 0) {
                        char topico[20];
                        sscanf(input + 10, "%s", topico);
                        commSubscribe(topico);
                    } else {
                        printf("[ERROR] Unknown command!\n");
                    }
                    break;
                case 'u':
                    if (strncmp(command, "unsubscribe", 11) == 0) {
                        char topico[20];
                        sscanf(input + 12, "%s", topico);
                        commUnsubscribe(topico);
                    } else {
                        printf("[ERROR] Unknown command!\n");
                    }
                    break;
                case 'm':
                    if (strncmp(command, "msg", 3) == 0) {
                        char topico[20];
                        int duracao;
                        char menssagem[300];
                        sscanf(input + 4, "%s %d %[^\n]", topico, &duracao, menssagem);
                        commMsg(topico, duracao, menssagem, username);
                    } else {
                        printf("[ERROR] Unknown command!\n");
                    }
                    break;
                case 'e':
                    if (strcmp(command, "exit") == 0) {
                        commExit(user_fifo);
                        break;
                    } else {
                        printf("[ERROR] Unknown command!\n");
                    }
                    break;
                default:
                    printf("[ERROR] Unknown command!\n");
                    break;
            }
        }

        if (FD_ISSET(fd_feed, &read_fds)) {
            int size = read(fd_feed, &msg, sizeof(msg));
            if (size > 0)
            {
                if(msg.fechado == 1) {
                    printf("Manager terminated!! Exiting...\n");
                    fflush(stdout);
                    unlink(user_fifo);
                    break;
                }
                if(msg.bloqueado == 1) {
                    printf("%s" , msg.menssagem);
                    fflush(stdout);
                } else {
                    printf("<%s> %s - %s \n" , msg.topico , msg.nome , msg.menssagem);
                    fflush(stdout);
                }

            }
        }
    }

    close(fd_feed);
    return 0;
}
