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
#define SHM_KEY 0x1234 // Used for conntection channel


pthread_t process[MAX_CLIENTS];
int num;                    // Current number of worker threads
int childShId[MAX_CLIENTS];
int shmid_t[MAX_CLIENTS];
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

int arithmeticFunctions(int num1, int num2, int operation)
{
    int ans = 0;
    switch(operation)
    {
      case 1: ans = num1+num2;
              break;

      case 2: ans = num1-num2;
              break;

      case 3: ans = num1*num2;
              break;

      case 4: ans = num1/num2;
              break;

    }
    return ans;
}

void oddOrEven(int num, char *ans)
{
    if(num%2==0)
    strcpy(ans,"Even");
    else strcpy(ans,"Odd");
} 

bool primeCheck(int n)
{
    if (n < 2)
        return false;
    for (int i = 2; i * i <= n; i++)
    {
        if (n % i == 0)
            return false;
    }
    return true;
}

// void handle_sigint(int sig)
// {
//     PRINT_INFO("\nClosing shared memory segment.....\n");

//     // Close connection Shared memory
//     shmctl(shmid, IPC_RMID, NULL);

//     // Close response Shared memory for each client
//     for (int i = 0; i < num; i++)
//     {
//         shmctl(childShId[i], IPC_RMID, NULL);
//     }

//     printf("Closing worker Threads....\n");
//     // Close worker Threads
//     for (int i = 0; i < num; i++)
//     {
//         kill(*(process + i), SIGKILL);
//     }
//     exit(-1);
// }

void *threadFunction(void *arg)
{ 
    int id = *(int *)arg;
    PRINT_INFO("\nWorker Thread Executing : {%d}\n", id);
    int shmid; // Connection shared memory id
    if ((shmid = shmget(SHM_KEY, sizeof(shared_data_t), 0644 | IPC_CREAT)) == -1)
    {
        PRINT_ERROR("Shared memory");
        return NULL;
    }
    shared_data_t *data;
    data = shmat(id, NULL, 0);
    pthread_mutex_lock(&(data->mutex));
    if(data->request.op == "prime")
    {
        bool ans = primeCheck(data->request.a);
        data->response.data.trueOrFalse = ans;
        data->response.response_code = 0;
        data->response.server_response_code++;
    }
    else if(data->request.op == "oddeven")
    {
        char ans[5];
        oddOrEven(data->request.a,ans);
        strcpy(data->response.data.oddOrEven,ans);
        data->response.response_code = 0;
        data->response.server_response_code++;
    }
    else if (data->request.op=="arithmetic") 
    {
        int operation = data->request.param;
        int ans = arithmeticFunctions(data->request.a,data->request.b,operation);
        data->response.data.answer = ans;
        data->response.response_code = 0;
        data->response.server_response_code++;
    };
    pthread_mutex_unlock(&(data->mutex));
}

int main()
{}