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
int total_requests_served = 0;
int number_of_connected_clients = 0;
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
        printf("%s \033[1;31mERROR\033[1;0m %d:%d %ld %s %s %d : " MSG "\n",          \
               __TIME__, getpid(), getppid(), pthread_self(), __FILE__, __FUNCTION__, \
               __LINE__, ##__VA_ARGS__);                                              \
    }

int connect_shmid;
bool id_arr[MAX_CLIENTS];
bool disconnet[MAX_CLIENTS];

struct connectInfo
{
    int requestcode;
    int responsecode;
    char username[NAME_SIZE];
    int id;
    pthread_mutex_t mutex;
};

struct clientInfo
{
    int clientid;
    char username[NAME_SIZE];
    int requests;
    int responses;
};

struct clientInfo clientinfo[MAX_CLIENTS];
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
    struct connectInfo *connectinfo;
    connectinfo = shmat(connect_shmid, NULL, 0);
    
    int id = *(int *)arg;
    PRINT_INFO("\nWorker Thread Executing : {%d}\n", id);
    shared_data_t *data;
    data = shmat(childShId[id], NULL, 0);
    PRINT_INFO("\033[1;32mCommunication Channel Created");
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
            PRINT_INFO("Successfully responded to client.");
        }
        else if(strcmp(data->request.op,"oddeven")==0)
        {
            char ans[5];
            oddOrEven(data->request.a,ans);
            strcpy(data->response.data.oddOrEven,ans);
            data->response.response_code = 0;
            data->response.server_response_code++;
            PRINT_INFO("Successfully responded to client.");
        }
        else if(strcmp(data->request.op,"unregister")==0)
        {
            pthread_mutex_lock(&connectinfo->mutex);
            id_arr[id] = false;
            disconnet[id] = true;
            strcpy(clientinfo[id].username, "");
            data->response.response_code = 0;
            data->response.server_response_code++;
            pthread_mutex_unlock(&connectinfo->mutex);

            pthread_mutex_unlock(&(data->mutex));
            shmctl(childShId[id], IPC_RMID, NULL);
            PRINT_INFO("\033[1;31mUnregistered successfully for client id: %d", id);
            number_of_connected_clients--;
            pthread_cancel(pthread_self());
        }
        else if(strcmp(data->request.op,"disconnect")==0)
        {
            pthread_mutex_lock(&connectinfo->mutex);
            id_arr[id] = true;
            disconnet[id] = true;   
            strcpy(clientinfo[id].username, "");
            data->response.response_code = 0;
            data->response.server_response_code++;
            pthread_mutex_unlock(&connectinfo->mutex);
            pthread_mutex_unlock(&(data->mutex));
            PRINT_INFO("\033[1;31mDisconnected successfully for client id: %d\033[0m ", id);
            number_of_connected_clients--;
            pthread_cancel(pthread_self());
        }
        else
        {
            int operation = data->request.param;
            float ans = arithmeticFunctions(data->request.a,data->request.b,operation);
            data->response.data.answer = ans;
            data->response.response_code = 0;
            data->response.server_response_code++;
            PRINT_INFO("Successfully responded to client.");

        };
        total_requests_served++;
        PRINT_INFO("\033[1;34mServer Summary:\033[0m");
        PRINT_INFO("\033[0m");
        PRINT_INFO("LIST OF CONNECTED CLIENTS: ");
        for(int i = 0; i< MAX_CLIENTS;i++)
        {
            if((strcmp(clientinfo[i].username,"")!=0)&&(strcmp(clientinfo[i].username,"\0")!=0))
            PRINT_INFO("%s",clientinfo[i].username);
        }
        PRINT_INFO("Number Of Requests Served of Client %s: %d",clientinfo[id].username,data->response.server_response_code);
        PRINT_INFO("Total Requests Served: %d",total_requests_served);
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

int GetClientID(int index){
    return index * PRIME + PRIME;
}

int GetClientIndex(int id){
    return id / PRIME - 1;
}

bool IsValidID(int id){
    return id % PRIME == 0 && GetClientIndex(id) >= 0;
}

int main()
{
    
    PRINT_INFO("\033[1;32mServer Started");
    PRINT_INFO("\033[0m");
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
    for (int i = 0; i < MAX_CLIENTS; i++)
    {
        disconnet[i] = true;
    }
    
    PRINT_INFO("\033[1;32mConnect Channel Successfully Created\033[0m");
    PRINT_INFO("\033[0m");
    signal(SIGINT, handle_sigint);
    pthread_mutexattr_t connect_server_mutex_attr;
    pthread_mutexattr_init(&connect_server_mutex_attr);
    pthread_mutexattr_setpshared(&connect_server_mutex_attr, PTHREAD_PROCESS_SHARED);
    pthread_mutex_init(&(connectinfo->mutex), &connect_server_mutex_attr);

    while (1)
    {
        // requestcode => 0-no request, 1-new user request, 2-existing user request
        // requestcode => 0-no response, 1-successful registration, 2-non unique id, 3-Server Full
        pthread_mutex_lock(&connectinfo->mutex);
        if (connectinfo->requestcode == 1)
        {
            int flag = 0;
            connectinfo->requestcode = 0;
            if(connectinfo->id != 0){
                int clientIndex = GetClientIndex(connectinfo->id);
                if(!IsValidID(connectinfo->id) || id_arr[clientIndex] == false || disconnet[clientIndex] == false){
                    flag = 1;
                }
            }
            if(flag == 0)
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
                int clientIndex = GetClientIndex(connectinfo->id);
                if(connectinfo->id == 0){
                    int i = 0;
                    for (i = 0; i < MAX_CLIENTS; i++)
                    {
                        if (id_arr[i] == false)
                        {
                            id_arr[i] = true;
                            clientIndex = i;
                            break;
                        }
                    }
                    if (i == MAX_CLIENTS)
                    {
                        connectinfo->responsecode = 3;
                        pthread_mutex_unlock(&connectinfo->mutex);
                        continue;
                    }
                }
                connectinfo->id = GetClientID(clientIndex);
                disconnet[clientIndex] = false;
                clientinfo[clientIndex].clientid = connectinfo->id;
                strcpy(clientinfo[clientIndex].username, connectinfo->username);
                
                if ((childShId[clientIndex] = shmget(GetClientID(clientIndex), sizeof(shared_data_t), 0644 | IPC_CREAT)) == -1)
                {
                    PRINT_ERROR("Shared memory");
                }
                process[clientIndex] = pthread_create(&process[clientIndex], NULL, threadFunction, (void *)&clientIndex);
                connectinfo->responsecode = 1;
                
                number_of_connected_clients++;
            }
        }
        pthread_mutex_unlock(&connectinfo->mutex);
        while (connectinfo->requestcode == 0)
        {
        }
    }
}