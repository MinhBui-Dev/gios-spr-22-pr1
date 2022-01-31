#include <stdlib.h>
#include "gfserver-student.h"


// Modify this file to implement the interface specified in
 // gfserver.h.
struct gfserver_t
{
    int port;
    int maxpending;
    void* handlerarg;
    gfh_error_t (*handler)(gfcontext_t **, const char *, void*);
    
};

struct gfcontext_t
{
    int clientSocket;
    struct sockaddr_storage clientAddr; //use to store incoming connection information
    socklen_t clientAddrSize; 
};

gfserver_t* gfserver_create(){
       
    gfserver_t* gfs;
    if ((gfs =  (gfserver_t*)malloc(sizeof(gfserver_t))) != NULL)
    {
        printf("[SERVER] Successful allocate memory for gfs.\n");
        return gfs;
    }
    
    
    return (gfserver_t *)NULL;
}

void gfserver_set_handler(gfserver_t **gfs, gfh_error_t (*handler)(gfcontext_t **, const char *, void*)){
    (*gfs)->handler = (*handler);
}

void gfserver_set_port(gfserver_t **gfs, unsigned short port){
    (*gfs)->port = port;
}

void gfserver_set_maxpending(gfserver_t **gfs, int max_npending){
    (*gfs)->maxpending = max_npending;
}

void gfserver_set_handlerarg(gfserver_t **gfs, void* arg){
    (*gfs)->handlerarg = arg;
}

void gfserver_serve(gfserver_t **gfs){
    
    char portNoChar[6]; //store port number in string
    char buffer[MAX_REQUEST_LEN]; //allocate max buffer size for recv()/send()
    int serverSocket; //serverSocket to listen() for incoming connection
    struct addrinfo hints, *servinfo,*p; //struc addrinfo to hold address info
    int status, recvByte/*, sendByte*/; //to store return from send()/recv()
    int yes = 1; //set option value for setsockopt 


    sprintf(portNoChar,"%d",(*gfs)->port); //convert port number from int to string

    memset(&hints,0, sizeof hints); //make sure hints is emptty
    hints.ai_family = AF_UNSPEC; //Make compatible with both IPv4 and IPv6
    hints.ai_socktype = SOCK_STREAM; //set type to stream socket
    hints.ai_flags = AI_PASSIVE; //fill out IP automatically

    //call getaddrinfo() to fill out struc addrinfo pointed by servinfo
    if ((status = getaddrinfo(NULL,portNoChar,&hints,&servinfo))!= 0)
    {
        fprintf(stderr,"[SERVER] Failed to get address information: %s\n",gai_strerror(status));
        exit(1);
    }
    
    //loop through all result in pointed by servinfo until bind() successfully
    for (p = servinfo; p != NULL; p = p->ai_next)
    {
        //calling socket() to get socket file descriptor
        if ((serverSocket = socket(p->ai_family,p->ai_socktype,p->ai_protocol)) == -1)
        {
        printf("[SERVER] Socket error: %s\n", strerror(errno)); //if socket() return -1, print out error message
        continue;
        }

        //setsockopt to resuse address for bind()
        if ((setsockopt(serverSocket,SOL_SOCKET,SO_REUSEADDR,&yes,sizeof(yes)))==-1)
        {
        printf("[SERVER] Failed to reuse address: %s\n", strerror(errno));
        continue;
        }
        //bind() serverSocket to the portnumber stored inside linkedlist pointed by servinfo
        if ((bind(serverSocket,p->ai_addr,p->ai_addrlen))== -1)
        {
        printf("[SERVER] Failed to bind: %s\n", strerror(errno));
        continue;
        }
        break;   
    }
    
    
    //check to see if server successfully bind() to a port
    if (p == NULL)
    {
        printf("[SERVER] Failed to bind: %s\n", strerror(errno));
        exit(1);
    }
    
    if(listen(serverSocket, (*gfs)->maxpending) == -1)
    {
        printf("[SERVER] Failed to listen(): %s\n", strerror(errno));
        exit(1);
    }
    
    freeaddrinfo(servinfo); //free up servinfo struct 
    
    struct gfcontext_t *ctx = malloc(sizeof(struct gfcontext_t));
    char message[MAX_REQUEST_LEN]; //char string to hold client message
    char header[MAX_REQUEST_LEN]; //char string to hold header for validationi 
    
    char Marker[] = "\r\n\r\n";

    
    memset(message, 0 , MAX_REQUEST_LEN);
    memset(header, 0 , MAX_REQUEST_LEN);

    while (1)
    {
        ctx->clientAddrSize = sizeof(ctx->clientAddr);
        if ((ctx->clientSocket = accept(serverSocket,(struct sockaddr*)&(ctx->clientAddr),&(ctx->clientAddrSize)))== -1)
        {
            printf("[SERVER] accept() error: %s\n", strerror(errno));
        }
        
        //call recvHeader 
        do
        {
            if ((recvByte = recv(ctx->clientSocket,buffer,MAX_REQUEST_LEN,0)) == -1)
            {
            printf("[SERVER] Failed to receive from client socket: %s\n", strerror(errno));
            }
            buffer[recvByte] = '\0';
            strcat(message,buffer);

        } while (strstr(message,MARKER) == NULL);
        
        strncpy(header,message,strstr(message,Marker)-message);

        printf("[SERVER] Received [CLIENT]'s header: %s\n",header);

        //Check Header
        const char *scheme_c = strtok(header," ");
        const char *method_c = strtok(NULL," ");
        const char *path_c = strtok(NULL," ");
        if (strcmp(scheme_c,SCHEME) == 0 && strcmp(method_c,METHOD) == 0)
        {
            printf("[SERVER] Header request is valid!\nSending [PATH] to handler now.\n");
            gfs_handler(&ctx,path_c,NULL);
        }
        else
        {
            printf("[SERVER] Invalid header request.\n");
            //code sending message to client GF_INVALID
        }

    }
    

}


ssize_t gfs_sendheader(gfcontext_t **ctx, gfstatus_t status, size_t file_len){
    // not yet implemented
    int sendByte;
    char responseHeader[MAX_REQUEST_LEN];
    memset(responseHeader,0,MAX_REQUEST_LEN);

    switch (status)
    {
    case 200: ;
        char lengthInChar[20];
        memset(lengthInChar,0,20);
        sprintf(lengthInChar,"%ld",file_len);
        strcpy(responseHeader,SCHEME);
        strcat(responseHeader," ");
        strcat(responseHeader,"OK");
        strcat(responseHeader," ");
        strcat(responseHeader,lengthInChar);
        strcat(responseHeader,MARKER);
        printf("[SERVER] Responder header is: %s",responseHeader);
        sendByte = send((*ctx)->clientSocket, responseHeader, MAX_REQUEST_LEN,0);
        return sendByte;
        break;
    case 400:
        strcpy(responseHeader,SCHEME);
        strcat(responseHeader," ");
        strcat(responseHeader,"FILE_NOT_FOUND");
        strcat(responseHeader,MARKER);
        printf("[SERVER] Responder header is: %s",responseHeader);
        sendByte = send((*ctx)->clientSocket, responseHeader, MAX_REQUEST_LEN,0);
        return sendByte;
        break;
    case 500: ;
        strcpy(responseHeader,SCHEME);
        strcat(responseHeader," ");
        strcat(responseHeader,"OK");
        strcat(responseHeader,MARKER);
        printf("[SERVER] Responder header is: %s",responseHeader);
        sendByte = send((*ctx)->clientSocket, responseHeader, MAX_REQUEST_LEN,0);
        return sendByte;
        break;    
    }
     
    return -1;
}

ssize_t gfs_send(gfcontext_t **ctx, const void *data, size_t len){
    // not yet implemented
    return -1;
}

void gfs_abort(gfcontext_t **ctx){

}

