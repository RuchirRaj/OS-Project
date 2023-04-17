#include<stdio.h>
#include<sys/ipc.h>
#include<sys/shm.h>
#include<sys/types.h>
#include<string.h>
#include<errno.h>
#include<stdlib.h>

#define NAME_SIZE 256
#define BUF_SIZE 1024
#define SHM_KEY 0x1234 //Used for conntection channel

struct conSeg{
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
    printf("\nEnter a client name:");
    scanf("%s", &name[0]);

    printf("Client name : {%s} Length {%d}\n", name, strlen(name));

    int conSegID;
    int resSegID;

    if((conSegID = shmget(SHM_KEY, sizeof(struct conSeg), 0)) == -1)
    {
        perror("Shared memory Not found, please start the server first\n");
        return 1;
    }

    struct conSeg *conSeg;

    // Attach to the segment to get a pointer to it.
    conSeg = shmat(conSegID, NULL, 0);
    if (conSeg == (void *) -1) {
        perror("Shared memory attach");
        return 1;
    }

    int registered = 0;
    struct resSeg *resSeg;

    while (1)
    {
        //Adding a request
        if(registered == 0)
        {
            if(conSeg->lock != 0 
            || conSeg->id != 0) //In case there is already a connection request
                continue;
            conSeg->lock = 2;
            printf("Adding connection Request....\n");
            conSeg->id = id;

            for (int i = 0; i < strlen(name); i++)
            {
                conSeg->name[i] = name[i];
            }
            
            registered = 1;
            conSeg->lock = 0;
        }

        //Waiting for response
        if(registered == 1)
        {
            if(conSeg->lock != 3)
            {
                //printf("Waiting for response....\n");
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
        printf("Response value : %d \n", resSeg->a);
        sleep(1);
    }
    printf("Exiting client\n", conSeg->lock);

    return 0;
}