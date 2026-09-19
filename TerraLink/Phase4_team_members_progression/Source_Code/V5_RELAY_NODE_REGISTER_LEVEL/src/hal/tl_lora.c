#include <avr/io.h>
#include <util/delay.h>
#include <string.h>
#include "tl_config.h"
#include "tl_spi.h"
#include "tl_lora.h"
#include "tl_packet.h"
#include "tl_time.h"
#define R_FIFO 0x00U
#define R_OP 0x01U
#define R_FRF_MSB 0x06U
#define R_FRF_MID 0x07U
#define R_FRF_LSB 0x08U
#define R_PA 0x09U
#define R_LNA 0x0CU
#define R_FIFO_PTR 0x0DU
#define R_TX_BASE 0x0EU
#define R_RX_BASE 0x0FU
#define R_RX_CUR 0x10U
#define R_IRQ 0x12U
#define R_RX_LEN 0x13U
#define R_SNR 0x19U
#define R_RSSI 0x1AU
#define R_CFG1 0x1DU
#define R_CFG2 0x1EU
#define R_CFG3 0x26U
#define R_PREAMBLE_MSB 0x20U
#define R_PREAMBLE_LSB 0x21U
#define R_SYNC 0x39U
#define R_DIO 0x40U
#define R_VERSION 0x42U
#define R_PAYLOAD_LEN 0x22U
#define M_LORA 0x80U
#define M_SLEEP 0x00U
#define M_STDBY 0x01U
#define M_TX 0x03U
#define M_RX 0x05U
#define IRQ_RX_DONE 0x40U
#define IRQ_CRC_ERR 0x20U
#define IRQ_TX_DONE 0x08U
#define NSS_LOW() (PORTB&=(uint8_t)~_BV(PORTB2))
#define NSS_HIGH() (PORTB|=_BV(PORTB2))
#define RST_LOW() (PORTB&=(uint8_t)~_BV(PORTB1))
#define RST_HIGH() (PORTB|=_BV(PORTB1))
static uint8_t rr(uint8_t r){uint8_t v;NSS_LOW();tl_spi_transfer(r&0x7FU);v=tl_spi_transfer(0);NSS_HIGH();return v;}
static void rw(uint8_t r,uint8_t v){NSS_LOW();tl_spi_transfer(r|0x80U);tl_spi_transfer(v);NSS_HIGH();}
static void txfifo(const uint8_t*d,uint8_t n){NSS_LOW();tl_spi_transfer(R_FIFO|0x80U);for(uint8_t i=0;i<n;i++)tl_spi_transfer(d[i]);NSS_HIGH();}
static void rx_mode(void){rw(R_OP,M_LORA|M_RX);}
bool tl_lora_init(void){tl_spi_init();DDRB|=_BV(DDB2)|_BV(DDB1);NSS_HIGH();RST_HIGH();_delay_ms(2);RST_LOW();_delay_ms(10);RST_HIGH();_delay_ms(10);if(rr(R_VERSION)!=0x12U)return false;rw(R_OP,M_LORA|M_SLEEP);_delay_ms(5);rw(R_FRF_MSB,TL_LORA_FRF_MSB);rw(R_FRF_MID,TL_LORA_FRF_MID);rw(R_FRF_LSB,TL_LORA_FRF_LSB);rw(R_TX_BASE,0);rw(R_RX_BASE,0);rw(R_LNA,0x23);rw(R_CFG1,0x72);rw(R_CFG2,0x74);rw(R_CFG3,0x04);rw(R_PREAMBLE_MSB,0);rw(R_PREAMBLE_LSB,8);rw(R_SYNC,TL_LORA_SYNC_WORD);rw(R_PA,0x8F);rw(R_DIO,0);rw(R_IRQ,0xFF);rx_mode();return true;}
bool tl_lora_send(const char*p){size_t n=strlen(p);if(n==0U||n>255U)return false;rw(R_OP,M_LORA|M_STDBY);rw(R_FIFO_PTR,0);rw(R_IRQ,0xFF);txfifo((const uint8_t*)p,(uint8_t)n);rw(R_PAYLOAD_LEN,(uint8_t)n);rw(R_OP,M_LORA|M_TX);unsigned long t=millis();while(!(rr(R_IRQ)&IRQ_TX_DONE)){if(millis()-t>3000UL){rx_mode();return false;}}rw(R_IRQ,IRQ_TX_DONE);rx_mode();return true;}
bool tl_lora_receive(TLPacket*p){uint8_t irq=rr(R_IRQ);if(!(irq&IRQ_RX_DONE))return false;rw(R_IRQ,0xFF);if(irq&IRQ_CRC_ERR){rx_mode();return false;}uint8_t n=rr(R_RX_LEN);if(n==0||n>=TL_PACKET_BUFFER_SIZE){rx_mode();return false;}rw(R_FIFO_PTR,rr(R_RX_CUR));char raw[TL_PACKET_BUFFER_SIZE];NSS_LOW();tl_spi_transfer(R_FIFO);for(uint8_t i=0;i<n;i++)raw[i]=(char)tl_spi_transfer(0);NSS_HIGH();raw[n]='\0';int16_t r=(int16_t)rr(R_RSSI)-164;p->rssi=(int8_t)r;p->snr=(int8_t)((int8_t)rr(R_SNR)/4);rx_mode();return tl_packet_parse(raw,p);}
