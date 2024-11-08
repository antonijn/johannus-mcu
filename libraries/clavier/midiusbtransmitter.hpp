#ifndef MIDIUSB_TRANSMITTER_HPP
#define MIDIUSB_TRANSMITTER_HPP

#include <MIDIUSB.h>

template<int Lowest_note, int Channel>
class MIDIUSBTransmitter {
private:
	static void send_note_on(int channel, int note)
	{
		byte note_on[] = { 0x09, 0x90 | channel, note, 0x7F };
		while (!MidiUSB.write(note_on, sizeof(note_on)))
			;
	}

	static void send_note_off(int channel, int note)
	{
		byte note_off[] = { 0x08, 0x90 | channel, note, 0x00 };
		while (!MidiUSB.write(note_off, sizeof(note_off)))
			;
	}

public:
	void transmit(int note, bool on)
	{
		if (on)
			send_note_on(Channel, Lowest_note + note);
		else
			send_note_off(Channel, Lowest_note + note);
	}
};

#endif
