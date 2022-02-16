#include <sys/socket.h>
#include <sys/types.h>
#include <errno.h>
#include <getopt.h>
#include <netdb.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>


#define BUFSIZE 512
#define MAXPENDING 5
#define USAGE                                            \
  "usage:\n"                                             \
  "  transferserver [options]\n"                         \
  "options:\n"                                           \
  "  -h                  Show this help message\n"       \
  "  -f                  Filename (Default: 6200.txt)\n" \
  "  -p                  Port (Default: 10823)\n"

/* OPTIONS DESCRIPTOR ====================================================== */
static struct option gLongOptions[] = {
    {"help", no_argument, NULL, 'h'},
    {"port", required_argument, NULL, 'p'},
    {"filename", required_argument, NULL, 'f'},
    {NULL, 0, NULL, 0}};

int main(int argc, char **argv) {
  char *filename = "CS6200.txt"; // file to transfer 
  int portno = 10666;          // port to listen on 
  int option_char;

  setbuf(stdout, NULL);  // disable buffering

  // Parse and set command line arguments
  while ((option_char =
              getopt_long(argc, argv, "hf:xp:", gLongOptions, NULL)) != -1) {
    switch (option_char) {
      case 'p':  // listen-port
        portno = atoi(optarg);
        break;
      case 'h':  // help
        fprintf(stdout, "%s", USAGE);
        exit(0);
        break;
      default:
        fprintf(stderr, "%s", USAGE);
        exit(1);
      case 'f':  // file to transfer
        filename = optarg;
        break;
    }
  }

  if ((portno < 1025) || (portno > 65535)) {
    fprintf(stderr, "%s @ %d: invalid port number (%d)\n", __FILE__, __LINE__,
            portno);
    exit(1);
  }

  if (NULL == filename) {
    fprintf(stderr, "%s @ %d: invalid filename\n", __FILE__, __LINE__);
    exit(1);
  }

  /* Socket Code Here */
  char portNoChar[6]; //store port number in string
  char buffer[BUFSIZE]; //allocate max buffer size for recv()/send()
  int serverSocket, clientSocket; //serverSocket to listen() for incoming connection
                                  //clientSocket to send() to client
  struct addrinfo hints, *servinfo,*p; //struc addrinfo to hold address info
  int status, sendByte, totalSend, byteLeft; //to store return from send()
  int yes = 1; //set option value for setsockopt 
  struct sockaddr_storage clientAddr; //use to store incoming connection information
  socklen_t clientAddrSize; 
  int readFile, readByte; //file descriptor to read from file

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
    if ((bind(serverSocket,p->ai_addr,p->ai_addrlen)) < 0)
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
  
  if((status = listen(serverSocket, MAXPENDING)) == -1)
  {
    printf("[SERVER] Failed to listen(): %s\n", strerror(errno));
    exit(1);
  }
  
  freeaddrinfo(servinfo); //free up servinfo struct 

  printf("Server listening....%d\n",status);

  while(1)
  {
    //create a new seperate socket file descriptor for incoming connection using accept()
    clientAddrSize = sizeof(clientAddr);
    if ((clientSocket = accept(serverSocket,(struct sockaddr*)&clientAddr,&clientAddrSize)) == -1)
    {
      printf("[SERVER] accept() error: %s\n", strerror(errno));
    }

    printf("Server accepted connection....\n");

    //create new file descriptor link to file in directory using open() and set flag to read-only
    readFile = open(filename,O_RDONLY,00400 );

    if(readFile == -1)
    {
      printf("[SERVER] Failed to open file: %s\n", strerror(errno));
      exit(1);
    }

    do
    {
      memset(buffer, 0 , BUFSIZE); //clear address memory
      readByte = read(readFile,buffer,BUFSIZE); //read from file and save to address memory pointed by buffer
      printf("[SERVER]: Read %d bytes.", readByte);

      if (readByte < 0)
      {
        printf("[SERVER] Failed to read from file: %s\n", strerror(errno));
        exit(1);
      }
      
      //call send() to send data read in buffer to client socket
      totalSend = 0;
      byteLeft = readByte;
      while (totalSend < readByte)
      {
        if ((sendByte = send(clientSocket, buffer+totalSend, byteLeft,0)) == -1 )
        {
          printf("[SERVER] Failed to send message to client socket: %s", strerror(errno));
        }
        printf("[SERVER]: Sent %d bytes\n", sendByte);
        totalSend += sendByte;
        byteLeft -= sendByte;
      }     

    } while (readByte != 0); //stop after read() reaches eof - return 0

    close(clientSocket); //close connection to client after finish transfering
    
  }
  
}
