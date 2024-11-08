#include "sysex.hpp"
#include <stdio.h>
#include <string.h>

bool clavier::parse_johannus_antonijn_sysex(uint8_t *sysex_data,
                                            unsigned length,
                                            enum johannus_antonijn_sysex_event& evt,
                                            int& value)
{
	++sysex_data;
	length -= 2;

	if (length != 24)
		return false;

	char sysExString[25];
	sysExString[24] = 0;
	memcpy(sysExString, sysex_data, 24);

	int key_int, value_int;
	if (sscanf(sysExString, "JOHANNUSANTONIJN%4x%4x", &key_int, &value_int) != 2)
		return false;

	evt = (enum johannus_antonijn_sysex_event)(uint16_t)key_int;
	value = (int)(uint16_t)value_int;
	return true;
}
