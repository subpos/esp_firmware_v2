#include "ets_sys.h"
#include "osapi.h"
#include "gpio.h"
#include "os_type.h"
#include "mem.h"
#include "user_config.h"
#include "user_interface.h"
#include "at_custom.h"

#define user_procTaskPrio        0
#define user_procTaskQueueLen    1
os_event_t    user_procTaskQueue[user_procTaskQueueLen];
static volatile os_timer_t beacon_timer;
uint8_t channel = 1;

#define packetSize    82

// Beacon Packet buffer
uint8_t packet_buffer[packetSize] = { 0x80, 0x00, 0x00, 0x00, 
                /*4*/   0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 
                /* SRC MAC */
                /*10*/  0x01, 0x02, 0x03, 0x04, 0x05, 0x06,
                /* DST MAC */
                /*16*/  0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 
                /*22*/  0xc0, 0x6c, 
                /*24*/  0x83, 0x51, 0xf7, 0x8f, 0x0f, 0x00, 0x00, 0x00, 
                /*32*/  0x64, 0x00, 
                /*34*/  0x01, 0x04, 
                /* SSID Part */
                /*36*/  0x00, 0x1f, 
                /* SSID Chars Start */
                /*38*/  0x01, 0x01, 0x01, 0x01, 0x01, 0x01,
                        0x01, 0x01, 0x01, 0x01, 0x01, 0x01,
                        0x01, 0x01, 0x01, 0x01, 0x01, 0x01,
                        0x01, 0x01, 0x01, 0x01, 0x01, 0x01,
                        0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 
                        0x01,
                /* SSID Chars End */
                        0x01, 0x08, 0x82, 0x84,
                        0x8b, 0x96, 0x24, 0x30, 0x48, 0x6c, 0x03, 0x01, 
                /* Channel */
                /*81*/  0x04};     



/* ==============================================
 * Promiscous callback structures, see ESP manual
 * ============================================== */
 
struct RxControl {
    signed rssi:8;
    unsigned rate:4;
    unsigned is_group:1;
    unsigned:1;
    unsigned sig_mode:2;
    unsigned legacy_length:12;
    unsigned damatch0:1;
    unsigned damatch1:1;
    unsigned bssidmatch0:1;
    unsigned bssidmatch1:1;
    unsigned MCS:7;
    unsigned CWB:1;
    unsigned HT_length:16;
    unsigned Smoothing:1;
    unsigned Not_Sounding:1;
    unsigned:1;
    unsigned Aggregation:1;
    unsigned STBC:2;
    unsigned FEC_CODING:1;
    unsigned SGI:1;
    unsigned rxend_state:8;
    unsigned ampdu_cnt:8;
    unsigned channel:4;
    unsigned:12;
};
 
/* Sends beacon packets. */
void beacon(void *arg)
{
    wifi_send_pkt_freedom(packet_buffer, packetSize, 0);
}

extern at_funcationType at_custom_cmd[];

uint8_t at_wifiMode;

int8_t ICACHE_FLASH_ATTR
at_dataStrCpy(void *pDest, const void *pSrc, int8_t maxLen, int8_t start)
{

    char *pTempD = pDest;
    const char *pTempS = pSrc;
    int8_t len;
    
    //Set start point
    maxLen = maxLen + start;
    pTempD = pTempD + start;

    if(*pTempS != '\"')
    {
        return -1;
    }
    pTempS++;
    
    for(len=start; len<maxLen; len++)
    {
        if(*pTempS == '\"')
        {
            *pTempD = '\0';
            break;
        }
        else
        {
            *pTempD++ = *pTempS++;
        }
    }
    if(len == maxLen)
    {
        return -1;
    }
    return len;
}

void ICACHE_FLASH_ATTR
at_setupCmdCwsapID(uint8_t id, char *pPara)
{
    int8_t len;

    pPara++; //Skip = char
    len = at_dataStrCpy(packet_buffer, pPara, 32, 38);
    
    if(len < 1)
    {
        at_response_error();
        return;
    }
    pPara += (len+3);

    channel = atoi(pPara);
    if(channel<1 || channel>13)
    {
        at_response_error();
        return;
    }
    ETS_UART_INTR_DISABLE();
    wifi_set_channel(channel);
    ETS_UART_INTR_ENABLE();
    at_response_ok();
}

void ICACHE_FLASH_ATTR
at_setupCmdCwsapCH(uint8_t id, char *pPara)
{
    int8_t len;
   
    pPara++;
    channel = atoi(pPara);
    if(channel<1 || channel>13)
    {
        at_response_error();
        return;
    }
    ETS_UART_INTR_DISABLE();
    wifi_set_channel(channel);
    ETS_UART_INTR_ENABLE();
    at_response_ok();
}

//These commands are the same as the regular AT commands, except 
//they utilise the beacon generator instead.
//AT+CWSAPID:
//Set parameters of beacon generator
//AT+CWSAPID="<ssid>",<channel num>

//AT+CWSAPCH: 
//Change beacon channel.
//AT+CWSAPCH=<channel num> 


extern void at_exeCmdCiupdate(uint8_t id);
at_funcationType at_custom_cmd[] = {
        {"+CWSAPID", 8, NULL, NULL, at_setupCmdCwsapID, NULL},
        {"+CWSAPCH", 8, NULL, NULL, at_setupCmdCwsapCH, NULL}
};

void ICACHE_FLASH_ATTR
user_init()
{
    //uart_init(115200, 115200);
    
    char buf[64] = {0};
    at_customLinkMax = 5;
    at_init();
    at_set_custom_info(buf);
    
    at_port_print("\r\nready\r\n");
    at_cmd_array_regist(&at_custom_cmd[0], sizeof(at_custom_cmd)/sizeof(at_custom_cmd[0]));

    //os_printf("\n\nSDK version:%s\n", system_get_sdk_version());
    
    // Promiscuous works only with station mode
    wifi_set_opmode(STATION_MODE);
    
    // Set timer for beacon
    os_timer_disarm(&beacon_timer);
    os_timer_setfn(&beacon_timer, (os_timer_func_t *) beacon, NULL);
    os_timer_arm(&beacon_timer, BEACON_INTERVAL_MS, 1);
    
    wifi_set_channel(channel);
    wifi_promiscuous_enable(1);
}
