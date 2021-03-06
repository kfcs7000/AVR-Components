#include "../shared.h"

void twi_slave(FRAME *frame)
{
	uint8_t byte_count = 0;
	uint8_t stream[FRAME_SIZE];
	
	// state: idle
	// action: select slave receiver mode
	TWCR = _BV(TWINT) | _BV(TWEA) | _BV(TWEN);
	loop_until_bit_is_set(TWCR, TWINT);

	while (1)
	{
		switch (TW_STATUS)
		{
		// state: received data and acknowledged
		// action: store data
		case TW_SR_DATA_ACK:
			stream[byte_count++] = TWDR;
		// state: received ping in write mode
		// action: send acknowledge
		case TW_SR_SLA_ACK:
			TWCR = _BV(TWINT) | _BV(TWEA) | _BV(TWEN);
			break;

		// state: transmission complete
		// action: end connection, check frame
		case TW_SR_STOP:
			memcpy(frame, stream, FRAME_SIZE);
			return;

		case TW_ST_SLA_ACK:
			memcpy(stream, frame, FRAME_SIZE);
		case TW_ST_DATA_ACK:
			TWDR = stream[byte_count++];

			if (byte_count < FRAME_SIZE)
			{
				TWCR = _BV(TWINT) | _BV(TWEA) | _BV(TWEN);
			}
			else
			{
				TWCR = _BV(TWINT) | _BV(TWEN);
			}
			break;

		case TW_ST_LAST_DATA:
			PORTD = ~PORTD;
			return;

		// state: error occured on bus
		// action: return from function
		default:
			return;
		}

		// wait until action complete
		loop_until_bit_is_set(TWCR, TWINT);
	}
}
void init_frame(FRAME *frame)
{
	frame->control = 0xFF;

	frame->crc = 0;
	for (size_t i = 0; i < BUFFER_SIZE; ++i)
	{
		frame->data[i] = 'a' + i;
		frame->crc = _crc8_ccitt_update(frame->crc, frame->data[i]);
	}
}
int main()
{
	__builtin_avr_cli();
	TWAR = SLAVE_ADDRESS;
	DDRD = 0xFF;

	__builtin_avr_sei();

	while (1)
	{
		FRAME my_frame;
		init_frame(&my_frame);
		twi_slave(&my_frame);
	}

	return 0;
}