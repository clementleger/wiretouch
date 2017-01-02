#include <Arduino.h>

#include <clsPCA9555.h>
#include <ST7036.h>
#include <Wire.h>
#include <EEPROM.h>
#include <avr/eeprom.h>

#define LCD_WIDTH  16
#define LCD_HEIGHT 2

#define TOUCH_PIN 2

#define ARRAY_SIZE(arr) (sizeof(arr) / sizeof((arr)[0]))

PCA9555 ioport(0x20);
ST7036 lcd(LCD_HEIGHT, LCD_WIDTH, 0x3E);

#define CHAR_UP  (byte(0))
byte up[8] = {
  0b00100,
  0b01110,
  0b11111,
  0b00000,
  0b00000,
  0b00000,
  0b00000,
  0b00000,
};

#define CHAR_UPDOWN 1
byte updown[8] = {
  0b00100,
  0b01110,
  0b11111,
  0b00000,
  0b11111,
  0b01110,
  0b00100,
  0b00000
};

#define CHAR_DOWN 2
byte down[8] = {
  0b00000,
  0b00000,
  0b00000,
  0b00000,
  0b11111,
  0b01110,
  0b00100,
  0b00000
};

void classic_game();

typedef struct {
	unsigned long long touch_count;
	unsigned long long touch_duration;
	unsigned long long total_duration;
} game_entry_t;

typedef struct {
	const char name[15];
	void (* sel_callback)(void);
	void (* disp_callback)(void);
} menu_entry_t;

menu_entry_t main_menu[] = {
	{
		"Partie normale",
		classic_game,
		NULL,
	},
	{
		"Historique",
		NULL,
		NULL,
	},
	{
		"Mort subite",
		NULL,
		NULL,
	}
};


const char *button[] = {
  "Up",
  "Ok",
  "Right",
  "Left",
  "Down"
};

typedef enum {
	BUTTON_NONE = 0,
	BUTTON_UP,
	BUTTON_OK,
	BUTTON_RIGHT,
	BUTTON_LEFT,
  BUTTON_DOWN,
} button_t;


static unsigned long long total_duration = 0, total_touch_duration = 0, touch_count = 0;

byte button_pressed()
{
  for (uint8_t i = 0; i < 5; i++){
    if (!ioport.digitalRead(i)) {
      delay(200);
      return i + 1;
    }
  }

  return BUTTON_NONE;
}

void disp_total_time()
{
	lcd.print((long) total_duration / 1000);
  lcd.print(" s ");
  lcd.print((long) total_duration % 1000);
  lcd.print(" ms");
}

void disp_touch_count()
{
	lcd.print((long)touch_count);
}

void disp_touch_duration()
{
	lcd.print((long)total_touch_duration);
  lcd.print(" ms");
}

menu_entry_t classic_game_menu[] = {
	{
		"Temps de jeu:",
		NULL,
		disp_total_time,
	},
	{
		"Touchettes:",
		NULL,
		disp_touch_count,
	},
	{
		"Duree",
		NULL,
		disp_touch_duration,
	}
};

bool get_touch_state()
{
  return !digitalRead(TOUCH_PIN);
}

static void
display_menu(byte sel, byte menu_size, menu_entry_t menu)
{
  lcd.clear();
  lcd.setCursor(0, 0);
  if (sel == 0)
    lcd.write((byte) CHAR_DOWN);
  else if (sel == (menu_size - 1))
    lcd.write((byte) CHAR_UP);
  else 
    lcd.write((byte) CHAR_UPDOWN);
  lcd.print(" ");
  lcd.print(menu.name);
  if (menu.disp_callback) {
    lcd.setCursor(1,0);
    menu.disp_callback();
  }
}

static byte
handle_menu(byte *cur_sel, menu_entry_t menu[], unsigned int entry_count)
{
  byte current_sel = *cur_sel;
  byte button;
  
  button = button_pressed();
  switch(button) {
    case BUTTON_OK:
      if (menu[current_sel].sel_callback) {
        menu[current_sel].sel_callback();
        display_menu(current_sel, entry_count, menu[current_sel]);
      }
    break;
    case BUTTON_DOWN:
      if (current_sel != (entry_count - 1))
        current_sel++;
    break;
    case BUTTON_UP:
      if (current_sel != 0)
        current_sel--;
    break;
  }

  if (current_sel != *cur_sel) {
    display_menu(current_sel, entry_count, menu[current_sel]);
    *cur_sel = current_sel; 
  }

  return button;
}

static void
classic_game()
{
	byte button;
	long touch_duration = 0;
	bool last_touch_state = false, touch_state;
	unsigned long last_touch_time = 0, start_time;
	bool updated = 1;
  byte cur_sel = 0;

	total_duration = 0;
	total_touch_duration = 0;
	touch_count = 0;

  lcd.clear();  
  lcd.setCursor(0, 0);
  lcd.print("Go ?");
	while (1) {
		button = button_pressed();
		if (button == BUTTON_OK)
			break;
		else if (button != BUTTON_NONE)
			return;
	}

	start_time = millis();
  delay(500);
	while (1) {
		button = button_pressed();
		if (button == BUTTON_OK)
			break;

		/* Check wire state */
		touch_state = get_touch_state();
		if (touch_state != last_touch_state) {
			if (touch_state == true) {
				last_touch_time = millis();
			} else {
				total_touch_duration += (millis() - last_touch_time);
				touch_count++;
				updated = 1;
			}
			/* Update touch_state */
			last_touch_state = touch_state;
		}
		if (updated) {		
			lcd.setCursor(0, 0);
			lcd.print("Touches: ");
			lcd.print((long)touch_count);
			lcd.setCursor(1,0);
			lcd.print("Duree: ");
			lcd.print((long)total_touch_duration);
      lcd.print(" ms");
      updated = 0;
		}
	}

  total_duration = millis() - start_time;
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Fini !");
  delay(2000);
  display_menu(0, ARRAY_SIZE(classic_game_menu), classic_game_menu[0]);
	do {
		button = handle_menu(&cur_sel, classic_game_menu, ARRAY_SIZE(classic_game_menu));
	} while(button != BUTTON_OK);

}



void
setup()
{
  Serial.begin(115200);

  pinMode(TOUCH_PIN, INPUT_PULLUP);

  for (uint8_t i = 0; i < 5; i++)
    ioport.pinMode(i, INPUT);

  ioport.pinMode(13, OUTPUT);
  
  ioport.digitalWrite(13, 1);

  lcd.init();
  lcd.load_custom_character(CHAR_UP, up);
  lcd.load_custom_character(CHAR_DOWN, down);
  lcd.load_custom_character(CHAR_UPDOWN, updown);

  lcd.clear();
  lcd.setCursor(0,0);
  lcd.print("Wire Touch V1");
  delay(2000);
  display_menu(0, ARRAY_SIZE(main_menu), main_menu[0]);   //  Print the first word

}

static byte cur_sel = 0;

void loop()
{
	handle_menu(&cur_sel, main_menu, ARRAY_SIZE(main_menu));
}
