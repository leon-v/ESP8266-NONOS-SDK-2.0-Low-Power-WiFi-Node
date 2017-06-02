#include "rest_server.h"


LOCAL int ICACHE_FLASH_ATTR REST_Get(struct jsontree_context *js_ctx) {
	char string[32];

	const char *path = jsontree_path_name(js_ctx, js_ctx->depth - 1);

	if (os_strncmp(path, "threashold", 10) == 0) {
		os_sprintf(string, "%s", "get threashold");

	}else if (os_strncmp(path, "min", 3) == 0) {
		os_sprintf(string, "%s", "get min");

	}else if (os_strncmp(path, "max", 3) == 0) {
		os_sprintf(string, "%s", "get max");

	}

	jsontree_write_string(js_ctx, string);

	return 0;
}
LOCAL int ICACHE_FLASH_ATTR REST_Set(struct jsontree_context *js_ctx, struct jsonparse_state *parser){
	int type;

	while ((type = jsonparse_next(parser)) != 0) {
		if (type == JSON_TYPE_PAIR_NAME) {
			if (jsonparse_strcmp_value(parser, "threashold") == 0) {
				jsonparse_next(parser);
				jsonparse_next(parser);
				uint8 threashold;
				threashold = jsonparse_get_value_as_int(parser);
				os_printf("set threashold %d\n", threashold);

			}else if (jsonparse_strcmp_value(parser, "min") == 0) {
				jsonparse_next(parser);
				jsonparse_next(parser);
				uint8 min;
				min = jsonparse_get_value_as_int(parser);
				os_printf("set min %d\n", min);

			}else if (jsonparse_strcmp_value(parser, "max") == 0) {
				jsonparse_next(parser);
				jsonparse_next(parser);
				uint8 max;
				max = jsonparse_get_value_as_int(parser);
				os_printf("set max %d\n", max);
			}
		}
	}

	return 0;
}

LOCAL struct jsontree_callback REST_Callback = JSONTREE_CALLBACK(REST_Get, REST_Set);

JSONTREE_OBJECT(ADCTree,
	JSONTREE_PAIR("threashold", &REST_Callback),
	JSONTREE_PAIR("min", &REST_Callback),
	JSONTREE_PAIR("max", &REST_Callback),
);
JSONTREE_OBJECT(RESTTree,
	JSONTREE_PAIR("get", &ADCTree),
	JSONTREE_PAIR("set", &ADCTree)
);

REST_Endpoint_t REST_Endpoint;
void ICACHE_FLASH_ATTR rest_server_endpoint_adc_init(){

	os_sprintf(REST_Endpoint.page, "adc");
	os_sprintf(REST_Endpoint.endpoint, "0");
	REST_Endpoint.JSONTree = (struct jsontree_value *) &RESTTree;

	rest_server_add_endpoint(REST_Endpoint);
}