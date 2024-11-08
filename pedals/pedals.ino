#include <clavier.hpp>
#include <bounce2interface.hpp>

#define CHANNEL 3

static Clavier<30, Bounce2Interface<false>> pedals;

class usbMIDITransmitter {
public:
	void transmit(int note, bool value)
	{
		if (value)
			usbMIDI.sendNoteOn(note + 36, 0x7F, CHANNEL);
		else
			usbMIDI.sendNoteOff(note + 36, 0, CHANNEL);
	}
};

void setup()
{
	pedals.init({7,19,31,4,24,11,28,9,26,14,22,23,16,2,21,17,1,3,0,12,5,20,29,18,6,15,10,8,33,32});
}

void loop()
{
	usbMIDITransmitter tx;
	pedals.update(tx);
}
