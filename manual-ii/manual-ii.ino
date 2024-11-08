#include <clavier.hpp>
#include <bounce2interface.hpp>
#include <midiusbtransmitter.hpp>

static Clavier<61, Bounce2Interface<true>> manual_ii;

void setup()
{
	manual_ii.init({  0, 15,  2,  3,  4,  5,  6,  7,  8,  9, 10, 11,
	                 12, 13, 14,  1, 16, 17, 18, 19, 66, 67, 23, 22,
	                 25, 34, 27, 26, 29, 40, 31, 44, 33, 35, 37, 32,
	                 39, 48, 41, 38, 43, 30, 24, 45, 36, 47, 28, 46,
	                 49, 50, 42, 51, 53, 52, 54, 55, 56, 57, 58, 59, 60 });
}

void loop()
{
	MIDIUSBTransmitter<36, 1> tx;
	manual_ii.update(tx);
}
