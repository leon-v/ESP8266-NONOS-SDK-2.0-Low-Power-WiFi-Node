#include "ets_sys.h"
#include "osapi.h"
#include "gpio.h"
#include "user_interface.h"
#include "espconn.h"

#include "simple_rest_server.h"

#include "driver/uart.h"

void ICACHE_FLASH_ATTR  wifi_handle_event_cb(System_Event_t *evt) {
    os_printf("event %x\n", evt->event);
	int res;
    switch (evt->event) {
         case EVENT_STAMODE_GOT_IP:

            break;
         default:
             break;
 }
}

static os_timer_t osTask_timer;
void osTask(void *arg){

	os_printf("Task Run\r\n");
	
	os_timer_disarm(&osTask_timer);
}

static os_timer_t osRechargeCapTask_timer;
void osRechargeCapTask(void *arg){
	os_timer_disarm(&osRechargeCapTask_timer);

	os_printf("Re-2\r\n");
	GPIO_DIS_OUTPUT(14);
}

void gpioInterruptTask(int * arg){

	uint32 gpio_status = GPIO_REG_READ(GPIO_STATUS_ADDRESS);
	
	GPIO_OUTPUT_SET(14, 0);
	GPIO_OUTPUT_SET(14, 1);

	// Let the interrupt capacitor discharge after 5 milliseconds
	os_timer_disarm(&osRechargeCapTask_timer);
	os_timer_arm(&osRechargeCapTask_timer, 3, 1);

	// Trigger an execution of your custom task
	os_timer_disarm(&osTask_timer);
	os_timer_arm(&osTask_timer, 1, 1);

	//clear	interrupt	status
	GPIO_REG_WRITE(GPIO_STATUS_W1TC_ADDRESS, gpio_status);

	
}


void ICACHE_FLASH_ATTR  user_init(void) {
	
    os_printf("SDK version:%s\n", system_get_sdk_version());
	
    struct station_config stationConf;
	wifi_set_opmode_current(STATION_MODE);
	os_memset(&stationConf, 0, sizeof(struct station_config));
	os_sprintf(stationConf.ssid, WIFI_SSID);
	os_sprintf(stationConf.password, WIFI_PASS);
	wifi_station_set_config_current(&stationConf);
	wifi_set_event_handler_cb(wifi_handle_event_cb);
	wifi_station_connect();

	PIN_FUNC_SELECT(PERIPHS_IO_MUX_MTMS_U, FUNC_GPIO14);
	PIN_PULLUP_DIS(PERIPHS_IO_MUX_MTMS_U);

	gpio_pin_wakeup_enable(14, GPIO_PIN_INTR_LOLEVEL);

	gpio_pin_intr_state_set(14, GPIO_PIN_INTR_LOLEVEL);
	ets_isr_attach(ETS_GPIO_INUM, (ets_isr_t)gpioInterruptTask, NULL);
	ETS_GPIO_INTR_ENABLE();

	// Create interrupt recharge task
	os_timer_disarm(&osRechargeCapTask_timer);
	os_timer_setfn(&osRechargeCapTask_timer, (os_timer_func_t *)osRechargeCapTask, NULL);
	
	// Create a user task
	os_timer_disarm(&osTask_timer);
	os_timer_setfn(&osTask_timer, (os_timer_func_t *)osTask, NULL);

	// Start loop by starting the re-charge cycle
	os_timer_arm(&osRechargeCapTask_timer, 1000, 1);


	init_simple_rest_server();


	
	wifi_set_sleep_type(LIGHT_SLEEP_T);
	wifi_fpm_set_sleep_type(LIGHT_SLEEP_T);
}

uint32 ICACHE_FLASH_ATTR user_rf_cal_sector_set(void){
    enum flash_size_map size_map = system_get_flash_size_map();
    uint32 rf_cal_sec = 0;

    switch (size_map) {
        case FLASH_SIZE_4M_MAP_256_256:
        	rf_cal_sec = 128 - 5;
        	break;

        case FLASH_SIZE_8M_MAP_512_512:
        	rf_cal_sec = 256 - 5;
        	break;

        case FLASH_SIZE_16M_MAP_512_512:
        case FLASH_SIZE_16M_MAP_1024_1024:
            rf_cal_sec = 512 - 5;
            break;

        case FLASH_SIZE_32M_MAP_512_512:
        case FLASH_SIZE_32M_MAP_1024_1024:
            rf_cal_sec = 1024 - 5;
            break;

        default:
            rf_cal_sec = 0;
            break;
    }

    return rf_cal_sec;
}