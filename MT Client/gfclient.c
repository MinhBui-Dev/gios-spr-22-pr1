
#include <stdlib.h>

#include "gfclient-student.h"



 // Modify this file to implement the interface specified in
 // gfclient.h.
struct gfcrequest_t
{
  //server and directory info
  unsigned short port;
  const char *server, *path;

  //callback function
  void *headerarg;
  void (*headerfunc)(void *header_buffer, size_t header_buffer_length, void *handlerarg);
  void *writearg;
  void (*writefunc)(void *data_buffer, size_t data_buffer_length, void *handlerarg);
  
  //status and data received from server response
  int bytesReceived, fileLen;
  gfstatus_t status;
};
// optional function for cleaup processing.
void gfc_cleanup(gfcrequest_t **gfr) {
  free((void*)*gfr);
  *gfr = NULL;
}

gfcrequest_t *gfc_create() {
  // dummy for now - need to fill this part in
  gfcrequest_t* gfr;
    if ((gfr =  (gfcrequest_t*)malloc(sizeof(gfcrequest_t))) != NULL)
    {
        //printf("[CLIENT] Successful allocate memory for gfs.\n");
        return gfr;
    }

  return (gfcrequest_t *)NULL;
}

size_t gfc_get_bytesreceived(gfcrequest_t **gfr) {
  if(*gfr != NULL)
  {
    return (*gfr)->bytesReceived;
  }
  return -1;
}

size_t gfc_get_filelen(gfcrequest_t **gfr) {
  if(*gfr != NULL)
  {
    return (*gfr)->fileLen;
  }
  return -1;
}

gfstatus_t gfc_get_status(gfcrequest_t **gfr) {
  if(*gfr != NULL)
  {
    return (*gfr)->status;
  }
  return -1;
}

void gfc_global_init() {}

void gfc_global_cleanup() {}

int gfc_perform(gfcrequest_t **gfr) {
  char portNoChar[6];       //store port number in char
  int serverSocket ; //serverSocket is socket descriptor for client to connet to 
  struct addrinfo hints, *servinfo, *p; //struct addrinfo to hold revelant information regarding socket address
  int status, recvByte, sendByte; //status is used to stored return getaddrinfo, recvByte store the number bytes receives
  //int writeFile,writeByte; //File descriptor for writing content from buffer

  sprintf(portNoChar, "%d", (*gfr)->port);

  memset(&hints,0,sizeof hints); //make sure that hints is empty
  hints.ai_family = AF_UNSPEC;  //Doesn't care whether IPv4 or IPv6
  hints.ai_socktype = SOCK_STREAM; //set to stream socket

  if ((status = getaddrinfo((*gfr)->server,portNoChar,&hints,&servinfo)) != 0) 
  {
    fprintf(stderr,"[CLIENT] Failed to get address information: %s\n",gai_strerror(status));
    free((void *)(*gfr));
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
    free((void *)(*gfr));
    exit(1);
  }

  freeaddrinfo(servinfo); //free up servinfo struct 

  


  //send request header
  int reqHeaderLen = strlen(SCHEME) + strlen(METHOD) + strlen((*gfr)->path) + strlen(MARKER) + 3;//2 spaces and null-terminator
  char *reqHeader = malloc(sizeof(char)*reqHeaderLen);
  memset(reqHeader,0,reqHeaderLen);
  sprintf(reqHeader,"%s %s %s%s",SCHEME,METHOD,(*gfr)->path,MARKER);
  printf("[CLIENT] Sending request header to server: %s",reqHeader);
  unsigned short totalSend = 0;
  do
  {
    sendByte = send(serverSocket,reqHeader,reqHeaderLen,0);
    totalSend += sendByte;
    //printf("Send byte %d and total send %d", sendByte, totalSend);
  } while (totalSend < reqHeaderLen);
  
  free((void*)reqHeader);

  //receive response header and data in block size of MAXBUFFER
  char *buffer = malloc(MAXBUFFER*sizeof(char)); //allocate memory for buffer to recv  
  char *message = malloc(MAXBUFFER*sizeof(char)); //allocate memory for buffer to recv
   
  memset(buffer,0,MAXBUFFER);
  memset(message,0,MAXBUFFER);
  int headerfound = 0;
  while (1)
  {
    if ((recvByte = recv(serverSocket,buffer,MAXBUFFER-1,0)) == -1)
    {
      printf("[CLIENT] Failed to receive from client socket: %s\n", strerror(errno));
      return -1;
    }

    //printf("Byte received from server %d \n", recvByte);

    buffer[recvByte] = '\0';
    strcat(message,buffer);
    if (strstr(message,MARKER) != NULL)
    {
      headerfound = 1;
      break;
    }

    if (recvByte == 0 && headerfound == 0)
    {
      printf("[CLIENT] Failed to receive correct header.\n");
      (*gfr)->status = GF_INVALID;
      free((void*)message);
      free((void*)buffer);
      return -1;
    }
    
  }
  
  
  int resHeaderSize = strstr(message,MARKER)-message + sizeof(MARKER);
  char *resHeader = malloc(resHeaderSize*sizeof(char)); //char string to hold client message

  strncpy(resHeader,message,resHeaderSize-1);
  resHeader[resHeaderSize -1] = '\0';

  //printf("[CLIENT] Header found: %s & header size %d", resHeader, resHeaderSize);
  
  /*if ((*gfr)->headerfunc != NULL)
  {
    (*gfr)->headerfunc(resHeader,strlen(resHeader),NULL);
  }
  */
  free((void*)message);


  char scheme[SCHEMESIZE]; 
  char resStatus[STATUSMAXSIZE];
  
  sscanf(resHeader, "%s %s %d\r\n\r\n", scheme, resStatus, &((*gfr)->fileLen));

  free((void*)resHeader);

  //validate status from response header
  if (strcmp(resStatus, "OK") == 0)
  {
    (*gfr)->status = GF_OK;
  }
  else if (strcmp(resStatus, "FILE_NOT_FOUND") == 0)
  {
    (*gfr)->status = GF_FILE_NOT_FOUND;
    free((void*)buffer);
    return 0;
  }
  else if (strcmp(resStatus, "ERROR") == 0)
  {
    (*gfr)->status = GF_ERROR;
    free((void*)buffer);
    return 0;
  }
  else
  {
    (*gfr)->status = GF_INVALID;
    free((void*)buffer);
    return -1;
  }

  

  (*gfr)->bytesReceived = recvByte - resHeaderSize +1;
 
  //printf("Byte receiced %d, File Len %d", (*gfr)->bytesReceived, (*gfr)->fileLen);
  //if there is some content from the first header, send to write function
  if ((*gfr)->bytesReceived > 0)
  {
    (*gfr)->writefunc(buffer +resHeaderSize - 1,recvByte-resHeaderSize-1,(*gfr)->writearg);
  }
  
  
  
  
  while ((*gfr)->bytesReceived < (*gfr)->fileLen)
  { 
    memset(buffer,0,MAXBUFFER);
    if ((recvByte = recv(serverSocket,buffer,MAXBUFFER-1,0)) == -1)
    {
      printf("[CLIENT] Failed to receive from server socket: %s\n", strerror(errno));
      return - 1;
    }
    
    buffer[recvByte] = '\0';
    (*gfr)->bytesReceived += recvByte;

    //printf("Byte received from server %d, Total Byte %d \n", recvByte, (*gfr)->bytesReceived);
    if (recvByte == 0 && (*gfr)->bytesReceived < (*gfr)->fileLen)
    {
      printf("[CLIENT] Failed to receive full data.\n");
      (*gfr)->status = GF_OK;
      free((void*)buffer);
      return -1;
    }
    
    (*gfr)->writefunc(buffer,recvByte,(*gfr)->writearg);
  }
  free((void*)buffer);
  close(serverSocket);
  return 0;
}

void gfc_set_headerarg(gfcrequest_t **gfr, void *headerarg) {
  (*gfr)->headerarg = headerarg;
}

void gfc_set_headerfunc(gfcrequest_t **gfr, void (*headerfunc)(void *, size_t, void *)) {
  (*gfr)->headerfunc = (*headerfunc);
}

void gfc_set_path(gfcrequest_t **gfr, const char *path) {
  (*gfr)->path = path;
}

void gfc_set_port(gfcrequest_t **gfr, unsigned short port) {
  (*gfr)->port = port;
}

void gfc_set_server(gfcrequest_t **gfr, const char *server) {
  (*gfr)->server = server;
}

void gfc_set_writearg(gfcrequest_t **gfr, void *writearg) {
  (*gfr)->writearg = writearg;
}

void gfc_set_writefunc(gfcrequest_t **gfr, void (*writefunc)(void *, size_t, void *)) {
  (*gfr)->writefunc = (*writefunc);
}

const char *gfc_strstatus(gfstatus_t status) {
  const char *strstatus = NULL;

  switch (status) {
    default: {
      strstatus = "UNKNOWN";
    } break;

    case GF_FILE_NOT_FOUND: {
      strstatus = "FILE_NOT_FOUND";
    } break;

    case GF_INVALID: {
      strstatus = "INVALID";
    } break;

    case GF_ERROR: {
      strstatus = "ERROR";
    } break;

    case GF_OK: {
      strstatus = "OK";
    } break;
  }

  return strstatus;
}
