#ifndef __SIMPLE_REST_SERVER_H__
#define __SIMPLE_REST_SERVER_H__

#include "osapi.h"
#include "user_interface.h"
#include "espconn.h"

#define SERVER_PORT 80
#define SERVER_SSL_PORT 443

#define URLSize 10

typedef enum ProtocolType {
    GET = 0,
    POST,
} ProtocolType;

typedef struct URL_Frame {
    enum ProtocolType Type;
    char pSelect[URLSize];
    char pCommand[URLSize];
    char pFilename[URLSize];
} URL_Frame;

void init_simple_rest_server(void);

#endif