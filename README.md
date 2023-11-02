# Arduino code for Shelly's toggle button

A push button is connected to the [ESP32](https://github.com/espressif/arduino-esp32) microcontroller. When its state changes, it will trigger an MQTT publish which will toggle the light controlled by a Shelly plus 1 relay on and off. 

❗Keep in mind that you should either use a capacitor (better) or providing some delay in the code to debounce the push button and filter out spurious changes.

<br>

<p align="center"><img src="https://github.com/TIT8/shelly_button_esp32/assets/68781644/ac7b491a-51ff-4d75-a617-74396759e7a7" alt="Schematich" width='300' /></p>

<br>

If you have a [Shelly](https://www.shelly.com/en-it/products/switching-and-triggering#unfiltered) in your wall and want to try out the MQTT connection without purchasing or wiring up the micontroller, on [Wokwi](https://wokwi.com/projects/380235936487757825) I've saved a prototype of the project.

For testing I use the [Hive MQ broker](https://www.hivemq.com/mqtt/public-mqtt-broker/).


## Future 

- I will make the ESP32 aware of the ligth bulb status subscribing to the rpc channel/topic where the Shelly's relay automatically publish the status (on or off).

- I will make possible an initial speech recognition through a microphone and a Keras neural network (via [Eddge Impulse](https://edgeimpulse.com/) and [Colab](https://colab.research.google.com/) for training it), like in [this video](https://www.youtube.com/watch?v=fRSVQ4Fkwjc) from DigiKey.

- Using Kicad, directly create a PCB with push button, microphone + opamp (considering noise constraint) and microcontroller on one board.


## Prerequisities

- I've used [Arduino IDE](https://docs.espressif.com/projects/arduino-esp32/en/latest/installing.html#installing-using-arduino-ide) to code, compile and upload the code to the ESP32 Wroom board.

- Remember to add the [CP2102 driver](https://www.silabs.com/developers/usb-to-uart-bridge-vcp-drivers?tab=downloads) to connect old ESP32 development board (with CP2102 as USB-UART bridge).
