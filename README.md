# MBusino 5S - Enhanced Fork
[![version](https://img.shields.io/badge/version-2.0.0-brightgreen.svg)](CHANGELOG.md)<br/>

> **Fork Notice:** This is a fork of the fantastic original [MBusino](https://github.com/Zeppelin500/MBusino) project by Zeppelin500. It focuses entirely on the 5-Slave (5S) version and introduces a completely redesigned responsive Web-UI, a dynamic JSON Profile Manager, and ESP-NOW support.
Please note, that i cannot provide Support - be aware that it possibly would take Time for me to reply and/or implement Fixes if Bugs occour.

### M-Bus --> MQTT-Gateway with shields for ESPs

A **Plug and Play** solution for smart home integration.

M-Bus decoding uses the library [**MBusinoLib**](https://github.com/Zeppelin500/MBusinoLib). The serial data link layer communication is handled by [**MBusCom**](https://github.com/Zeppelin500/MBusCom).

## ✨ New Features in this Fork

* **Completely Redesigned Web-UI:** A modern, dark-themed, and fully mobile-responsive interface. 
* **Dynamic Profile Manager:** No more hardcoded profiles! Upload custom `.json` M-Bus profiles directly via the Web-UI, assign them to slaves, or download system templates to edit them.
* **ESP-NOW Integration:** Send sensor and M-Bus data directly to other ESP-NOW capable devices without needing an MQTT broker in between.
* **Smart Calibration UI:** Easily calibrate your OneWire DS18B20 sensors to an average value or a specific target temperature with a single click.
* **Bulletproof Layout:** Fixed severe template engine bugs from the underlying webserver that previously broke the UI on mobile devices.

## Hardware & Setup

This fork supports the same hardware as the original MBusino 5s. 
- M-Bus e.g. heatmeter (up to five slaves)
- OneWire 5x DS18B20, temperature
- I²C BME280, temperatur, r. humidity, air pressure

You will find the original provided 3D-printable PCB case inside the `case` folder.

### Access Point to configure
* SSID **MBusino Setup Portal** IP(normally not needed): 192.168.4.1
* If Mbusino does not find a known network, it starts an AP for 5 minutes. After this period, it will restart and search again.

## Credits & License
* **Original Creator:** Zeppelin500 (Thank you for the amazing work!)
* **UI/UX & Feature Enhancements:** [aut0mat3d](https://github.com/aut0mat3d)
* AllWize for the MbusPayload library
* HWHardsoft and TrystanLea for the M-Bus communication

This program is free software: you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, either version 3 of the License, or (at your option) any later version.
