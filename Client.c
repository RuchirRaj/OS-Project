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

#define NAME_SIZE 256
#define BUF_SIZE 1024
#define SHM_KEY 0x1234 // Used for conntection channel

struct conSeg
{
    int lock;
    int shid;
    int id;
    char name[NAME_SIZE];
};
struct resSeg
{
    int a;
    int b;
    char op[BUF_SIZE];
};

int main(int argc, char *argv[])
{
    int id = 200;
    char name[NAME_SIZE];
    PRINT_INFO("\nEnter a client name:");
    scanf("%s", &name[0]);

    PRINT_INFO("Client name : {%s} Length {%ld} \n", name, strlen(name));

    int conSegID;
    int resSegID;

    if ((conSegID = shmget(SHM_KEY, sizeof(struct conSeg), 0)) == -1)
    {
        PRINT_ERROR("Shared memory Not found, please start the server first");
        return 1;
    }

    struct conSeg *conSeg;

    // Attach to the segment to get a pointer to it.
    conSeg = shmat(conSegID, NULL, 0);
    if (conSeg == (void *)-1)
    {
        PRINT_ERROR("Shared memory attach");
        return 1;
    }

    int registered = 0;
    struct resSeg *resSeg;

    while (1)
    {
        // Adding a request
        if (registered == 0)
        {
            if (conSeg->lock != 0 || conSeg->id != 0) // In case there is already a connection request
                continue;
            conSeg->lock = 2;
            PRINT_INFO("Adding connection Request....\n");
            conSeg->id = id;

            for (int i = 0; i < strlen(name); i++)
            {
                conSeg->name[i] = name[i];
            }

            registered = 1;
            conSeg->lock = 0;
        }

        // Waiting for response
        if (registered == 1)
        {
            if (conSeg->lock != 3)
            {
                // printf("Waiting for response....\n");
                continue;
            }
            conSeg->lock = 2;
            resSegID = conSeg->shid;
            resSeg = shmat(resSegID, NULL, 0);
            resSeg->a = 0;
            resSeg->b = 5;
            registered++;
            conSeg->lock = 0;
        }
        PRINT_INFO("Response value : %d \n", resSeg->a);
        sleep(1);
    }
    PRINT_INFO("Exiting client %d \n", conSeg->lock);

    return 0;
}