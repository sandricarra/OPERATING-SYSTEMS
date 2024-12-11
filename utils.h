
#ifndef UTILS_H
#define UTILS_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <signal.h>
#include <errno.h>
#include <pthread.h>
#include <sys/select.h>
#include <sys/types.h>
#include <sys/stat.h>

// Constantes
#define MAN_PIPE "manager_pipe"
#define FEED_PIPE "user_pipe_%d"
#define MAX_USERS 10
#define MAX_TOPICS 20
#define MAX_MESSAGES 5
#define MAX_MSG_SIZE 300
#define MAX_NAME_LENGTH 20
#define MAX_TOPIC_LENGTH 20
#define MAX_FIFO_LENGTH 128



// Estructura para representar un tema
typedef struct {
    char topicName[MAX_TOPIC_LENGTH];
    int subscriberCount;
    int state;  // 0->Desbloqueado 1->Bloqueado
} Topic;

// Estructura para un usuario
typedef struct {
    int pid;
    char name[MAX_NAME_LENGTH];
    int topicCount;
    char FIFO[MAX_FIFO_LENGTH];
    Topic subscribedTopics[MAX_USERS];
} User;

// Estructura para un mensaje estándar
typedef struct {
    char message[MAX_MSG_SIZE];
    int pid;
    int duration;
    char name[MAX_NAME_LENGTH];
    char topic[MAX_TOPIC_LENGTH];
    int closed;
    int blocked;
} Message;

// Estructura para un mensaje persistente
typedef struct {
    char message[MAX_MSG_SIZE];
    int pid;
    int lifetime;
    char topic[MAX_TOPIC_LENGTH];
    char name[MAX_NAME_LENGTH];
} PersistMsg;

// Estructura para solicitudes de cliente
typedef struct {
    char topic[MAX_TOPIC_LENGTH];
    char username[MAX_NAME_LENGTH];
    int pid;
    char FIFO[MAX_FIFO_LENGTH];
    int type;  // 0->Subscribirse 1->Desubscribirse 2->Registro Usuario 3->Eliminación usuario
} Request;

// Estructura para mensajes de error
typedef struct {
    char errorMessage[MAX_MSG_SIZE];
    int pid;
} Error;

// Estructura de datos para sincronización
typedef struct {
    pthread_mutex_t *mutex;
    int stop;
} TDATA;

#endif // UTILS_H
