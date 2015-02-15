#include "el_one_wire.h"

static uint8_t ow_pin;
static uint32_t ow_mux;
static uint32_t ow_bitmask;
static uint8_t ow_func;

#ifdef ONEWIRE_SEARCH
// search state
static unsigned char ROM_NO[8];
static uint8_t last_discrepancy;
static uint8_t last_family_discrepancy;
static uint8_t last_device_flag;
#endif

#define DBG printf

#define READ_PIN() ((GPIO_REG_READ(GPIO_IN_ADDRESS)  >> (GPIO_ID_PIN(ow_pin))) & 0x01)
#define PIN_LOW() do {\
	GPIO_REG_WRITE(GPIO_PIN_ADDR(ow_pin), 0); \
	GPIO_REG_WRITE(GPIO_OUT_W1TC_ADDRESS, ow_bitmask); \
} while(0)
#define PIN_HIGH() do {\
	GPIO_REG_WRITE(GPIO_PIN_ADDR(ow_pin), 0); \
	GPIO_REG_WRITE(GPIO_OUT_W1TS_ADDRESS, ow_bitmask); \
} while (0)

/* enable open drain output, then sent the output high. This should disable the output drivers
 * and allow external components to pull the pin low
 */
#define PIN_FLOAT() do {\
	GPIO_REG_WRITE(GPIO_PIN_ADDR(ow_pin), 0x04); \
	GPIO_REG_WRITE(GPIO_OUT_W1TS_ADDRESS, ow_bitmask); \
} while (0)


void 
ow_init(uint8_t pin, uint32_t mux, uint8_t func)
{
	ow_pin = pin;
	ow_mux = mux;
	ow_func = func;
	ow_bitmask = 1 << ow_pin;
	// set required function for the 1wire pin
	PIN_FUNC_SELECT(ow_mux, ow_func);
	PIN_FLOAT();
	// enable output driver. Use GPIO_ENABLE_W1TS register to save OR with the existing value
	GPIO_REG_WRITE(GPIO_ENABLE_W1TS_ADDRESS, ow_bitmask);
}

/*
 * Returns 0=device found
 * 2 = bus short
 * 1 = no device
 */
uint8_t
ow_reset()
{
	uint8_t r;
	uint8_t retries = 125;

	portENTER_CRITICAL();
	// Allow other devices to drive the pin
	PIN_FLOAT();
	portEXIT_CRITICAL();
	// wait until the wire is high... just in case
	do {
		if (--retries == 0) return 2; // bus short
		os_delay_us(2);
	} while (READ_PIN() == 0);
	portENTER_CRITICAL();
	// drive output low
	PIN_LOW();
	portEXIT_CRITICAL();
	os_delay_us(480);
	portENTER_CRITICAL();
	// Allow other devices to drive the pin
	PIN_FLOAT();
	os_delay_us(70);
	r = READ_PIN();
	portEXIT_CRITICAL();
	os_delay_us(410);
	return r;
}

static void 
ow_write_bit(uint8_t v)
{
	if (v & 1) {
		portENTER_CRITICAL();
		PIN_LOW();
		os_delay_us(10);
		PIN_HIGH();
		portEXIT_CRITICAL();
		os_delay_us(55);
	} else {
		portENTER_CRITICAL();
		PIN_LOW();
		os_delay_us(65);
		PIN_HIGH();
		portEXIT_CRITICAL();
		os_delay_us(5);
	}
}

static uint8_t 
ow_read_bit(void)
{
	uint8_t r;

	portENTER_CRITICAL();
	PIN_LOW();
	os_delay_us(3);
	PIN_FLOAT();
	os_delay_us(10);
	r = READ_PIN();
	portEXIT_CRITICAL();
	os_delay_us(53);
	return r;
}

uint8_t 
ow_read() 
{
    uint8_t bitMask;
    uint8_t r = 0;

    for (bitMask = 0x01; bitMask; bitMask <<= 1) 
	{
		if ( ow_read_bit()) 
		{
			r |= bitMask;
		}
    }
    return r;
}

void 
ow_read_bytes(uint8_t *buf, uint16_t count) 
{
	uint16_t i;
	for (i = 0 ; i < count ; i++)
	{
		buf[i] = ow_read();
	}
}

void
ow_write(uint8_t v, uint8_t power)
{
	uint8_t bitMask;

    for (bitMask = 0x01; bitMask; bitMask <<= 1) {
		ow_write_bit( (bitMask & v)?1:0);
    }
    if ( !power) {
		portENTER_CRITICAL();
		PIN_FLOAT();
		portEXIT_CRITICAL();
    }
}

void 
ow_write_bytes(const uint8_t *buf, uint16_t count, uint8_t power)
{
	uint16_t i;
	for (i = 0 ; i < count ; i++)
	{
		ow_write(buf[i], power);
	}
}

void
ow_skip(void)
{
    ow_write(0xCC, 0);           // Skip ROM
}

//
// Do a ROM select
//
void 
ow_select(const uint8_t rom[8])
{
    uint8_t i;
    ow_write(0x55, 0);           // match ROM
	ow_write_bytes(rom, 8, 0);
}

void 
ow_depower(void)
{
	portENTER_CRITICAL();
	PIN_FLOAT();
	portEXIT_CRITICAL();
}

#ifdef ONEWIRE_SEARCH
//
// You need to use this function to start a search again from the beginning.
// You do not need to do it for the first search, though you could.
//
void 
ow_reset_search()
{
	int i;
	// reset the search state
	last_discrepancy = 0;
	last_device_flag = FALSE;
	last_family_discrepancy = 0;
	for(i=0;i<8;i++)
	{
		ROM_NO[i] = 0;
	}
}

// Setup the search to find the device type 'family_code' on the next call
// to search(*newAddr) if it is present.
//
void 
ow_target_search(uint8_t family_code)
{
	ow_reset_search();
	// set the search state to find SearchFamily type devices
	ROM_NO[0] = family_code;
	last_discrepancy = 64;
}

//
// Perform a search. If this function returns a '1' then it has
// enumerated the next device and you may retrieve the ROM from the
// OneWire::address variable. If there are no devices, no further
// devices, or something horrible happens in the middle of the
// enumeration then a 0 is returned.  If a new device is found then
// its address is copied to newAddr.  Use OneWire::reset_search() to
// start over.
//
// --- Replaced by the one from the Dallas Semiconductor web site ---
//--------------------------------------------------------------------------
// Perform the 1-Wire Search Algorithm on the 1-Wire bus using the existing
// search state.
// Return TRUE  : device found, ROM number in ROM_NO buffer
//        FALSE : device not found, end of search
//
uint8_t 
ow_search(uint8_t *newAddr)
{
	uint8_t id_bit_number;
	uint8_t last_zero, rom_byte_number, search_result;
	uint8_t id_bit, cmp_id_bit;
	int i;
	unsigned char rom_byte_mask, search_direction;

	// initialize for search
	id_bit_number = 1;
	last_zero = 0;
	rom_byte_number = 0;
	rom_byte_mask = 1;
	search_result = 0;

	// if the last call was not the last one
	if (!last_device_flag)
	{
		// 1-Wire reset
		if (ow_reset())
		{
			// reset the search
			last_discrepancy = 0;
			last_device_flag = FALSE;
			last_family_discrepancy = 0;
			return FALSE;
		}

	  // issue the search command
	  ow_write(0xF0, 0);

	  // loop to do the search
	  do
	  {
		 // read a bit and its complement
		 id_bit = ow_read_bit();
		 cmp_id_bit = ow_read_bit();

		 // check for no devices on 1-wire
		 if ((id_bit == 1) && (cmp_id_bit == 1))
			break;
		 else
		 {
			// all devices coupled have 0 or 1
			if (id_bit != cmp_id_bit)
			   search_direction = id_bit;  // bit write value for search
			else
			{
			   // if this discrepancy if before the Last Discrepancy
			   // on a previous next then pick the same as last time
			   if (id_bit_number < last_discrepancy)
				  search_direction = ((ROM_NO[rom_byte_number] & rom_byte_mask) > 0);
			   else
				  // if equal to last pick 1, if not then pick 0
				  search_direction = (id_bit_number == last_discrepancy);

			   // if 0 was picked then record its position in LastZero
			   if (search_direction == 0)
			   {
				  last_zero = id_bit_number;

				  // check for Last discrepancy in family
				  if (last_zero < 9)
					 last_family_discrepancy = last_zero;
			   }
			}

			// set or clear the bit in the ROM byte rom_byte_number
			// with mask rom_byte_mask
			if (search_direction == 1)
			  ROM_NO[rom_byte_number] |= rom_byte_mask;
			else
			  ROM_NO[rom_byte_number] &= ~rom_byte_mask;

			// serial number search direction write bit
			ow_write_bit(search_direction);

			// increment the byte counter id_bit_number
			// and shift the mask rom_byte_mask
			id_bit_number++;
			rom_byte_mask <<= 1;

			// if the mask is 0 then go to new SerialNum byte rom_byte_number and reset mask
			if (rom_byte_mask == 0)
			{
				rom_byte_number++;
				rom_byte_mask = 1;
			}
		 }
	  }
	  while(rom_byte_number < 8);  // loop until through all ROM bytes 0-7

	  // if the search was successful then
	  if (!(id_bit_number < 65))
	  {
		 // search successful so set last_discrepancy,last_device_flag,search_result
		 last_discrepancy = last_zero;

		 // check for last device
		 if (last_discrepancy == 0)
			last_device_flag = TRUE;

		 search_result = TRUE;
	  }
	}

	// if no device found then reset counters so next 'search' will be like a first
	if (!search_result || !ROM_NO[0])
	{
	  last_discrepancy = 0;
	  last_device_flag = FALSE;
	  last_family_discrepancy = 0;
	  search_result = FALSE;
	}
	for (i = 0; i < 8; i++) 
	{
		newAddr[i] = ROM_NO[i];
	}
	return search_result;
}

#endif

// This table comes from Dallas sample code where it is freely reusable,
// though Copyright (C) 2000 Dallas Semiconductor Corporation
static const uint8_t dscrc_table[] = {
      0, 94,188,226, 97, 63,221,131,194,156,126, 32,163,253, 31, 65,
    157,195, 33,127,252,162, 64, 30, 95,  1,227,189, 62, 96,130,220,
     35,125,159,193, 66, 28,254,160,225,191, 93,  3,128,222, 60, 98,
    190,224,  2, 92,223,129, 99, 61,124, 34,192,158, 29, 67,161,255,
     70, 24,250,164, 39,121,155,197,132,218, 56,102,229,187, 89,  7,
    219,133,103, 57,186,228,  6, 88, 25, 71,165,251,120, 38,196,154,
    101, 59,217,135,  4, 90,184,230,167,249, 27, 69,198,152,122, 36,
    248,166, 68, 26,153,199, 37,123, 58,100,134,216, 91,  5,231,185,
    140,210, 48,110,237,179, 81, 15, 78, 16,242,172, 47,113,147,205,
     17, 79,173,243,112, 46,204,146,211,141,111, 49,178,236, 14, 80,
    175,241, 19, 77,206,144,114, 44,109, 51,209,143, 12, 82,176,238,
     50,108,142,208, 83, 13,239,177,240,174, 76, 18,145,207, 45,115,
    202,148,118, 40,171,245, 23, 73,  8, 86,180,234,105, 55,213,139,
     87,  9,235,181, 54,104,138,212,149,203, 41,119,244,170, 72, 22,
    233,183, 85, 11,136,214, 52,106, 43,117,151,201, 74, 20,246,168,
    116, 42,200,150, 21, 75,169,247,182,232, 10, 84,215,137,107, 53};

//
// Compute a Dallas Semiconductor 8 bit CRC. These show up in the ROM
// and the registers.  (note: this might better be done without to
// table, it would probably be smaller and certainly fast enough
// compared to all those sn_delay_us() calls.  But I got
// confused, so I use this table from the examples.)
//
uint8_t ow_crc8(const uint8_t *addr, uint8_t len)
{
	uint8_t crc = 0;

	while (len--) {
		crc = dscrc_table[(crc ^ *addr++)];
	}
	return crc;
}

#ifdef ONEWIRE_DS18X20
void
ow_ds18x20_convert(const uint8_t *addr, uint8_t power)
{
	if (addr == NULL)
	{
		ow_skip();
	}
	else
	{
		ow_select(addr);
	}
	ow_write(0x44, power); // Start conversion
}

uint8_t
ow_ds18x20_read_temperature(const uint8_t *addr, int16_t *temp)
{
	uint8_t data[9];
	uint8_t ret;
	int16_t sign=1;
	if (ret=ow_reset())
	{
		return ret;
	}
	if (addr == NULL)
	{
		ow_skip();
	}
	else
	{
		ow_select(addr);
	}
	ow_write(0xBE, 0);
	ow_read_bytes(data, 9);
	ret = ow_crc8(data, 9);
	DBG("data[0],[1], crc=[%02x],[%02x] %02x\r\n", data[0], data[1], ret);
	if (temp)
	{
		int16_t t = data[1];
		t <<= 8;
		t += data[0];
		t = t *10 / 16;
		*temp = t;
		DBG("*temp=%d\r\n", *temp);
	}
	return ret;
}
#endif
// vim: ts=4 sw=4 noexpandtab

