# Introduction

This document is a reproduction of [Shawn Hymel](https://github.com/ShawnHymel)'s work on [Youtube](https://www.youtube.com/watch?v=fRSVQ4Fkwjc), with no additional content. I've trained a machine learning model on [Edge Impulse](https://edgeimpulse.com/) to recognize the keywords _"accendi-luce"_ and _"spegni-luce"_ in my speech. Edge Impulse provided me with an [Arduino library](https://github.com/TIT8/shelly_button_esp32_arduino/blob/master/speech_recognition/ei-speech_recognition-arduino-1.0.9.zip).

Using the Arduino IDE, I successfully uploaded the code to both the Arduino Nano 33 BLE Sense board and Arduino Nano RP2040 Connect. Inside these devices, a lookup table is used to make inferences on the trained model (quantized, uint8).

The system works well, achieving impressive results, sometimes perfect and sometimes less.

<br>

## The Interesting Part

The [PDM library](https://github.com/arduino/ArduinoCore-mbed/tree/main/libraries/PDM) of Arduino conceals the fascinating details. The CPU receives PDM samples, decimates and filters the data train, then makes inferences on the buffered data, toggling the LED. All of this happens while the PDM microphone continuously fills the data buffer without waiting for the CPU. How does it work?

- On the nrf52840, there is a [special dedicated hardware](https://infocenter.nordicsemi.com/index.jsp?topic=%2Fcom.nordic.infocenter.nrf52832.ps.v1.1%2Fpdm.html) to handle the digital signal part. It buffers the data in memory and raises an interrupt when the buffer is full via the [EasyDMA](https://infocenter.nordicsemi.com/topic/com.nordic.infocenter.nrf52832.ps.v1.1/easydma.html?cp=5_2_0_9#easydma) module. The [PDM library on the nrf52](https://github.com/arduino/ArduinoCore-mbed/tree/main/libraries/PDM/src/nrf52) utilizes the HAL drivers from Nordic to handle events when [interrupts arrive](https://github.com/arduino/ArduinoCore-mbed/blob/main/libraries/PDM/src/nrf52/PDM.cpp#L196). Note that the application runs as a [task](https://github.com/arduino/ArduinoCore-mbed/blob/main/libraries/PDM/src/nrf52/PDM.cpp#L132) of MBED OS.

- On the rp2040, there's no dedicated hardware to receive PDM samples and process them. However, the PIO interface, a programmable hardware feature, facilitates communication with external peripherals. Therefore, the [PDM library on the rp2040](https://github.com/arduino/ArduinoCore-mbed/tree/main/libraries/PDM/src/rp2040) focuses on using one of the four PIOs to receive samples and talks to the DMA to forward them in the main memory. The DSP must be implemented in [software](https://github.com/arduino/ArduinoCore-mbed/blob/main/libraries/PDM/src/rp2040/OpenPDMFilter.c) inside the CPU.

❗**Note the difference: the nrf52 has dedicated hardware to perform the PDM to PCM conversion**.

<br>

## Rule of Buffers

So the EasyDMA will raise an interrupt to the CPU when the transfer is complete and the data in the buffer will already be processed.

In the rp2040, the PIO makes it easy to interface with the PDM train on the input bus. Then, the DMA will raise the interrupt. However, the CPU has to process the data inside the [interrupt service routine](https://github.com/arduino/ArduinoCore-mbed/blob/main/libraries/PDM/src/rp2040/PDM.cpp#L206) (via the OPENPDMFilter library). This will consume CPU time and, without an FPU, it will be long enough to significantly slow down the rp2040 compared to the nrf52840 (independently from the number of cores and clock frequencies; ***here what matters are the hardware accelerators***).

The [double buffer](https://github.com/arduino/ArduinoCore-mbed/tree/main/libraries/PDM/src/utility) will keep the train of sample coming from the microphone. Now the Edge Impulse code copies the microphone buffer and processes the audio data inside it. This part, unlike before, is CPU intensive for both nrf52 and rp2040, but as before, the _one-slow-core-with-FPU_ nrf52 will be faster than the _two-fast-cores-without-FPU_ rp2040.  

The number of cores is useless without software able to do parallel computing on the rp2040. Also here the accelerators make the difference, as described in the section below. [^1]

❗**Note the similarities: the PDM callback in either case will run in the IRQ handler, so do not block inside it**.

[^1]: If you find errors in my way of thinking, please open an ISSUE and let me know.

<br>

## Choose the Right CPU

The rp2040 is a great CPU, and the PIO hardware is a significant feature. However, without the floating-point unit, the intensive digital signal processing on the audio samples becomes too much for the ARM Cortex MO+ inside it. Consequently, it ends up using too much time to process the sample in the buffer, resulting in inference only after a significant latency. This is noticeable.

Even though the rp2040 is a dual-core CPU at 133 MHz, the ARM Cortex M4 inside the nrf52 is faster in the digital signal processing part thanks to the FPU. This makes inference on the readily available sampled data possible with very low latency. As a human, you can't feel it.

In the table below, I list the speed results from my tests. The Raspberry Pi 4 and ESP32 are just for reference.

| CPU | RP2040 | nrf52 | RPI 4 | ESP-EYE |
| ---- | :----: | :----: | :----: | :----: |
| ISA | ARM Cortex M0+ | ARM Cortex M4 | ARM Cortex-A72 | Xtensa LX6 |
| CPU specs | <ul><li>Dual-core 133 MHz</li><li>Without FPU</li><li>PIO for PDM samples + DMA</li></ul> | <ul><li>One core 64 MHz</li><li>With FPU</li><li>PDM hardware + Easy DMA</li></ul> | <ul><li>Quad-core 1.8 GHz</li><li>With FPU and more</li><li>External MIC interface</li></ul> | <ul><li>Dual-core 240 MHz</li><li>With FPU</li><li>I2S MIC + DMA</li></ul> |
| DSP time | 950 ms | 291 ms | 5 ms | 408 ms |
| Inference time | 11 ms | 5 ms | 1 ms | 6ms |
| **Total** | **961 ms** | **296 ms** | **6 ms** | **414 ms** |

I believe that the difference between the ESP-EYE and the nrf52 is due to the _sampling-and-processing-from-the-mic_ part, instead of the audio digital processing (they both have an FPU). **The dedicated hardware on the nrf52 makes it faster also than the ESP32**, so a PDM microphone here is better than a classic microphone connected via I2S.

***Moving to a faster CPU is not always better, until the increase in clock frequency is not an order of magnitudes higher (MHz -> GHz).***

<br>

[^2]: Data coming from Edge Impulse. The real behavior follows the data, from my tests.

## Tweak the Probabilities for Your Use Case

You can check your probabilities to make actions after the inference:

```C
if(result.classification[2].value >= 0.99) {
  digitalWrite(LED_BUILTIN, HIGH);
} else if(result.classification[3].value >= 0.99) {
  digitalWrite(LED_BUILTIN, LOW);
}
```

Here is 99%, to make sure. You can lower it, but then you must expect lower quality (for example, "spegni" can tweak the probability for "spegni-luce" up to 93% according to my tests).

<br>

## Taking Actions on the World

If you have to send a command over Bluetooth or WiFi to the Shelly device, then you will send it in the probabilities check section of the loop function.

- On the rp2040, you can use the second core (unlike the computation part; here you can benefit from the parallel cores). It can communicate with the Nina-EPS32 module via SPI and send the command to the MQTT broker, for example. So one task on the first core and the other (synced via semaphores) on the other, using the CMSIS-RTOS library provided by the [scheduler wrapper](https://github.com/arduino/ArduinoCore-mbed/tree/main/libraries/Scheduler) in the Arduino Core or directly via the MBED OS.

- On the nrf52, you will use the MBED driver to talk to the Bluetooth driver and send the message. But here you have one core, so this will decrease the overall performance. So you will end up using an RTOS to switch back and forth (when needed) between the sending task and the inference task and make the illusion of the concurrency on only one core. The [Arduino Core for nrf52](https://github.com/arduino/ArduinoCore-mbed/tree/main/cores/arduino/mbed) is already built on MBED OS and will use [CMSI-RTOS](https://github.com/arduino/ArduinoCore-mbed/blob/main/variants/ARDUINO_NANO33BLE/mbed_config.h#L299) for Bluetooth. [^3]

---------------------------------

❗ **Keep in mind that communication with other devices can significantly decrease performance on the nrf52.**   
 &nbsp; &nbsp; &nbsp; Please see [errata-corrige section](#errata-corrige) ⏬.

----------------------------------

Now, the multicore architecture makes it smoother to run multiple tasks. However, on the nrf24, wireless communication may add too much overhead (I haven't tested it, but Serial read/write works well while listening and inferencing). For the nrf52, you can choose another way if Bluetooth/BLE is not fast enough, such as connecting directly to the ESP32 that handles the button or other hardware via UART.

### Errata-corrige

See [Updates](#updates) below for more information. It's important to note that you'll need a Real-Time Operating System (RTOS), and I discovered that the default CMSIS-RTOS tick frequency for Arm processors is 1 KHz. The PDM driver operates within an interrupt context triggered by a dedicated hardware accelerator (specifically, there's a special-purpose hardware on the nrf52840 I'm utilizing for PDM), which runs in the background and signals the CPU upon completion (EasyDMA proves useful for the accelerator here). Additionally, Mbed OS offers the capability to [schedule events](https://os.mbed.com/docs/mbed-os/v6.16/apis/eventqueue.html). With this in mind, you can assign high priority to the speech recognition thread and allocate a lower priority thread to handle BLE tasks. You can then schedule BLE actions on the event loop, which consistently runs in the background of the nrf52840. <ins>Therefore, I'm uncertain whether enabling BLE will compromise the functionality of speech recognition</ins>. Given the opportunity, I would conduct further testing.

<br>

## Updates :construction_worker:

Actually, I've figured it out: knowing Mbed OS better along with PDM/DMA/Serial hardware and software, I have a [working example](https://github.com/TIT8/BLE-sensor_PDM-microphone) of speech recognition in my home. The speech recognition part is not on the nrf52, but online. So totally another approach, but still useful for learning. 

<br>

[^3]: If you find errors in my way of thinking, please open an ISSUE and let me know.
