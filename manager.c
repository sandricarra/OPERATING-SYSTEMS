#include "utils.h"

cliente clientesLista[MAX_CLIENTES];
int contCliente = 0;

msgPersStruct menssagensLista[MAX_MENSSAGENS];
int contMenssagens = 0;

topico topicosLista[MAX_TOPICOS];
int contTopicos = 0;



void commFecha() {
    msgStruct termina;
    termina.fechado = 1;

    // Recorrer todos los clientes conectados
    for (int i = 0; i < contCliente; i++) {
        int fd = open(clientesLista[i].FIFO, O_WRONLY);
        
        // Verificar si no se pudo abrir el FIFO del cliente
        if (fd == -1) {
            printf("[ERROR] Failed to open user pipe %d!\n", clientesLista[i].pid);
            continue;
        }

        // Enviar el mensaje de desconexión al cliente
        if (write(fd, &termina, sizeof(termina)) == -1) {
            printf("[ERROR] Failed to send disconnection request to user %d!\n", clientesLista[i].pid);
        }

        // Cerrar el descriptor del archivo
        close(fd);
    }

    // Resetear el contador de clientes después de desconectarlos
    contCliente = 0;
    printf("All clients have been disconnected.\n");

    // Eliminar el FIFO utilizado por el manager
    if (unlink(MAN_FIFO) == -1) {
        printf("[ERROR] Failed to remove manager pipe.\n");
    }
    printf("Session ended.\n");

    exit(0);
}


void enviaMenssagensPersistentes(cliente *client, char *topico) {
    for (int i = 0; i < contMenssagens; i++) {
        if (strcmp(menssagensLista[i].topico, topico) == 0) {
            if(menssagensLista[i].tempoVida > 0){
                msgStruct msg;
                strncpy(msg.topico, menssagensLista[i].topico, sizeof(msg.topico) - 1);
                msg.topico[sizeof(msg.topico) - 1] = '\0';
                strncpy(msg.nome, menssagensLista[i].nome, sizeof(msg.nome) - 1);
                msg.nome[sizeof(msg.nome) - 1] = '\0';
                strncpy(msg.menssagem, menssagensLista[i].menssagem, sizeof(msg.menssagem) - 1);
                msg.menssagem[sizeof(msg.menssagem) - 1] = '\0';
                msg.duracao = 0;
                msg.pid = menssagensLista[i].pid;

                int fd = open(client->FIFO, O_WRONLY);
                if (fd == -1) {
                    printf("[ERRO] Falha ao abrir FIFO do cliente %d!\n", client->pid);
                    continue;
                }

                if (write(fd, &msg, sizeof(msg)) == -1) {
                    printf("[ERRO] Falha ao enviar mensagem persistente para o cliente %d!\n", client->pid);
                }

                close(fd);
            }
        }
    }
}


void criarTopico(const char *topico, int pid) {
    // Verificar si el tema ya existe
    for (int i = 0; i < contTopicos; i++) {
        if (strcmp(topicosLista[i].nomeTopico, topico) == 0) {
            printf("The topic '%s' already exists!\n", topico);
            return; // El tema ya existe, no se agrega
        }
    }

    // Si no existe, verificar si se puede agregar un nuevo tema
    if (contTopicos < MAX_TOPICOS) {

        // Copiar el nombre del tema de manera segura
        snprintf(topicosLista[contTopicos].nomeTopico, sizeof(topicosLista[contTopicos].nomeTopico), "%s", topico);

        // Inicializar contador de inscritos en el tema
        topicosLista[contTopicos].contInscritos = 0;

        contTopicos++;

        printf("Topic '%s' created as requested by user %d.\n", topico, pid);
    } else {
        printf("[ERROR] Maximum number of topics reached!\n");
    }
}


void commRemoveCliente(char *nome) {
    int clienteEncontrado = 0;

    // Iterar sobre la lista de clientes
    for (int i = 0; i < contCliente; i++) {
        if (strcmp(clientesLista[i].nome, nome) == 0) {
            clienteEncontrado = 1;

            // Reemplazar el cliente eliminado por el último en la lista
            clientesLista[i] = clientesLista[contCliente - 1];

            // Preparar el mensaje para desconectar al cliente
            msgStruct termina;
            termina.fechado = 1;

            // Abrir el FIFO del cliente para enviar el mensaje
            int fd = open(clientesLista[i].FIFO, O_WRONLY);
            if (fd == -1) {
                printf("[ERROR] Failed to open FIFO for client %d!\n", clientesLista[i].pid);
                continue;
            }

            // Enviar la solicitud de desconexión al cliente
            if (write(fd, &termina, sizeof(termina)) == -1) {
                printf("[ERROR] Failed to send disconnection request to client %d!\n", clientesLista[i].pid);
            }

            close(fd);

            // Decrementar el contador de clientes
            contCliente--;

            // Mensaje de éxito
            printf("Client '%s' successfully removed.\n", nome);
            break;
        }
    }

    // Si el cliente no fue encontrado, imprimir un error
    if (!clienteEncontrado) {
        printf("[ERROR] Client '%s' not found!\n", nome);
    }
}


void commUsers() {
    // Verificar si hay usuarios registrados
    if (contCliente > 0) {
        printf("Registered clients:\n");

        // Mostrar la lista de usuarios en formato tabular
        printf("====================================\n");
        printf("| PID       | Index   | Name       |\n");
        printf("====================================\n");

        for (int i = 0; i < contCliente; i++) {
            // Imprimir cada usuario en formato de tabla
            printf("| %-9d | %-7d | %-10s |\n", 
                   clientesLista[i].pid, i, clientesLista[i].nome);
        }

        printf("====================================\n");
    } else {
        printf("No registered clients found.\n");
    }
}

void commListaTopicos() {
    // Verificar si existen tópicos registrados
    if (contTopicos > 0) {
        printf("Active topics list:\n");

        // Formato tabular para mostrar los tópicos
        printf("===================================\n");
        printf("| Index   | Topic Name            |\n");
        printf("===================================\n");

        for (int i = 0; i < contTopicos; i++) {
            // Mostrar índice y nombre del tópico
            printf("| %-7d | %-20s |\n", i, topicosLista[i].nomeTopico);
        }

        printf("===================================\n");
    } else {
        printf("No active topics found.\n");
    }
}

void commMostraTopico(char *topico) {
    int encontrado = 0;  // Variable para verificar si se encuentra el tema

    // Buscar mensajes persistentes relacionados con el tópico
    for (int i = 0; i < contMenssagens; i++) {
        if (strcmp(menssagensLista[i].topico, topico) == 0) {
            if (!encontrado) {
                printf("Persistent messages for topic '%s':\n", topico);
                encontrado = 1; // Marcar como encontrado
            }

            // Mostrar detalles de cada mensaje
            printf("    - [%s] %s: \"%s\" (Duration: %d seconds remaining)\n", 
                   menssagensLista[i].topico, menssagensLista[i].nome, 
                   menssagensLista[i].menssagem, 
                   menssagensLista[i].tempoVida - time(NULL));
        }
    }

    // Mensaje en caso de no encontrar mensajes para el tópico
    if (!encontrado) {
        printf("[ERROR] No persistent messages found for topic '%s'.\n", topico);
    }
}



void commBloqueiaTopico(char *topico) {
    for (int i = 0; i < contTopicos; i++) {
        if (strcmp(topico, topicosLista[i].nomeTopico) == 0) {
            // Verificar si el tópico ya está bloqueado
            if (topicosLista[i].estado == 0) {
                topicosLista[i].estado = 1;  // Bloquear el tópico
                printf("Topic '%s' successfully locked.\n", topico);
            } else {
                printf("Topic '%s' is already locked.\n", topico);
            }
            return; 
        }
    }
    printf("[ERROR] Topic '%s' not found in the list.\n", topico);
}

void commDesbloqueiaTopico(char *topico) {
    for (int i = 0; i < contTopicos; i++) {
        if (strcmp(topico, topicosLista[i].nomeTopico) == 0) {
            // Verificar si el tópico ya está desbloqueado
            if (topicosLista[i].estado == 1) {
                topicosLista[i].estado = 0;  // Desbloquear el tópico
                printf("Topic '%s' successfully unlocked.\n", topico);
            } else {
                printf("Topic '%s' is already unlocked.\n", topico);
            }
            return; 
        }
    }
    printf("[ERROR] Topic '%s' not found in the list.\n", topico);
}


void enviaMenssagem(msgStruct *msg) {
    // Procesar mensaje persistente si es necesario
    if (msg->duracao > 0) {
        if (contMenssagens < MAX_MENSSAGENS) {
            msgPersStruct persistentMsg;

            // Configuración de la estructura de la mensage persistente
            persistentMsg.pid = msg->pid;
            persistentMsg.tempoVida = time(NULL) + msg->duracao;
            strncpy(persistentMsg.menssagem, msg->menssagem, sizeof(persistentMsg.menssagem) - 1);
            strncpy(persistentMsg.topico, msg->topico, sizeof(persistentMsg.topico) - 1);
            strncpy(persistentMsg.nome, msg->nome, sizeof(persistentMsg.nome) - 1);

            persistentMsg.menssagem[sizeof(persistentMsg.menssagem) - 1] = '\0';
            persistentMsg.topico[sizeof(persistentMsg.topico) - 1] = '\0';
            persistentMsg.nome[sizeof(persistentMsg.nome) - 1] = '\0';

            menssagensLista[contMenssagens++] = persistentMsg;
            printf("Persistent message added to topic '%s'.\n", persistentMsg.topico);
        } else {
            printf("[ERROR] Maximum number of persistent messages reached.\n");
        }
    }

    // Enviar mensaje a los clientes suscritos
    for (int i = 0; i < contCliente; i++) {
        int subscribed = 0;

        // Verifica si el cliente está inscrito en el tópico
        for (int j = 0; j < clientesLista[i].numTopicos; j++) {
            if (strcmp(clientesLista[i].topicosIns[j].nomeTopico, msg->topico) == 0) {
                subscribed = 1;
                break;
            }
        }

        if (subscribed) {
            int fd = open(clientesLista[i].FIFO, O_WRONLY);
            if (fd != -1) {
                if (msg->duracao == 0) {
                    write(fd, msg, sizeof(*msg));
                } else {
                    enviaMenssagensPersistentes(&clientesLista[i], msg->topico);
                }
                close(fd);
            } else {
                printf("[ERROR] Failed to open user pipe %d.\n", clientesLista[i].pid);
            }
        }
    }
}

void *threadVerificaTimer(void *data) {
    TDADOS *threadData = (TDADOS *) data;

    while (threadData->stop) {
        time_t currentTime = time(NULL);

        for (int i = 0; i < contMenssagens; i++) {
            // Verificar si el tiempo de vida de la mensagem expiró
            if (menssagensLista[i].tempoVida <= currentTime) {
                printf("Persistent message in topic '%s' expired and removed.\n", menssagensLista[i].topico);
                fflush(stdout);

                // Reemplazar el mensaje expirado con el último de la lista
                menssagensLista[i] = menssagensLista[contMenssagens - 1];
                contMenssagens--;
                i--; // Ajustar el índice para verificar el mensaje recién movido
            }
        }
        sleep(1); // Esperar un segundo antes de la siguiente verificación
    }

    pthread_exit(NULL); // Terminar el hilo
}

// Tipo 0: Subscribe al tema
void handleTipo0(pedidoStruct *pedido) {
    for (int i = 0; i < contCliente; i++) {
        if (clientesLista[i].pid == pedido->pid) {
            // Mira si esta subscrito
            int alreadySubscribed = 0;
            for (int j = 0; j < clientesLista[i].numTopicos; j++) {
                if (strcmp(clientesLista[i].topicosIns[j].nomeTopico, pedido->topico) == 0) {
                    printf("Client '%d' is already subscribed to this topic. \n", pedido->pid, pedido->topico);
                    alreadySubscribed = 1;
                    break;
                }
            }

            if (alreadySubscribed) break;

            // Crea el tema si no esta creado
            if (clientesLista[i].numTopicos < MAX_TOPICOS) {
                criarTopico(pedido->topico, pedido->pid);

                // Subscribe el usuario al tema
                strncpy(clientesLista[i].topicosIns[clientesLista[i].numTopicos].nomeTopico, pedido->topico, 
                        sizeof(clientesLista[i].topicosIns[clientesLista[i].numTopicos].nomeTopico) - 1);
                clientesLista[i].topicosIns[clientesLista[i].numTopicos].nomeTopico[sizeof(clientesLista[i].topicosIns[clientesLista[i].numTopicos].nomeTopico) - 1] = '\0';
                clientesLista[i].numTopicos++;

                printf("Client '%d' subscribed to topic '%s'.\n", pedido->pid, pedido->topico);
            } else {
                printf("[ERROR] Client '%d' has reached the maximum number of topics.\n", pedido->pid);
            }
            break;
        }
    }
}

// Tipo 1: Desubcribe del tema
void handleTipo1(pedidoStruct *pedido) {
    int clientFound = 0;
    for (int i = 0; i < contCliente; i++) {
        if (clientesLista[i].pid == pedido->pid) {
            clientFound = 1;
            for (int j = 0; j < clientesLista[i].numTopicos; j++) {
                if (strcmp(clientesLista[i].topicosIns[j].nomeTopico, pedido->topico) == 0) {
                    // Desubcribe al usuario del tema
                    for (int k = j; k < clientesLista[i].numTopicos - 1; k++) {
                        clientesLista[i].topicosIns[k] = clientesLista[i].topicosIns[k + 1];
                    }
                    clientesLista[i].numTopicos--;
                    printf("\n[MANAGER] Client '%d' unsubscribed from topic '%s'!\n", pedido->pid, pedido->topico);
                    break;
                }
            }
        }
    }

    if (!clientFound) {
        printf("[ERROR] Client '%d' is not subscribed to topic '%s'.\n", pedido->pid, pedido->topico);
    }
}

// Tipo 2: Registro cliente.
void handleTipo2(pedidoStruct *pedido) {
    int userExists = 0;
    for (int i = 0; i < contCliente; i++) {
        if (strcmp(pedido->username, clientesLista[i].nome) == 0) {
            userExists = 1;
            break;
        }
    }

    int resposta = userExists ? 0 : 1; // 0 -> User already exists, 1 -> User accepted

    if (resposta == 0) {
        printf("A client with this name already exists, denying access.\n");
    } else {
        if (contCliente < MAX_CLIENTES) {
            clientesLista[contCliente].pid = pedido->pid;
            snprintf(clientesLista[contCliente].FIFO, sizeof(clientesLista[contCliente].FIFO), "/tmp/client_fifo_%d", pedido->pid);
            strncpy(clientesLista[contCliente].nome, pedido->username, sizeof(clientesLista[contCliente].nome) - 1);
            clientesLista[contCliente].nome[sizeof(clientesLista[contCliente].nome) - 1] = '\0';
            clientesLista[contCliente].numTopicos = 0;
            contCliente++;
        } else {
            printf("[ERROR] Maximum number of clients reached.\n");
            resposta = 0;
        }
    }

    // Envia la respuesta directamente
    int resp_fd = open(pedido->FIFO, O_WRONLY);
    if (write(resp_fd, &resposta, sizeof(resposta)) == -1) {
        printf("[ERROR] Failed to send response to user pipe.\n");
    }
    close(resp_fd);
}


// Tipo 3 : Elimina al usuario
void handleTipo3(pedidoStruct *pedido) {
    int index = -1;
    // Buscar cliente por PID
    for (int i = 0; i < contCliente; i++) {
        if (clientesLista[i].pid == pedido->pid) {
            index = i;
            break;
        }
    }
    if (index != -1) {
        // Desplazar el resto de los clientes para llenar el espacio
        for (int i = index; i < contCliente - 1; i++) {
            clientesLista[i] = clientesLista[i + 1];
        }
        contCliente--;
        printf("User with PID %d removed successfully,\n", pedido->pid);
    } else {
        printf("[ERROR] Client with PID %d not found.\n", pedido->pid);
    }
}


void *threadRecebeMenssagens(void *dados) {
    TDADOS *pdados = (TDADOS *)dados;
    int fd = open(MAN_FIFO, O_RDWR);

    if (fd == -1) {
        printf("\n[ERROR] Failed to open manager pipe.\n");
        exit(-1);
    }

    char buffer[sizeof(msgStruct) > sizeof(pedidoStruct) ? sizeof(msgStruct) : sizeof(pedidoStruct)];

    do {
        pthread_mutex_lock(pdados->m);

        int size = read(fd, buffer, sizeof(buffer));
        if (size > 0) {
            if (size == sizeof(pedidoStruct)) {
                pedidoStruct *pedido = (pedidoStruct *)buffer;

                // Handle different request types
                switch (pedido->tipo) {
                    case 0:
                        handleTipo0(pedido);
                        break;
                    case 1:
                        handleTipo1(pedido);
                        break;
                    case 2:
                        handleTipo2(pedido);
                        break;
                    case 3:
                        handleTipo3(pedido);
                        break;
                    default:
                        printf("[ERROR] Unknown request type: %d\n", pedido->tipo);
                }
            } else if (size == sizeof(msgStruct)) {
                msgStruct *recvMsg = (msgStruct *)buffer;
                enviaMenssagem(recvMsg);
                printf("Result: %s - %s\n", recvMsg->nome, recvMsg->menssagem);
            }
        } else if (size == 0) {
            // Reopen FIFO if needed
            close(fd);
            fd = open(MAN_FIFO, O_RDONLY);
            if (fd == -1) {
                printf("[ERROR] Failed to reopen manager pipe.\n");
                pthread_mutex_unlock(pdados->m);
                return NULL;
            }
        } else {
            printf("[ERROR] Failed to read from manager pipe.\n");
            pthread_mutex_unlock(pdados->m);
            break;
        }

        pthread_mutex_unlock(pdados->m);
    } while (pdados->stop);

    close(fd);
    pthread_exit(NULL);
}

// Creación de hilos
void createThreads(pthread_t *recebeMenssagem, pthread_t *vigiaTimer, TDADOS *v1, TDADOS *v2, pthread_mutex_t *mutex) {
    v1->stop = 1;
    v1->m = mutex;
    v2->stop = 1;
    v2->m = mutex;
    pthread_create(recebeMenssagem, NULL, &threadRecebeMenssagens, v1);
    pthread_create(vigiaTimer, NULL, &threadVerificaTimer, v2);
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
    if (mkfifo(MAN_FIFO, 0666) == -1) {
        if (errno == EEXIST) {
            printf("[ERROR] Manager pipe already exits! Deleting manager pipe.\n");
            unlink(MAN_FIFO);
        }
        
        if (mkfifo(MAN_FIFO, 0666) == -1) {
            printf("[ERROR] Failed to create manager pipe. \n");
            return 1;
        }
    }

    // Inicia los hilos
    pthread_t recebeMenssagem, vigiaTimer;
    TDADOS v1, v2;
    createThreads(&recebeMenssagem, &vigiaTimer, &v1, &v2, &mutex);

    char input[100];

    while (1) {  // Bucle infinito
        printf("Enter the command you want to execute: \n");
        fgets(input, sizeof(input), stdin);
        input[strcspn(input, "\n")] = 0;

        switch (1) {  // Esto simula las comprobaciones de comandos
            case 1:  // Check "users"
                if (strcmp(input, "users") == 0) {
                    commUsers();
                    break;
                }
            case 2:  // Check "remove"
                if (strncmp(input, "remove ", 7) == 0) {
                    char clienteNome[20];
                    sscanf(input + 7, "%s", clienteNome);
                    commRemoveCliente(clienteNome);
                    break;
                }
            case 3:  // Check "show"
                if (strncmp(input, "show ", 5) == 0) {
                    char topico[20];
                    sscanf(input + 5, "%s", topico);
                    commMostraTopico(topico);
                    break;
                }
            case 4:  // Check "topics"
                if (strcmp(input, "topics") == 0) {
                    commListaTopicos();
                    break;
                }
            case 5:  // Check "lock"
                if (strncmp(input, "lock ", 5) == 0) {
                    char topico[20];
                    sscanf(input + 5, "%s", topico);
                    commBloqueiaTopico(topico);
                    break;
                }
            case 6:  // Check "unlock"
                if (strncmp(input, "unlock ", 7) == 0) {
                    char topico[20];
                    sscanf(input + 7, "%s", topico);
                    commDesbloqueiaTopico(topico);
                    break;
                }
            case 7:  // Check "close"
                if (strcmp(input, "close") == 0) {
                    commFecha();
                    break;
                }
            default:
                printf("[ERROR] Unknown command!\n");
    }
}
    // Espera que los hilos terminen 
    v1.stop = 0;
    v2.stop = 0;   
    pthread_join(recebeMenssagem, NULL);
    pthread_join(vigiaTimer, NULL);

     // Destruye mutex y cierra manager pipe
    pthread_mutex_destroy(&mutex);
    unlink(MAN_FIFO);
    return 0;
}
