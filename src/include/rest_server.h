#ifndef __SIMPLE_REST_SERVER_H__
#define __SIMPLE_REST_SERVER_H__

#include "ets_sys.h"
#include "os_type.h"
#include "osapi.h"
#include "mem.h"

#include "json/jsonparse.h"
#include "json/jsontree.h"

#include "user_interface.h"

#include "espconn.h"

#include "upgrade.h"

//#include "ssl/cert.h"
//#include "ssl/private_key.h"

#define SERVER_PORT 80
#define SERVER_SSL_PORT 443

#define URLSize 10

#define ENDPOINT_COUNT 10

#define jsonSize   2*1024



typedef enum _ParmType {
    ADC = 0
} ParmType;

typedef enum ProtocolType {
    GET = 0,
    POST,
} ProtocolType;

typedef struct URL_Frame {
    enum ProtocolType Type;
    char Path[URLSize];
} URL_Frame;

typedef struct REST_Endpoint_t{
	char page[URLSize];
	char endpoint[URLSize];
	struct jsontree_value * JSONTree;
} REST_Endpoint_t;

void init_rest_server(void);
void ICACHE_FLASH_ATTR rest_server_add_endpoint(struct REST_Endpoint_t restEndpoint);

#endif