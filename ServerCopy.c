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

int connect_shmid;
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
    pthread_mutex_t connect_server_mutex;
};

struct clientInfo
{
    int clientid;
    char username[NAME_SIZE];
    int requests;
    int responses;
};

int hash(unsigned char *str)
{
    int hash = 5381;
    int c;

    while (c = *str++)
        hash = ((hash << 5) + hash) + c; /* hash * 33 + c */

    return hash;
}

void handle_sigint(int sig)
{
    PRINT_INFO("\nClosing shared memory segment.....\n");

    // Close connection Shared memory
    shmctl(connect_shmid, IPC_RMID, NULL);
    exit(-1);
}

int main()
{

    struct clientInfo clientinfo[MAX_CLIENTS];
    if ((connect_shmid = shmget(SHM_KEY, sizeof(struct connectInfo), 0644 | IPC_CREAT)) == -1)
    {
        PRINT_ERROR("Unable to Create Shared Memory");
        return 1;
    }
    PRINT_INFO("%d", connect_shmid);
    struct connectInfo *connectinfo;
    connectinfo = shmat(connect_shmid, NULL, 0);
    if (connectinfo == (void *)-1)
    {
        PRINT_ERROR("Unable to Attact Shared Memory");
        return 1;
    }
    PRINT_INFO("Shared Memory Successfully created");
    signal(SIGINT, handle_sigint);
    pthread_mutexattr_t connect_server_mutex_attr;
    pthread_mutexattr_init(&connect_server_mutex_attr);
    pthread_mutexattr_setpshared(&connect_server_mutex_attr, PTHREAD_PROCESS_SHARED);
    pthread_mutex_init(&(connectinfo->connect_server_mutex), &connect_server_mutex_attr);
    bool flag;
    while (1)
    {
        // requestcode => 0-no request, 1-new user request, 2-existing user request
        // requestcode => 0-no response, 1-successful registratoin, 2-non unique id
        // pthread_mutex_lock(&connectinfo->connect_server_mutex);
        if (connectinfo->requestcode == 1)
        {
            bool flag = false;
            for (int i = 0; i < MAX_CLIENTS; i++)
            {
                if ((strcmp(connectinfo->username, clientinfo[i].username)) == 0)
                {
                    connectinfo->requestcode = 0;
                    strcpy(connectinfo->username, "");
                    flag = true;
                }
            }
            if (flag == true)
            {
                connectinfo->requestcode = 0;
                connectinfo->responsecode = 2;
            }
            else
            {
                clientinfo[(connectinfo->id - PRIME) / PRIME].clientid = connectinfo->id;
                strcpy(clientinfo[(connectinfo->id - PRIME) / PRIME].username, connectinfo->username);
                connectinfo->requestcode = 0;
                connectinfo->responsecode = 1;
            }
        }
        // pthread_mutex_unlock(&connectinfo->connect_server_mutex);
        while (connectinfo->requestcode == 0)
        {
        }

        // while (connectinfo->requestcode == 0)
        // {
        // }
        // flag = false;
        // if (connectinfo->requestcode == 1)
        // {
        //     for (int i = 0; i < MAX_CLIENTS; i++)
        //     {
        //         PRINT_INFO("%s \t %s \n", clientinfo[i].username, connectinfo->username);
        //         if ((strcmp(connectinfo->username, clientinfo[i].username)) == 0)
        //         {
        //             PRINT_INFO("%d \n", 69);
        //             connectinfo->requestcode = 0;
        //             connectinfo->responsecode = 2;
        //             flag = true;
        //         }
        //     }
        //     if (flag == false)
        //     {
        //         clientinfo[(connectinfo->id - PRIME) / PRIME].clientid = connectinfo->id;
        //         strcpy(clientinfo[(connectinfo->id - PRIME) / PRIME].username, connectinfo->username);
        //         connectinfo->requestcode = 0;
        //         connectinfo->responsecode = 1;
        //     }
        //     PRINT_INFO("%d \n", connectinfo->responsecode);
    }
}