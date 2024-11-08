#include <Adafruit_CharacterOLED.h>
#include <Bounce2.h>
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

const static int rs = 0, rw = 1, en = 2, db4 = 6, db5 = 4, db6 = 3, db7 = 5;
static Adafruit_CharacterOLED oled(OLED_V2, rs, rw, en, db4, db5, db6, db7);

#define REPEAT_TIME_MILLIS       500UL
#define REPEAT_PULSE_TIME_MILLIS 60UL

#define DEBOUNCE_INTERVAL 10

class button {
	Bounce bnce;
	unsigned long last_release_time = 0;
	unsigned long press_time = 0;
	const char *tty_data;

public:
	void init(int pin, const char *tty_data_, int interval = DEBOUNCE_INTERVAL)
	{
		pinMode(pin, INPUT_PULLUP);
		bnce.attach(pin);
		tty_data = tty_data_;
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

	const char *get_tty_data() const
	{
		return tty_data;
	}
};

#define LEFT      0
#define LEFT_PIN  15
#define DOWN      1
#define DOWN_PIN  14
#define UP        2
#define UP_PIN    13
#define RIGHT     3
#define RIGHT_PIN 10

#define NUM_BUTTONS 4

static button buttons[NUM_BUTTONS];

void setup()
{
	Serial.begin(9600);

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

	oled.clear();
	oled.setCursor(0, 0);
	oled.print("Loading...");

	buttons[LEFT ].init(LEFT_PIN,  "\x1b[D");
	buttons[DOWN ].init(DOWN_PIN,  "\x1b[B");
	buttons[UP   ].init(UP_PIN,    "\x1b[A");
	buttons[RIGHT].init(RIGHT_PIN, "\x1b[C");
}

static void read_line(char *buf, size_t len)
{
	memset(buf, 0, len + 1);

	for (size_t i = 0; ; ++i) {
		int ch;
		while ((ch = Serial.read()) < 0)
			;

		if (ch == '\n')
			break;

		if (i >= len) {
			memmove(buf, buf + 1, len - 1);
			i = len - 1;
		}

		buf[i] = ch;
	}
}

void loop()
{
	for (button& b : buttons) {
		b.update();
		if (b.pressed())
			Serial.print(b.get_tty_data());
	}

	if (Serial.available()) {
		char line_one[17];
		char line_two[17];

		read_line(line_one, sizeof(line_one) - 1);
		read_line(line_two, sizeof(line_two) - 1);

		oled.clear();

		oled.setCursor(0, 0);
		oled.print(line_one);

		oled.setCursor(0, 1);
		oled.print(line_two);
	}
}
