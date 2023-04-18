#include <stdio.h>
#include <sys/ipc.h>
#include <unistd.h>
#include <sys/wait.h>
#include <sys/shm.h>
#include <sys/types.h>
#include <string.h>
#include <errno.h>
#include <stdlib.h>
#include <signal.h>
#include <pthread.h>
#include <time.h>

#define MAX_CLIENTS 10
#define MAX_MESSAGE_LENGTH 100
#define BUF_SIZE 1024

#define PRINT_INFO(MSG, ...)                                                          \
    {                                                                                 \
        setenv("TZ", "Asia/Kolkata", 1);                                              \
        tzset();                                                                      \
        time_t current_time = time(NULL);                                             \
        printf("%s INFO %d:%d %ld %s %s %d : " MSG "\n",                              \
               __TIME__, getpid(), getppid(), pthread_self(), __FILE__, __FUNCTION__, \
               __LINE__, ##__VA_ARGS__);                                              \
    }

#define PRINT_ERROR(MSG, ...)                                                         \
    {                                                                                 \
        setenv("TZ", "Asia/Kolkata", 1);                                              \
        tzset();                                                                      \
        time_t current_time = time(NULL);                                             \
        printf("%s ERROR %d:%d %ld %s %s %d : " MSG "\n",                             \
               __TIME__, getpid(), getppid(), pthread_self(), __FILE__, __FUNCTION__, \
               __LINE__, ##__VA_ARGS__);                                              \
    }

typedef struct 
{
    char name[20];
    int key;
    int connected;
} client_info;

typedef struct 
{
  int response_code;
  int client_response_code;
  int server_response_code;
  int data;
  client_info client;
} response_info;

typedef struct
{
  char op[BUF_SIZE];
  char param;
} operation_t;

typedef struct
{
    int a;
    int b;
    operation_t operation;
} request_info;

typedef struct 
{
    pthread_mutex_t mutex;
    request_info requests[MAX_CLIENTS];
    response_info responses[MAX_CLIENTS]
} shared_data_t;
