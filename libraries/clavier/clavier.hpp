#ifndef CLAVIER_HPP
#define CLAVIER_HPP

#include <array>

template<int Num_notes, typename TBounce>
class Clavier {
public:
	void init(const std::array<int, Num_notes>& pins);

	template<typename Transmitter>
	void update(Transmitter& tx);

	bool is_down(int button) const;

private:
	TBounce bouncers[Num_notes];
};

template<int Num_notes, typename TBounce>
void Clavier<Num_notes, TBounce>
::init(const std::array<int, Num_notes>& mapping)
{
	for (int note = 0; note < Num_notes; ++note) {
		int pin = mapping[note];
		bouncers[note].setup(pin);
	}
}

template<int Num_notes, typename TBounce>
template<typename Transmitter>
void Clavier<Num_notes, TBounce>::update(Transmitter& tx)
{
	for (int i = 0; i < Num_notes; ++i) {
		if (!bouncers[i].update())
			continue;

		bool rose = bouncers[i].rose();
		tx.transmit(i, rose);
	}
}

#endif
