#include "utils.h"

// Definición de variables globales
User user[MAX_USERS];
Topic topic[MAX_TOPICS]; 
PersistMsg message[MAX_MESSAGES]; 

int user_count = 0;
int topic_count = 0; 
int message_count = 0; 

// unción para desconectar usuarios de la sesion
void endSession() {
    Message end;
    end.closed = 1;

    // Recorrer todos los usuarios conectados
    for (int i = 0; i < user_count; i++) {
        if (access(user[i].FIFO, F_OK) != 0) {
            printf("[ERROR] User pipe %d (FIFO: %s) does not exist!\n", user[i].pid, user[i].FIFO);
            continue;
        }

        int fd = open(user[i].FIFO, O_WRONLY);

        // Verificar si no se pudo abrir el FIFO del usuario
        if (fd == -1) {
            perror("open");
            printf("[ERROR] Failed to open user pipe %d (FIFO: %s)!\n", user[i].pid, user[i].FIFO);
            continue;
        }

        // Enviar el mensaje de desconexión al usuario
        if (write(fd, &end, sizeof(end)) == -1) {
            perror("write");
            printf("[ERROR] Failed to send disconnection request to user %d!\n", user[i].pid);
        }

        // Cerrar el descriptor del archivo
        close(fd);
    }

    // Resetear el contador de usuarios después de desconectarlos
    user_count = 0;
    printf("All users have been disconnected.\n");

    // Eliminar el pipe del manager
    if (unlink(MAN_PIPE) == -1) {
        perror("unlink");
        printf("[ERROR] Failed to remove manager pipe.\n");
    } else {
        printf("Manager pipe removed successfully.\n");
    }

    printf("Session ended.\n");
    exit(0);
}

// Funcion que envia mensaje persistente al usuario.
void sendPersistMsg(User *user, char *topic) {
    for (int i = 0; i < message_count; i++) {
        if (strcmp(message[i].topic, topic) == 0) {
            if(message[i].lifetime > 0){
                Message msg;
                strncpy(msg.topic, message[i].topic, sizeof(msg.topic) - 1);
                msg.topic[sizeof(msg.topic) - 1] = '\0';
                strncpy(msg.name, message[i].name, sizeof(msg.name) - 1);
                msg.name[sizeof(msg.name) - 1] = '\0';
                strncpy(msg.message, message[i].message, sizeof(msg.message) - 1);
                msg.message[sizeof(msg.message) - 1] = '\0';
                msg.duration = 0;
                msg.pid = message[i].pid;

                int fd = open(user->FIFO, O_WRONLY);
                if (fd == -1) {
                    printf("[ERROR] Failed to open user FIFO %d!\n", user->pid);
                    continue;
                }

                if (write(fd, &msg, sizeof(msg)) == -1) {
                    printf("[ERROR] Failed to send persistent message to user %d!\n", user->pid);
                }

                close(fd);
            }
        }
    }
}

// Function to create a new topic
void createTopic(const char *topicName, int pid) {
    // Verificar si el tema ya existe
    for (int i = 0; i < topic_count; i++) {
        if (strcmp(topic[i].topicName, topicName) == 0) {
            printf("The topic '%s' already exists!\n", topicName);
            return; // El tema ya existe, no se agrega
        }
    }

    // Si no existe, verificar si se puede agregar un nuevo tema
    if (topic_count < MAX_TOPICS) {

        // Copiar el nombre del tema de manera segura
        snprintf(topic[topic_count].topicName, sizeof(topic[topic_count].topicName), "%s", topicName);

        // Inicializar contador de suscriptores en el tema
        topic[topic_count].subscriberCount = 0;

        topic_count++;

        printf("Topic '%s' created as requested by user %d.\n", topicName, pid);
    } else {
        printf("[ERROR] Maximum number of topics reached!\n");
    }
}


// Función que elimina al usuario
void removeUser(char *name) {
    int userFound = 0;
    for (int i = 0; i < user_count; i++) {
        if (strcmp(user[i].name, name) == 0) {
            userFound = 1;

            // Remplaza los usuarios eliminados por el ultimo en la lista
            user[i] = user[user_count - 1];

            // Desconecta usuario
            Message end;
            end.closed = 1;

            //Abre pipe del usuario
            int fd = open(user[i].FIFO, O_WRONLY);
            if (fd == -1) {
                printf("[ERROR] Failed to open FIFO for user %d!\n", user[i].pid);
                continue;
            }

            // Envia petición de desconexión usuario
            if (write(fd, &end, sizeof(end)) == -1) {
                printf("[ERROR] Failed to send disconnection request to user %d!\n", user[i].pid);
            }

            close(fd);
            user_count--;
            printf("User '%s' successfully removed.\n", name);
            break;
        }
    }
    if (!userFound) {
        printf("[ERROR] User '%s' not found!\n", name);
    }
}

// Lista los usuarios registrados
void listUsers() {
    // Verifica que haya usuarios conectados
    if (user_count > 0) {
        printf("Registered users:\n");
        printf("====================================\n");
        printf("| PID       | Index   | Name       |\n");
        printf("====================================\n");

        for (int i = 0; i < user_count; i++) {
            printf("| %-9d | %-7d | %-10s |\n", 
                   user[i].pid, i, user[i].name);
        }

        printf("====================================\n");
    } else {
        printf("No registered users found.\n");
    }
}

// Lista los temas activos
void listTopics() {
    // Verifica que haya temas activos
    if (topic_count > 0) {
        printf("Active topics list:\n");
        printf("===================================\n");
        printf("| Index   | Topic Name            |\n");
        printf("===================================\n");

        for (int i = 0; i < topic_count; i++) {
            printf("| %-7d | %-20s |\n", i, topic[i].topicName);
        }

        printf("===================================\n");
    } else {
        printf("No active topics found.\n");
    }
}

// Funcion que muestra los mensajes persistentes por tema
void showTopics(char *topicName) {
    int found = 0;  // Variable verifica si el tema se encuentra
    for (int i = 0; i < message_count; i++) {
        if (strcmp(message[i].topic, topicName) == 0) {
            if (!found) {
                printf("Persistent messages for topic '%s':\n", topicName);
                found = 1; // Encontrado
            }
            printf("    - [%s] %s: \"%s\" (Duration: %d seconds remaining)\n", 
                   message[i].topic, message[i].name, 
                   message[i].message, 
                   message[i].lifetime - time(NULL));
        }
    }
    if (!found) {
        printf("[ERROR] No persistent messages found for topic '%s'.\n", topicName);
    }
}


// Función para bloquear un tema
void lockTopic(char *topico) {
    for (int i = 0; i < topic_count; i++) {
        if (strcmp(topico, topic[i].topicName) == 0) {
            // Verificar si el tema ya está bloqueado
            if (topic[i].state == 0) {
                topic[i].state = 1;  // Bloquear el tema
                printf("Topic '%s' successfully locked.\n", topico);
            } else {
                printf("Topic '%s' is already locked.\n", topico);
            }
            return; 
        }
    }
    printf("[ERROR] Topic '%s' not found in the list.\n", topico);
}

// Función para desbloquear un tema
void unlockTopic(char *topico) {
    for (int i = 0; i < topic_count; i++) {
        if (strcmp(topico, topic[i].topicName) == 0) {
            // Verificar si el tema ya está desbloqueado
            if (topic[i].state == 1) {
                topic[i].state = 0;  // Desbloquear el tema
                printf("Topic '%s' successfully unlocked.\n", topico);
            } else {
                printf("Topic '%s' is already unlocked.\n", topico);
            }
            return; 
        }
    }
    printf("[ERROR] Topic '%s' not found in the list.\n", topico);
}


// Función para enviar un mensaje
void sendMsg(Message *msg) {
    // Procesar mensaje persistente si es necesario
    if (msg->duration > 0) {
        if (message_count < MAX_MESSAGES) {
            PersistMsg persistentMsg;

            // Configuración de la estructura del mensaje persistente
            persistentMsg.pid = msg->pid;
            persistentMsg.lifetime = time(NULL) + msg->duration;
            strncpy(persistentMsg.message, msg->message, sizeof(persistentMsg.message) - 1);
            strncpy(persistentMsg.topic, msg->topic, sizeof(persistentMsg.topic) - 1);
            strncpy(persistentMsg.name, msg->name, sizeof(persistentMsg.name) - 1);

            persistentMsg.message[sizeof(persistentMsg.message) - 1] = '\0';
            persistentMsg.topic[sizeof(persistentMsg.topic) - 1] = '\0';
            persistentMsg.name[sizeof(persistentMsg.name) - 1] = '\0';

            message[message_count++] = persistentMsg;
            printf("Persistent message added to topic '%s'.\n", persistentMsg.topic);
        } else {
            printf("[ERROR] Maximum number of persistent messages reached.\n");
        }
    }

    // Enviar el mensaje a los clientes suscritos
    for (int i = 0; i < user_count; i++) {
        int subscribed = 0;

        // Verificar si el cliente está suscrito al tópico
        for (int j = 0; j < user[i].topicCount; j++) {
            if (strcmp(user[i].subscribedTopics[j].topicName, msg->topic) == 0) {
                subscribed = 1;
                break;
            }
        }

        if (subscribed) {
            int fd = open(user[i].FIFO, O_WRONLY);
            if (fd != -1) {
                if (msg->duration == 0) {
                    write(fd, msg, sizeof(*msg));
                } else {
                    sendPersistMsg(&user[i], msg->topic);
                }
                close(fd);
            } else {
                printf("[ERROR] Failed to open user pipe %d.\n", user[i].pid);
            }
        }
    }
}

// Función para verificar el temporizador de los mensajes persistentes
void *threadTime(void *data) {
    TDATA *threadData = (TDATA *) data;

    while (threadData->stop) {
        time_t currentTime = time(NULL);

        for (int i = 0; i < message_count; i++) {
            // Verificar si el tiempo de vida del mensaje ha expirado
            if (message[i].lifetime <= currentTime) {
                printf("Persistent message in topic '%s' expired and removed.\n", message[i].topic);
                fflush(stdout);

                // Reemplazar el mensaje expirado con el último de la lista
                message[i] = message[message_count - 1];
                message_count--;
                i--; // Ajustar el índice para verificar el mensaje recién movido
            }
        }
        sleep(1); // Esperar un segundo antes de la siguiente verificación
    }

    pthread_exit(NULL); // Terminar el hilo
}

// Tipo 0: Subscribirse a un tema
void handleType0(Request *request) {
    for (int i = 0; i < user_count; i++) {
        if (user[i].pid == request->pid) {
            // Verificar si ya está suscrito
            int alreadySubscribed = 0;
            for (int j = 0; j < user[i].topicCount; j++) {
                if (strcmp(user[i].subscribedTopics[j].topicName, request->topic) == 0) {
                    printf("User '%d' is already subscribed to topic '%s'.\n", request->pid, request->topic);
                    alreadySubscribed = 1;
                    break;
                }
            }

            if (alreadySubscribed) break;

            // Crear el tema si no existe
            if (user[i].topicCount < MAX_TOPICS) {
                // Crear tema si no está creado
                createTopic(request->topic, request->pid);

                // Suscribir al usuario al tema
                strncpy(user[i].subscribedTopics[user[i].topicCount].topicName, request->topic,
                        sizeof(user[i].subscribedTopics[user[i].topicCount].topicName) - 1);
                user[i].topicCount++;

                printf("User '%d' subscribed to topic '%s'.\n", request->pid, request->topic);
            } else {
                printf("[ERROR] User '%d' has reached the maximum number of topics.\n", request->pid);
            }
            break;
        }
    }
}

// Tipo 1: Desubscribirse de un tema
void handleType1(Request *request) {
    int userFound = 0;
    for (int i = 0; i < user_count; i++) {
        if (user[i].pid == request->pid) {
            userFound = 1;
            for (int j = 0; j < user[i].topicCount; j++) {
                if (strcmp(user[i].subscribedTopics[j].topicName, request->topic) == 0) {
                    // Desuscribir al usuario del tema
                    for (int k = j; k < user[i].topicCount - 1; k++) {
                        user[i].subscribedTopics[k] = user[i].subscribedTopics[k + 1];
                    }
                    user[i].topicCount--;
                    printf("User '%d' unsubscribed from topic '%s'.\n", request->pid, request->topic);
                    break;
                }
            }
        }
    }

    if (!userFound) {
        printf("[ERROR] User '%d' is not subscribed to topic '%s'.\n", request->pid, request->topic);
    }
}

// Tipo 2: Registro de un usuario
void handleType2(Request *request) {
    int userExists = 0;
    for (int i = 0; i < user_count; i++) {
        if (strcmp(request->username, user[i].name) == 0) {
            userExists = 1;
            break;
        }
    }

    int response = userExists ? 0 : 1; // 0 -> El usuario ya existe, 1 -> Usuario aceptado

    if (response == 0) {
        printf("A user with this name already exists, denying access.\n");
    } else {
        if (user_count < MAX_USERS) {
            user[user_count].pid = request->pid;
            snprintf(user[user_count].FIFO, sizeof(user[user_count].FIFO), FEED_PIPE, request->pid);
            strncpy(user[user_count].name, request->username, sizeof(user[user_count].name) - 1);
            user[user_count].name[sizeof(user[user_count].name) - 1] = '\0';
            user[user_count].topicCount = 0;
            user_count++;
        } else {
            printf("[ERROR] Maximum number of users reached.\n");
            response = 0;
        }
    }

    // Enviar respuesta directamente al cliente
    int resp_fd = open(request->FIFO, O_WRONLY);
    if (write(resp_fd, &response, sizeof(response)) == -1) {
        printf("[ERROR] Failed to send response to user pipe.\n");
    }
    close(resp_fd);
}

// Tipo 3: Eliminar un usuario
void handleType3(Request *request) {
    int index = -1;
    // Buscar usuario por PID
    for (int i = 0; i < user_count; i++) {
        if (user[i].pid == request->pid) {
            index = i;
            break;
        }
    }
    if (index != -1) {
        // Desplazar los demás usuarios para llenar el espacio
        for (int i = index; i < user_count - 1; i++) {
            user[i] = user[i + 1];
        }
        user_count--;
        printf("User with PID %d removed successfully.\n", request->pid);
    } else {
        printf("[ERROR] User with PID %d not found.\n", request->pid);
    }
}



void *threadReceiveMessages(void *data) {
    TDATA *pdata = (TDATA *)data;
    int fd = open(MAN_PIPE, O_RDWR);

    if (fd == -1) {
        printf("\n[ERROR] Failed to open manager pipe.\n");
        exit(-1);
    }

    char buffer[sizeof(Message) > sizeof(Request) ? sizeof(Message) : sizeof(Request)];

    do {
        pthread_mutex_lock(pdata->mutex);

        int size = read(fd, buffer, sizeof(buffer));
        if (size > 0) {
            if (size == sizeof(Request)) {
                Request *request = (Request *)buffer;

                // Manejar los diferentes tipos de solicitud
                switch (request->type) {
                    case 0:
                        handleType0(request);
                        break;
                    case 1:
                        handleType1(request);
                        break;
                    case 2:
                        handleType2(request);
                        break;
                    case 3:
                        handleType3(request);
                        break;
                    default:
                        printf("[ERROR] Unknown request type: %d\n", request->type);
                }
            } else if (size == sizeof(Message)) {
                Message *recvMsg = (Message *)buffer;
                sendMsg(recvMsg);
                printf("Result: %s - %s\n", recvMsg->name, recvMsg->message);
            }
        } else if (size == 0) {
            // Reabrir FIFO si es necesario
            close(fd);
            fd = open(MAN_PIPE, O_RDONLY);
            if (fd == -1) {
                printf("[ERROR] Failed to reopen manager pipe.\n");
                pthread_mutex_unlock(pdata->mutex);
                return NULL;
            }
        } else {
            printf("[ERROR] Failed to read from manager pipe.\n");
            pthread_mutex_unlock(pdata->mutex);
            break;
        }

        pthread_mutex_unlock(pdata->mutex);
    } while (pdata->stop);

    close(fd);
    pthread_exit(NULL);
}


// Creación de hilos
void createThreads(pthread_t *receiveMessagesThread, pthread_t *timerWatcherThread, TDATA *v1, TDATA *v2, pthread_mutex_t *mutex) {
    v1->stop = 1;
    v1->mutex = mutex;
    v2->stop = 1;
    v2->mutex = mutex;

    // Crear hilo para recibir mensajes
    pthread_create(receiveMessagesThread, NULL, &threadReceiveMessages, v1);

    // Crear hilo para vigilar el temporizador
    pthread_create(timerWatcherThread, NULL, &threadTime, v2);
}

int main(int argc, char *argv[]) {
    // Inicializa mutex
    pthread_mutex_t mutex;
    pthread_mutex_init(&mutex, NULL);

    if (argc != 1) {
        printf("[ERROR] Invalid number of arguments.\n");
        return 1;
    }

    // Creación del user pipe
    if (mkfifo(MAN_PIPE, 0666) == -1) {
        if (errno == EEXIST) {
            printf("[ERROR] Manager pipe already exists! Cannot proceed.\n");
            return 1; // Salimos del programa si ya existe
        }
    }

    // Inicia los hilos
    pthread_t receiveMessagesThread, timerWatcherThread;
    TDATA v1, v2;
    createThreads(&receiveMessagesThread, &timerWatcherThread, &v1, &v2, &mutex);

    char input[100];

    while (1) {  // Bucle infinito
        printf("Enter the command you want to execute: \n");
        fgets(input, sizeof(input), stdin);
        input[strcspn(input, "\n")] = 0; // Eliminar el salto de línea

        if (strcmp(input, "users") == 0) {
            listUsers();
        } else if (strncmp(input, "remove ", 7) == 0) {
            char name[MAX_NAME_LENGTH];
            sscanf(input + 7, "%s", name);
            removeUser(name);
        } else if (strncmp(input, "show ", 5) == 0) {
            char topic[MAX_TOPIC_LENGTH];
            sscanf(input + 5, "%s", topic);
            showTopics(topic);
        } else if (strcmp(input, "topics") == 0) {
            listTopics();
        } else if (strncmp(input, "lock ", 5) == 0) {
            char topic[MAX_TOPIC_LENGTH];
            sscanf(input + 5, "%s", topic);
            lockTopic(topic);
        } else if (strncmp(input, "unlock ", 7) == 0) {
            char topic[MAX_TOPIC_LENGTH];
            sscanf(input + 7, "%s", topic);
            unlockTopic(topic);
        } else if (strcmp(input, "close") == 0) {
            endSession();
            break; // Salir del bucle si el comando es "close"
        } else {
            printf("[ERROR] Unknown command!\n");
        }
    }

    // Espera que los hilos terminen
    v1.stop = 0;
    v2.stop = 0;
    pthread_join(receiveMessagesThread, NULL);
    pthread_join(timerWatcherThread, NULL);

    // Destruye mutex y cierra manager pipe
    pthread_mutex_destroy(&mutex);
    unlink(MAN_PIPE);

    return 0;
}
