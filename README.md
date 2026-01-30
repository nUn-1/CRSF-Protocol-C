# CRSF-Protocol-C
The Crossfire Protocol (CRSF) is a serial communication protocol primarily used in the FPV drones for ultra-low latency and long-range control links. This C code helps to decode and transmit messages over a single serial interface connected to the Flight Controller.

Reading and writing frames executed on different threads. Only RC packed message is supported for tx. Use with your program by updating the message.

Tested on Orange Pi5 Max board (Rockchip RK3588 ,ARM CPU) and Skystars F7 Mini HDPro , Betaflight Flight Controller (Firmware 2025.12.1)

1. Install serial library  
sudo apt install libserialport-dev  

2. Update the interface /dev/tty , configure the flight controller to send CRSF telemetry. Update the arm and disarm messages according to your thresholds in BetaFlight , build your own custom messages.

3. Compile with -lserialport

## 

### Helpful info on CRFS

Protocol was developed by Team BlackSheep. Here is its specifications. And basic addressesing index.

https://github.com/tbs-fpv/tbs-crsf-spec/blob/98f4d71e/crsf.md

0x00 / 0	CRSF_ADDRESS_BROADCAST	Broadcast (all devices process packet)  
0x10 / 16	CRSF_ADDRESS_USB	?  
0x12 / 18	CRSF_ADDRESS_BLUETOOTH	Bluetooth module  
0x80 / 128	CRSF_ADDRESS_TBS_CORE_PNP_PRO	?  
0x8A / 138	CRSF_ADDRESS_RESERVED1	Reserved, for one  
0xC0 / 192	CRSF_ADDRESS_CURRENT_SENSOR	External current sensor  
0xC2 / 194	CRSF_ADDRESS_GPS	External GPS  
0xC4 / 196	CRSF_ADDRESS_TBS_BLACKBOX	External Blackbox logging device  
0xC8 / 200	CRSF_ADDRESS_FLIGHT_CONTROLLER	Flight Controller (Betaflight / iNav)  
0xCA / 202	CRSF_ADDRESS_RESERVED2	Reserved, for two  
0xCC / 204	CRSF_ADDRESS_RACE_TAG	Race tag?  
0xEA / 234	CRSF_ADDRESS_RADIO_TRANSMITTER	Handset (EdgeTX), not transmitter  
0xEC / 236	CRSF_ADDRESS_CRSF_RECEIVER	Receiver hardware (TBS Nano RX / RadioMaster RP1)  
0xEE / 238	CRSF_ADDRESS_CRSF_TRANSMITTER	Transmitter module, not handset  
0xEF / 239	CRSF_ADDRESS_ELRS_LUA	!!Non-Standard!! Source address used by ExpressLRS Lua  