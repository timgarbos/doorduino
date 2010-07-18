#ifndef ALLINONE
#include <arduino/serial.h>

#define EXPORT
#endif

static volatile struct {
	uint8_t buf[SERIAL_INBUF];
	uint8_t start;
	uint8_t end;
} serial_input;

static volatile struct {
	uint8_t buf[SERIAL_OUTBUF];
	uint8_t start;
	uint8_t end;
} serial_output;

serial_interrupt_rx()
{
	uint8_t end = serial_input.end;

	serial_input.buf[end] = serial_read();
	serial_input.end = (end + 1) & (SERIAL_INBUF - 1);
}

serial_interrupt_dre()
{
	uint8_t start = serial_output.start;

	if (start == serial_output.end)
		serial_interrupt_dre_disable();
	else {
		serial_write(serial_output.buf[start]);
		serial_output.start = (start + 1) & (SERIAL_OUTBUF - 1);
	}
}

EXPORT char
serial_getchar()
{
	uint8_t start = serial_input.start;
	char r;

	if (start == serial_input.end)
		return '\0';

	r = serial_input.buf[start];
	start = (start + 1) & (SERIAL_INBUF - 1);

	cli();
	if (start == serial_input.end)
		events &= ~EV_SERIAL;
	sei();

	serial_input.start = start;
	return r;
}

EXPORT void
serial_print(const char *str)
{
	uint8_t end = serial_output.end;

	while ((serial_output.buf[end] = *str++))
		end = (end + 1) & (SERIAL_OUTBUF - 1);

	serial_output.end = end;
	serial_interrupt_dre_enable();
}

static char hex_digit[] = {
	'0', '1', '2', '3', '4', '5', '6', '7',
	'8', '9', 'A', 'B', 'C', 'D', 'E', 'F'
};

EXPORT void
serial_hexdump(const void *data, size_t len)
{
	const uint8_t *p = data;
	uint8_t end = serial_output.end;

	for (; len > 0; len--, p++) {
		uint8_t c = *p;

		serial_output.buf[end] = hex_digit[c >> 4];
		end = (end + 1) & (SERIAL_OUTBUF - 1);
		serial_output.buf[end] = hex_digit[c & 0x0F];
		end = (end + 1) & (SERIAL_OUTBUF - 1);
	}

	serial_output.end = end;
	serial_interrupt_dre_enable();
}