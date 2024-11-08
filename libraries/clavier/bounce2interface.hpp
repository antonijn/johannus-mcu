#ifndef CLAVIER_BOUNCE2INTERFACE_HPP
#define CLAVIER_BOUNCE2INTERFACE_HPP

#include <Bounce2.h>

template<bool Pull_down, int Debounce_interval = 5>
class Bounce2Interface {
private:
	Bounce bouncer;

public:
	void setup(int pin)
	{
		pinMode(pin, Pull_down ? INPUT : INPUT_PULLUP);
		bouncer.attach(pin);
		bouncer.interval(Debounce_interval);
	}

	bool update()
	{
		return bouncer.update();
	}

	bool rose()
	{
		bool rose = bouncer.rose();
		return (rose && Pull_down) || (!rose && !Pull_down);
	}
};

#endif
