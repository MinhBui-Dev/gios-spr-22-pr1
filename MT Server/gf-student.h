/*
 *  This file is for use by students to define anything they wish.  It is used by both the gf server and client implementations
 */
#ifndef __GF_STUDENT_H__
#define __GF_STUDENT_H__
#include <string.h>
#include <sys/socket.h>
#include <getopt.h>
#include <netdb.h>
#include <netinet/in.h>
#include <resolv.h>
#include <errno.h>
#include <sys/signal.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include <stdio.h>
#include <pthread.h>
#include "steque.h"

#define SCHEME "GETFILE"
#define METHOD "GET"
#define MARKER "\r\n\r\n"
#define MAXBUFFER 4000
#define SCHEMESIZE 30
#define STATUSMAXSIZE 30

 #endif // __GF_STUDENT_H__
