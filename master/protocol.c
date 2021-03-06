#include	"protocol.h"

const char channel[16] = {85, 87, 90, 93, 96, 99, 102, 105,\
													 107, 110, 113, 115, 117, 120, 123, 126};

uint8_t select_channel(void) {
uint32_t ticks;	
uint8_t ch, ccr, start;

	ccr = 0;
	ch = 0;
	start = get_uid()%sizeof(channel);
	
	while(1) {
		set_rf_channel(channel[(start+ch)%16]);
		ticks = get_systick();
		while(1) {
			iwdg_refresh();
			if(rf_read_reg(0x09) == 1) {
				ccr = 1;
				break;
			}
			if(get_systick()-ticks>1) {
				ccr = 0;
				break;
			}
		}
		if(ccr == 0) {
			break;
		}
		
		ch++;
	}
	
	while(1) {
		iwdg_refresh();
		led_on();
		rf_write_payload("X", 1);
		delay(50);
		led_off();
		delay(150);
		if(is_rf_sent()) {
			rf_write_reg(0x05, rf_read_reg(0x05));
			break;
		}
		putchar(0xF0);
	}	
	
	return rf_read_reg(0x05);
}

uint32_t get_uid(void) {
	return (uint32_t)(atol((const char *)(DEV_ID_LOC+6)));
}

uint8_t *get_uid_char(void) {
	return (uint8_t *)DEV_ID_LOC;
}

void set_uid(uint8_t *id, uint8_t len) {
uint8_t *p = (uint8_t *)DEV_ID_LOC;

	if(!(FLASH_IAPSR & 0x80)) {
		FLASH_DUKR = 0xAE;
		FLASH_DUKR = 0x56;
	}
	
	while(len--) {
		*p++ = *id++;
	}

	FLASH_DUKR = 0xDE;
	FLASH_DUKR = 0xAD;
}

void config_mode(void) {
uint32_t ticks;
uint8_t loop=0;

	ticks = get_systick();
	
	while(1) {
	uint8_t cnt, *pbuf;
		
		iwdg_refresh();
		if(is_uart_received()) {
			pbuf = get_uart_buf();
			cnt = get_uart_cnt();		
			if(memcmp("XZ201\r\n", pbuf, cnt) == 0) {
				loop = 1;
				printf("REVOK");
			} else if(memcmp("RXADD\r\n", pbuf, cnt) == 0) {
				printf("RX|Z|%10s|\r\n", get_uid_char());
			} else if(memcmp("WR|Z|", pbuf, 5) == 0) {
				set_uid(pbuf+5, 10);
				printf("WR|Z|%10s|\r\n", get_uid_char());
			} else if(memcmp("RUN\r\n", pbuf, cnt) == 0) {
				printf("RUN\r\n");
				return;
			} else {
				printf("ERROR\r\n");
			}
		}
		
		if(!loop && (get_systick()-ticks>2)) {
			return;
		}		
	}
}
