# esp_firmware_v2

Custom ESP8266 Firmware for SubPos Nodes Based on esp_iot_sdk_v1.3.0_15_08_08

This release uses the wifi_send_pkt_freedom() function to generate beacon frames 
instead of using the AP mode. These commands are the same as the regular AT 
commands, except they utilise the beacon generator instead.

AT+CWSAPID:
Set parameters of beacon generator
AT+CWSAPID="<ssid>",<channel num>

AT+CWSAPCH: 
Change beacon channel.
AT+CWSAPCH=<channel num> 

This method of generation allows for tighter control of beacon frame rates and 
increases security.
