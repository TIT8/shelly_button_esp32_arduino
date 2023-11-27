# Introduction

This is a simple copy of what [Shawn Hymel](https://github.com/ShawnHymel) did on [Youtube](https://www.youtube.com/watch?v=fRSVQ4Fkwjc), nothing more. 
I've trained a machine learning model on [Edge Impulse](https://edgeimpulse.com/) to recognize the keywords _"accendi-luce"_ and _"spegni-luce"_ in my speech. Then Edge Impulse gave to me an [Arduino library](https://github.com/TIT8/shelly_button_esp32_arduino/blob/master/speech_recognition/ei-speech_recognition-arduino-1.0.9.zip). 

Using the Arduino IDE I was able to upload the code to the Arduino Nano 33 BLE Sense board and Arduino Nano RP2040 Connect. Inside it, there is a look up table, generated during the training sessions, used to make inference.

It works pretty well, sometimes perfectly, sometimes less perfectly. Impressive.

## The interesting part

Until now all is a copy of something done from others, the [PDM library](https://github.com/arduino/ArduinoCore-mbed/tree/main/libraries/PDM) of Arduino hides the interesting part. 
So the CPU receive the PDM sample. It has to decimate and filter the train of data, then make inference on the buffered data and toggle the LED. All happens while the PDM microphone fill the data buffer continuously, it doesn't wait the CPU. HOW?!?!

- On the nrf52840 there is a [special dedicated hardware](https://infocenter.nordicsemi.com/index.jsp?topic=%2Fcom.nordic.infocenter.nrf52832.ps.v1.1%2Fpdm.html) to handle the digital signal part, buffering the data in memory and raising interrupt when the buffer is full via the [EasyDMA](https://infocenter.nordicsemi.com/index.jsp?topic=%2Fcom.nordic.infocenter.nrf52832.ps.v1.1%2Fppi.html) module. This makes the Nordic chip really fast and advanced to use with sensor + Sigma Delta Modulator (as ADC). The documentation is really well written. So the [PDM library on the nrf52](https://github.com/arduino/ArduinoCore-mbed/tree/main/libraries/PDM/src/nrf52) use the HAL drivers from Nordic to handle the events when [interrupt arrive](https://github.com/arduino/ArduinoCore-mbed/blob/main/libraries/PDM/src/nrf52/PDM.cpp#L196). Remember that your application is running as a [task](https://github.com/arduino/ArduinoCore-mbed/blob/main/libraries/PDM/src/nrf52/PDM.cpp#L132) of MBED OS.

- On the rp2040 there's not a dedicated hardware to receive the PDM samples and process them. But you can use the PIO interface: programmable hardware (yes, silicon inside the package) to make communication with external peripheral easy. So the [PDM library on the rp2040](https://github.com/arduino/ArduinoCore-mbed/tree/main/libraries/PDM/src/rp2040) is focused on using one of the four PIO to receive the samples and talking to the DMA to foward them in the main memory. Then you have to do DSP and here you will have to implement it in [software](https://github.com/arduino/ArduinoCore-mbed/blob/main/libraries/PDM/src/rp2040/OpenPDMFilter.c) inside the CPU. 

❗**Note the difference: the nrf52 has a dedicated hardware to make the PDM to PCM conversion, filter the samples and decimate it**. 

## Rule of buffers

So the EasyDMA will raise an interrupt to the CPU when the transfer is complete. The data in the buffer will be already processed.  

In the rp2040 the PIO make it easy to interface with the PDM train on the input bus, then the DMA will raise the interrupt. But now the CPU has to process the data inside the [interrupt service routine](https://github.com/arduino/ArduinoCore-mbed/blob/main/libraries/PDM/src/rp2040/PDM.cpp#L206) (via the OPENPDMFilter library). This will consume CPU time and without FPU it will be long enough to make very slow the rp2040 compared to the nrf52840 (indipendently from number of core and clock frequencies, ***here what matter are the hardware accelerators***).

Interesting also the [double buffer](https://github.com/arduino/ArduinoCore-mbed/tree/main/libraries/PDM/src/utility) implementation to overwrite continuously the data inside it.

❗**Note the similarities: the PDM callback in either case will be runned in the IRQ handler, so do not block inside it**.

Here the Edge Impulse code copy the microphone buffer to a buffer used to and it will process the audio data inside it. Now this part, unlike before, is CPU intensive for both, nrf52 and rp2040, but as before the _one-slow-core-with-FPU_ nrf52 will be faster than the _two-fast-cores-without-FPU_ rp2040. Also here the accelerators make the difference, as described in the section below.

## Choose the right CPU

The rp2040 is a great CPU, the PIO hardware is a great feature, but without the floating point unit, the intensive digital signal processing on the audio samples becomes too much for the ARM Cortex MO+ inside it. So it will end up using too much time to process the sample in the buffer and make inference only after a great latency. You will notice it. 

Even if the rp2040 is a dual core CPU at 133 MHz, the ARM Cortex M4 inside the nrf52 is faster in the digital signal processing part thanks to the FPU, making possible the inference on the readily available sampled data with very low latency. As a human, you can't feel it. 

In the table below I list the speed results from my tests. The Raspberry pi 4 is just for reference:

| CPU | RP2040 | nrf52 | RPI 4 |
| ---- | :----: | :----: | :----: |
| ISA | ARM Cortex M0+ | ARM Cortex M4 | ARM Cortex-A72 |
| CPU specs | <ul><li>Dual core 133 MHz</li><li>Without FPU</li><li>PIO for PDM samples + DMA</li></ul> | <ul><li>One core 64 MHz</li><li>With FPU</li><li>PDM hardware + Easy DMA</li></ul> | <ul><li>Quad core 1.8 GHz</li><li>With FPU and more</li><li>External MIC interface</li></ul> |
| DSP time [^1] | 970 ms | 280 ms | 10 ms |
| Inference time | 6 ms | 5 ms | 1 ms |

Moving to faster CPU is not always better. This is why in today CPU, the AI part is made faster via special processor (directly training it on the CPU, not in the cloud).

[^1]: Data coming from Edge Impulse. The real behaviour follows the data, from my tests.

## Tweak the probabilities for your use case

You can check your probabilities to make actions after the inference:

```C
if(result.classification[2].value >= 0.99) {
  digitalWrite(LED_BUILTIN, HIGH);
} else if(result.classification[3].value >= 0.99) {
  digitalWrite(LED_BUILTIN, LOW);
}
```

Here is 99%, to make sure. You can lower it, but then you must expect lower quality (for example "spegni" can tweak the probability for "spegni-luce" up to 93% according to my tests).

## The end

If you have to send a command over Bluetooth (on Arduino Nano 33 Ble sense) or WIFI (on Arduino Nano rp2040 Connect) to the Shelly device, then you will send it in the probabilities check section of the loop function.

- On the rp2040 you can use the second core (unlike the computation part, here you can benefit from the parallel cores). It can communicate with the Nina-EPS32 module via SPI and send the command to the MQTT broker for example.
- On the nrf52 you will use the MBED driver to talk to the Bluetooth driver and send the message. But here you have one core, so this will decrease the overall performance. So you will end up using an RTOS to switch back and forth (when needed) between the sending task and the inference task and make the illusion of the concurrency on only one core (like it was in the past to go on the Moon). The [Arduino Core for nrf52](https://github.com/arduino/ArduinoCore-mbed/tree/main/cores/arduino/mbed) is already built on MBED OS and will use CMSI-RTOS for Bluetooth.
