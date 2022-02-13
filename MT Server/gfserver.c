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
    int clientSocket; //use to store incoming connection information
    struct sockaddr_storage clientAddr; 
    socklen_t clientAddrSize; 
    int fileSize;
};

gfserver_t* gfserver_create(){
       
    gfserver_t* gfs;
    if ((gfs =  (gfserver_t*)malloc(sizeof(gfserver_t))) != NULL)
    {
        //printf("[SERVER] Successful allocate memory for gfs.\n");
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
        free((void *)*gfs);
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
        free((void *)*gfs);
        exit(1);
    }
    
    if(listen(serverSocket, (*gfs)->maxpending) == -1)
    {
        printf("[SERVER] Failed to listen(): %s\n", strerror(errno));
        free((void *)*gfs);
        exit(1);
    }
    //printf("[SERVER] Started and listening for incoming connection...\n");
    freeaddrinfo(servinfo); //free up servinfo struct 
    
    
   

    while (1)
    {
        char message[MAX_REQUEST_LEN]; //char string to hold client message
        char header[MAX_REQUEST_LEN]; //char string to hold header for validationi   
        memset(message, 0 , MAX_REQUEST_LEN);
        memset(header, 0 , MAX_REQUEST_LEN);
        memset(buffer,0,MAX_REQUEST_LEN);

        struct gfcontext_t *ctx;
        ctx = (gfcontext_t*)malloc(sizeof(struct gfcontext_t));

        ctx->clientAddrSize = sizeof(ctx->clientAddr);
        if ((ctx->clientSocket = accept(serverSocket,(struct sockaddr*)&(ctx->clientAddr),&(ctx->clientAddrSize)))== -1)
        {
            printf("[SERVER] accept() error: %s\n", strerror(errno));
        }
        //printf("[SERVER] Accepted connection from client...\n");
        
        
        //call recvHeader 
        do
        {
            if ((recvByte = recv(ctx->clientSocket,buffer,MAX_REQUEST_LEN-1,0)) == -1)
            {
                printf("[SERVER] Failed to receive from client socket: %s\n", strerror(errno));
            }
            buffer[recvByte] = '\0'; //In case client request header in chunk
            strcat(message,buffer);

        } while (strstr(message,MARKER) == NULL);    
        
            
        strncpy(header,message,strstr(message,MARKER)-message);
        //printf("[SERVER] Received [CLIENT]'s header: %s\n",header);

        //Check Header
        const char *scheme_c = strtok(header," ");
        const char *method_c = strtok(NULL," ");
        const char *path_c = strtok(NULL," ");
        
        if (strcmp(scheme_c,SCHEME) == 0 && strcmp(method_c,METHOD) == 0 && strncmp(path_c,"/",1) == 0)
        {
            //printf("[SERVER] Header request is valid! Sending [PATH] to handler now.\n");
            (*gfs)->handler(&ctx,path_c,(*gfs)->handlerarg);
        }
        else
        {
            printf("[SERVER] Header request is invalid! Sending [GF_INVALID] back to client.\n");
            gfs_sendheader(&ctx,GF_INVALID,0);
        }
            
        free(ctx);

    }
    
    free((void *)*gfs);

}


ssize_t gfs_sendheader(gfcontext_t **ctx, gfstatus_t status, size_t file_len){
    // not yet implemented
    int sendByte;
    char responseHeader[MAX_REQUEST_LEN];
    memset(responseHeader,0,MAX_REQUEST_LEN);
    (*ctx)->fileSize = file_len;
    //Logic for sending response header base on return value of status
    switch (status)
    {
    case 200: ; //status = OK
        char lengthInChar[20];
        memset(lengthInChar,0,20);
        sprintf(lengthInChar,"%ld",file_len); //changing length from int to string
        strcpy(responseHeader,SCHEME);
        strcat(responseHeader," OK ");
        strcat(responseHeader,lengthInChar);
        strcat(responseHeader,MARKER); //<scheme> <status> <length>\r\n\r\n
        //printf("[SERVER] Response header sent: %s\n",responseHeader);
        sendByte = send((*ctx)->clientSocket, responseHeader, strlen(responseHeader),0);
        //printf("SERVER SENT %d Number of Byte",sendByte);
        return sendByte;
    case 400: //status = FILE_NOT_FOUND
        strcpy(responseHeader,SCHEME);
        strcat(responseHeader," FILE_NOT_FOUND");
        
        strcat(responseHeader,MARKER); //<scheme> <status>\r\n\r\n
        sendByte = send((*ctx)->clientSocket, responseHeader, strlen(responseHeader),0);
        return sendByte;
    case 500: ; //status = ERROR
        strcpy(responseHeader,SCHEME);
        strcat(responseHeader," ERROR");
        strcat(responseHeader,MARKER); //<scheme> <status>\r\n\r\n
        //printf("[SERVER] Response header sent: %s\n",responseHeader);
        sendByte = send((*ctx)->clientSocket, responseHeader, strlen(responseHeader),0);
        return sendByte;   
    case 600: ; //status = INVALID
        strcpy(responseHeader,SCHEME);
        strcat(responseHeader," INVALID");
        strcat(responseHeader,MARKER); //<scheme> <status>\r\n\r\n
        //printf("[SERVER] Response header sent: %s\n",responseHeader);
        sendByte = send((*ctx)->clientSocket, responseHeader, strlen(responseHeader),0);
        return sendByte;   
    }
     
    return -1;
}

/**
 * @brief gfs_send functions is to be called from within a callback 
 * registered with gfserver_set_handler function
 * 
 * @param ctx client socketfd for server to send data to
 * @param data content to be sent to client 
 * @param len length of content
 * @return ssize_t 
 */
ssize_t gfs_send(gfcontext_t **ctx, const void *data, size_t len){
   
    size_t sendBytes;
    sendBytes = send((*ctx)->clientSocket,data,len,0);
    if (sendBytes == -1)
    {
        printf("[SERVER] Failed to send data to client: %s\n", strerror(errno));
        return -1;
    }
    return sendBytes;
}

void gfs_abort(gfcontext_t **ctx){
    close((*ctx)->clientSocket);
    free((void*)*ctx); //cast cast ctx as void pointer and free it
}

