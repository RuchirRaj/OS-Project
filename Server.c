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
pthread_t process[MAX_CLIENTS];
int childShId[MAX_CLIENTS]; 
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
} connectInfo;

struct clientInfo
{
    int clientid;
    char username[NAME_SIZE];
    int requests;
    int responses;
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
    float answer;
  } data;
} response_info;
typedef struct
{
    float a;
    float b;
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

float arithmeticFunctions(float num1, float num2, int operation)
{
    float ans = 0;
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
void *threadFunction(void *arg)
{ 
    int id = *(int *)arg;
    PRINT_INFO("\nWorker Thread Executing : {%d}\n", id);
    shared_data_t *data;
    data = shmat(id, NULL, 0);
    data->response.response_code = -2;
    while(1)
    {
        while(data->response.response_code!=-1){};
        pthread_mutex_lock(&(data->mutex));
        if(strcmp(data->request.op ,"prime")==0)
        {
            bool ans = primeCheck(data->request.a);
            data->response.data.trueOrFalse = ans;
            data->response.response_code = 0;
            data->response.server_response_code++;
        }
        else if(strcmp(data->request.op,"oddeven")==0)
        {
            char ans[5];
            oddOrEven(data->request.a,ans);
            strcpy(data->response.data.oddOrEven,ans);
            data->response.response_code = 0;
            data->response.server_response_code++;
        }
        else
        {
            int operation = data->request.param;
            float ans = arithmeticFunctions(data->request.a,data->request.b,operation);
            data->response.data.answer = ans;
            data->response.response_code = 0;
            data->response.server_response_code++;
        };
        pthread_mutex_unlock(&(data->mutex));
    }
}

void handle_sigint(int sig)
{
    PRINT_INFO("\nClosing shared memory segment.....\n");

    // Close connection Shared memory
    shmctl(connect_shmid, IPC_RMID, NULL);

    // Close response Shared memory for each client
    for (int i = 0; i < MAX_CLIENTS; i++)
    {
        shmctl(childShId[i], IPC_RMID, NULL);
    }

    PRINT_INFO("Closing worker Threads....\n");
    // Close worker Threads
    for (int i = 0; i < MAX_CLIENTS; i++)
    {
        kill(*(process + i), SIGKILL);
    }
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

    while (1)
    {
        // requestcode => 0-no request, 1-new user request, 2-existing user request
        // responsecode => 0-no response, 1-successful registratoin, 2-non unique id
        pthread_mutex_lock(&connectinfo->connect_server_mutex);
        if (connectinfo->requestcode == 1)
        {
            int flag = 0;
            connectinfo->requestcode = 0;
            for (int i = 0; i < MAX_CLIENTS; i++)
            {
                if (strcmp(connectinfo->username, clientinfo[i].username) == 0)
                {
                    flag = 1;
                    break;
                }
            }
            if(flag == 1)
            {
                connectinfo->responsecode = 2;
            }else{
                clientinfo[(connectinfo->id - PRIME) / PRIME].clientid = connectinfo->id;
                strcpy(clientinfo[(connectinfo->id - PRIME) / PRIME].username, connectinfo->username);
                int client_id = (connectinfo->id - PRIME) / PRIME;
                if ((childShId[client_id] = shmget(client_id + 1, sizeof(shared_data_t), 0644 | IPC_CREAT)) == -1)
                {
                    PRINT_ERROR("Shared memory");
                    exit(0);
                }
                process[client_id] = pthread_create(&process[client_id], NULL, threadFunction, (void *)&childShId[client_id]);
                
                
                connectinfo->responsecode = 1;
            }
        }
        pthread_mutex_unlock(&connectinfo->connect_server_mutex);
        while (connectinfo->requestcode == 0)
        {
        }
    }
}