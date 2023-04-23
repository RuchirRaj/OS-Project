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

#define MAX_MESSAGE_LENGTH 100
#define BUF_SIZE 1024
#define SHM_SIZE 1024
#define NAME_SIZE 256
#define SHM_KEY 0x1234
#define TIMEOUT 1
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
pthread_t thread;
struct connectInfo
{
    int requestcode;
    int responsecode;
    char username[NAME_SIZE];
    int id;
    pthread_mutex_t mutex;
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

void sleep_ms(int milliseconds) {
    struct timespec ts;
    ts.tv_sec = milliseconds / 1000;
    ts.tv_nsec = (milliseconds % 1000) * 1000000;
    nanosleep(&ts, NULL);
}

void flushInput(){
  int c;
  while((c = getchar()) != EOF && c != '\n')
        /* discard */ ;  
}

void *checkServerConnection(void *arg)
{ 
    int shmid;
    while(1)
    {
        if ((shmid = shmget(client_ID, sizeof(shared_data_t), 0)) == -1)
        {
            PRINT_ERROR("\033[1;31mServer is not available, Server has to be started first\033[1;0m");
            exit(1);
        }
        sleep_ms(TIMEOUT * 1000);
    }
}

void handle_sigint(int sig)
{
    printf("\033[1;0m");
    PRINT_INFO("\033[1;31mExiting.....\033[1;0m");
    if(!connected){
        exit(-1);
    }
    int choice;
    PRINT_INFO("\033[1;36m1.\033[1;0m Disconnect from the server and exit.");
    PRINT_INFO("\033[1;36m2.\033[1;0m Unregister from the server and exit.");
    PRINT_INFO("\033[1;35mEnter an Option: ");
    while(scanf("%d", &choice) != 1)
    {
        flushInput();
        printf("\033[1;0m");
        PRINT_ERROR("\033[1;31mEnter a valid Choice\033[1;35m");
    }
    flushInput();
    printf("\033[1;0m");
    switch(choice)
    {   
        case 1: PRINT_INFO("\033[1;36mDisconnecting");
                strcpy(op,"disconnect");
                break;

        case 2: PRINT_INFO("\033[1;36mUnregistering");
                strcpy(op,"unregister");
                break;
        default:
                PRINT_ERROR("\033[1;31mEnter a valid Choice");
                break;
    }
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
    switch (choice)
    {
        case 1:
            PRINT_INFO("\033[1;32mDisconnected successfully");
            break;
        case 2:
            PRINT_INFO("\033[1;32mUnRegistered successfully");
            break;
    }
    printf("\033[1;0m");
    PRINT_INFO("\033[1;31mExiting Client!\033[1;0m");
    exit(-1);
}

int main()
{
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
    pthread_mutex_init(&(connectinfo->mutex), &attr);

    char tmp[NAME_SIZE];
    bool askID = true;
    // requestcode => 0-no request, 1-new user request, 2-existing user request
    // requestcode => 0-no response, 1-successful registration, 2-non unique id, 3-Server Full
    while (1)
    {
        int exited = 0;
        if (access == 0)
        {
            if(askID)
            while(1)
            {
                int choice = 0;
                PRINT_INFO("Welcome, Kindly select one of the option.");
                PRINT_INFO("\033[1;36m1.\033[1;0m New user");
                PRINT_INFO("\033[1;36m2.\033[1;0m Existing Useer.");
                PRINT_INFO("\033[1;35mEnter an Option: ");
                while(scanf("%d", &choice) != 1)
                {
                    flushInput();
                    printf("\033[1;0m");
                    PRINT_ERROR("\033[1;31mEnter a valid Choice\033[1;35m");
                }
                flushInput();
                printf("\033[1;0m");
                idAssigned = true;
                switch (choice)
                {
                    case 1:
                        client_ID = 0;
                        break;
                    case 2:
                        int enteredId = 0;
                        PRINT_INFO("\033[1;33mEnter your ID:");
                        while(scanf("%d",&enteredId) != 1)
                        {
                            flushInput();
                            PRINT_ERROR("\033[1;31mEnter a valid ID\033[1;0m");
                        }
                        flushInput();
                        printf("\033[1;0m");
                        client_ID = enteredId;
                        break;
                    default:
                        idAssigned = false;
                        PRINT_ERROR("\033[1;31mEnter a valid Choice\033[1;0m");
                        break;
                }
                
                printf("\033[1;0m");
                if(idAssigned)
                    break;
            }
            if(askID){
                PRINT_INFO("\033[1;33mPlease Enter a username");
                while(scanf("%s", tmp) != 1){
                    PRINT_ERROR("\033[1;31mEnter a valid Username\033[1;0m");
                    flushInput();
                }
                printf("\033[1;0m");
            }
            int ret;
            struct timespec ts;

            // Get the current time
            clock_gettime(CLOCK_REALTIME, &ts);

            // Add a timeout of 1 seconds
            ts.tv_sec += 1;
            pthread_mutex_timedlock(&connectinfo->mutex, &ts);
            if(connectinfo->requestcode != 0 || connectinfo->responsecode != 0)
            {
                pthread_mutex_unlock(&connectinfo->mutex);
                PRINT_ERROR("\033[1;31mWaiting\033[1;0m");
                sleep_ms(100);
                askID = false;
                continue;
            }
            strcpy(connectinfo->username, tmp);
            connectinfo->id = client_ID;
            connectinfo->requestcode = 1;
            PRINT_INFO("Making register request to server");    
            pthread_mutex_unlock(&connectinfo->mutex);
            while (connectinfo->responsecode == 0)
            {
            }

            // Get the current time
            clock_gettime(CLOCK_REALTIME, &ts);
            // Add a timeout of 1 seconds
            ts.tv_sec += 1;

            pthread_mutex_timedlock(&connectinfo->mutex, &ts);
            if (connectinfo->responsecode == 1)
            {
                client_ID = connectinfo->id;
                PRINT_INFO("\033[1;0mServer has been sucessfully assigned. ID :{\033[1;32m%d\033[1;0m}\033[1;0m", client_ID);
                connectinfo->requestcode = 0;
                connectinfo->responsecode = 0;
                access = 1;
            }
            else if (connectinfo->responsecode == 2)
            {
                PRINT_ERROR("\033[1;31mInvalid Username/ID\033[1;33m");
                connectinfo->requestcode = 0;
                connectinfo->responsecode = 0;
                askID = true;
            }
            else if (connectinfo->responsecode == 3)
            {
                PRINT_INFO("\033[1;31mServer is full, Try again later\033[1;33m");
                connectinfo->requestcode = 0;
                connectinfo->responsecode = 0;
                askID = true;
            }
            pthread_mutex_unlock(&connectinfo->mutex);
            printf("\033[1;0m");
        }
        else if (access == 1)
        {
            int comm_shmid = 0;
            if ((comm_shmid = shmget(client_ID, sizeof(shared_data_t), 0)) == -1)
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
            if(pthread_create(&thread, NULL, checkServerConnection, (void *)&client_ID))
            {
                PRINT_ERROR("\033[1;31mThread Creation for server status\033[1;0m");
            }
            while(1)
            {
                printf("\n");
                PRINT_INFO("Welcome, Kindly select one of the option.");
                PRINT_INFO("\033[1;36m1.\033[1;0m Add Two Numbers.");
                PRINT_INFO("\033[1;36m2.\033[1;0m Subtract Two Numbers.");
                PRINT_INFO("\033[1;36m3.\033[1;0m Multiply Two Numbers.");
                PRINT_INFO("\033[1;36m4.\033[1;0m Divide Two Numbers.");
                PRINT_INFO("\033[1;36m5.\033[1;0m Check If A Number is Prime.");
                PRINT_INFO("\033[1;36m6.\033[1;0m Check If A Number is Odd or Even.");
                PRINT_INFO("\033[1;36m7.\033[1;0m Disconnect from the server and exit.");
                PRINT_INFO("\033[1;36m8.\033[1;0m Unregister from the server and exit.");
                PRINT_INFO("\033[1;35mEnter an Option: ")
                scanf("%d",&choice);
                printf("\033[1;0m");
                switch(choice)
                {
                    case 1: PRINT_INFO("\033[1;36mEnter First Number!");
                            while(scanf("%f",&num1) != 1){ flushInput(); }
                            printf("\033[1;0m");
                            PRINT_INFO("\033[1;36mEnter Second Number!")
                            while(scanf("%f",&num2) != 1){ flushInput(); }
                            printf("\033[1;0m");
                            param = 1;
                            strcpy(op,"arithmetic");
                            break;

                    case 2: PRINT_INFO("\033[1;36mEnter First Number!");
                            while(scanf("%f",&num1) != 1){ flushInput(); }
                            printf("\033[1;0m");
                            PRINT_INFO("\033[1;36mEnter Second Number!")
                            while(scanf("%f",&num2) != 1){ flushInput(); }
                            printf("\033[1;0m");
                            param = 2;
                            strcpy(op,"arithmetic");
                            break;

                    case 3: PRINT_INFO("\033[1;36mEnter First Number!");
                            while(scanf("%f",&num1) != 1){ flushInput(); }
                            printf("\033[1;0m");
                            PRINT_INFO("\033[1;36mEnter Second Number!")
                            while(scanf("%f",&num2) != 1){ flushInput(); }
                            printf("\033[1;0m");
                            param = 3;
                            strcpy(op,"arithmetic");
                            break;

                    case 4: PRINT_INFO("\033[1;36mEnter First Number!");
                            while(scanf("%f",&num1) != 1){ flushInput(); }
                            printf("\033[1;0m");
                            PRINT_INFO("\033[1;36mEnter Second Number!")
                            while(scanf("%f",&num2) != 1){ flushInput(); }
                            printf("\033[1;0m");
                            param = 4;
                            strcpy(op,"arithmetic");
                            break;
                    
                    case 5: PRINT_INFO("\033[1;36mEnter The Number");
                            while(scanf("%f",&num1) != 1){ flushInput(); }
                            printf("\033[1;0m");
                            strcpy(op,"prime");
                            break;

                    
                    case 6: PRINT_INFO("\033[1;36mEnter The Number");
                            while(scanf("%f",&num1) != 1){ flushInput(); }
                            printf("\033[1;0m");
                            strcpy(op,"oddeven");
                            break;
                    case 7: PRINT_INFO("\033[1;36mDisconnecting");
                            strcpy(op,"disconnect");
                            break;

                    case 8: PRINT_INFO("\033[1;36mUnregistering");
                            strcpy(op,"unregister");
                            break;
                    default:
                            PRINT_ERROR("\033[1;31mEnter a valid Choice");
                            break;
                }
                flushInput();
                if ((comm_shmid = shmget(client_ID, sizeof(shared_data_t), 0)) == -1)
                {
                    PRINT_ERROR("\033[1;31mServer is not available, Server has to be started first\033[1;0m");
                    exit(1);
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
                            exited = 1;
                            break;
                    case 8:
                            PRINT_INFO("\033[1;32mUnRegistered successfully");
                            access = 0;
                            exited = 1;
                            break;
                    }
                printf("\033[1;0m");
                pthread_mutex_unlock(&(data->mutex));
                if(exited == 1)
                    break;
            }
        }
        if(exited == 1)
            break;
    }
    PRINT_INFO("\033[1;31mExiting Client!\033[1;0m");
}