#include <clavier.hpp>
#include <bounce2interface.hpp>
#include <midiusbtransmitter.hpp>

static Clavier<61, Bounce2Interface<false>> manual_i;

void setup()
{
	manual_i.init({  0,  1,  2,  3,  4,  5,  6,  7, 61, 62, 63, 64,
	                65, 66, 14, 15, 16, 17, 18, 19, 20, 21, 22, 23,
	                24, 25, 26, 27, 28, 29, 30, 31, 32, 33, 34, 35,
	                36, 37, 38, 39, 40, 41, 42, 43, 44, 45, 46, 47,
	                48, 49, 50, 51, 52, 53, 54, 55, 56, 57, 58, 59, 60 });
}

void loop()
{
	MIDIUSBTransmitter<36, 0> tx;
	manual_i.update(tx);
}
