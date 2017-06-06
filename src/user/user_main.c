#include "ets_sys.h"
#include "osapi.h"
#include "gpio.h"
#include "user_interface.h"
#include "espconn.h"
#include "rest_server.h"
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

unsigned int counter = 0;
static os_timer_t osRechargeCapTask_timer;
void osRechargeCapTask(void *arg){

	RESTEndpointADCCheckStatus();

	os_printf("Discharge %d\r\n", counter);
	ETS_GPIO_INTR_ENABLE();// Enable interrupts
	GPIO_DIS_OUTPUT(14);
	os_timer_disarm(&osRechargeCapTask_timer);
	GPIO_DIS_OUTPUT(14);
	wifi_fpm_set_sleep_type(LIGHT_SLEEP_T);
	GPIO_DIS_OUTPUT(14);
	wifi_set_sleep_type(LIGHT_SLEEP_T);
	GPIO_DIS_OUTPUT(14);
}

void gpioInterruptTask(int * arg){

	ETS_GPIO_INTR_DISABLE();// Enable interrupts
	wifi_fpm_set_sleep_type(NONE_SLEEP_T);
	wifi_set_sleep_type(NONE_SLEEP_T);

	uint32 gpio_status = GPIO_REG_READ(GPIO_STATUS_ADDRESS);

	counter++;

	GPIO_OUTPUT_SET(14, 1);

	// Let the interrupt capacitor discharge after 3 milliseconds
	os_timer_disarm(&osRechargeCapTask_timer);
	os_timer_arm(&osRechargeCapTask_timer, 100, 1);

	//clear	interrupt	status
	GPIO_REG_WRITE(GPIO_STATUS_W1TC_ADDRESS, gpio_status);

	// Add some delay to make sure interrupt GPIO is high before ending, It crashes otherwise
	//os_delay_us(10);
}


void ICACHE_FLASH_ATTR  user_init(void) {
	
    os_printf("SDK version:%s\n", system_get_sdk_version());
	
	// Configure the Wifi connection
    struct station_config stationConf;
	wifi_set_opmode_current(STATION_MODE);
	os_memset(&stationConf, 0, sizeof(struct station_config));
	os_sprintf(stationConf.ssid, WIFI_SSID);
	os_sprintf(stationConf.password, WIFI_PASS);
	wifi_station_set_config_current(&stationConf);
	wifi_set_event_handler_cb(wifi_handle_event_cb);
	wifi_station_connect();

	// Configure GPIO14 to act as an interrupt to wake the IC and to run a task.
	// Running a task on interrupt instead of timers enabled the IC to go back to sleep sooner.
	PIN_FUNC_SELECT(PERIPHS_IO_MUX_MTMS_U, FUNC_GPIO14);
	PIN_PULLUP_DIS(PERIPHS_IO_MUX_MTMS_U); // Or else the capacitor on the pin won't discharge.

	gpio_pin_wakeup_enable(14, GPIO_PIN_INTR_LOLEVEL);// Attach wake from light sleep when low to GPIO14
	gpio_pin_intr_state_set(14, GPIO_PIN_INTR_LOLEVEL);// Attach interrupt task to run on GPIO14
	ets_isr_attach(ETS_GPIO_INUM, (ets_isr_t)gpioInterruptTask, NULL);// ^
	ETS_GPIO_INTR_ENABLE();// Enable interrupts

	// Create the task to start the capacitor discharge phase
	os_timer_disarm(&osRechargeCapTask_timer);
	os_timer_setfn(&osRechargeCapTask_timer, (os_timer_func_t *)osRechargeCapTask, NULL);
	os_timer_arm(&osRechargeCapTask_timer, 15000, 1);

	

	// Start listening for connections
	RESTEndpointADCInit();
	RESTServerInit();
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