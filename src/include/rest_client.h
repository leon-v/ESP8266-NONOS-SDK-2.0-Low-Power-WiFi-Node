#ifndef __SIMPLE_REST_CLIENT_H__
#define __SIMPLE_REST_CLIENT_H__

#include "osapi.h"
#include "user_interface.h"
#include "espconn.h"
#include "mem.h"
#include "limits.h"
#include "rest_server.h"

#define BUFFER_SIZE_MAX jsonSize // Size of http responses that will cause an error

// Internal state.
typedef struct {
	char * path;
	int port;
	char * post_data;
	char * headers;
	char * hostname;
	char * buffer;
	int buffer_size;
	bool secure;
} request_args;


void http_get(const char * url, const char * headers);
void RESTClientJSONGet(struct jsontree_value *tree, const char *path, char *pbuf);

#endif