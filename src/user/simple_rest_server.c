#include "simple_rest_server.h"

LOCAL char *precvbuffer;
static uint32 dat_sumlength = 0;





LOCAL bool ICACHE_FLASH_ATTR check_data(char *precv, uint16 length){
	//bool flag = true;
	char length_buf[10] = {0};
	char *ptemp = NULL;
	char *pdata = NULL;
	char *tmp_precvbuffer;
	uint16 tmp_length = length;
	uint32 tmp_totallength = 0;

	ptemp = (char *)os_strstr(precv, "\r\n\r\n");

	if (ptemp == NULL) {
		return true;
	}


	tmp_length -= ptemp - precv;
	tmp_length -= 4;
	tmp_totallength += tmp_length;

	pdata = (char *)os_strstr(precv, "Content-Length: ");

	if (pdata == NULL){
		return true;
	}

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

	return true;
}


LOCAL bool ICACHE_FLASH_ATTR save_data(char *precv, uint16 length) {
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


LOCAL void ICACHE_FLASH_ATTR parse_url(char *precv, URL_Frame *purl_frame){
	char *str = NULL;
	uint8 length = 0;
	char *pbuffer = NULL;
	char *pbufer = NULL;

	if (purl_frame == NULL || precv == NULL) {
		return;
	}

	pbuffer = (char *)os_strstr(precv, "Host:");

	if (pbuffer != NULL) {
		length = pbuffer - precv;
		pbufer = (char *)os_zalloc(length + 1);
		pbuffer = pbufer;
		os_memcpy(pbuffer, precv, length);
		os_memset(purl_frame->pSelect, 0, URLSize);
		os_memset(purl_frame->pCommand, 0, URLSize);
		os_memset(purl_frame->pFilename, 0, URLSize);

		if (os_strncmp(pbuffer, "GET ", 4) == 0) {
			purl_frame->Type = GET;
			pbuffer += 4;
		} else if (os_strncmp(pbuffer, "POST ", 5) == 0) {
			purl_frame->Type = POST;
			pbuffer += 5;
		}

		pbuffer ++;
		str = (char *)os_strstr(pbuffer, "?");

		if (str != NULL) {
			length = str - pbuffer;
			os_memcpy(purl_frame->pSelect, pbuffer, length);
			str ++;
			pbuffer = (char *)os_strstr(str, "=");

			if (pbuffer != NULL) {
				length = pbuffer - str;
				os_memcpy(purl_frame->pCommand, str, length);
				pbuffer ++;
				str = (char *)os_strstr(pbuffer, "&");

				if (str != NULL) {
					length = str - pbuffer;
					os_memcpy(purl_frame->pFilename, pbuffer, length);
				} else {
					str = (char *)os_strstr(pbuffer, " HTTP");

					if (str != NULL) {
						length = str - pbuffer;
						os_memcpy(purl_frame->pFilename, pbuffer, length);
					}
				}
			}
		}

		os_free(pbufer);
	} else {
		return;
	}
}




LOCAL void ICACHE_FLASH_ATTR data_send(void *arg, bool responseOK, char *psend) {
	uint16 length = 0;
	char *pbuf = NULL;
	char httphead[256];
	struct espconn *ptrespconn = arg;
	os_memset(httphead, 0, 256);

	if (responseOK) {
		os_sprintf(httphead,
			"HTTP/1.0 200 OK\r\nContent-Length: %d\r\nServer: lwIP/1.4.0\r\n",
			psend ? os_strlen(psend) : 0);

		if (psend) {
			os_sprintf(httphead + os_strlen(httphead),
				"Content-type: application/json\r\nExpires: Fri, 10 Apr 2008 14:00:00 GMT\r\nPragma: no-cache\r\n\r\n");
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
			Content-Length: 0\r\nServer: lwIP/1.4.0\r\n\n");
		length = os_strlen(httphead);
	}

	if (psend) {
	#ifdef SERVER_SSL_ENABLE
		espconn_secure_sent(ptrespconn, pbuf, length);
	#else
		espconn_sent(ptrespconn, pbuf, length);
	#endif
	} else {
	#ifdef SERVER_SSL_ENABLE
		espconn_secure_sent(ptrespconn, httphead, length);
	#else
		espconn_sent(ptrespconn, httphead, length);
	#endif
	}

	if (pbuf) {
		os_free(pbuf);
		pbuf = NULL;
	}
}







LOCAL void ICACHE_FLASH_ATTR response_send(void *arg, bool responseOK){
	struct espconn *ptrespconn = arg;
	data_send(ptrespconn, responseOK, NULL);
}






LOCAL void ICACHE_FLASH_ATTR webserver_recv(void *arg, char *pusrdata, unsigned short length) {
	URL_Frame *pURL_Frame = NULL;
	char *pParseBuffer = NULL;
	bool parse_flag = false;
	struct espconn *ptrespconn = arg;

	os_printf("len:%u\n",length);

	if(check_data(pusrdata, length) == false){
		os_printf("Skip Packet\n");
		goto _temp_exit;
	}

	parse_flag = save_data(pusrdata, length);

	if (parse_flag == false) {
		response_send(ptrespconn, false);
	}

	os_printf("BUFFER DATA:\n%s\n\n", precvbuffer);
	pURL_Frame = (URL_Frame *)os_zalloc(sizeof(URL_Frame));
	parse_url(precvbuffer, pURL_Frame);

	switch (pURL_Frame->Type) {
		case GET:

			os_printf("We have a GET request.\n");

			response_send(ptrespconn, true);
			break;

		case POST:

			os_printf("We have a POST request.\n");
			pParseBuffer = (char *)os_strstr(precvbuffer, "\r\n\r\n");

			if (pParseBuffer == NULL) {
				break;
			}

			pParseBuffer += 4;

			if (precvbuffer != NULL){
				os_free(precvbuffer);
				precvbuffer = NULL;
			}

			os_free(pURL_Frame);
			pURL_Frame = NULL;

			response_send(ptrespconn, true);
			break;
	}

	if (precvbuffer != NULL){
		os_free(precvbuffer);
		precvbuffer = NULL;
	}

	os_free(pURL_Frame);
	pURL_Frame = NULL;
	_temp_exit:
		;
}

LOCAL ICACHE_FLASH_ATTR void webserver_recon(void *arg, sint8 err) {
	struct espconn *pesp_conn = arg;

	os_printf("webserver's %d.%d.%d.%d:%d err %d reconnect\n", 
		pesp_conn->proto.tcp->remote_ip[0],
		pesp_conn->proto.tcp->remote_ip[1],pesp_conn->proto.tcp->remote_ip[2],
		pesp_conn->proto.tcp->remote_ip[3],pesp_conn->proto.tcp->remote_port, err
	);
}

LOCAL ICACHE_FLASH_ATTR void webserver_discon(void *arg) {
	struct espconn *pesp_conn = arg;

	os_printf("webserver's %d.%d.%d.%d:%d disconnect\n",
		pesp_conn->proto.tcp->remote_ip[0],
		pesp_conn->proto.tcp->remote_ip[1],pesp_conn->proto.tcp->remote_ip[2],
		pesp_conn->proto.tcp->remote_ip[3],pesp_conn->proto.tcp->remote_port
	);
}

LOCAL void ICACHE_FLASH_ATTR webserver_listen(void *arg) {
    struct espconn *pesp_conn = arg;

    espconn_regist_recvcb(pesp_conn, webserver_recv);
    espconn_regist_reconcb(pesp_conn, webserver_recon);
    espconn_regist_disconcb(pesp_conn, webserver_discon);
}

void ICACHE_FLASH_ATTR init_simple_rest_server() {
	LOCAL struct espconn esp_conn;
	LOCAL esp_tcp esptcp;

	esp_conn.type = ESPCONN_TCP;
	esp_conn.state = ESPCONN_NONE;
	esp_conn.proto.tcp = &esptcp;
	esp_conn.proto.tcp->local_port = 80;
	espconn_regist_connectcb(&esp_conn, webserver_listen);

	#ifdef SERVER_SSL_ENABLE
		espconn_secure_set_default_certificate(default_certificate, default_certificate_len);
		espconn_secure_set_default_private_key(default_private_key, default_private_key_len);
		espconn_secure_accept(&esp_conn);
	#else
		espconn_accept(&esp_conn);
	#endif
}