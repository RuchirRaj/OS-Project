#include<stdio.h>
#include<sys/ipc.h>
#include<sys/shm.h>
#include<sys/types.h>
#include<string.h>
#include<errno.h>
#include<stdlib.h>
#include<signal.h>
#include <pthread.h>

#define NAME_SIZE 256
#define BUF_SIZE 1024
#define SHM_KEY 0x1234 //Used for conntection channel
#define MAX_THREADS 100

int shmid; //Connection shared memory id
pthread_t process[MAX_THREADS];
int num; //Current number of worker threads
int childShId[MAX_THREADS]; //Shared memory id for each child worker/client

//Struct for connection request
struct conSeg{
    int lock;
    int shid;
    int id;
    char name[NAME_SIZE];
};

//Struct for indivisual responses
struct resSeg
{
    int a;
    int b;
    char op[BUF_SIZE];
};

void handle_sigint(int sig)
{
    printf("\nClosing shared memory segment.....\n");
    
    //Close connection Shared memory
    shmctl(shmid, IPC_RMID, NULL);
    
    //Close response Shared memory for each client
    for (int i = 0; i < num; i++)
    {
        shmctl(childShId[i], IPC_RMID, NULL);
    }
    
    printf("Closing worker Threads....\n");
    //Close worker Threads
    for (int i = 0; i < num; i++)
    {
        kill(*(process + i), SIGKILL);
    }
    exit(-1);
}

void* ResponseThread(void* d)
{
    printf("\nWorker Thread Executing\n");
    int id = *(int*)d;
    struct resSeg *resSeg;
    resSeg = shmat(id, NULL, 0);
    //Just a sample response
    while(1){
        resSeg->a += resSeg->b;
        sleep(1);
    }
    return d;
}

int main(int argc, char *argv[]) 
{
    if((shmid = shmget(SHM_KEY, sizeof(struct conSeg), 0644|IPC_CREAT)) == -1)
    {
        perror("Shared memory");
        return 1;
    }

    struct conSeg *conSeg;

    // Attach to the segment to get a pointer to it.
    conSeg = shmat(shmid, NULL, 0);
    if (conSeg == (void *) -1) {
        perror("Shared memory attach");
        return 1;
    }
    
    conSeg->id = 0;
	signal(SIGINT, handle_sigint);

    while (1)
    {
        if(conSeg->lock != 0)
            continue;
        conSeg-> lock = 1;
        if(conSeg->id != 0)
        {
            printf("New Connection Request");
            childShId[num] = shmget(ftok(conSeg->name, conSeg->id), sizeof(struct resSeg), 0644|IPC_CREAT);
            process[num] = pthread_create(&process[num], NULL, ResponseThread, (void*)&childShId[num]);
            conSeg->id = 0;
            conSeg->shid = childShId[num];
            num++;
            conSeg->lock = 3;
            continue;
        }
        conSeg->lock = 0;
    }
    
    return 0;
}