#include "rest_server.h"

LOCAL char *precvbuffer;
static uint32 dat_sumlength = 0;

struct REST_Endpoint_t restEndpoints[ENDPOINT_COUNT];



int restEndpointCount = 0;
void ICACHE_FLASH_ATTR RESTServerAddEndpoint(struct REST_Endpoint_t restEndpoint) {
	restEndpoints[restEndpointCount++] = restEndpoint;
	os_printf("REST Endpoint %s/%s added.", restEndpoint.Service, restEndpoint.Endpoint);
}


LOCAL char *jsonBuffer;
LOCAL int jsonBufferIndex;
LOCAL int jsonBufferSize;
int ICACHE_FLASH_ATTR RESTServerJSONPutchar(int character) {
    if (jsonBuffer != NULL && jsonBufferIndex <= jsonBufferSize) {
        jsonBuffer[jsonBufferIndex++] = character;
        return character;
    }

    return 0;
}
void ICACHE_FLASH_ATTR RESTServerJSONParse(struct jsontree_context *json, char *jsonMessage){
    /* Set value */
    struct jsontree_value *v;
    struct jsontree_callback *c;
    struct jsontree_callback *c_bak = NULL;

    while ((v = jsontree_find_next(json, JSON_TYPE_CALLBACK)) != NULL) {
        c = (struct jsontree_callback *)v;

        if (c == c_bak) {
            continue;
        }

        c_bak = c;

        if (c->set != NULL) {
            struct jsonparse_state js;

            jsonparse_setup(&js, jsonMessage, os_strlen(jsonMessage));
            c->set(json, &js);
        }
    }
}

struct jsontree_value *ICACHE_FLASH_ATTR RESTServerJSONFindPath(struct jsontree_context *json, const char *path) {
	struct jsontree_value *v;
	const char *start;
	const char *end;
	int len;

	v = json->values[0];
	start = path;

	do {
		end = (const char *)os_strstr(start, "/");

		if (end == start) {
			break;
		}

		if (end != NULL) {
			len = end - start;
			end++;
		} else {
			len = os_strlen(start);
		}

		if (v->type != JSON_TYPE_OBJECT) {
			v = NULL;
		} else {
			struct jsontree_object *o;
			int i;

			o = (struct jsontree_object *)v;
			v = NULL;

			for (i = 0; i < o->count; i++) {
				if (os_strncmp(start, o->pairs[i].name, len) == 0) {
					v = o->pairs[i].value;
					json->index[json->depth] = i;
					json->depth++;
					json->values[json->depth] = v;
					json->index[json->depth] = 0;
					break;
				}
			}
		}

		start = end;
	} while (end != NULL && *end != '\0' && v != NULL);

	json->callback_state = 0;
	return v;
}
void ICACHE_FLASH_ATTR RESTServerJSONSend(struct jsontree_value *tree, const char *path, char *pbuf) {
	struct jsontree_context json;
    /* maxsize = 128 bytes */
	jsonBuffer = (char *)os_malloc(jsonSize);

    /* reset state and set max-size */
    /* NOTE: packet will be truncated at 512 bytes */
	jsonBufferIndex = 0;
	jsonBufferSize = jsonSize;

	json.values[0] = (struct jsontree_value *)tree;
	jsontree_reset(&json);
	RESTServerJSONFindPath(&json, path);
	json.path = json.depth;
	json.putchar = RESTServerJSONPutchar;

	while (jsontree_print_next(&json) && json.path <= json.depth);

	jsonBuffer[jsonBufferIndex] = 0;
	os_memcpy(pbuf, jsonBuffer, jsonBufferIndex);
	os_free(jsonBuffer);
}





























LOCAL bool ICACHE_FLASH_ATTR RESTServerPacketCheckSize(char *precv, uint16 length) {
        //bool flag = true;
	char length_buf[10] = {0};
	char *ptemp = NULL;
	char *pdata = NULL;
	char *tmp_precvbuffer;
	uint16 tmp_length = length;
	uint32 tmp_totallength = 0;

	ptemp = (char *)os_strstr(precv, "\r\n\r\n");

	if (ptemp != NULL) {
		tmp_length -= ptemp - precv;
		tmp_length -= 4;
		tmp_totallength += tmp_length;

		pdata = (char *)os_strstr(precv, "Content-Length: ");

		if (pdata != NULL){
			pdata += 16;
			tmp_precvbuffer = (char *)os_strstr(pdata, "\r\n");

			if (tmp_precvbuffer != NULL){
				os_memcpy(length_buf, pdata, tmp_precvbuffer - pdata);
				dat_sumlength = atoi(length_buf);
				os_printf("A_dat:%u,tot:%u,lenght:%u\n",dat_sumlength,tmp_totallength,tmp_length);
				if(dat_sumlength != tmp_totallength){
					return false;
				}
			}
		}
	}
	return true;
}


LOCAL bool ICACHE_FLASH_ATTR RESTServerPacketSave(char *precv, uint16 length) {
	bool flag = false;
	char length_buf[10] = {0};
	char *ptemp = NULL;
	char *pdata = NULL;
	uint16 headlength = 0;
	static uint32 totallength = 0;

	ptemp = (char *)os_strstr(precv, "\r\n\r\n");

	if (ptemp != NULL) {
		length -= ptemp - precv;
		length -= 4;
		totallength += length;
		headlength = ptemp - precv + 4;
		pdata = (char *)os_strstr(precv, "Content-Length: ");

		if (pdata != NULL) {
			pdata += 16;
			precvbuffer = (char *)os_strstr(pdata, "\r\n");

			if (precvbuffer != NULL) {
				os_memcpy(length_buf, pdata, precvbuffer - pdata);
				dat_sumlength = atoi(length_buf);
			}
		} else {
			if (totallength != 0x00){
				totallength = 0;
				dat_sumlength = 0;
				return false;
			}
		}
		if ((dat_sumlength + headlength) >= 1024) {
			precvbuffer = (char *)os_zalloc(headlength + 1);
			os_memcpy(precvbuffer, precv, headlength + 1);
		} else {
			precvbuffer = (char *)os_zalloc(dat_sumlength + headlength + 1);
			os_memcpy(precvbuffer, precv, os_strlen(precv));
		}
	} else {

		if (precvbuffer != NULL) {
			totallength += length;
			os_memcpy(precvbuffer + os_strlen(precvbuffer), precv, length);

		} else {
			totallength = 0;
			dat_sumlength = 0;
			return false;
		}
	}

	if (totallength == dat_sumlength) {
		totallength = 0;
		dat_sumlength = 0;
		return true;
	} else {
		return false;
	}
}


LOCAL void ICACHE_FLASH_ATTR RESTServerURLParse(char *precv, URL_Frame *purl_frame){
	char *str = NULL;
	uint8 length = 0;
	char *pbuffer = NULL;
	char *pbufer = NULL;
	char *pmethod = NULL;

	if (purl_frame == NULL || precv == NULL) {
		return;
	}

	pbuffer = (char *)os_strstr(precv, "Host:");

	if (pbuffer == NULL) {
		return;
	}
	length = pbuffer - precv;
	pbufer = (char *)os_zalloc(length + 1);
	pbuffer = pbufer;
	os_memcpy(pbuffer, precv, length);
	os_memset(purl_frame->Path, 0, URLSize);

	if (os_strncmp(pbuffer, "GET ", 4) == 0) {
		pbuffer += 4;
		purl_frame->Type = GET;
		

	} else if (os_strncmp(pbuffer, "POST ", 5) == 0) {
		pbuffer += 5;
		purl_frame->Type = POST;
		

	} else if (os_strncmp(pbuffer, "OPTIONS ", 8) == 0) {
		pbuffer += 8;

		pmethod = (char *)os_strstr(precv, "Access-Control-Request-Method: ");

		if (pmethod != NULL){
			pmethod+= 31;
			if (os_strncmp(pmethod, "POST\r\n", 6) == 0){
				purl_frame->Type = POST;
			} else if (os_strncmp(pmethod, "GET\r\n", 5) == 0) {
				purl_frame->Type = GET;
			}
		}
	}

	// Skip past the space ?
	pbuffer++;

	// Find the Service in the URL
	str = (char *)os_strstr(pbuffer, "/");

	if (str != NULL) {

		length = str - pbuffer;
		os_memcpy(purl_frame->Service, pbuffer, length);

		pbuffer = str;

		pbuffer++;// Skip the /

		// Find the end of the URL
		str = (char *)os_strstr(pbuffer, "?");
		if (str == NULL) {
			str = (char *)os_strstr(pbuffer, " ");
		}

		// No End !? ERROR! DOES NOT COMPUTE
		if (str == NULL) {
			os_free(pbufer);
			return;
		}

		length = str - pbuffer;
		os_memcpy(purl_frame->Endpoint, pbuffer, length);
	} else {
		// Find the end of the URL
		str = (char *)os_strstr(pbuffer, "?");
		if (str == NULL) {
			str = (char *)os_strstr(pbuffer, " ");
		}

		// No End !? ERROR! DOES NOT COMPUTE
		if (str == NULL) {
			os_free(pbufer);
			return;
		}

		length = str - pbuffer;
		os_memcpy(purl_frame->Service, pbuffer, length);
	}


	// After this we can look for URL paramaters, but REST doesnt use those, so im not going to add it now

	// After this we can look for HTTP version, but REST doesnt use those, so im not going to add it now

	os_free(pbufer);
}




LOCAL void ICACHE_FLASH_ATTR RESTServerSend(void *arg, bool responseOK, char *psend) {
	uint16 length = 0;
	char *pbuf = NULL;
	char httphead[256];
	struct espconn *ptrespconn = arg;
	os_memset(httphead, 0, 256);

	if (responseOK) {
		os_sprintf(httphead,
			"HTTP/1.0 200 OK\r\nContent-Length: %d\r\nServer: LVLPREST/0.5.0\r\n",
			psend ? os_strlen(psend) : 0);

		if (psend) {
			os_sprintf(httphead + os_strlen(httphead),
				"Access-Control-Allow-Origin: *\r\nContent-type: application/json\r\nExpires: Fri, 10 Apr 2008 14:00:00 GMT\r\nPragma: no-cache\r\n\r\n");
			length = os_strlen(httphead) + os_strlen(psend);
			pbuf = (char *)os_zalloc(length + 1);
			os_memcpy(pbuf, httphead, os_strlen(httphead));
			os_memcpy(pbuf + os_strlen(httphead), psend, os_strlen(psend));
		} else {
			os_sprintf(httphead + os_strlen(httphead), "\n");
			length = os_strlen(httphead);
		}
	} else {
		os_sprintf(httphead, "HTTP/1.0 400 BadRequest\r\n\
			Content-Length: 0\r\nServer: LVLPREST/0.5.0\r\n\n");
		length = os_strlen(httphead);
	}

	if (psend) {
		//espconn_secure_sent(ptrespconn, pbuf, length);
		espconn_sent(ptrespconn, pbuf, length);
	} else {
		//espconn_secure_sent(ptrespconn, httphead, length);
		espconn_sent(ptrespconn, httphead, length);
	}

	if (pbuf) {
		os_free(pbuf);
		pbuf = NULL;
	}
}







LOCAL void ICACHE_FLASH_ATTR response_send(void *arg, bool responseOK){
	struct espconn *ptrespconn = arg;
	RESTServerSend(ptrespconn, responseOK, NULL);
}



LOCAL void ICACHE_FLASH_ATTR RESTServerEndpointGet(URL_Frame *pURL_Frame, struct espconn * ptrespconn) {

	char *pbuf = NULL;
	pbuf = (char *)os_zalloc(jsonSize);

	int restEndpointIndex = 0;
	REST_Endpoint_t restEndpoint;
	while (restEndpointIndex < restEndpointCount) {

		restEndpoint = restEndpoints[restEndpointIndex];

		if (os_strncmp(restEndpoint.Service, pURL_Frame->Service, os_strlen(pURL_Frame->Service)) != 0){
			restEndpointIndex++;
			continue;
		}

		if (os_strncmp(restEndpoint.Endpoint, pURL_Frame->Endpoint, os_strlen(pURL_Frame->Endpoint)) != 0){
			restEndpointIndex++;
			continue;
		}
		
		os_printf("Processing REST GET Endpoint: %s/%s\n", pURL_Frame->Service, pURL_Frame->Endpoint);

		RESTServerJSONSend(restEndpoint.JSONTree, "get", pbuf);
		break;
	}

	RESTServerSend(ptrespconn, true, pbuf);
	os_free(pbuf);
	pbuf = NULL;
}




bool ICACHE_FLASH_ATTR RESTServerEndpointSet(URL_Frame *pURL_Frame, char *pParseBuffer){

	int restEndpointIndex = 0;
	REST_Endpoint_t restEndpoint;
	while (restEndpointIndex < restEndpointCount) {

		restEndpoint = restEndpoints[restEndpointIndex];

		if (os_strncmp(restEndpoint.Service, pURL_Frame->Service, os_strlen(pURL_Frame->Service)) != 0){
			restEndpointIndex++;
			continue;
		}

		if (os_strncmp(restEndpoint.Endpoint, pURL_Frame->Endpoint, os_strlen(pURL_Frame->Endpoint)) != 0){
			restEndpointIndex++;
			continue;
		}
		
		os_printf("Processing REST Endpoint: %s/%s\n", pURL_Frame->Service, pURL_Frame->Endpoint);

		struct jsontree_context js;
		jsontree_setup(&js, restEndpoint.JSONTree, RESTServerJSONPutchar);
		RESTServerJSONParse(&js, pParseBuffer);

		return true;
	}
	return false;
}

LOCAL void ICACHE_FLASH_ATTR RESTServerPacketReceive(void *arg, char *pusrdata, unsigned short length) {
	URL_Frame *pURL_Frame = NULL;
	char *pParseBuffer = NULL;
	bool parse_flag = false;
	struct espconn *ptrespconn = arg;

	os_printf("len:%u\n",length);

	if(RESTServerPacketCheckSize(pusrdata, length) == false){
		RESTServerPacketSave(pusrdata, length);
		os_printf("Skip Packet\n");
		return;
	}

	parse_flag = RESTServerPacketSave(pusrdata, length);

	if (parse_flag == false) {
		response_send(ptrespconn, false);
	}

	pURL_Frame = (URL_Frame *)os_zalloc(sizeof(URL_Frame));
	RESTServerURLParse(precvbuffer, pURL_Frame);


	//str = (char *)os_strstr(pbuffer, "?");

	switch (pURL_Frame->Type) {
		case GET:
			os_printf("GET request.\n");

			response_send(ptrespconn, true);
			break;

		case POST:
			os_printf("POST request.\n");

			pParseBuffer = (char *)os_strstr(precvbuffer, "\r\n\r\n");

			if (pParseBuffer == NULL) {
				break;
			}

			pParseBuffer += 4;

			if (precvbuffer != NULL){
				os_free(precvbuffer);
				precvbuffer = NULL;
			}

			//os_printf("JSON: %s\n\n", pParseBuffer);
			
			if (RESTServerEndpointSet(pURL_Frame, pParseBuffer)) {
				RESTServerEndpointGet(pURL_Frame, ptrespconn);
			} else {
				response_send(ptrespconn, false);
			}

			os_free(pURL_Frame);
			pURL_Frame = NULL;
			
			break;
	}

	if (precvbuffer != NULL){
		os_free(precvbuffer);
		precvbuffer = NULL;
	}

	os_free(pURL_Frame);
	pURL_Frame = NULL;
}

LOCAL ICACHE_FLASH_ATTR void RESTServerReconnect(void *arg, sint8 err) {
	struct espconn *pesp_conn = arg;

	// os_printf("webserver's %d.%d.%d.%d:%d err %d reconnect\n", 
	// 	pesp_conn->proto.tcp->remote_ip[0],
	// 	pesp_conn->proto.tcp->remote_ip[1],pesp_conn->proto.tcp->remote_ip[2],
	// 	pesp_conn->proto.tcp->remote_ip[3],pesp_conn->proto.tcp->remote_port, err
	// );
}

LOCAL ICACHE_FLASH_ATTR void RESTServerDisconnect(void *arg) {
	struct espconn *pesp_conn = arg;

	// os_printf("webserver's %d.%d.%d.%d:%d disconnect\n",
	// 	pesp_conn->proto.tcp->remote_ip[0],
	// 	pesp_conn->proto.tcp->remote_ip[1],pesp_conn->proto.tcp->remote_ip[2],
	// 	pesp_conn->proto.tcp->remote_ip[3],pesp_conn->proto.tcp->remote_port
	// );
}

LOCAL void ICACHE_FLASH_ATTR RESTServerStartListen(void *arg) {
    struct espconn *pesp_conn = arg;

    espconn_regist_recvcb(pesp_conn, RESTServerPacketReceive);
    espconn_regist_reconcb(pesp_conn, RESTServerReconnect);
    espconn_regist_disconcb(pesp_conn, RESTServerDisconnect);
}

void ICACHE_FLASH_ATTR RESTServerInit() {
	LOCAL struct espconn esp_conn;
	LOCAL esp_tcp esptcp;

	esp_conn.type = ESPCONN_TCP;
	esp_conn.state = ESPCONN_NONE;
	esp_conn.proto.tcp = &esptcp;
	esp_conn.proto.tcp->local_port = 80;
	espconn_regist_connectcb(&esp_conn, RESTServerStartListen);

	#ifdef SERVER_SSL_ENABLE
		espconn_secure_set_default_certificate(default_certificate, default_certificate_len);
		espconn_secure_set_default_private_key(default_private_key, default_private_key_len);
		espconn_secure_accept(&esp_conn);
	#else
		espconn_accept(&esp_conn);
	#endif
}