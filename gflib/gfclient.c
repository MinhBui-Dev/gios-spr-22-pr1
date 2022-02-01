
#include <stdlib.h>

#include "gfclient-student.h"

 // Modify this file to implement the interface specified in
 // gfclient.h.
struct gfcrequest_t
{
  unsigned short port;
  const char *server, *path;
  void *headerarg;
  void (*headerfunc)(void *header_buffer, size_t header_buffer_length, void *handlerarg);
  void *writearg;
  void (*writefunc)(void *data_buffer, size_t data_buffer_length, void *handlerarg);
};
// optional function for cleaup processing.
void gfc_cleanup(gfcrequest_t **gfr) {}

gfcrequest_t *gfc_create() {
  // dummy for now - need to fill this part in
  gfcrequest_t* gfr;
    if ((gfr =  (gfcrequest_t*)malloc(sizeof(gfcrequest_t))) != NULL)
    {
        printf("[CLIENT] Successful allocate memory for gfs.\n");
        return gfr;
    }

  return (gfcrequest_t *)NULL;
}

size_t gfc_get_bytesreceived(gfcrequest_t **gfr) {
  // not yet implemented
  return -1;
}

size_t gfc_get_filelen(gfcrequest_t **gfr) {
  // not yet implemented
  return -1;
}

gfstatus_t gfc_get_status(gfcrequest_t **gfr) {
  // not yet implemented
  return -1;
}

void gfc_global_init() {}

void gfc_global_cleanup() {}

int gfc_perform(gfcrequest_t **gfr) {
  /* currently not implemented.  You fill this part in. */
  return -1;
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
