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

typedef struct shared_data_t
{
      pthread_mutex_t mutex;
      request_info request;
      response_info response;
} shared_data_t;

bool connected;
bool idAssigned;
int client_ID;
struct shared_data_t *data;
char op[NAME_SIZE];
struct connectInfo *connectinfo;

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
    PRINT_INFO("\033[1;31mExiting.....\033[1;0m");
    if(!connected){
        if(idAssigned){
            connectinfo->id_arr[client_ID/PRIME - 1] = false;
        }
        exit(-1);
    }
    int choice;
    PRINT_INFO("1. Disconnect from the server and exit.");
    PRINT_INFO("2. Unregister from the server and exit.");
    PRINT_INFO("\033[1;35mEnter an Option: ");
    scanf("%d",&choice);
    switch(choice)
    {   
        case 1: PRINT_INFO("\033[1;36mDisconnecting");
                strcpy(op,"disconnect");
                break;

        case 2: PRINT_INFO("\033[1;36mUnregistering");
                strcpy(op,"unregister");
                break;
        default:
                PRINT_INFO("\033[1;31mEnter a valid Choice");
                break;
    }
    printf("adasd\033[1;0m");

    pthread_mutex_lock(&(data->mutex));
    data->request.a = 0;
    data->request.b = 0;
    strcpy(data->request.op,op);
    printf("\033[1;0m");
    PRINT_INFO("\033[1;0mRequested operation : \033[1;36m%s\033[1;0m",data->request.op);
    data->request.param = 0;
    data->response.response_code = -1;
    pthread_mutex_unlock(&(data->mutex));
    while(data->response.response_code==-1);
    pthread_mutex_lock(&(data->mutex));
    if(data->response.response_code==404)
    {
        PRINT_ERROR("Error Occured. Exiting.");
        exit(0);
    }
    pthread_mutex_unlock(&(data->mutex));
    exit(-1);
}

int main()
{
    int clientid;
    int access = 0;

    if ((connect_shmid = shmget(SHM_KEY, sizeof(struct connectInfo), 0)) == -1)
    {
        PRINT_ERROR("\033[1;31mServer is not available, Server has to be started first\033[1;0m");
        return 1;
    }
    else
    {
        PRINT_INFO("\033[1;32mServer Exists\033[1;0m");
    }
    PRINT_INFO("%d", connect_shmid);
    connectinfo = shmat(connect_shmid, NULL, 0);

    if (connectinfo == (void *)-1)
    {
        PRINT_ERROR("Unable to attach shared memory");
        return 1;
    }

    signal(SIGINT, handle_sigint);
    pthread_mutexattr_t attr;
    pthread_mutexattr_init(&attr);
    pthread_mutexattr_setpshared(&attr, PTHREAD_PROCESS_SHARED);
    pthread_mutex_init(&(connectinfo->id_mutex), &attr);

    while(1)
    {
        int c = 0;
        PRINT_INFO("Welcome, Kindly select one of the option.");
        PRINT_INFO("1. New user");
        PRINT_INFO("2. Existing Useer.");
        PRINT_INFO("\033[1;35mEnter an Option: ");
        scanf("%d", &c);
        printf("\033[1;0m");
        idAssigned = false;
        switch (c)
        {
        case 1:
            pthread_mutex_lock(&connectinfo->id_mutex);
            int i = 0;
            for (i = 0; i < MAX_CLIENTS; i++)
            {
                if (connectinfo->id_arr[i] == false)
                {
                    connectinfo->id_arr[i] = true;
                    clientid = i * PRIME + PRIME;
                    PRINT_INFO("\033[1;32mYou have been assigned a server, you have been assigned a client id:%d. \nPlease remember this id\033[1;0m", clientid);
                    idAssigned = true;
                    break;
                }
            }
            if (i == MAX_CLIENTS)
            {
                PRINT_INFO("\033[1;31mServer is full, Check again later\033[1;0m");
            }
            pthread_mutex_unlock(&connectinfo->id_mutex);
            if(!idAssigned)
                sleep(2);
            break;
        case 2:
            int enteredId = 0;
            PRINT_INFO("\033[1;33mEnter your ID:");
            scanf("%d",&enteredId);
            
            if(enteredId % PRIME != 0 || (enteredId / PRIME) -1 > MAX_CLIENTS || (enteredId / PRIME) - 1 < 0 
            || connectinfo->id_arr[enteredId/PRIME - 1] == false){
                PRINT_INFO("\033[1;31mEnter a valid ID");
                break;
            }
            if(connectinfo->disconnet[(enteredId / PRIME) - 1] == false)
                PRINT_INFO("\033[1;31mID already in use, Client is still active");
            clientid = enteredId;
            pthread_mutex_lock(&connectinfo->id_mutex);
            connectinfo->id_arr[client_ID/PRIME - 1] = true;
            pthread_mutex_unlock(&connectinfo->id_mutex);
            idAssigned = true;
            break;
        default:
            PRINT_INFO("\033[1;31mEnter a valid Choice");
            break;
        }
        
        printf("\033[1;0m");
        if(idAssigned){
            break;
        }
    }
    client_ID = clientid * PRIME + PRIME;

    char tmp[NAME_SIZE];
    // requestcode => 0-no request, 1-new user request, 2-existing user request
    // requestcode => 0-no response, 1-successful registratoin, 2-non unique id
    while (1)
    {
        int xyz = 0;
        if (access == 0)
        {
            PRINT_INFO("\033[1;33mPlease Enter a username");
            scanf("%s", tmp);
            printf("\033[1;0m");
            pthread_mutex_lock(&connectinfo->connect_server_mutex);
            strcpy(connectinfo->username, tmp);
            connectinfo->id = clientid;
            connectinfo->requestcode = 1;
            PRINT_INFO("Making register request to server");    
            pthread_mutex_unlock(&connectinfo->connect_server_mutex);
            while (connectinfo->responsecode == 0)
            {
            }

            pthread_mutex_lock(&connectinfo->connect_server_mutex);
            if (connectinfo->responsecode == 1)
            {
                PRINT_INFO("\033[1;32mServer has been sucessfully assigned\033[1;0m");
                
                connectinfo->responsecode = 0;
                access = 1;
            }
            if (connectinfo->responsecode == 2)
            {
                PRINT_INFO("\033[1;31mUsername already exists in server\033[1;33m");
                connectinfo->responsecode = 0;
            }
            pthread_mutex_unlock(&connectinfo->connect_server_mutex);
            printf("\033[1;0m");
        }
        else if (access == 1)
        {
            int comm_shmid = 0;
            int client_id_temp = (clientid-PRIME)/PRIME ;
            if ((comm_shmid = shmget(client_id_temp + 1, sizeof(shared_data_t), 0)) == -1)
            {
                PRINT_ERROR("\033[1;31mServer is not available, Server has to be started first\033[1;0m");
                return 1;
            }
            // else
            // {
            //     PRINT_INFO("CONNECTED TO COMMUNICATION CHANNEL");
            // }
            PRINT_INFO("%d",comm_shmid);
            // struct shared_data_t *data;
            data = shmat(comm_shmid, NULL, 0);
            connected = true;
            PRINT_INFO("CONNECTED TO COMMUNICATION CHANNEL");
            pthread_mutexattr_t attr2;
            pthread_mutexattr_init(&attr2);
            pthread_mutexattr_setpshared(&attr2, PTHREAD_PROCESS_SHARED);
            if (pthread_mutex_init(&(data->mutex), &attr2) != 0) {
                PRINT_INFO("\033[1;31mMutex initialization failed\033[1;0m");
                return 1;
            }
            // pthread_mutex_init(data->mutex, );
            int choice = -1;
            float num1 = -1;
            float num2 = -1;
            int param = -1;
            while(1)
            {
                PRINT_INFO("Welcome, Kindly select one of the option.");
                PRINT_INFO("1. Add Two Numbers.");
                PRINT_INFO("2. Subtract Two Numbers.");
                PRINT_INFO("3. Multiply Two Numbers.");
                PRINT_INFO("4. Divide Two Numbers.");
                PRINT_INFO("5. Check If A Number is Prime.");
                PRINT_INFO("6. Check If A Number is Odd or Even.");
                PRINT_INFO("7. Disconnect from the server and exit.");
                PRINT_INFO("8. Unregister from the server and exit.");
                PRINT_INFO("\033[1;35mEnter an Option: ")
                scanf("%d",&choice);
                printf("\033[1;0m");
                switch(choice)
                {
                    case 1: PRINT_INFO("\033[1;36mEnter First Number!");
                            scanf("%f",&num1);
                            PRINT_INFO("\033[1;36mEnter Second Number!")
                            scanf("%f",&num2);
                            param = 1;
                            strcpy(op,"arithmetic");
                            break;

                    case 2: PRINT_INFO("\033[1;36mEnter First Number!");
                            scanf("%f",&num1);
                            PRINT_INFO("\033[1;36mEnter Second Number!")
                            scanf("%f",&num2);
                            param = 2;
                            strcpy(op,"arithmetic");
                            break;

                    case 3: PRINT_INFO("\033[1;36mEnter First Number!");
                            scanf("%f",&num1);
                            PRINT_INFO("\033[1;36mEnter Second Number!")
                            scanf("%f",&num2);
                            param = 3;
                            strcpy(op,"arithmetic");
                            break;

                    case 4: PRINT_INFO("\033[1;36mEnter First Number!");
                            scanf("%f",&num1);
                            PRINT_INFO("\033[1;36mEnter Second Number!")
                            scanf("%f",&num2);
                            param = 4;
                            strcpy(op,"arithmetic");
                            break;
                    
                    case 5: PRINT_INFO("\033[1;36mEnter The Number");
                            scanf("%f",&num1);
                            strcpy(op,"prime");
                            break;

                    
                    case 6: PRINT_INFO("\033[1;36mEnter The Number");
                            scanf("%f",&num1);
                            strcpy(op,"oddeven");
                            break;
                    case 7: PRINT_INFO("\033[1;36mDisconnecting");
                            strcpy(op,"disconnect");
                            break;

                    case 8: PRINT_INFO("\033[1;36mUnregistering");
                            strcpy(op,"unregister");
                            break;
                    default:
                            PRINT_INFO("\033[1;31mEnter a valid Choice");
                            break;
                }
                pthread_mutex_lock(&(data->mutex));
                data->request.a = num1;
                data->request.b = num2;
                strcpy(data->request.op,op);
                printf("\033[1;0m");
                PRINT_INFO("\033[1;0mRequested operation : \033[1;36m%s\033[1;0m",data->request.op);
                data->request.param = param;
                data->response.response_code = -1;
                PRINT_INFO("Sending request to server through communication channel");
                pthread_mutex_unlock(&(data->mutex));


                while(data->response.response_code==-1);
                pthread_mutex_lock(&(data->mutex));
                PRINT_INFO("Recieved a response from server.");
                if(data->response.response_code==404)
                {
                    PRINT_ERROR("Error Occured. Exiting.");
                    exit(0);
                }
                int ans;
                switch(choice)
                {
                    case 1: ans= data->response.data.answer;
                            PRINT_INFO("\033[1;32mThe Answer is %d",ans);
                            break;
                    case 2: ans= data->response.data.answer;
                            PRINT_INFO("\033[1;32mThe Answer is %d",ans);
                            break;
                    case 3: ans= data->response.data.answer;
                            PRINT_INFO("\033[1;32mThe Answer is %d",ans);
                            break;
                    case 4: ans= data->response.data.answer;
                            PRINT_INFO("\033[1;32mThe Answer is %d",ans) ;                          
                            break;

                    case 5: bool isPrime = data->response.data.trueOrFalse;
                            if(isPrime==true)
                            PRINT_INFO("\033[1;32mYes! It is a Prime Number.")
                            else PRINT_INFO("\033[1;32mNo, It is not a Prime Number");
                            break;

                    case 6: char oddOrEven[5];
                            strcpy(oddOrEven,data->response.data.oddOrEven);
                            PRINT_INFO("\033[1;32mThe Entered Number is %s",oddOrEven);
                            break;
                    case 7:
                            PRINT_INFO("\033[1;32mDisconnected successfully");
                            access = 0;
                            xyz = 1;
                            break;
                    case 8:
                            PRINT_INFO("\033[1;32mUnRegistered successfully");
                            access = 0;
                            xyz = 1;
                            break;
                    }
                printf("\033[1;0m");
                pthread_mutex_unlock(&(data->mutex));
                if(xyz == 1)
                    break;
            }
        }
        if(xyz == 1)
            break;
    }
    PRINT_INFO("\033[1;31mExiting Client!\033[1;0m");
}