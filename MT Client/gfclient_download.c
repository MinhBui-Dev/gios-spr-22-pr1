#include <stdlib.h>

#include "gfclient-student.h"

#define MAX_THREADS (1024)
#define PATH_BUFFER_SIZE 1221

#define USAGE                                                    \
  "usage:\n"                                                     \
  "  webclient [options]\n"                                      \
  "options:\n"                                                   \
  "  -h                  Show this help message\n"               \
  "  -s [server_addr]    Server address (Default: 127.0.0.1)\n"  \
  "  -p [server_port]    Server port (Default: 10823)\n"         \
  "  -r [num_requests]   Request download total (Default: 21)\n" \
  "  -t [nthreads]       Number of threads (Default 12)\n"       \
  "  -w [workload_path]  Path to workload file (Default: workload.txt)\n"

/* OPTIONS DESCRIPTOR ====================================================== */
static struct option gLongOptions[] = {
    {"help", no_argument, NULL, 'h'},
    {"nrequests", required_argument, NULL, 'r'},
    {"server", required_argument, NULL, 's'},
    {"port", required_argument, NULL, 'p'},
    {"workload-path", required_argument, NULL, 'w'},
    {"nthreads", required_argument, NULL, 't'},
    {NULL, 0, NULL, 0}};

unsigned short port = 10823;
char *server = "localhost";
pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t work_cond = PTHREAD_COND_INITIALIZER;
steque_t *requestQueue;
int nrequests;
char *requestDone = "No More Request";

static void Usage() { fprintf(stderr, "%s", USAGE); }

void *gfc_request(void *arg);

static void localPath(char *req_path, char *local_path) {
  static int counter = 0;

  sprintf(local_path, "%s-%06d", &req_path[1], counter++);
}

static FILE *openFile(char *path) {
  char *cur, *prev;
  FILE *ans;

  /* Make the directory if it isn't there */
  prev = path;
  while (NULL != (cur = strchr(prev + 1, '/'))) {
    *cur = '\0';

    if (0 > mkdir(&path[0], S_IRWXU)) {
      if (errno != EEXIST) {
        perror("Unable to create directory");
        exit(EXIT_FAILURE);
      }
    }

    *cur = '/';
    prev = cur;
  }

  if (NULL == (ans = fopen(&path[0], "w"))) {
    perror("Unable to open file");
    exit(EXIT_FAILURE);
  }

  return ans;
}

/* Callbacks ========================================================= */
static void writecb(void *data, size_t data_len, void *arg) {
  FILE *file = (FILE *)arg;

  fwrite(data, 1, data_len, file);
}

/* Main ========================================================= */
int main(int argc, char **argv) {
  /* COMMAND LINE OPTIONS ============================================= */
  
  char *workload_path = "workload.txt";
  int option_char = 0;
  nrequests = 7;
  int nthreads = 7;
  char *req_path = NULL;
  int i = 0;
  int tID;
  
  setbuf(stdout, NULL);  // disable caching

  // Parse and set command line arguments
  while ((option_char = getopt_long(argc, argv, "p:n:hs:t:r:w:", gLongOptions,
                                    NULL)) != -1) {
    switch (option_char) {
      case 'r':
        nrequests = atoi(optarg);
        break;
      case 'n':  // nrequests
        break;
      default:
        Usage();
        exit(1);
      case 'p':  // port
        port = atoi(optarg);
        break;
      case 'h':  // help
        Usage();
        exit(0);
        break;
      case 's':  // server
        server = optarg;
        break;
      case 't':  // nthreads
        nthreads = atoi(optarg);
        break;
      case 'w':  // workload-path
        workload_path = optarg;
        break;
    }
  }

  if (EXIT_SUCCESS != workload_init(workload_path)) {
    fprintf(stderr, "Unable to load workload file %s.\n", workload_path);
    exit(EXIT_FAILURE);
  }

  if (nthreads < 1) {
    nthreads = 1;
  }
  if (nthreads > MAX_THREADS) {
    nthreads = MAX_THREADS;
  }

  gfc_global_init();

  
  pthread_t workerThreads[nthreads];
  requestQueue = malloc(sizeof(steque_t));

  steque_init(requestQueue);
  int threadID[nthreads];
  // add your threadpool creation here 
  for ( i = 0; i < nthreads; i++)
  {
    threadID[i] = i;
    if (pthread_create(&workerThreads[i],NULL,gfc_request,&threadID[i]) != 0)
    {
      printf("Unable to create worker thread %d",threadID[i]);
    }
    //printf("Creating thread ID#%d\n", threadID[i]);
  }
  
  /* Build your queue of requests here */
  for (i = 0; i < nrequests; i++) {
    /* Note that when you have a worker thread pool, you will need to move this
     * logic into the worker threads */
    req_path = workload_get_path();

    if (strlen(req_path) > 1024) {
      fprintf(stderr, "Request path exceeded maximum of 1024 characters\n.");
      exit(EXIT_FAILURE);
    }
    pthread_mutex_lock(&mutex);
    steque_enqueue(requestQueue,req_path);
    pthread_mutex_unlock(&mutex);
    pthread_cond_signal(&work_cond);
  }

  //adding poison pill
  
  req_path = requestDone;
  pthread_mutex_lock(&mutex);
  steque_enqueue(requestQueue,req_path);
  pthread_mutex_unlock(&mutex);
  pthread_cond_signal(&work_cond);

  for ( tID = 0; tID < nthreads; tID++)
  {
    if (pthread_join(workerThreads[tID],NULL) != 0)
    {
      printf("Unable to join worker thread %d",i);
    }
  }

  gfc_global_cleanup();  /* use for any global cleanup for AFTER your thread
                          pool has terminated. */
  pthread_mutex_destroy(&mutex); 
  pthread_cond_destroy(&work_cond);

  //destroy the poison pill from the queue
  if (steque_isempty(requestQueue) == 0)
  {
   // printf("Emptying Queue\n");
    steque_destroy(requestQueue);
  }
  if (steque_isempty(requestQueue) == 1)
  {
    printf("Free resource and Exiting...\n");
    free(requestQueue);
  }
  
  return 0;
}

void *gfc_request(void *arg)
{
  int returncode = 0;
  char local_path[PATH_BUFFER_SIZE];
  gfcrequest_t *gfr = NULL;
  FILE *file = NULL;
  char *req_path = NULL;
  for (int i = 0; i < nrequests; i++)
  {
    pthread_mutex_lock(&mutex);
    while (steque_isempty(requestQueue))
    {
      pthread_cond_wait(&work_cond,&mutex);
    }
    req_path = steque_pop(requestQueue);
    if (strcmp(req_path,requestDone) == 0) //poison pill 
    {
      steque_enqueue(requestQueue, req_path);
      pthread_mutex_unlock(&mutex);
      pthread_cond_signal(&work_cond);
      //printf("Thread ID# %d exiting\n", *(int*)arg);
      pthread_exit(NULL);
    }
    
    else
    {
      pthread_mutex_unlock(&mutex);

      //printf("thread ID# %d is working\n", *(int*)arg);
    

      localPath(req_path, local_path);

      file = openFile(local_path);

      gfr = gfc_create();
      gfc_set_server(&gfr, server);
      gfc_set_path(&gfr, req_path);
      gfc_set_port(&gfr, port);
      gfc_set_writefunc(&gfr, writecb);
      gfc_set_writearg(&gfr, file);

      fprintf(stdout, "Requesting %s%s\n", server, req_path);

      if (0 > (returncode = gfc_perform(&gfr))) {
        fprintf(stdout, "gfc_perform returned an error %d\n", returncode);
        fclose(file);
        if (0 > unlink(local_path))
          fprintf(stderr, "warning: unlink failed on %s\n", local_path);
      } else {
        fclose(file);
      }

      if (gfc_get_status(&gfr) != GF_OK) 
      {
        if (0 > unlink(local_path))
          fprintf(stderr, "warning: unlink failed on %s\n", local_path);
      }

      fprintf(stdout, "Status: %s\n", gfc_strstatus(gfc_get_status(&gfr)));
      fprintf(stdout, "Received %zu of %zu bytes\n", gfc_get_bytesreceived(&gfr),
              gfc_get_filelen(&gfr));

      gfc_cleanup(&gfr);

    }
  }
  return 0;
}