
#include <stdlib.h>
#include <pthread.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include "steque.h"

#include "gfserver-student.h"

#define USAGE                                                               \
  "usage:\n"                                                                \
  "  gfserver_main [options]\n"                                             \
  "options:\n"                                                              \
  "  -h                  Show this help message.\n"                         \
  "  -t [nthreads]       Number of threads (Default: 21)\n"                 \
  "  -m [content_file]   Content file mapping keys to content files\n"      \
  "  -p [listen_port]    Listen port (Default: 10823)\n"                    \
  "  -d [delay]          Delay in content_get, default 0, range 0-5000000 " \
  "(microseconds)\n "

/* OPTIONS DESCRIPTOR ====================================================== */
static struct option gLongOptions[] = {
    {"help", no_argument, NULL, 'h'},
    {"content", required_argument, NULL, 'm'},
    {"nthreads", required_argument, NULL, 't'},
    {"port", required_argument, NULL, 'p'},
    {"delay", required_argument, NULL, 'd'},
    {NULL, 0, NULL, 0}};

extern unsigned long int content_delay;

extern gfh_error_t gfs_handler(gfcontext_t **ctx, const char *path, void *arg);

pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;

pthread_cond_t work_cond = PTHREAD_COND_INITIALIZER;

steque_t *requestQueue;

void *gfs_transfer(void *arg);

static void _sig_handler(int signo) {
  if ((SIGINT == signo) || (SIGTERM == signo)) {
    exit(signo);
  }
}

/* Main ========================================================= */
int main(int argc, char **argv) {
  gfserver_t *gfs = NULL;
  int nthreads = 12;
  int option_char = 0;
  unsigned short port = 10823;
  char *content_map = "content.txt";

  setbuf(stdout, NULL);

  if (SIG_ERR == signal(SIGINT, _sig_handler)) {
    fprintf(stderr, "Can't catch SIGINT...exiting.\n");
    exit(EXIT_FAILURE);
  }

  if (SIG_ERR == signal(SIGTERM, _sig_handler)) {
    fprintf(stderr, "Can't catch SIGTERM...exiting.\n");
    exit(EXIT_FAILURE);
  }

  // Parse and set command line arguments
  while ((option_char = getopt_long(argc, argv, "p:d:rhm:t:", gLongOptions,
                                    NULL)) != -1) {
    switch (option_char) {
      case 't':  /* nthreads */ 
        nthreads = atoi(optarg);
        break;
      case 'p':  /* listen-port */
        port = atoi(optarg);
        break;
      case 'm':  /* file-path */ 
        content_map = optarg;
        break;
      default:
        fprintf(stderr, "%s", USAGE);
        exit(1);
      case 'd':  /* delay */
        content_delay = (unsigned long int)atoi(optarg);
        break;
      case 'h':  /* help */ 
        fprintf(stdout, "%s", USAGE);
        exit(0);
        break;
    }
  }

  /* not useful, but it ensures the initial code builds without warnings */
  if (nthreads < 1) {
    nthreads = 1;
  }

  if (content_delay > 5000000) {
    fprintf(stderr, "Content delay must be less than 5000000 (microseconds)\n");
    exit(__LINE__);
  }

  content_init(content_map);

  requestQueue = malloc(sizeof(steque_t));
  steque_init(requestQueue);

  /* Initialize thread management */
  pthread_t workerThread[nthreads];

  for (int i = 0; i < nthreads; i++)
  {
    if (pthread_create(&workerThread[i],NULL,gfs_transfer,NULL) != 0)
    {
      printf("Unable to create worker thread %d",i);
    }
    
  }
  
  /*Initializing server*/
  gfs = gfserver_create();

  //Setting options
  gfserver_set_port(&gfs, port);
  gfserver_set_maxpending(&gfs, 86);
  gfserver_set_handler(&gfs, gfs_handler);
  gfserver_set_handlerarg(&gfs, NULL);  // doesn't have to be NULL!

  /*Loops forever*/
  gfserver_serve(&gfs);
  free(requestQueue);
}

void *gfs_transfer(void *arg)
{
	while (1)
	{
		file_request *request;
		pthread_mutex_lock(&mutex);

		while(steque_isempty(requestQueue))
		{
			pthread_cond_wait(&work_cond,&mutex);
		}
		request = steque_pop(requestQueue);

    pthread_mutex_unlock(&mutex);

		int fileD;

		fileD = content_get(request->path);

		if (fileD == -1)
		{
			gfs_sendheader(&(request->ctx),GF_FILE_NOT_FOUND,0);
			exit(1);
		}

		struct stat fileStat;
    int fileLen;
    
    fstat(fileD,&fileStat);
    fileLen = fileStat.st_size;

    gfs_sendheader(&(request->ctx),GF_OK,fileLen);

    int readByte, SendByte, totalRead, totalSend;
    char buffer[BUFFERSIZE];

    totalRead = 0;
    
    while (totalRead < fileLen)
    {
      //memset(buffer,0,BUFFERSIZE);
      readByte = pread(fileD,buffer,BUFFERSIZE,totalRead);

      
      if (readByte < 0)
      {
        printf("[SERVER] Failed to read from file: %s\n", strerror(errno));
        exit(1);
      }
      totalSend = 0;
      while (totalSend < readByte)
      {
        SendByte = gfs_send(&(request->ctx),buffer,BUFFERSIZE);
        totalSend += SendByte;
      }

      totalRead += readByte;
      
    }
    printf("total send byte %d and file length %d\n",totalRead,fileLen);
    free(request);
    pthread_exit(NULL);
	}
}

