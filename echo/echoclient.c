#include <unistd.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <getopt.h>
#include <netdb.h>
#include <netinet/in.h>

// A buffer large enough to contain the longest allowed string 
#define BUFSIZE 256

#define MAXBUFFER 16

#define USAGE                                                          \
  "usage:\n"                                                           \
  "  echoclient [options]\n"                                           \
  "options:\n"                                                         \
  "  -s                  Server (Default: localhost)\n"                \
  "  -p                  Port (Default: 10823)\n"                      \
  "  -m                  Message to send to server (Default: \"Hello " \
  "Summer.\")\n"                                                       \
  "  -h                  Show this help message\n"

/* OPTIONS DESCRIPTOR ====================================================== */
static struct option gLongOptions[] = {
    {"server", required_argument, NULL, 's'},
    {"port", required_argument, NULL, 'p'},
    {"message", required_argument, NULL, 'm'},
    {"help", no_argument, NULL, 'h'},
    {NULL, 0, NULL, 0}};

/* Main ========================================================= */
int main(int argc, char **argv) {
  unsigned short portno = 10823;
  int option_char = 0;
  char *message = "Hello Summer!!";
  char *hostname = "localhost";

  // Parse and set command line arguments
  while ((option_char =
              getopt_long(argc, argv, "p:s:m:hx", gLongOptions, NULL)) != -1) {
    switch (option_char) {
      default:
        fprintf(stderr, "%s", USAGE);
        exit(1);
      case 's':  // server
        hostname = optarg;
        break;
      case 'p':  // listen-port
        portno = atoi(optarg);
        break;
      case 'h':  // help
        fprintf(stdout, "%s", USAGE);
        exit(0);
        break;
      case 'm':  // message
        message = optarg;
        break;
    }
  }

  setbuf(stdout, NULL);  // disable buffering

  if ((portno < 1025) || (portno > 65535)) {
    fprintf(stderr, "%s @ %d: invalid port number (%d)\n", __FILE__, __LINE__,
            portno);
    exit(1);
  }

  if (NULL == message) {
    fprintf(stderr, "%s @ %d: invalid message\n", __FILE__, __LINE__);
    exit(1);
  }

  if (NULL == hostname) {
    fprintf(stderr, "%s @ %d: invalid host name\n", __FILE__, __LINE__);
    exit(1);
  }

  /* Socket Code Here */

  char portNoChar[6];       //store port number in char
  char buffer[MAXBUFFER];   //allocate max buffer size for send()/recv()
  int serverSocket ; //serverSocket is socket descriptor for client to connet to 
  struct addrinfo hints, *servinfo, *p; //struct addrinfo to hold revelant information regarding socket address
  int status, messageLen, sendByte, recvByte; //status is used to stored return getaddrinfo, sendByte stored the actual number of byte send
                                  //recvByte store the number bytes receives

  sprintf(portNoChar, "%d", portno);

  memset(&hints,0,sizeof hints); //make sure that hints is empty
  hints.ai_family = AF_UNSPEC;  //Doesn't care whether IPv4 or IPv6
  hints.ai_socktype = SOCK_STREAM; //set to stream socket

  if ((status = getaddrinfo(hostname,portNoChar,&hints,&servinfo)) != 0) 
  {
    fprintf(stderr,"[CLIENT] Failed to get address information: %s\n",gai_strerror(status));
    exit(1);
  }

  //loop through servinfo until connect() success
  for (p = servinfo; p != NULL; p = p->ai_next)
  {
    //calling socket() to get socket descriptor file
    if ((serverSocket = socket(p->ai_family,p->ai_socktype,p->ai_protocol)) == -1)
    {
      printf("Socket error: %s\n", strerror(errno)); //if socket() return -1, print out error message
      continue;
    }

    if (connect(serverSocket,p->ai_addr,p->ai_addrlen) == -1)
    {
      printf("[CLIENT] Failure to connect(): %s\n", strerror(errno)); //if connect() return -1, print out error message
      continue;
    }
    
    break;
    
  }

  freeaddrinfo(&servinfo); //free up servinfo struct 
  
  messageLen = strlen(message);

  //send message to the connect()ing serverSocket
  if ((sendByte = send(serverSocket,message,messageLen,0)) == -1)
  {
    printf("[CLIENT] Failure to send() message: %s\n", strerror(errno)); //print out error message if send() return -1
  }
  
  if (sendByte != buffer)
  {
    printf("[CLIENT] Message sent incomplete: %d of Bytes sent", sendByte);
  }
  
  //recv echo message from the connect()ing serverSocket

  if ((recvByte = recv(serverSocket,buffer,MAXBUFFER-1,0)) == -1)
  {
    printf("[CLIENT] Failed to receive response: %s\n", strerror(errno));
  }
  
  buffer[recvByte]= '\0'; //add null-terminated to message received

  printf("[CLIENT] Received Message: %s", buffer); //print out message echo back from server

}
