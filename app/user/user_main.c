#include "ets_sys.h"
#include "osapi.h"
#include "gpio.h"
#include "os_type.h"
#include "mem.h"
#include "user_interface.h"
#include "at_custom.h"

#define user_procTaskPrio        0
#define user_procTaskQueueLen    1
os_event_t    user_procTaskQueue[user_procTaskQueueLen];
static volatile os_timer_t beacon_timer;
uint8_t channel = 1;
uint32_t beacon_rate = 100; //Default 100ms

#define packetSize    82

// Beacon Packet buffer
//http://mrncciew.com/2014/10/08/802-11-mgmt-beacon-frame/
                                      /* Beacon Frame Type */
uint8_t packet_buffer[packetSize] = { 0x80, 0x00, 0x00, 0x00, 
                /* DST MAC (Broadcast)*/
                /*4*/   0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 
                /* SRC MAC */
                /*10*/  0x01, 0x02, 0x03, 0x04, 0x05, 0x06,  
                /* BSSID */
                /*16*/  0x01, 0x02, 0x03, 0x04, 0x05, 0x06,  
                /* Counter */
                /*22*/  0xc0, 0x6c,
                /* Timestamp */
                /*24*/  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
                /* Beacon Interval */
                /*32*/  0x64, 0x00, 
                /* Capability Info; set WEP so open networks don't accidentally get 
                stored in client Wi-Fi list when clicked (open networks allow one 
                click adding, WEP asks for password first). */ 
                /*34*/  0x11, 0x04, 
                /* SSID Element ID and Length of SSID */
                /*36*/  0x00, 0x1f, 
                /* SSID Octets */
                /*38*/  0x01, 0x01, 0x01, 0x01, 0x01, 0x01,
                /*44*/  0x01, 0x01, 0x01, 0x01, 0x01, 0x01,
                /*50*/  0x01, 0x01, 0x01, 0x01, 0x01, 0x01,
                /*56*/  0x01, 0x01, 0x01, 0x01, 0x01, 0x01,
                /*62*/  0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 
                /*68*/  0x01,
                /* Config */
                /*69*/  0x01, 0x08, 0x82, 0x84,
                /*73*/  0x8b, 0x96, 0x24, 0x30, 0x48, 0x6c, 0x03, 0x01, 
                /* Channel Num */
                /*81*/  0x01};     

 
/* Sends beacon packets. */
void beacon(void *arg)
{
    int i = wifi_send_pkt_freedom(packet_buffer, packetSize, 0);
    uint32_t timestamp = (uint32_t)packet_buffer[27] << 24 | (uint32_t)packet_buffer[26] << 16 |(uint32_t)packet_buffer[25] << 8 | packet_buffer[24];
    timestamp = timestamp + (beacon_rate * 1000);
    packet_buffer[24] = (unsigned char)(timestamp & 0xff);
    packet_buffer[25] = (unsigned char)(timestamp >> 8 & 0xff);
    packet_buffer[26] = (unsigned char)(timestamp >> 16 & 0xff);
    packet_buffer[27] = (unsigned char)(timestamp >> 24 & 0xff);
    /*if (i < 0)
    {
        at_port_print("\r\nError sending beacon frame.\r\n");
        at_response_error();
        
    } else {
        at_port_print(".");

    }*/
}

extern at_funcationType at_custom_cmd[];

uint8_t at_wifiMode;

int8_t ICACHE_FLASH_ATTR
at_dataStrLen(const void *pSrc, int8_t maxLen)
{

    //char *pTempD = pDest;
    const char *pTempS = pSrc;
    int8_t len;

    if(*pTempS != '\"')
    {
        return -1;
    }
    pTempS++;
    for(len=0; len<maxLen; len++)
    {
        if(pTempS[len] == '\"')
        {
            //*pTempD = '\0';
            break;
        }
        //else
        //{
            //*pTempD++ = *pTempS++;
        //}
    }
    if(len == maxLen)
    {
        return -1;
    }
    return len;
}

void print_hex(unsigned char *hex, unsigned int len) {
  uint8 buffer[5] = {0};
  
  int i;
  for (i=0;i<len;i++) {
    os_sprintf(buffer, "0x%02x ", hex[i]);
    at_port_print(buffer);
    if (i%8==0 && i!=0)
      at_port_print("\r\n");
  }
  at_port_print("\r\n");
}

void ICACHE_FLASH_ATTR
at_setupCmdCwsapID(uint8_t id, char *pPara)
{
    if(at_wifiMode != STATION_MODE)
    {
        at_port_print("\r\nNot in station mode.\r\n");
        at_response_error();
        return;
    }
    int8_t len,i,j;
    uint8_t channel_tmp;
    pPara++; //Skip "=" char
    len = at_dataStrLen(pPara, 32); 
    //Maxlen == 32 which means 31chars
    //if(len == maxLen)
    //Do not change this since some firmwares are
    //programmed like this
    if(len < 8)
    {
        at_port_print("\r\nSSID too short.\r\n");
        at_response_error();
        return;
    }
    pPara++; //Skip "
    j=0;
    for(i=38; i<=68; i++)
    {
        if(pPara[j] == '\"')
        { 
            break;
        }
        else
        {
            packet_buffer[i] = (unsigned char)pPara[j];
        }
        j++;
    }
    
    pPara += (len+2); //Skip ",
   
    channel_tmp = atoi(pPara);
    if(channel_tmp<1 || channel_tmp>13)
    {
        at_port_print("\r\nInvalid channel.\r\n");
        at_response_error();
        return;
    }
    ETS_UART_INTR_DISABLE();
    channel = channel_tmp;
    packet_buffer[81] = (unsigned char)channel;
    wifi_set_channel(channel);
    
    //i = wifi_send_pkt_freedom(packet_buffer, packetSize, 0);
    ETS_UART_INTR_ENABLE();
    /*if (i < 0)
    {
        at_port_print("\r\nError sending beacon frame.\r\n");
        at_response_error();
        return;
    }*/
    
    print_hex(packet_buffer,packetSize);
    at_response_ok();
}

void ICACHE_FLASH_ATTR
at_setupCmdCwsapCH(uint8_t id, char *pPara)
{
    if(at_wifiMode != STATION_MODE)
    {
        at_port_print("\r\nNot in station mode.\r\n");
        at_response_error();
        return;
    }
    int8_t len;
    uint8_t channel_tmp;
    pPara++;
    channel_tmp = atoi(pPara);
    if(channel_tmp<1 || channel_tmp>13)
    {
        at_port_print("\r\nInvalid channel.\r\n");
        at_response_error();
        return;
    }
    ETS_UART_INTR_DISABLE();
    channel = channel_tmp;
    packet_buffer[81] = (unsigned char)channel;
    wifi_set_channel(channel);
    ETS_UART_INTR_ENABLE();
    at_response_ok();
}

void ICACHE_FLASH_ATTR
at_setupCmdCwsapBR(uint8_t id, char *pPara)
{
    if(at_wifiMode != STATION_MODE)
    {
        at_response_error();
        return;
    }
    int8_t len;
    uint32_t beacon_rate_tmp;
    pPara++;
    beacon_rate_tmp = atol(pPara);
    if(beacon_rate_tmp<1 || beacon_rate_tmp>1000)
    {
        at_port_print("\r\nInvalid beacon rate.\r\n");
        at_response_error();
        return;
    }
    ETS_UART_INTR_DISABLE();
    beacon_rate = beacon_rate_tmp;
    // Set timer for beacon
    os_timer_disarm(&beacon_timer);
    os_timer_setfn(&beacon_timer, (os_timer_func_t *) beacon, NULL);
    os_timer_arm(&beacon_timer, beacon_rate, 1);
    ETS_UART_INTR_ENABLE();
    at_response_ok();
}

void ICACHE_FLASH_ATTR
at_setupCmdCwsapEN(uint8_t id)
{
    
    if(at_wifiMode != STATION_MODE)
    {
        at_response_error();
        return;
    }
    // Set timer for beacon
    os_timer_disarm(&beacon_timer);
    os_timer_setfn(&beacon_timer, (os_timer_func_t *) beacon, NULL);
    os_timer_arm(&beacon_timer, beacon_rate, 1);

    at_response_ok();
}

void ICACHE_FLASH_ATTR
at_setupCmdCwsapDS(uint8_t id)
{
    // Set timer for beacon
    os_timer_disarm(&beacon_timer);

    at_response_ok();
}

//These commands are the same as the previous rev AT commands, except 
//they utilise the beacon generator instead.
//AT+CWSAPID:
//Set parameters of beacon generator
//AT+CWSAPID="<ssid>",<channel num>

//AT+CWSAPCH: 
//Change beacon channel.
//AT+CWSAPCH=<channel num> 

//AT+CWSAPBR: 
//Change beacon rate.
//AT+CWSAPBR=<delay ms> 

//AT+CWSAPEN: 
//Enable beacons (disabled by default).
//AT+CWSAPEN

//AT+CWSAPDS: 
//Disable beacons.
//AT+CWSAPDS


extern void at_exeCmdCiupdate(uint8_t id);
at_funcationType at_custom_cmd[] = {
        {"+CWSAPBR", 8, NULL, NULL, at_setupCmdCwsapBR, NULL},
        {"+CWSAPID", 8, NULL, NULL, at_setupCmdCwsapID, NULL},
        {"+CWSAPDS", 8, NULL, NULL, NULL, at_setupCmdCwsapDS},
        {"+CWSAPEN", 8, NULL, NULL, NULL, at_setupCmdCwsapEN},
        {"+CWSAPCH", 8, NULL, NULL, at_setupCmdCwsapCH, NULL}
};

void ICACHE_FLASH_ATTR
user_init()
{
    //uart_init(115200, 115200);
    uint8_t macaddr[6];	
    char buf[64] = {0};
    at_customLinkMax = 5;
    at_init();
    at_set_custom_info(buf);
    
    at_port_print("\r\nready\r\n");
    at_cmd_array_regist(&at_custom_cmd[0], sizeof(at_custom_cmd)/sizeof(at_custom_cmd[0]));

    //os_printf("\n\nSDK version:%s\n", system_get_sdk_version());
    
    //Set MAC address to stored address. 01:02:03:04:05:06 seems to be blocked on devices.
    wifi_get_macaddr(0x00, macaddr);
    packet_buffer[16] = packet_buffer[10] = macaddr[0];
    packet_buffer[17] = packet_buffer[11] = macaddr[1];
    packet_buffer[18] = packet_buffer[12] = macaddr[2];
    packet_buffer[19] = packet_buffer[13] = macaddr[3];
    packet_buffer[20] = packet_buffer[14] = macaddr[4];
    packet_buffer[21] = packet_buffer[15] = macaddr[5];
    
    //Promiscuous works only with station mode
    //Should set this manually to save flash.
    //Code will error if not in station mode.
    //wifi_set_opmode(STATION_MODE);
    //AT+CWMODE=1
    wifi_set_channel(channel);
    wifi_promiscuous_enable(1);
    
    
    // Disabled by default
    // Set timer for beacon
    os_timer_disarm(&beacon_timer);
    //os_timer_setfn(&beacon_timer, (os_timer_func_t *) beacon, NULL);
    //os_timer_arm(&beacon_timer, beacon_rate, 1);
    

}
