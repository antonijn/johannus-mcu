#ifndef CLAVIER_SYSEX_HPP
#define CLAVIER_SYSEX_HPP

#include <stdint.h>

namespace clavier {
	enum johannus_antonijn_sysex_event {
		TRANSPOSE    = 0x01,
		TEMPERAMENT  = 0x02,
		TUNING       = 0x03,

		INSTRUMENT   = 0x04,

		GRANDORGUE_READY = 0x100,
		GRANDORGUE_GAIN = 0x101,
		GRANDORGUE_POLYPHONY = 0x102,

		GRANDORGUE_START_RECORDING = 0x141,
		GRANDORGUE_STOP_RECORDING = 0x142,
	};

	bool parse_johannus_antonijn_sysex(uint8_t *sysex_data,
					   unsigned len,
					   enum johannus_antonijn_sysex_event& evt,
					   int& value);
}

#endif
