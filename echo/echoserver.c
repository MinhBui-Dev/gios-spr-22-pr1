#include <errno.h>
#include <netinet/in.h>
#include <sys/types.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <getopt.h>
#include <netdb.h>

#define BUFSIZE 256

#define MAXBUFFER 16

#define USAGE                                                        \
  "usage:\n"                                                         \
  "  echoserver [options]\n"                                         \
  "options:\n"                                                       \
  "  -p                  Port (Default: 10823)\n"                    \
  "  -m                  Maximum pending connections (default: 5)\n" \
  "  -h                  Show this help message\n"

/* OPTIONS DESCRIPTOR ====================================================== */
static struct option gLongOptions[] = {
    {"help", no_argument, NULL, 'h'},
    {"port", required_argument, NULL, 'p'},
    {"maxnpending", required_argument, NULL, 'm'},
    {NULL, 0, NULL, 0}};

int main(int argc, char **argv) {
  int maxnpending = 5;
  int option_char;
  int portno = 10823; /* port to listen on */

  // Parse and set command line arguments
  while ((option_char =
              getopt_long(argc, argv, "hx:m:p:", gLongOptions, NULL)) != -1) {
    switch (option_char) {
      case 'm':  // server
        maxnpending = atoi(optarg);
        break;
      case 'p':  // listen-port
        portno = atoi(optarg);
        break;
      case 'h':  // help
        fprintf(stdout, "%s ", USAGE);
        exit(0);
        break;
      default:
        fprintf(stderr, "%s ", USAGE);
        exit(1);
    }
  }

  setbuf(stdout, NULL);  // disable buffering

  if ((portno < 1025) || (portno > 65535)) {
    fprintf(stderr, "%s @ %d: invalid port number (%d)\n", __FILE__, __LINE__,
            portno);
    exit(1);
  }
  if (maxnpending < 1) {
    fprintf(stderr, "%s @ %d: invalid pending count (%d)\n", __FILE__, __LINE__,
            maxnpending);
    exit(1);
  }

  // Socket Code Here
  char portNoChar[6]; //store port number in string
  char buffer[MAXBUFFER]; //allocate max buffer size for recv()/send()
  int serverSocket, clientSocket; //serverSocket to listen() for incoming connection
                                  //clientSocket to recv()/send() to client
  struct addrinfo hints, *servinfo,*p; //struc addrinfo to hold address info
  int status, recvByte, sendByte; //to store return from send()/recv()
  int yes = 1; //set option value for setsockopt 
  struct sockaddr_storage clientAddr; //use to store incoming connection information
  socklen_t clientAddrSize; 


  sprintf(portNoChar,"%d",portno); //convert port number from int to string

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
  
  if(listen(serverSocket, maxnpending) == -1)
  {
    printf("[SERVER] Failed to listen(): %s\n", strerror(errno));
    exit(1);
  }
  
  freeaddrinfo(servinfo); //free up servinfo struct 
  
  while(1)
  {
    //create a new seperate socket file descriptor for incoming connection using accept()
    clientAddrSize = sizeof(clientAddr);
    if ((clientSocket = accept(serverSocket,(struct sockaddr*)&clientAddr,&clientAddrSize)) == -1)
    {
      printf("[SERVER] accept() error: %s\n", strerror(errno));
    }

    if ((recvByte = recv(clientSocket,buffer,MAXBUFFER -1,0)) == -1)
    {
      printf("[SERVER] Failed to receive from client socket: %s\n", strerror(errno));
    }
    
    buffer[recvByte] = '\0';
    printf("Max buffer %d and Byte received %d", MAXBUFFER,recvByte);
    printf("[SERVER] Message received from Client: %s\n", buffer);


    sendByte = send(clientSocket, buffer, MAXBUFFER,0);
    /*if ((sendByte = send(clientSocket, buffer, MAXBUFFER,0))== -1 )
    {
      printf("[SERVER] Failed to send message to client socket: %s", strerror(errno));
    }
    */
    
    if (sendByte != (MAXBUFFER))
    {
      printf("Message sent incomplete: %d number of bytes sent", sendByte);
    }
    
    printf("Server send byte %d", sendByte);
    
  }

  

}
