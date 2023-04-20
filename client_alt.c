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
};
typedef struct 
{
  int response_code;
  int client_response_code;
  int server_response_code;
  union 
  {
    bool trueOrFalse;
    char oddOrEven[5];
    int answer;
  } data;
} response_info;
typedef struct
{
    int a;
    int b;
    char op[NAME_SIZE];
    int param;
} request_info;

typedef struct 
{
      pthread_mutex_t mutex;
      request_info request;
      response_info response;
} shared_data_t;

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

    char tmp[NAME_SIZE];
    PRINT_INFO("Please Enter a username");
    scanf("%s", tmp);
    // requestcode => 0-no request, 1-new user request, 2-existing user request
    // requestcode => 0-no response, 1-successful registratoin, 2-non unique id
    while (1)
    {
        if (access == 0)
        {
            strcpy(connectinfo->username, tmp);
            connectinfo->id = clientid;
            connectinfo->requestcode = 1;
            while (connectinfo->responsecode == 0)
            {
            }
            if (connectinfo->responsecode == 1)
            {
                PRINT_INFO("Server has been sucessfully assigned");
                access = 1;
            }
            if (connectinfo->responsecode == 2)
            {
                PRINT_INFO("username already exists in server, Enter another username");
                scanf("%s", tmp);
            }
        }
        else if (access == 1)
        {
            int comm_shmid = 0;
            int client_id_temp = (clientid-PRIME)/PRIME ;
            if ((comm_shmid = shmget(client_id_temp, sizeof(shared_data_t), 0)) == -1)
            {
                PRINT_ERROR("Server is not available, Server has to be started first");
                return 1;
            }
            // else
            // {
            //     PRINT_INFO("CONNECTED TO COMMUNICATION CHANNEL");
            // }
            PRINT_INFO("%d",comm_shmid);
            shared_data_t *data;
            data = shmat(comm_shmid, NULL, 0);
            PRINT_INFO("CONNECTED TO COMMUNICATION CHANNEL");
            pthread_mutexattr_t attr2;
            pthread_mutexattr_init(&attr2);
            pthread_mutexattr_setpshared(&attr2, PTHREAD_PROCESS_SHARED);
            sleep(2);
            if (pthread_mutex_init(&(data->mutex), &attr2) != 0) {
        PRINT_INFO("mutex initialization failed\n");
        return 1;
    }
            // pthread_mutex_init(data->mutex, );
            int choice = -1;
            int num1 = -1;
            int num2 = -1;
            int param = -1;
            char op[NAME_SIZE];
            while(1)
            {
                PRINT_INFO("\nWelcome, Kindly select one of the option.");
                PRINT_INFO("\n1. Add Two Numbers.");
                PRINT_INFO("\n2. Subtract Two Numbers.");
                PRINT_INFO("\n3. Multiply Two Numbers.");
                PRINT_INFO("\n4. Divide Two Numbers.");
                PRINT_INFO("\n5. Check If A Number is Prime.");
                PRINT_INFO("\n6. Check If A Number is Odd or Even.");
                PRINT_INFO("\n0. Unregister from the server and exit.");
                scanf("%d",&choice);
                switch(choice)
                {
                    case 1: PRINT_INFO("\nEnter First Number!");
                            scanf("%d",&num1);
                            PRINT_INFO("\nEnter Second Number!")
                            scanf("%d",&num2);
                            param = 1;
                            strcpy(op,"arithmetic");
                            break;

                    case 2: PRINT_INFO("\nEnter First Number!");
                            scanf("%d",&num1);
                            PRINT_INFO("\nEnter Second Number!")
                            scanf("%d",&num2);
                            param = 2;
                            strcpy(op,"arithmetic");
                            break;

                    case 3: PRINT_INFO("\nEnter First Number!");
                            scanf("%d",&num1);
                            PRINT_INFO("\nEnter Second Number!")
                            scanf("%d",&num2);
                            param = 3;
                            strcpy(op,"arithmetic");
                            break;

                    case 4: PRINT_INFO("\nEnter First Number!");
                            scanf("%d",&num1);
                            PRINT_INFO("\nEnter Second Number!")
                            scanf("%d",&num2);
                            param = 4;
                            strcpy(op,"arithmetic");
                            break;
                    
                    case 5: PRINT_INFO("\nEnter The Number");
                            scanf("%d",&num1);
                            strcpy(op,"prime");
                            break;

                    
                    case 6: PRINT_INFO("\nEnter The Number");
                            scanf("%d",&num1);
                            strcpy(op,"oddeven");
                            break;

                    case 0: PRINT_INFO("TO BE DONE.");
                            break;
                }
                pthread_mutex_lock(&(data->mutex));
                data->request.a = num1;
                data->request.b = num2;
                strcpy(data->request.op,op);
                PRINT_INFO("%s",data->request.op);
                data->request.param = param;
                data->response.response_code = -1;
                pthread_mutex_unlock(&(data->mutex));


                while(data->response.response_code==-1);
                pthread_mutex_lock(&(data->mutex));
                if(data->response.response_code==404)
                {
                    PRINT_ERROR("Error Occured. Exiting.");
                    exit(0);
                }
                int ans;
                switch(choice)
                {
                    case 1: ans= data->response.data.answer;
                            PRINT_INFO("\nThe Answer is %d",ans);
                            break;
                    case 2: ans= data->response.data.answer;
                            PRINT_INFO("\nThe Answer is %d",ans);
                            break;
                    case 3: ans= data->response.data.answer;
                            PRINT_INFO("\nThe Answer is %d",ans);
                            break;
                    case 4: ans= data->response.data.answer;
                            PRINT_INFO("\nThe Answer is %d",ans) ;                          
                            break;

                    case 5: bool isPrime = data->response.data.trueOrFalse;
                            if(isPrime==true)
                            PRINT_INFO("\nYes! It is a Prime Number.")
                            else PRINT_INFO("\nNo, It is not a Prime Number");
                            break;

                    case 6: char oddOrEven[5];
                            strcpy(oddOrEven,data->response.data.oddOrEven);
                            PRINT_INFO("\nThe Entered Number is %s",oddOrEven);
                            break;
                }
                pthread_mutex_unlock(&(data->mutex));
            }
        }
    }
    shmdt(connectinfo);
}