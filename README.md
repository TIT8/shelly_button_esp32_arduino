# Arduino code for Shelly's toggle button

A push button is connected to the [ESP32](https://github.com/espressif/arduino-esp32) microcontroller. When its state changes, it will trigger an [MQTT](https://mqtt.org/) publish which will toggle the light controlled by a `Shelly plus 1 relay`. 

❗Keep in mind that you should either use a capacitor (better) or providing some delay in the code to debounce the push button and filter out spurious changes. Pull down the push button via a 10k resistor, **if you don't set the pull-up mode on the input pin**.

<br>

<p align="center"><img src="https://github.com/TIT8/shelly_esp32_button_espidf/assets/68781644/42b67ad6-4091-4f7f-9a1e-e24e876d9295" alt="Schematich" width='600' /></p>

<br>

If you have a [Shelly](https://www.shelly.com/en-it/products/switching-and-triggering#unfiltered) in your wall and want to try out the MQTT connection without purchasing or wiring up the micontroller, I've saved a prototype of the project on ***[Wokwi](https://wokwi.com/projects/380235936487757825)***.

- For testing I use the [Hive MQ public broker](https://www.hivemq.com/mqtt/public-mqtt-broker/).

- For production I use [Mosquitto](https://mosquitto.org/) from a Docker container inside my local environment (see the [Docker compose file](https://github.com/TIT8/shelly_button_esp32/blob/master/compose.yaml)).

❗ The code should already handle reconnections if something goes wrong.

## Prerequisities

1. [Arduino IDE](https://docs.espressif.com/projects/arduino-esp32/en/latest/installing.html#installing-using-arduino-ide) to code, compile and upload the code to the ESP32 Wroom board.
2. Remember to add the [CP2102 driver](https://www.silabs.com/developers/usb-to-uart-bridge-vcp-drivers?tab=downloads) to connect old ESP32 development board (with CP2102 as USB-UART bridge).
3. [ArduinJson](https://arduinojson.org/), [pubsubclient](https://github.com/knolleary/pubsubclient) and [ArduinoOta](https://github.com/espressif/arduino-esp32/tree/master/libraries/ArduinoOTA) libraries.
4. [How to](https://github.com/sukesh-ak/setup-mosquitto-with-docker) setup a local broker in a Docker container.

## Future 

- ~~I will make the ESP32 aware of the ligth bulb status subscribing to the rpc channel/topic where the Shelly's relay automatically publish the status (on or off).~~ &nbsp; [DONE ✔️]

- ~~I will make possible an initial speech recognition through a microphone and a Keras neural network (via [Eddge Impulse](https://edgeimpulse.com/) and [Colab](https://colab.research.google.com/) for training it), like in [this video](https://www.youtube.com/watch?v=fRSVQ4Fkwjc) from DigiKey.~~ &nbsp; [[DONE ✔️](https://github.com/TIT8/shelly_button_esp32_arduino/tree/master/speech_recognition)]

- ~~I won't use the [official ESP-IDF](https://github.com/espressif/esp-idf) with the built-in MQTT library (or via [Cesanta Mongoose MQTT](https://mongoose.ws/documentation/tutorials/mqtt-client/) library) because is too much power for this project.~~ &nbsp; [[DONE ✔️](https://github.com/TIT8/shelly_esp32_button_espidf/tree/master)]

- Using Kicad, directly create a PCB with push button, microphone + opamp (considering noise constraint) and microcontroller on the same board.

## Why use C++ and not Micropython or Rust?

The answer lies in the low level of the libraries used. Python will depend on low level C code, while Rust can go to bare metal, but it's by far more tedious. Try doing [this](https://wokwi.com/projects/362016607277953025) on Rust, I find it better on C++.  

Then the library support for microcontroller is not yet ready as in C/C++. 

Take a look also at the [ESP-IDF version](https://github.com/TIT8/shelly_esp32_button_espidf/tree/master) of this project. Don't look too much at it, otherwise you won't never come back to Arduino 😍 🙈.

## Shelly options

- Useful [installation video](https://www.youtube.com/watch?v=-i3d_4FLR0k) for Shelly's relays.
- How Shelly handle MQTT connections in the [official doc](https://shelly-api-docs.shelly.cloud/gen2/ComponentsAndServices/Mqtt#mqtt-control).

![Screenshot (31)](https://github.com/TIT8/shelly_button_esp32/assets/68781644/e6de6e83-4aeb-428b-a845-5be89e2eb7bd)

