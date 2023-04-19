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
#include <stdbool.h>

#define MAX_CLIENTS 10
#define MAX_MESSAGE_LENGTH 100
#define BUF_SIZE 1024
#define SHM_SIZE 1024
#define NAME_SIZE 256
#define SHM_KEY 0x1234
#define PRIME 1543
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

struct connectInfo
{
    int requestcode;
    int responsecode;
    char username[NAME_SIZE];
    int id;
    bool id_arr[MAX_CLIENTS];
    bool waitingid[MAX_CLIENTS];
    bool disconnet[MAX_CLIENTS];
    pthread_mutex_t id_mutex;
};

int hash(unsigned char *str)
{
    int hash = 5381;
    int c;

    while (c = *str++)
        hash = ((hash << 5) + hash) + c; /* hash * 33 + c */

    return hash;
}

int main()
{
    int connect_shmid;
    int clientid;
    if (connect_shmid = shmget(SHM_KEY, sizeof(struct connectInfo), 0) == -1)
    {
        PRINT_ERROR("Server is not available, Server has to be started first");
    }

    struct connectInfo *connectinfo;

    connectinfo = shmat(connect_shmid, NULL, 0);

    if (connectinfo == (void *)-1)
    {
        PRINT_ERROR("Unable to attach shared memory");
        return 1;
    }

    pthread_mutexattr_t attr;
    pthread_mutexattr_init(&attr);
    pthread_mutexattr_setpshared(&attr, PTHREAD_PROCESS_SHARED);
    pthread_mutex_init(&(connectinfo->id_mutex), &attr);

    pthread_mutex_lock(&connectinfo->id_mutex);
    int i = 0;
    for (i = 0; i < MAX_CLIENTS; i++)
    {
        if (connectinfo->id_arr[i] == false)
        {
            connectinfo->id_arr[i] = true;
            clientid = i * PRIME;
            PRINT_INFO("You have been assigned a server, you have been assigned a client id:%d. Please remember this id", clientid);
            break;
        }
    }
    if (i == MAX_CLIENTS)
    {
        PRINT_INFO("Server is full, Check again later");
    }
    pthread_mutex_lock(&connectinfo->id_mutex);
}