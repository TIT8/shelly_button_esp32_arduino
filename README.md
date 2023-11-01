# Arduino code for Shelly's toggle button

A push button is connected to the [ESP32](https://github.com/espressif/arduino-esp32) microcontroller. When its state changes, it will trigger an MQTT publish which will toggle the light controlled by a Shelly plus 1 relay on and off.

<br>

<p align="center"><img src="https://github.com/TIT8/shelly_button_esp32/assets/68781644/ac7b491a-51ff-4d75-a617-74396759e7a7" alt="Schematich" width='300' /></p>

<br>

If you have a [Shelly](https://www.shelly.com/en-it/products/switching-and-triggering#unfiltered) in your wall and want to try out the MQTT connection without buying or wiring up the micontroller, on [Wokwi](https://wokwi.com/projects/380235936487757825) I've saved a prototype of the project.

For testing I use the [Hive MQ broker](https://www.hivemq.com/mqtt/public-mqtt-broker/).

## Future 

In the future I will make the ESP32 aware of the ligth bulb status subscribing to the rpc channel/topic where the Shelly's relay automatically publish the status (on or off).

I hope to also have the time to set a MQTT broker on my personal rasperry pi at home
