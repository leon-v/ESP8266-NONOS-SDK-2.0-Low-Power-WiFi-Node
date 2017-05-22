#include "simple_rest_server.h"

void http_recv_simple_rest_server(void *arg, char *pdata, unsigned short len) {
	os_printf("%s\n\r", pdata);

	char data[256] = "<html>\
<head>\
	<title>ESP8266</title>\
</head>\
<body>\
	<script>\
		setTimeout(function(){\
			location.reload();\
		}, 10000);\
	</script>\
</body>\
</html>\0";
	char header[512];
	os_sprintf(header, "HTTP/1.1 200 OK\r\n\
Content-type: text/html\r\n\
Content-length: %d\r\n\
\r\n%s\0", strlen(data), data);

  //returns 0 on success
	if (espconn_sent((struct espconn *) arg, data, strlen(data)) != ESPCONN_OK) {
		os_printf("Response failed\n\r");
	} else {
		os_printf("Response sent\n\r");
	}
	
	if (espconn_disconnect((struct espconn *) arg) != ESPCONN_OK) {
		os_printf("Disconnect failed\n\r");
	} else {
		os_printf("Disconnected\n\r");
	}
}


void server_connect_simple_rest_server(void *arg){
	int i;
	struct espconn *pespconn = (struct espconn *)arg;

	//espconn's have a extra flag you can associate extra information with a connection.
	//pespconn->reverse = my_http;

	//Let's register a few callbacks, for when data is received or a disconnect happens.
	espconn_regist_recvcb( pespconn, http_recv_simple_rest_server);
	espconn_regist_disconcb( pespconn, NULL );
}


void ICACHE_FLASH_ATTR init_simple_rest_server(void){
	os_printf("Starting HTTP Server\r\n");
	//Initialize the ESPConn
	espconn_create( &espconn_simple_rest_server);
	espconn_simple_rest_server.type = ESPCONN_TCP;
	espconn_simple_rest_server.state = ESPCONN_NONE;

	//Make it a TCP connection.
	espconn_simple_rest_server.proto.tcp = &tcp_simple_rest_server;
	espconn_simple_rest_server.proto.tcp->local_port = 80;

	espconn_regist_connectcb(&espconn_simple_rest_server, server_connect_simple_rest_server);

	//Start listening!
	espconn_accept(&espconn_simple_rest_server);
}