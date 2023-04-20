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

    int clientid;
    int access = 0;

    if ((connect_shmid = shmget(SHM_KEY, sizeof(struct connectInfo), 0)) == -1)
    {
        PRINT_ERROR("Server is not available, Server has to be started first");
        return 1;
    }
    else
    {
        PRINT_INFO("Server Exists");
    }
    PRINT_INFO("%d", connect_shmid);
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
    pthread_mutexattr_t connect_server_mutex_attr;
    pthread_mutexattr_init(&connect_server_mutex_attr);
    pthread_mutexattr_setpshared(&connect_server_mutex_attr, PTHREAD_PROCESS_SHARED);
    pthread_mutex_init(&(connectinfo->connect_server_mutex), &connect_server_mutex_attr);

    pthread_mutex_lock(&connectinfo->id_mutex);
    int i = 0;
    for (i = 0; i < MAX_CLIENTS; i++)
    {
        if (connectinfo->id_arr[i] == false)
        {
            connectinfo->id_arr[i] = true;
            clientid = i * PRIME + PRIME;
            PRINT_INFO("You have been assigned a server, you have been assigned a client id:%d. Please remember this id", clientid);
            break;
        }
    }
    if (i == MAX_CLIENTS)
    {
        PRINT_INFO("Server is full, Check again later");
    }
    pthread_mutex_unlock(&connectinfo->id_mutex);

    // requestcode => 0-no request, 1-new user request, 2-existing user request
    // requestcode => 0-no response, 1-successful registratoin, 2-non unique id
    while (1)
    {
        if (access == 0)
        {
            char tmp[NAME_SIZE];
            PRINT_INFO("Please Enter a username");
            scanf("%s", tmp);
            PRINT_INFO("Name has been entered");
            PRINT_INFO("%s", tmp);
            pthread_mutex_lock(&connectinfo->connect_server_mutex);
            PRINT_INFO("%s", tmp);
            strcpy(connectinfo->username, tmp);
            PRINT_INFO("Requested Server");
            connectinfo->id = clientid;
            connectinfo->requestcode = 1;

            pthread_mutex_unlock(&connectinfo->connect_server_mutex);

            while (connectinfo->responsecode == 0)
            {
            }

            pthread_mutex_lock(&connectinfo->connect_server_mutex);
            if (connectinfo->responsecode == 1)
            {
                PRINT_INFO("Server has been successfuly Assigned");
                connectinfo->responsecode = 0;
                access = 1;
            }
            else if (connectinfo->responsecode == 2)
            {
                connectinfo->responsecode = 0;
                PRINT_INFO("Username is already taken,enter a username when prompted");
            }
            // pthread_mutex_unlock(&connectinfo->connect_server_mutex);
        }
        else if (access == 1)
        {
        }

        //     if (access == 0)
        //     {
        //         strcpy(connectinfo->username, tmp);
        //         connectinfo->id = clientid;
        //         connectinfo->requestcode = 1;
        //         while (connectinfo->responsecode == 0)
        //         {
        //         }
        //         if (connectinfo->responsecode == 1)
        //         {
        //             PRINT_INFO("Server has been sucessfully assigned");
        //             PRINT_INFO("%d \n", connectinfo->responsecode);
        //             access = 1;
        //         }
        //         else if (connectinfo->responsecode == 2)
        //         {
        //             PRINT_INFO("username already exists in server, Enter another username");
        //             scanf("%s", tmp);
        //         }
        //     }
        //     else if (access == 1)
        //     {
        //     }
        // }
        //shmdt(connectinfo); //why the fuck was this hereW
    }
}