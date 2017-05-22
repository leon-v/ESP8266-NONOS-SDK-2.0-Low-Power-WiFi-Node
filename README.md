# ESP8266_NONOS_SDK


Shunt on + of 3.31v supply: 4.3mA & 5.15mA
Shunt on + of 18650 4.11v  supply: 6.11mA & 5.66mA (with CE021 buck boost)

Shunt on + of 18650 4.11v  supply: 4.50mA, 4.64mA (with LM3940) Mean for 13300 seconds: 3.7577mAs

LM3940 datasheet shows this is out of its operation parameters. And output was 3.2V during the test, so ill ignore those results.
I have purchased a cheap, and likely fake AMS1117 module to check its efficiency.

CE021 buck boost seems to be getting more efficient as the 18650 voltage drops a bit. At 4.04V as of writing this and a mean for 1000 seconds of 4.3000mA.
I have removed the capacitor from the Vin of the CE021 module, and added a single large low ESR capacitor (10V 2200uF) to the 3.3V rail. This may be where the efficiency is coming from. since the regulator is likely working less of the time.

I have just added a basic web server to the code to make sue that the code will respond to TCP requests, And it does, exactly as expected.
The ESP8266 only responds to the HTTP request on the interrupt. Which is fine. It just appears as a slow web server.

I am getting issues where the IC isn't going to sleep.
It appears to keep the CPU awake and uses an extra ~2mA.

After a long time (1 minute or so) it does appear to go back to its usual sleep cycles.
Scrap that, it still does go into a state where it will not go back to sleep.

I added `wifi_fpm_set_sleep_type(NONE_SLEEP_T);` and `wifi_fpm_set_sleep_type(LIGHT_SLEEP_T);` which i think is resetting some internal circuirty and forcing it to go back to sleep.

I have tried a few things to make it go into the state where it doesn't go back to sleep, but it only seems to do it until the next interrupt. Which is good.

Been running another battery powered test.. Getting 2.9mA mean current drain. And that was with some reboots.

