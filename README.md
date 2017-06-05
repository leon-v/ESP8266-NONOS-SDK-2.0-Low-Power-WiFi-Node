Low power real time ESP8266 with REST interface.


The Environment:
So you understand this build environment, here is its description.

I am using Windows with the linux bash shell. crosstoon-NG is installed in /opt/Espresiif.
There is a script in /src/misc/getcompiler.sh to install dependencies and install required tools.

I am editing with Sublime text 3 and have a build script to simply execute gen_misc.sh in the bash shell.

You may well need to edit gen_misc.sh to suit your device. I have an ESP12E using the generic FTDI RS232 to USB module.

There is a test REST script ESP_Test.html is an example of how to make a REST AJAX request from a browser.
This is the script i use while doing the power analysis to give it some real world conditions.

The Software:
This firmware will eventually be able to take and send REST requests when events are triggered.
So far i have only thought about hot the ADC will be configured.

It will respont to request to /adc/0 thought to /adc/7
The ADC will be attached to an 74HC4052 which will be used to switch between channels.
Each channel will have:
A Maximum - Will not fire events over this value
A Minimum - Will not fire events under this value
A Threshold - When the ADC value goes +- this value, an event will be triggered and that value will be used to compare against to trigger the next event
An Interval - If an event is not fired within this time, an event will be triggerd
Endpoint - The URL to the endpoint that this ADC will make a request to when firing an event.

The 74HC4052 will run from a power circuit that will be switch on and off, and i will be trying to charge a large capacitor, and disconnecting the power source at a predictable time before taking a reading so there is a stable / low noise power source to the analog circuits.
The ESP8266 can make a hell of a lot of noise with its radio.


The Hardware.
I am using an ESP12E soldered onto a DIP breakout which has requisite pull down/ups.

The ESP8266 will go into Auto Light sleep, and needs to be woken via an external interrupt.
The interrupt is triggered by a discharging capacitor on GPIO14.
After a lot of testing with various ways to trigger the wake, including using my scope to test many different signals.
The best results by far were by using a capacitor on GPIO14.
You can't just put the cap directly on GPIO14, there must be a resistor in between.

I am using a 10Kohm resistor on GPIO14, through a 4.7uF capacitor to ground. With a 1Mohm across it to discharge the capacitor.
The IC will crash if it gets a current surge on GPIO14, so don't go below 10K attached to it.
Other resistor & capacitor values can be changed to get your timing right.
I found that a 5 second delay is OK. The delay should only affect the amount 