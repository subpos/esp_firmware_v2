# esp_firmware_v2

Custom ESP8266 Firmware for SubPos Nodes Based on ESP8266_NONOS_SDK_V1.4.0_15_09_18

http://bbs.espressif.com/viewtopic.php?p=3756#p3756

This release uses the wifi_send_pkt_freedom() function to generate beacon frames 
instead of using the AP mode. These commands are the same as the regular AT 
commands, except they utilise the beacon generator instead.

AT+CWSAPID:
Set parameters of beacon generator
AT+CWSAPID="<ssid>",<channel num>

AT+CWSAPCH: 
Change beacon channel.
AT+CWSAPCH=<channel num> 

This method of generation allows for tigher control of beacon frame rates and 
increases security.
