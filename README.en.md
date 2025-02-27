#### Language Selection:
[![en](https://img.shields.io/badge/lang-en-red.svg)](README.en.md) 
[![de](https://img.shields.io/badge/lang-de-blue.svg)](README.md)

# Rotex / Daikin - HPSU CAN/Serial
**Daikin-Rotex-HPSU-CAN-Serial** is a flexible standalone solution for reading and controlling Rotex/Daikin air-to-water heat pumps via the CAN bus and/or the [serial interface](https://github.com/wrfz/esphome-components).
Control and data access can be achieved either through the built-in web server or conveniently via Home Assistant or ioBroker.
Multilingual support makes it possible to flexibly define all sensors in the desired language.

## Required Hardware
An **ESP32** is required as the hardware, which supports a baud rate of **20 kbit/s**.
- A sleek solution, ideal for those with little experience in ESP or DIY projects, is the click/plug system [ATOM S3 Lite](https://docs.m5stack.com/en/core/AtomS3%20Lite) combined with an [ATOM CAN Module](https://docs.m5stack.com/en/atom/atom_can).
- Similarly, an [AtomS3U](https://docs.m5stack.com/en/core/AtomS3U) with a [Unit Mini CAN](https://docs.m5stack.com/en/unit/Unit-Mini%20CAN) can be set up easily.
- Successfully tested options also include the **ESP32-S3-WROOM** board and a **WaveShare ESP32-S3 Mini** in combination with a compatible **Waveshare SN65HVD230** (3.3V) CAN transceiver.

An overview of supported ESP32 variants and their compatible bitrates can be found in this [table](https://esphome.io/components/canbus/esp32_can).


## Home Assistant - Dashboard
[![Bild 1](images/ha-dashboard-thumb.png)](images/ha-dashboard.png)

The [HPSU Dashboard for Home Assistant](https://github.com/wrfz/daikin-rotex-hpsu-dashboard) is an add-on that helps to easily understand the heat pump's operation in real time.

## Home Assistant - Sensors
[![Bild 1](images/ha-can-sensors-small.png)](images/ha-can-sensors.png)
[![Bild 1](images/ha-uart-sensors-small.png)](images/ha-uart-sensors.png)

The CAN and serial data from the refrigeration circuit enhance understanding of the pump's operation and make it easier to quickly diagnose issues in case of malfunctions.

## Home Assistant - Settings
[![Bild 1](images/ha-settings-small.png)](images/ha-settings.png)
[![Bild 1](images/ha-settings-small-2.png)](images/ha-settings-2.png)

Both standard settings, such as target temperatures for domestic hot water or space heating, and advanced configurations affecting the pump's core behavior can be conveniently adjusted remotely.

## Integrated ESP Web Server (Usable without Home Assistant)

[![Bild 1](images/esp-webserver-thumb.png)](images/esp-webserver.png)
[![Bild 1](images/esp-webserver-settings-thumb.png)](images/esp-webserver-settings.png)

The integrated ESP web server is ideal for less tech-savvy users seeking a straightforward solution to remotely control the heat pump and quickly access all relevant data without relying on Home Assistant.
<br>

# Warning!!

Using Daikin-Rotex-HPSU-CAN can potentially damage your heating system. Use it at your own risk. I accept no liability for any resulting damage.

Please note that using Daikin-Rotex-HPSU-CAN may void your warranty and manufacturer support!

<br><br>

# Rotex/Daikin HPSU CAN Integration with ESPHome and Home Assistant

This guide describes how to integrate your Rotex HPSU Compact via CAN bus using an ESP32 and ESPHome into Home Assistant without multiple flashing steps.

### Requirements

- Home Assistant with the ESPHome Add-on
- ESP32-S3 microcontroller
- USB-C cable for the initial connection

## Step 1: Install the ESPHome Add-on in Home Assistant

1. Open Home Assistant and navigate to **Settings > Add-ons > ESPHome**.
2. Install the ESPHome Add-on and start it afterward.
3. Open the ESPHome Add-on user interface.

## Step 2: Configure the ESP32 in ESPHome and Initial Flashing

1. In the ESPHome interface, click on **“New Device”**.
2. Enter a name for the device (e.g., `Rotex_HPSU`) and select the **ESP32-S3** platform.
3. Enter your Wi-Fi SSID and password so the ESP32 can connect to your home network later.
4. Download the configuration file required for the initial flashing.
5. **Connect the ESP32 to the computer running Home Assistant using a USB-C cable** (or any other computer on the network with ESPHome).
6. If Home Assistant is installed in a virtual machine, ensure the USB ports are properly passed through.
7. Select **“Plug into this computer”** and follow the instructions to flash the ESP32 directly from ESPHome.
8. After successful flashing, the ESP32 will automatically connect to your Wi-Fi and appear in the ESPHome device list in Home Assistant.

## Step 3: Update the Configuration in ESPHome and Flash Wirelessly

1. In ESPHome, select the added device (e.g., `Rotex_HPSU`) and click **“Edit”** to open the configuration file.
2. Replace the file’s content with the configuration from [examples/full_en.yaml](https://github.com/Trunks1982/Daikin-Rotex-HPSU-CAN-Seriell/blob/main/examples/full_en.yaml), optimized for your Rotex HPSU Compact.
3. Save the changes and click **“Install”**. The configuration will be sent wirelessly to the ESP32, eliminating the need for another USB flash.

## Step 4: Integrate the ESP32 into Home Assistant

1. The ESP32 should automatically be detected as a new device in Home Assistant.
2. Navigate to **Settings > Devices & Services** and verify if the device (e.g., `Rotex_HPSU`) has been added.
3. The configured sensors and controls should now be available in Home Assistant and ready to use.



## Conclusion

The installation is now complete, and you can connect the ESP32 to the Rotex/Daikin system as shown in the wiring diagrams.


**Note**: Additional sensors or controls can easily be added by including new entries in the ESPHome YAML configuration.

<br><br>

# Installation Guide (Standalone):

### Step 1: Preparation

1. **Browser:** Use Google Chrome (Safari and Firefox are not supported).
2. **Firmware:** Download the file [rotex.factory-tx05-rx06.bin](bin/rotex.factory-tx05-rx06.bin) from the bin folder.

### Step 2: Flashing the ESP32

1. Open the website https://web.esphome.io.
2. Connect the ESP32 to your PC using a USB-C cable.
3. Click "**Connect**" on the website and select the USB serial interface in the popup window.
4. Then, click "**Install**".
5. Select the downloaded file `rotex.factory-tx05-rx06.bin` and click "Install" again. The process will take about 2 minutes.

### Step 3: Configuring the ESP32 Wi-Fi Connection

1. Restart the ESP32 and wait about 1–2 minutes until the "Rotex Fallback Hotspot" is created by the ESP32.
2. Connect to the hotspot (password: `H3jGqANSa7aL`).
3. Open the website http://192.168.4.1 or http://rotex.local in your browser.
4. Enter your Wi-Fi SSID and Wi-Fi password to connect the ESP32 to your network.

### Step 4: Verifying the Network Connection

- The ESP32 should now be accessible via http://rotex.local.
- If this doesn't work, check the ESP32's IP address in your router.

## Conclusion

The installation is now complete, and the system can be connected to the Rotex/Daikin unit following the **GPIO 5 and 6 pinout diagram (bin file)**.

<br><br>

## Features:

- Single Hot Water Button without Heating Element (sets to 70°C for 10 seconds and then reverts to the previously set temperature).
- Thermal performance calculation.
- Adjustment of heating curves in 0.01 increments (standard is 0.1 increments).
- Two new switches to toggle heating/cooling mode via the thermostat inputs on the Rotex/Daikin.
- Error Code Display with error descriptions and manual page references.

<br>


For defrosting issues, you can use the new Defrost feature directly via the ESP. This uses a maximum of 0.7°C of hot water from the storage tank.


Tested With:
- Rotex HPSU Compact 508 8kW with Rocon BM1
- Rotex HPSU Compact Ultra
- Daikin ECH2O (up to 01/2022)

## GPIO 5 and 6 Pin Mapping (Binary File)

![TxPin5_RxPin6](https://github.com/user-attachments/assets/b0e3ae0d-2354-4871-b295-c156836afddf)

## Pin Layout for the Rotex HPSU

![Rotex CAN Anschluss](https://github.com/Trunks1982/Daikin-Rotex-HPSU-CAN/assets/62701386/05c36ae7-ddc9-4a1e-8a73-4559c765f132)



## DIY Circuit Board by (Dornieden)

For more details, visit the discussion forum: https://github.com/Trunks1982/Daikin-Rotex-HPSU-CAN/discussions/35:<br> 
https://github.com/Trunks1982/Daikin-Rotex-HPSU-CAN/blob/main/DIY-Platine/README

<img width="554" alt="357024972-fda9cc55-63de-4862-963a-ea39f58e5028" src="https://github.com/user-attachments/assets/1640db7a-693b-45a5-8315-dcbc2e84892c">



# Telegram Group Chat
https://t.me/+C1iVhAjaENw0ODY0

<br><br>
A coffee for the next late night in front of the computer would be appreciated but is, of course, not mandatory.
<br>[![Paypal](https://user-images.githubusercontent.com/41958506/212499642-b2fd097a-0938-4bfc-b37b-74df64592c58.png)](https://www.paypal.com/donate/?hosted_button_id=H94LZRQW9PFQ4)
