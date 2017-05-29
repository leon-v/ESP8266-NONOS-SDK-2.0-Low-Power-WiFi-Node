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







LOCAL void ICACHE_FLASH_ATTR webserver_recv(void *arg, char *pusrdata, unsigned short length) {
	URL_Frame *pURL_Frame = NULL;
	char *pParseBuffer = NULL;
	bool parse_flag = false;
	struct espconn *ptrespconn = arg;

	os_printf("len:%u\n",length);

	if(check_data(pusrdata, length) == false){
		os_printf("goto\n");
		goto _temp_exit;
	}

	parse_flag = save_data(pusrdata, length);
	if (parse_flag == false) {
		response_send(ptrespconn, false);
	}

	os_printf(precvbuffer);
	pURL_Frame = (URL_Frame *)os_zalloc(sizeof(URL_Frame));
	parse_url(precvbuffer, pURL_Frame);

	switch (pURL_Frame->Type) {
		case GET:

			os_printf("We have a GET request.\n");
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
			_temp_exit:
		break;
	}
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