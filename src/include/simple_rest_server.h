#ifndef __SIMPLE_REST_SERVER_H__
#define __SIMPLE_REST_SERVER_H__

#include "osapi.h"
#include "user_interface.h"
#include "espconn.h"

struct espconn espconn_simple_rest_server;
esp_tcp tcp_simple_rest_server;

void init_simple_rest_server(void);

#endif