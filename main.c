/*

    CLIENT SERVER CHAT APPLICATION IN C

*/


#include <stdio.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <stdbool.h>
#include <pthread.h>


#define MAX_PENDING 32 //Maximum Number of Clients

#define MAX_BUFFER 256 //Maximum length of a message

#define SERVER_IP "172.26.24.130"

#define PORT 2002

void *receiveThreadClient(void *arg);
void *receiveThreadServer(void *args);

int socketId = 0;
int socket_of_client = 0;

//To be passed as an arguments for the various threads
struct threadArgs
{
    int cltsock;
};

struct client
{
    int id;
    char name[25];
    struct sockaddr_in address_info;
};

struct client clients[MAX_PENDING] = {};
int numOfClients = 0;
char buffer[MAX_BUFFER];

//Creates a new Socket
void createSocket()
{
    socketId = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (socketId < 0)
    {
        printf("Creation of socket failed\n");
        exit(1);
    }


}

//Binds Server socket onto a port
void bindSocket()
{
    struct sockaddr_in addrport;
    addrport.sin_family = AF_INET;
    addrport.sin_port = htons(PORT);
    addrport.sin_addr.s_addr = htonl(INADDR_ANY);

    int result = bind(socketId, (struct sockaddr *)&addrport, sizeof(addrport));
    if (result == -1)
    {
        printf("Bind unsuccessful\n");
        exit(1);
    }

    
}

//Listens for incoming connection request
void listenSocket()
{
    int result = listen(socketId, MAX_PENDING);

    if (result < 0)
    {
        printf("Listen failed\n");
        exit(1);
    }

    printf("Server is Listening For connections...\n");
}

//accept incoming request
void acceptConnection()
{
    struct client newClient;
    struct sockaddr_in clientAddr;
    socklen_t sizeOfClientAddr = sizeof(clientAddr);
    int res = accept(socketId, (struct sockaddr *)&clientAddr, &sizeOfClientAddr);

    if (res < 0)
    {
        printf("Accept failed\n");
        exit(1);
    }
    socket_of_client = res;
    newClient.id = res;
    clients[numOfClients] = newClient;
    numOfClients++;
    
    puts("Server accepted a client with IP address: ");
    puts(inet_ntoa(clientAddr.sin_addr));
}

//Connect to the server with the client
void connectToServer()
{
    struct sockaddr_in serverAddr;
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_addr.s_addr = inet_addr(SERVER_IP);
    serverAddr.sin_port = htons(PORT);

    int result = connect(socketId, (struct sockaddr *)&serverAddr, sizeof(serverAddr));
    if (result < 0)
    {
        printf("Connect to server unsuccessful\n");
        exit(1);
    }

    printf("Connected to server successfully\n");
}


void sendMessage(char *message, int id)
{

    int result = send(id, message, strlen(message), 0);

    if (result != strlen(message))
    {
        printf("Sending message '%s' failed\n", message);
        exit(1);
    }
}

// Send Message from a client to all other clients
void broadcastMessage(char *message, int id)
{

    for (int i = 0; i < numOfClients; i++)
    {
        if (clients[i].id != id)
            sendMessage(message, clients[i].id);
    }
}

//Send Message to user about the other users on the Network and their Ids
void broadcastUsers()
{
    for (int i = 0; i < numOfClients; i++)
    {
        char message[500];
        sprintf(message, "%d: %s \n", i, clients[i].name);
        for (int j = 0; j < numOfClients; j++)
        {
            if (j != i)
                sendMessage(message, clients[j].id);
        }
    }
}

void receiveMessage(int id)
{

    int msgSize = recv(id, buffer, MAX_BUFFER, 0);

    if (msgSize < 0)
    {
        printf("Receive failed\n");
        exit(1);
    }
    puts("\nReceived message: ");
    puts(buffer);
}

void closeCommunication()
{
    close(socketId);
    printf("Communication Closed");
}

void clientSubroutine()
{
    int choice = 0;
    int id;

    char message[128];
    char sid[200];
    
    do
    {
        puts("\n##### CLIENT #####\n");
        puts("1. Send Message to Single Client\n");
        puts("2. Send to all Clients\n");
        puts("3. Exit\n");
        puts("Choice: ");
        scanf("%d", &choice);
        switch (choice)
        {
        case 1:
            puts("Enter the users ID: ");
            scanf("%d", &id);
            sprintf(sid, "%d:", id);
            puts("Message: ");
            int c;
            // Used to clear the stdin buffer
            while ((c = getchar()) != '\n')
                ;
            scanf("%[^\n]%*c", message);
            strcat(sid, message);
            sendMessage(sid, socketId);
            sid[0] = '\0';
            message[0] = '\0';
            break;
        case 2:
            int q;
            // Used to clear the stdin buffer
            while ((q = getchar()) != '\n')
                ;
            puts("Enter Message: ");
            scanf("%[^\n]%*c", message);
            strcat(message, "~");
            sendMessage(message, socketId);
            break;
        case 3:
            closeCommunication();
            exit(0);
        default:
            break;
        }

    } while (1);
}

int main()
{
    pthread_t threads;
    struct threadArgs *args;
    char value[1];
    printf("Is this the server?");
    scanf("%c", value);
    if (strcmp(value, "t") == 0)
    {
        puts("\n##### SERVER #####\n");
        createSocket();
        bindSocket();
        listenSocket();

        for (;;)
        {
            acceptConnection();
            args = (struct threadArgs *)malloc(sizeof(struct threadArgs));
            args->cltsock = socket_of_client;
            pthread_create(&threads, NULL, receiveThreadServer, (void *)args);
        }
        closeCommunication();
    }
    else
    {
        char name[25];
        printf("Enter your name:");
        scanf("%s", name);
        //append an at for server to filter out names
        name[strlen(name)] = '@';
        createSocket();
        connectToServer();
        sendMessage(name, socketId);

        args = (struct threadArgs *)malloc(sizeof(struct threadArgs));
        args->cltsock = socketId;
        // Receive thread
        pthread_create(&threads, NULL, receiveThreadClient, (void *)args);
       
        // Main client routine
        clientSubroutine();
    }
    return 0;
}

// client thread to receive messages
void *receiveThreadClient(void *args)
{
    int clientSock = ((struct threadArgs *)args)->cltsock;
    
    for (;;)
    {
        receiveMessage(clientSock);
        buffer[0] = '\0';
    }
    return (NULL);
}

// server thread to receive client messages
void *receiveThreadServer(void *args)
{
    int clientSock = ((struct threadArgs *)args)->cltsock;
    char newString[2];
    // the id as a string
    char id_s[10];
    // the id as an int
    int id;
    int sender_id;
    // message without the id
    char newmessage[500];
    // client name
    char client_name[50];
    int i;
    
    for (;;)
    {

        receiveMessage(clientSock);
        sprintf(newString, "%c", buffer[strlen(buffer) - 1]);
        
        if (strcmp(newString, "@") == 0)
        {
         buffer[strlen(buffer) - 1] = '\0';
            strcpy(clients[numOfClients - 1].name, buffer);
            buffer[0] = '\0';
            broadcastUsers();
        }
        else if (strcmp(newString, "~") == 0)
        {
            broadcastMessage(buffer, clientSock);
        }
        else
        {
            for (i = 0; buffer[i] != ':'; i++)
            {
                id_s[i] = buffer[i];
            }
            int k=0;
            for (int j = i + 1 ; buffer[j] != '\0'; j++)
            {
                newmessage[k] = buffer[j];
                k++;
            }
            k=0;

            //to get the id of the sender
            for (int i = 0; i < numOfClients; i++)
            {
                if (clients[i].id == clientSock)
                {
                    sender_id = i;
                    break;
                }
            }

            id = atoi(id_s);
            sprintf(client_name, "\nFrom: %s\n", clients[sender_id].name);
            strcat(newmessage, client_name);
            sendMessage(newmessage, clients[id].id);
        }
    }
    return (NULL);
}

