#include <sysex.hpp>
#include <Adafruit_MCP23017.h>
#include <array>
#include <bitset>
#include <Bounce2.h>

//#define DIAG
#define REG_DEBOUNCE_INTERVAL 48
#define PISTON_DEBOUNCE_INTERVAL 10

#define CHANNEL 4

class ioexp {
private:
	Adafruit_MCP23017 mcp;
	uint16_t states[16];
	unsigned long prev_poll_time;
	unsigned long debounce_poll_interval;
	uint16_t ab;
	std::array<int, 16> reg_indices;

	void debounce()
	{
		uint16_t res = 0;
		for (int i = 0; i < 16; ++i) {
			uint16_t state_bit = (ab >> i) & 1;
			uint16_t state = (states[i] << 1U) | state_bit;
			if (state == 0xFFFF)
				res |= 1U << i;
			states[i] = state;
		}
		ab = res;
	}

public:
	void setup(byte addr, std::array<int, 16> reg_indices, unsigned long debounce_poll_interval = 3)
	{
		mcp.begin(addr);
		for (int i = 0; i < 16; ++i) {
			mcp.pinMode(i, INPUT);
			mcp.pullUp(i, HIGH);
		}

		this->debounce_poll_interval = debounce_poll_interval;
		this->reg_indices = reg_indices;
	}

	uint16_t readGPIOAB()
	{
		unsigned long time = millis();
		if (time - prev_poll_time > debounce_poll_interval) {
			ab = ~mcp.readGPIOAB();
			debounce();

			prev_poll_time = time;
		}

		return ab;
	}

	void read_regs(std::bitset<37>& regs)
	{
		readGPIOAB();
		for (int i = 0; i < 16; ++i)
			regs[reg_indices[i]] = (ab >> i) & 1;
	}
};

ioexp mcp[2];
static Bounce pin_regs[5];
static Bounce pistons[7];

enum preset {
	PIANISSIMO = -1,
	PIANO,
	MEZZOFORTE,
	FORTE,
	FORTISSIMO,
	TUTTI,
	MANUAL_REGISTRATION,
};

#define REED_VALVE ((int)MANUAL_REGISTRATION + 1)

static void get_preset(const std::bitset<7>& pistons, enum preset& preset, bool& reed_valve)
{
	int res = MANUAL_REGISTRATION;
	while (res >= 0 && !pistons[res])
		--res;

	preset = (enum preset)res;
	reed_valve = pistons[REED_VALVE];
}

static void get_preset_stops(std::bitset<37>& stop, enum preset preset)
{
	if (preset > TUTTI)
		return;

	static const std::bitset<37> presets[] = {
		std::bitset<37>(0b000000010'000000000000100'0000001'000000),
		std::bitset<37>(0b001001010'000000000010110'0000011'000000),
		std::bitset<37>(0b001011011'000100001111111'0000111'000000),
		std::bitset<37>(0b011011011'000110001111111'0001111'000000),
		std::bitset<37>(0b111011111'110111001111111'1011111'000000),
		std::bitset<37>(0b111111111'111111111111111'1111111'000000),
	};

	stop |= presets[preset - PIANISSIMO];
}
static void setupRegBouncer(int idx, int pin)
{
	pinMode(pin, INPUT_PULLUP);
	pin_regs[idx].attach(pin);
	pin_regs[idx].interval(REG_DEBOUNCE_INTERVAL);
}

static void setupPistonBouncer(int idx, int pin, int input = INPUT_PULLUP)
{
	pinMode(pin, INPUT_PULLUP);
	pistons[idx].attach(pin);
	pistons[idx].interval(PISTON_DEBOUNCE_INTERVAL);
}

static void get_stops(std::bitset<37>& phys_values)
{
	const static int pin_reg_idces[] = {
		1, 5, 9, 8, 4
	};

	mcp[0].read_regs(phys_values);
	mcp[1].read_regs(phys_values);

	for (int i = 0; i < sizeof(pin_reg_idces) / sizeof(pin_reg_idces[0]); ++i) {
		pin_regs[i].update();
		int reg = pin_reg_idces[i] - 1;
		bool val = !pin_regs[i].read(); /* inverted because pull-up */
		phys_values[reg] = val;
	}
}

static void get_pistons(std::bitset<7>& piston_values)
{
	for (int i = 0; i < 7; ++i) {
		pistons[i].update();

		bool val = pistons[i].read();
		val = !val;

		piston_values[i] = val;
	}
}

static void send_midi(const std::bitset<37>& stops, const std::bitset<37>& mask)
{
	for (int i = 0; i < 37; ++i) {
		if (!mask[i])
			continue;

		if (stops[i])
			usbMIDI.sendNoteOn(i + 36, 0x7F, CHANNEL);
		else
			usbMIDI.sendNoteOff(i + 36, 0, CHANNEL);
	}
}

static void send_midi(const std::bitset<37>& stops)
{
	send_midi(stops, std::bitset<37>(~0ULL));
}

#ifdef DIAG
void serial_print_bits37(const std::bitset<37>& bits)
{
	for (int i = 36; i >= 0; --i) {
		Serial.print(bits[i]);
	}
	Serial.println();
}
void serial_print_bits7(const std::bitset<7>& bits)
{
	for (int i = 6; i >= 0; --i) {
		Serial.print(bits[i]);
	}
	Serial.println();
}
#endif

static void get_logical_stops(std::bitset<37>& stops)
{
	static const std::bitset<37> always_manually_selectable(0b000000000'0000000000000000'000000'111111ULL);
	static const std::bitset<37> reed_stops(0b110000000'1110000000000000'111000'000000ULL);

#ifdef DIAG
	static unsigned long last_diag_time;
	unsigned long time = millis();
	bool diag = (time > last_diag_time + 500);
	if (diag)
		last_diag_time = time;
#endif

	get_stops(stops);

	std::bitset<7> pistons;
	get_pistons(pistons);

	enum preset preset;
	bool reed_valve;
	get_preset(pistons, preset, reed_valve);

#ifdef DIAG
	if (diag) serial_print_bits37(stops);
#endif

	if (preset != MANUAL_REGISTRATION) {
		stops &= always_manually_selectable;
		get_preset_stops(stops, preset);
	}

	if (reed_valve)
		stops &= ~reed_stops;
}

static void handle_ready_sysex(uint8_t *sysExData, unsigned length)
{
	clavier::johannus_antonijn_sysex_event key;
	int value;
	if (clavier::parse_johannus_antonijn_sysex(sysExData, length, key, value)
	 && key == clavier::GRANDORGUE_READY)
	{
		std::bitset<37> stops;
		get_logical_stops(stops);
		send_midi(stops);
	}
}

// the setup function runs once when you press reset or power the board
void setup()
{
	static const std::array<int, 16> reg_idces_1({
		13, 15, 17, 21, 19, 26, 18, 24,
		20, 16, 22, 14, 12, 10,  9, 11,
	});
	static const std::array<int, 16> reg_idces_2({
		33, 35, 32, 34, 29, 2, 31, 36,
		5, 6, 28, 30, 25, 27, 25, 1,
	});

	mcp[0].setup(0b001, reg_idces_1);
	mcp[1].setup(0b011, reg_idces_2);

	setupRegBouncer(0, 12);
	setupRegBouncer(1, 14);
	setupRegBouncer(2, 15);
	setupRegBouncer(3, 16);
	setupRegBouncer(4, 17);

	setupPistonBouncer(0, 3);
	setupPistonBouncer(1, 0);
	setupPistonBouncer(2, 2);
	setupPistonBouncer(3, 5);
	setupPistonBouncer(4, 1);
	setupPistonBouncer(5, 4);
	setupPistonBouncer(6, 13, INPUT);

	usbMIDI.setHandleSystemExclusive(handle_ready_sysex);
}

// the loop function runs over and over again forever
void loop()
{
	static std::bitset<37> prev_stops;

	usbMIDI.read();

	std::bitset<37> stops;
	get_logical_stops(stops);

#ifndef DIAG
	send_midi(stops, stops ^ prev_stops);
#endif

	prev_stops = stops;

#ifdef DIAG
	if (diag) serial_print_bits37(stops);
	if (diag) serial_print_bits7(pistons);
	if (diag) Serial.println();
	if (diag) {
		Serial.print(digitalRead(12));
		Serial.print(digitalRead(14));
		Serial.print(digitalRead(15));
		Serial.print(digitalRead(16));
		Serial.print(digitalRead(17));
		Serial.println();
	}
#endif
}
