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
    char name[NAME_SIZE];
    int key;
    int connected;
} client_info;

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
    char op[BUF_SIZE];
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
  char name[NAME_SIZE];
  PRINT_INFO("\nEnter a client name:");
  scanf("%s", &name[0]);
  PRINT_INFO("Client name : {%s} Length {%ld} \n", name, strlen(name));  
    //  Connect Channel Needs to be implemented
    int id = 0;
  // Connect Channel to be implemented
  int shmid;
  int comm_shm_key = hash(name);
  if ((shmid = shmget(comm_shm_key, sizeof(shared_data_t), 0644 | IPC_CREAT)) == -1)
    {
        PRINT_ERROR("Shared memory");
        exit(0);
    }
    shared_data_t *data;
    data = shmat(id, NULL, 0);

    pthread_mutexattr_t attr;
    pthread_mutexattr_init(&attr);
    pthread_mutexattr_setpshared(&attr, PTHREAD_PROCESS_SHARED);
    pthread_mutex_init(&(data->mutex), &attr);
    int choice = -1;
    int num1 = -1;
    int num2 = -1;
    int param = -1;
    char op[NAME_SIZE];
    //Menu Implementation
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
                    strcpy(op,"arithmetic");;
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
        switch(choice)
        {
            case 1:
            case 2:
            case 3:
            case 4: int answer = data->response.data.answer;
                    PRINT_INFO("\nThe Answer is %d",answer);
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