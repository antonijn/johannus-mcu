#include <Adafruit_CharacterOLED.h>
#include <Bounce2.h>
#include <limits.h>
#include <string.h>

#define GLYPH_QUAVER    "\x01"
#define GLYPH_ENTER     "\x02"
#define GLYPH_LEFT      "\x03"
#define GLYPH_RIGHT     "\x04"
#define GLYPH_DOT       "\x05"
#define GLYPH_HALF      "\x06"
#define GLYPH_THIRD     "\x07"
#define GLYPH_QUARTER   "\x08"
#define GLYPH_FIFTH     "\x09"

enum AntonijnSysexEvent {
	TRANSPOSE    = 0x01,
	TEMPERAMENT  = 0x02,
	TUNING       = 0x03,

	INSTRUMENT   = 0x04,

	GRANDORUGE_READY = 0x100,
	GRANDORGUE_GAIN = 0x101,
	GRANDORGUE_POLYPHONY = 0x102,

	GRANDORGUE_START_RECORDING = 0x141,
	GRANDORGUE_STOP_RECORDING = 0x142,
};

static void int_bytes_network_order(uint8_t *dest, int16_t value)
{
	uint16_t bits = (uint16_t)value;
	uint8_t bytes[] = {
		(bits >> 12) & 0xFU,
		(bits >> 8) & 0xFU,
		(bits >> 4) & 0xFU,
		(bits >> 0) & 0xFU,
	};

	for (int i = 0; i < sizeof(bytes); ++i)
		dest[i] = (bytes[i] < 10) ? ('0' + bytes[i]) : ('A' + (bytes[i] - 10));
}

static void sendAntonijnSysEx(enum AntonijnSysexEvent event, int value)
{
	static const uint8_t prefix[] = { 'J', 'O', 'H', 'A', 'N', 'N', 'U', 'S', 'A', 'N', 'T', 'O', 'N', 'I', 'J', 'N' };

	uint8_t message[sizeof(prefix) + 4 + 4];
	memcpy(message, prefix, sizeof(prefix));
	int_bytes_network_order(message + sizeof(prefix), (int)event);
	int_bytes_network_order(message + sizeof(prefix) + 4, value);
	usbMIDI.sendSysEx(sizeof(message), message, false);
}

const static int rs = 0, rw = 1, en = 2, db4 = 6, db5 = 4, db6 = 3, db7 = 5;
static Adafruit_CharacterOLED oled(OLED_V2, rs, rw, en, db4, db5, db6, db7);

static int npow(int b, int e)
{
	if (e == 0)
		return 1;

	int e1 = e >> 1;
	int e2 = e & 1;

	int p1 = npow(b, e1);
	int p2 = e2 ? b : 1;
	return (p1 * p1) * p2;
}

// character count, not strictly speaking a log10
static int nlog10(int n)
{
	if (n < 0)
		return 1 + nlog10(-n);

	if (n < 10)
		return 1;

	return 1 + nlog10(n / 10);
}

class SubMenu;

class Screen {
public:
	Screen *left_, *right_;
	SubMenu *up_;

	virtual void up(Screen *& screen);
	virtual void down(Screen *& screen) = 0;
	virtual void left(Screen *& screen);
	virtual void right(Screen *& screen);
	virtual void show() = 0;

	void right_of(Screen& scr);
	void place_in(SubMenu& scr);
};

class SubMenu: public Screen {
	const char *text_;

public:
	Screen *down_;

	void init(const char *text);

	void down(Screen *& screen) override;
	void show() override;
};

void Screen::up(Screen *& screen)
{
	if (up_ != nullptr) {
		screen = up_;
		up_->show();

		up_->down_ = this;
	}
}
void Screen::left(Screen *& screen)
{
	if (left_ != nullptr) {
		screen = left_;
		left_->show();
	}
}
void Screen::right(Screen *& screen)
{
	if (right_ != nullptr) {
		screen = right_;
		right_->show();
	}
}

void Screen::right_of(Screen& scr)
{
	scr.right_ = this;
	left_ = &scr;
	up_ = scr.up_;
}

void Screen::place_in(SubMenu& scr)
{
	up_ = &scr;
	scr.down_ = this;
}

void SubMenu::init(const char *text)
{
	text_ = text;
}

void SubMenu::down(Screen *& screen)
{
	screen = down_;
	down_->show();
}

void SubMenu::show()
{
	oled.clear();
	oled.setCursor(2, 0);
	oled.print(text_);
	oled.setCursor(0, 1);
	oled.print(GLYPH_ENTER " (menu)");

	if (left_ != nullptr) {
		oled.setCursor(0, 0);
		oled.print("<");
	}
	if (right_ != nullptr) {
		oled.setCursor(14, 0);
		oled.print(" >");
	}
}

class IntOption: public Screen {
public:
	typedef void (*Callback)(int);
protected:
	const char *name_;
	int value_;
	int min_, max_, dflt_;
	bool active_ = false;
	Callback change_callback;

	virtual void print_num() = 0;

public:
	void init(const char *name,
	          int dflt = 0,
	          int min = INT_MIN, int max = INT_MAX)
	{
		name_ = name;
		value_ = dflt_ = dflt;
		min_ = min;
		max_ = max;
	}

	void set_callback(Callback change_callback)
	{
		this->change_callback = change_callback;
	}

	void up(Screen *& screen) override
	{
		if (active_) {
			active_ = false;
			show();
		} else {
			Screen::up(screen);
		}
	}

	void down(Screen *& screen) override
	{
		if (!active_) {
			active_ = true;
			show();
		}
	}

	void left(Screen *& screen) override
	{
		if (active_) {
			if (value_ > min_) {
				--value_;
				if (change_callback != nullptr)
					change_callback(value_);
				show();
			}
		} else {
			Screen::left(screen);
		}
	}
	void right(Screen *& screen) override
	{
		if (active_) {
			if (value_ < max_) {
				++value_;
				if (change_callback != nullptr)
					change_callback(value_);
				show();
			}
		} else {
			Screen::right(screen);
		}
	}

	void show() override
	{
		oled.clear();
		oled.setCursor(2, 0);
		oled.print(name_);

		if (active_) {
			if (value_ > min_) {
				oled.setCursor(0, 1);
				oled.print("<");
			}
			if (value_ < max_) {
				oled.setCursor(14, 1);
				oled.print(" >");
			}
		} else {
			if (left_ != nullptr) {
				oled.setCursor(0, 0);
				oled.print("<");
			}
			if (right_ != nullptr) {
				oled.setCursor(14, 0);
				oled.print(" >");
			}
		}

		print_num();
	}
};

class SimpleIntOption: public IntOption {
public:
	int power_of_10 = 0;
	const char *postfix = nullptr;

protected:
	void print_num() override
	{
		int num_cursor_col = 2;

		oled.setCursor(num_cursor_col, 1);
		if (min_ < 0 && value_ >= 0) {
			if (value_ > 0)
				oled.print("+");
			++num_cursor_col;
		}

		oled.setCursor(num_cursor_col, 1);

		if (power_of_10 < 0) {
			int p = npow(10, -power_of_10);
			int sign = value_ < 0 ? -1 : 1;
			int v = value_ * sign;
			int before_point = v / p;
			int after_point = v - before_point * p;
			int npadding_zeros = max(0, -power_of_10 - nlog10(after_point));

			if (sign < 0)
				oled.print("-");
			oled.print(before_point);
			oled.print(".");
			for (int i = 0; i < npadding_zeros; ++i)
				oled.print("0");
			oled.print(after_point);
		} else {
			int p = npow(10, power_of_10);
			oled.print(p * value_);
		}

		if (postfix != nullptr) {
			oled.setCursor(13 - strlen(postfix), 1);
			oled.print(" ");
			oled.print(postfix);
		}
	}
};

class StringIntOption: public IntOption {
	const char **strings;

public:
	void init(const char *name, const char **strings, int num_strings)
	{
		IntOption::init(name, 0, 0, num_strings - 1);
		this->strings = strings;
	}

	void print_num() override
	{
		oled.setCursor(2, 1);
		oled.print(strings[value_]);
	}
};

class RecordingOption: public Screen {
private:
	bool recording;
public:
	void up(Screen *& screen) override
	{
		if (recording) {
			recording = false;
			sendAntonijnSysEx(GRANDORGUE_STOP_RECORDING, 0);
			show();
		} else {
			Screen::up(screen);
		}
	}

	void down(Screen *& screen) override
	{
		if (!recording) {
			recording = true;
			sendAntonijnSysEx(GRANDORGUE_START_RECORDING, 0);
			show();
		}
	}

	void left(Screen *& screen) override
	{
		if (!recording)
			Screen::left(screen);
	}
	void right(Screen *& screen) override
	{
		if (!recording)
			Screen::right(screen);
	}

	void show() override
	{
		oled.clear();
		oled.setCursor(2, 0);
		oled.print("Audio recorder");

		if (!recording) {
			if (left_ != nullptr) {
				oled.setCursor(0, 0);
				oled.print("<");
			}
			if (right_ != nullptr) {
				oled.setCursor(14, 0);
				oled.print(" >");
			}

			oled.setCursor(2, 1);
			oled.print("START REC");
		} else {
			oled.setCursor(0, 1);
			oled.print("^ STOP REC");
		}
	}
};

#define REPEAT_TIME_MILLIS       500UL
#define REPEAT_PULSE_TIME_MILLIS 60UL

#define DEBOUNCE_INTERVAL 10

class button {
	Bounce bnce;
	unsigned long last_release_time = 0;
	unsigned long press_time = 0;

public:
	void init(int pin, int interval = DEBOUNCE_INTERVAL)
	{
		pinMode(pin, INPUT_PULLUP);
		bnce.attach(pin);
		bnce.interval(interval);
	}

	void update()
	{
		bnce.update();
		/* inverted */
		if (!bnce.read())
			last_release_time = millis();

		if (bnce.fell())
			press_time = millis();
	}

	bool pressed()
	{
		if (bnce.fell())
			return true;

		auto time = millis();
		if (!bnce.read() && time - press_time > REPEAT_TIME_MILLIS) {
			press_time = time - (REPEAT_TIME_MILLIS - REPEAT_PULSE_TIME_MILLIS);
			return true;
		}

		return false;
	}
};

#define LEFT    0
#define DOWN    1
#define UP      2
#define RIGHT   3

#define NUM_BUTTONS 4

static button buttons[NUM_BUTTONS];

static Screen *active_screen;
static SubMenu technical_opts;
static SimpleIntOption transpose, tuning, gain, polyphony;
static StringIntOption instrument, temperament;
static RecordingOption recorder;

void setup()
{
	oled.begin(16, 2);

	static uint8_t char_enter_map[][8] = {
		{
			0b00100,
			0b00110,
			0b00101,
			0b00101,
			0b11100,
			0b11100,
			0b11000,
			0b00000,
		},
		{
			0b00000,
			0b10000,
			0b10000,
			0b10000,
			0b10010,
			0b11111,
			0b00010,
			0b00000,
		},
		{
			0b00010,
			0b00110,
			0b01110,
			0b11110,
			0b01110,
			0b00110,
			0b00010,
			0b00000,
		},
		{
			0b01000,
			0b01100,
			0b01110,
			0b01111,
			0b01110,
			0b01100,
			0b01000,
			0b00000,
		},
		{
			0b00001,
			0b00000,
			0b00000,
			0b00000,
			0b00000,
			0b00000,
			0b00000,
			0b00000,
		},
		{
			0b10001,
			0b10010,
			0b00100,
			0b01000,
			0b10111,
			0b00010,
			0b00100,
			0b00111,
		},
		{
			0b10001,
			0b10010,
			0b00100,
			0b01110,
			0b10001,
			0b00111,
			0b00001,
			0b00111,
		},
		{
			0b10001,
			0b10010,
			0b00100,
			0b01000,
			0b10101,
			0b00101,
			0b00011,
			0b00001,
		},
		{
			0b10001,
			0b10010,
			0b00100,
			0b01000,
			0b10111,
			0b00101,
			0b00011,
			0b00111,
		},
	};
	oled.createChar(GLYPH_QUAVER[0], char_enter_map[0]);
	oled.createChar(GLYPH_ENTER[0], char_enter_map[1]);
	oled.createChar(GLYPH_LEFT[0], char_enter_map[2]);
	oled.createChar(GLYPH_RIGHT[0], char_enter_map[3]);
	oled.createChar(GLYPH_DOT[0], char_enter_map[4]);
	oled.createChar(GLYPH_HALF[0], char_enter_map[5]);
	oled.createChar(GLYPH_THIRD[0], char_enter_map[6]);
	oled.createChar(GLYPH_QUARTER[0], char_enter_map[7]);
	oled.createChar(GLYPH_FIFTH[0], char_enter_map[8]);

	static const int button_pins[NUM_BUTTONS] = { 15, 14, 13, 10 };
	for (int i = 0; i < NUM_BUTTONS; ++i)
		buttons[i].init(button_pins[i]);

	technical_opts.init("GrandOrgue");

	static const char *instr_strings[] = {
		"Modern Organ",
		"Positif",
		"Harpsichord",
	};
	const auto num_instrs = sizeof(instr_strings) / sizeof(instr_strings[0]);
	instrument.init("Instrument", instr_strings, num_instrs);
	instrument.set_callback([](int i) {
		sendAntonijnSysEx(INSTRUMENT, i);
	});
	transpose.init("Transpose", 0, -11, 11);
	transpose.set_callback([](int t) {
		sendAntonijnSysEx(TRANSPOSE, t);
	});
	tuning.init("Tuning", 440, 415, 480);
	tuning.postfix = "Hz";
	tuning.set_callback([](int t) {
		sendAntonijnSysEx(TUNING, t);
	});

	static const char *tmpnt_strings[] = {
		"Original",
		"Equal",
		"Kirnberger",
		"Werckmeister",
		"Bach/Lehman",
		"1/4 Meantone",
		"1/5 Meantone",
		"1/6 Meantone",
		"2/7 Meantone",
		"Pythagorean",
	};
	const auto num_temps = sizeof(tmpnt_strings) / sizeof(tmpnt_strings[0]);
	temperament.init("Temperament", tmpnt_strings, num_temps);
	temperament.set_callback([](int t) {
		sendAntonijnSysEx(TEMPERAMENT, t);
	});
	gain.init("Gain", -15, -30, 10);
	gain.postfix = "dB";
	gain.set_callback([](int g) {
		sendAntonijnSysEx(GRANDORGUE_GAIN, g);
	});
	polyphony.init("Polyphony", 25, 5, 50);
	polyphony.power_of_10 = 2;
	polyphony.set_callback([](int poly) {
		sendAntonijnSysEx(GRANDORGUE_POLYPHONY, poly);
	});

	instrument.right_of(technical_opts);
	transpose.right_of(instrument);
	temperament.right_of(transpose);
	tuning.right_of(temperament);
	recorder.right_of(tuning);

	gain.place_in(technical_opts);
	polyphony.right_of(gain);

	active_screen = &instrument;
	active_screen->show();
}

static bool button_rose(int btn)
{
	buttons[btn].update();
	return buttons[btn].pressed();
}

void loop()
{
	auto up = button_rose(UP);
	auto down = button_rose(DOWN);
	auto left = button_rose(LEFT);
	auto right = button_rose(RIGHT);

	static auto last_press = 0UL;
	auto time = millis();

	static auto previously_timed_out = false;

	if (up | down | left | right)
		last_press = time;

	auto timed_out = time - last_press > 2ul * 60ul * 1000ul;

	if (!timed_out) {
		if (previously_timed_out) {
			active_screen->show();
		} else {
			if (up)
				active_screen->up(active_screen);
			else if (down)
				active_screen->down(active_screen);
			else if (left)
				active_screen->left(active_screen);
			else if (right)
				active_screen->right(active_screen);
		}
	} else if (!previously_timed_out) {
		oled.clear();
	}

	previously_timed_out = timed_out;
}
