#include "rest_server.h"
#include "rest_client.h"

// Define the endpoint for the REST server
REST_Endpoint_t RESTADCEndpoint;


// Define the status structure of the ADC
typedef struct  RESTEndpointADCStatus_T{
    uint32 Interval;
    uint16 Threshold;
    uint16 Minimum;
    uint16 Maximum;
    uint16 Current;
    uint16 LastSent;
    char URL[128];
} RESTEndpointADCStatus_T;

RESTEndpointADCStatus_T restEndpointADCStatus;


LOCAL int ICACHE_FLASH_ATTR RESTEndpointGetADC(struct jsontree_context *js_ctx) {
	char string[32];

	const char *path = jsontree_path_name(js_ctx, js_ctx->depth - 1);

	if (os_strncmp(path, "Time", 4) == 0) {
		os_sprintf(string, "%d", system_get_time());
		
	}else if (os_strncmp(path, "Interval", 8) == 0) {
		os_sprintf(string, "%d", restEndpointADCStatus.Interval);

	}else if (os_strncmp(path, "Threshold", 10) == 0) {
		os_sprintf(string, "%d", restEndpointADCStatus.Threshold);

	}else if (os_strncmp(path, "Minimum", 7) == 0) {
		os_sprintf(string, "%d", restEndpointADCStatus.Minimum);

	}else if (os_strncmp(path, "Maximum", 7) == 0) {
		os_sprintf(string, "%d", restEndpointADCStatus.Maximum);

	}else if (os_strncmp(path, "Current", 7) == 0) {
		os_sprintf(string, "%d", restEndpointADCStatus.Current);

	}else if (os_strncmp(path, "URL", 3) == 0) {
		os_sprintf(string, "%s", restEndpointADCStatus.URL);

	}


	jsontree_write_string(js_ctx, string);

	return 0;
}
LOCAL int ICACHE_FLASH_ATTR RESTEndpointSetADC(struct jsontree_context *js_ctx, struct jsonparse_state *parser){
	int type;

	while ((type = jsonparse_next(parser)) != 0) {

		if (type == JSON_TYPE_PAIR_NAME) {
			if (jsonparse_strcmp_value(parser, "Interval") == 0) {
				jsonparse_next(parser);
				jsonparse_next(parser);
				restEndpointADCStatus.Interval = jsonparse_get_value_as_int(parser);
				os_printf("set Interval %d\n", restEndpointADCStatus.Interval);
				
			}else if (jsonparse_strcmp_value(parser, "Threshold") == 0) {
				jsonparse_next(parser);
				jsonparse_next(parser);
				restEndpointADCStatus.Threshold = jsonparse_get_value_as_int(parser);
				os_printf("set Threshold %d\n", restEndpointADCStatus.Threshold);

			}else if (jsonparse_strcmp_value(parser, "Minimum") == 0) {
				jsonparse_next(parser);
				jsonparse_next(parser);
				restEndpointADCStatus.Minimum = jsonparse_get_value_as_int(parser);
				os_printf("set Minimum %d\n", restEndpointADCStatus.Minimum);

			}else if (jsonparse_strcmp_value(parser, "Maximum") == 0) {
				jsonparse_next(parser);
				jsonparse_next(parser);
				restEndpointADCStatus.Maximum = jsonparse_get_value_as_int(parser);
				os_printf("set Maximum %d\n", restEndpointADCStatus.Maximum);

			}else if (jsonparse_strcmp_value(parser, "Current") == 0) {
				jsonparse_next(parser);
				jsonparse_next(parser);
				restEndpointADCStatus.Current = jsonparse_get_value_as_int(parser);
				os_printf("set Current %d\n", restEndpointADCStatus.Current);

			}else if (jsonparse_strcmp_value(parser, "URL") == 0) {
				jsonparse_next(parser);
				jsonparse_next(parser);
				jsonparse_copy_value(parser, restEndpointADCStatus.URL, sizeof restEndpointADCStatus.URL);
				os_printf("set URL %s\n", restEndpointADCStatus.URL);
			}
		}
	}

	return 0;
}

LOCAL struct jsontree_callback RESTEndpointADCCallback = JSONTREE_CALLBACK(RESTEndpointGetADC, RESTEndpointSetADC);

JSONTREE_OBJECT(ADCTree,
	JSONTREE_PAIR("Time", &RESTEndpointADCCallback),
	JSONTREE_PAIR("Interval", &RESTEndpointADCCallback),
	JSONTREE_PAIR("Threshold", &RESTEndpointADCCallback),
	JSONTREE_PAIR("Minimum", &RESTEndpointADCCallback),
	JSONTREE_PAIR("Maximum", &RESTEndpointADCCallback),
	JSONTREE_PAIR("URL", &RESTEndpointADCCallback),
	
);
JSONTREE_OBJECT(RESTTree,
	JSONTREE_PAIR("get", &ADCTree),
	JSONTREE_PAIR("set", &ADCTree)
);

void ICACHE_FLASH_ATTR RESTEndpointADCInit(){

	restEndpointADCStatus.Interval = 0;
	restEndpointADCStatus.Threshold = 0;
	restEndpointADCStatus.Minimum = 0;
	restEndpointADCStatus.Maximum = 0;
	restEndpointADCStatus.Current = 0;
	restEndpointADCStatus.LastSent = 0;
	os_sprintf(restEndpointADCStatus.URL, "");

	os_sprintf(RESTADCEndpoint.Service, "adc");
	os_sprintf(RESTADCEndpoint.Endpoint, "0");
	
	RESTADCEndpoint.JSONTree = (struct jsontree_value *) &RESTTree;

	RESTServerAddEndpoint(RESTADCEndpoint);
}


void ICACHE_FLASH_ATTR RESTEndpointADCCheckStatus() {

	bool sendUpdate = false;
	if (restEndpointADCStatus.Current > restEndpointADCStatus.Maximum) {
		sendUpdate = true;

	} else if (restEndpointADCStatus.Minimum < restEndpointADCStatus.Maximum) {
		sendUpdate = true;

	}

	if (!sendUpdate) {
		return;
	}

	if (os_strlen(restEndpointADCStatus.URL) <= 11) {
		os_printf("No URL to send to");
		return;
	}

	restEndpointADCStatus.LastSent = restEndpointADCStatus.Current;

	char *pbuf = NULL;
	pbuf = (char *)os_zalloc(jsonSize);

	RESTClientJSONGet((struct jsontree_value *) &RESTTree, "get", pbuf);

	os_printf("Sending Data:\n%s\n\n", pbuf);

	http_post(restEndpointADCStatus.URL, pbuf);
}