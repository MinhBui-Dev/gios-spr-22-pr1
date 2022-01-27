#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <getopt.h>
#include <netdb.h>
#include <netinet/in.h>
#include <unistd.h>
#include <fcntl.h>

#define BUFSIZE 512

#define USAGE                                                \
  "usage:\n"                                                 \
  "  transferclient [options]\n"                             \
  "options:\n"                                               \
  "  -h                  Show this help message\n"           \
  "  -p                  Port (Default: 10823)\n"            \
  "  -s                  Server (Default: localhost)\n"      \
  "  -o                  Output file (Default cs6200.txt)\n" 

/* OPTIONS DESCRIPTOR ====================================================== */
static struct option gLongOptions[] = {{"server", required_argument, NULL, 's'},
                                       {"output", required_argument, NULL, 'o'},
                                       {"port", required_argument, NULL, 'p'},
                                       {"help", no_argument, NULL, 'h'},
                                       {NULL, 0, NULL, 0}};

/* Main ========================================================= */
int main(int argc, char **argv) {
  unsigned short portno = 10666;
  int option_char = 0;
  char *hostname = "localhost";
  char *filename = "cs6200.txt";

  setbuf(stdout, NULL);

  /* Parse and set command line arguments */ 
  while ((option_char =
              getopt_long(argc, argv, "xp:s:h:o:", gLongOptions, NULL)) != -1) {
    switch (option_char) {
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
      default:
        fprintf(stderr, "%s", USAGE);
        exit(1);
      case 'o':  // filename
        filename = optarg;
        break;
    }
  }

  if (NULL == hostname) {
    fprintf(stderr, "%s @ %d: invalid host name\n", __FILE__, __LINE__);
    exit(1);
  }

  if (NULL == filename) {
    fprintf(stderr, "%s @ %d: invalid filename\n", __FILE__, __LINE__);
    exit(1);
  }

  if ((portno < 1025) || (portno > 65535)) {
    fprintf(stderr, "%s @ %d: invalid port number (%d)\n", __FILE__, __LINE__,
            portno);
    exit(1);
  }

  // Socket Code Here 
  char portNoChar[6];       //store port number in char
  char buffer[BUFSIZE];   //allocate BUFSIZE of 512 to buffer for recv()
  int serverSocket ; //serverSocket is socket descriptor for client to connet to 
  struct addrinfo hints, *servinfo, *p; //struct addrinfo to hold revelant information regarding socket address
  int status, recvByte; //status is used to stored return getaddrinfo, recvByte store the number bytes receives
  int writeFile,writeByte; //File descriptor for writing content from buffer

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

  if (p == NULL)
  {
    printf("[CLIENT] Failed to connect to server address!\n");
    exit(1);
  }

  freeaddrinfo(servinfo); //free up servinfo struct 
  
  writeFile = open(filename,O_WRONLY |O_APPEND|O_CREAT,00200 ); //open or create new file with write-only and apprend mode
  do
  {
    memset(buffer,0,BUFSIZE); //clear buffer memory

    recvByte =  recv(serverSocket, buffer, BUFSIZE,0); //call recv() to receive data from serverSocket
    if (recvByte == -1)
    {
      printf("[CLIENT] Failed to receive: %s\n", strerror(errno));
      exit(1);
    }

    //buffer[recvByte] = '\0';

    writeByte = write(writeFile,buffer,recvByte);
    
    printf("Received %d, Wroted %d\n", recvByte, writeByte);


  } while (recvByte != 0); //recvByte return 0 for close connection, signifying transfer complete. 

  close(writeFile); //close file descriptor

  close(serverSocket); //close server descriptor

  return 0;

}
