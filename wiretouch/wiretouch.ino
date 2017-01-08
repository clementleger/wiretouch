#include <Arduino.h>

#include <clsPCA9555.h>
#include <ST7036.h>
#include <Wire.h>
#include <EEPROM.h>
#include <Bounce2.h>
#include <avr/eeprom.h>

#define NOTE_B0  31
#define NOTE_C1  33
#define NOTE_CS1 35
#define NOTE_D1  37
#define NOTE_DS1 39
#define NOTE_E1  41
#define NOTE_F1  44
#define NOTE_FS1 46
#define NOTE_G1  49
#define NOTE_GS1 52
#define NOTE_A1  55
#define NOTE_AS1 58
#define NOTE_B1  62
#define NOTE_C2  65
#define NOTE_CS2 69
#define NOTE_D2  73
#define NOTE_DS2 78
#define NOTE_E2  82
#define NOTE_F2  87
#define NOTE_FS2 93
#define NOTE_G2  98
#define NOTE_GS2 104
#define NOTE_A2  110
#define NOTE_AS2 117
#define NOTE_B2  123
#define NOTE_C3  131
#define NOTE_CS3 139
#define NOTE_D3  147
#define NOTE_DS3 156
#define NOTE_E3  165
#define NOTE_F3  175
#define NOTE_FS3 185
#define NOTE_G3  196
#define NOTE_GS3 208
#define NOTE_A3  220
#define NOTE_AS3 233
#define NOTE_B3  247
#define NOTE_C4  262
#define NOTE_CS4 277
#define NOTE_D4  294
#define NOTE_DS4 311
#define NOTE_E4  330
#define NOTE_F4  349
#define NOTE_FS4 370
#define NOTE_G4  392
#define NOTE_GS4 415
#define NOTE_A4  440
#define NOTE_AS4 466
#define NOTE_B4  494
#define NOTE_C5  523
#define NOTE_CS5 554
#define NOTE_D5  587
#define NOTE_DS5 622
#define NOTE_E5  659
#define NOTE_F5  698
#define NOTE_FS5 740
#define NOTE_G5  784
#define NOTE_GS5 831
#define NOTE_A5  880
#define NOTE_AS5 932
#define NOTE_B5  988
#define NOTE_C6  1047
#define NOTE_CS6 1109
#define NOTE_D6  1175
#define NOTE_DS6 1245
#define NOTE_E6  1319
#define NOTE_F6  1397
#define NOTE_FS6 1480
#define NOTE_G6  1568
#define NOTE_GS6 1661
#define NOTE_A6  1760
#define NOTE_AS6 1865
#define NOTE_B6  1976
#define NOTE_C7  2093
#define NOTE_CS7 2217
#define NOTE_D7  2349
#define NOTE_DS7 2489
#define NOTE_E7  2637
#define NOTE_F7  2794
#define NOTE_FS7 2960
#define NOTE_G7  3136
#define NOTE_GS7 3322
#define NOTE_A7  3520
#define NOTE_AS7 3729
#define NOTE_B7  3951
#define NOTE_C8  4186
#define NOTE_CS8 4435
#define NOTE_D8  4699
#define NOTE_DS8 4978


#define LCD_WIDTH  16
#define LCD_HEIGHT 2

#define TOUCH_PIN 2
#define SPEAKER_PIN 8

#define ARRAY_SIZE(arr) (sizeof(arr) / sizeof((arr)[0]))

PCA9555 ioport(0x20);
ST7036 lcd(LCD_HEIGHT, LCD_WIDTH, 0x3E);
Bounce debouncer = Bounce(); 

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
void death_game();

struct game_entry {
	unsigned long long touch_count;
	unsigned long long touch_duration;
	unsigned long long total_duration;
};

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
		"Mort subite",
		death_game,
		NULL,
	},
  {
    "Historique",
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
  debouncer.update();
  return !debouncer.read();
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
        tone(SPEAKER_PIN, NOTE_B3, 100);
        menu[current_sel].sel_callback();
        display_menu(current_sel, entry_count, menu[current_sel]);
      }
    break;
    case BUTTON_DOWN:
      tone(SPEAKER_PIN, NOTE_G4, 100);
      if (current_sel != (entry_count - 1))
        current_sel++;
    break;
    case BUTTON_UP:
      tone(SPEAKER_PIN, NOTE_G4, 100);
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

int game_start()
{
 byte button;
 
  lcd.clear();  
  lcd.setCursor(0, 0);
  lcd.print("Go ?");
    delay(500);
 while (1) {
    button = button_pressed();
    if (button == BUTTON_OK)
      break;
    else if (button != BUTTON_NONE)
      return 1;
  }

  lcd.clear();  
  for (int i = 0; i < 3; i++) {
    lcd.setCursor(0, 0);
    lcd.println(3 - i);
    tone(SPEAKER_PIN, NOTE_E5, 700);
    delay(1000);
  }
  tone(SPEAKER_PIN, NOTE_E6, 1000);

  return 0;
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

  if (game_start())
    return;

	start_time = millis();
	while (1) {
		button = button_pressed();
		if (button == BUTTON_OK)
			break;

		/* Check wire state */
		touch_state = get_touch_state();
		if (touch_state != last_touch_state) {
			if (touch_state == true) {
				last_touch_time = millis();
       tone(SPEAKER_PIN, NOTE_C4);
			} else {
				total_touch_duration += (millis() - last_touch_time);
				touch_count++;
				updated = 1;
       noTone(SPEAKER_PIN);
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
}

 static void
death_game()
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

  if (game_start())
    return;
 
  start_time = millis();
  while (1) {
    button = button_pressed();
    if (button == BUTTON_OK)
      break;

    /* Check wire state */
    touch_state = get_touch_state();
    if (touch_state != last_touch_state) {
      if (touch_state == true) {
        last_touch_time = millis();
       tone(SPEAKER_PIN, NOTE_C4);
      } else {
        total_touch_duration += (millis() - last_touch_time);
        touch_count++;
        updated = 1;
        noTone(SPEAKER_PIN);
        break;
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
  lcd.print("Fin !");
  tone(SPEAKER_PIN, NOTE_E5, 200);
  delay(250);
  tone(SPEAKER_PIN, NOTE_E5, 200);
  delay(250);
  display_menu(0, ARRAY_SIZE(classic_game_menu), classic_game_menu[0]);
	do {
		button = handle_menu(&cur_sel, classic_game_menu, ARRAY_SIZE(classic_game_menu));
	} while(button != BUTTON_OK);

}

void
setup()
{
  Serial.begin(115200);

  Serial.println("TOTO1\n");
  pinMode(TOUCH_PIN, INPUT_PULLUP);
  debouncer.attach(TOUCH_PIN);
  debouncer.interval(5);

  Serial.println("TOTO2\n");
  for (uint8_t i = 0; i < 5; i++)
    ioport.pinMode(i, INPUT);

  ioport.pinMode(13, OUTPUT);
  
  ioport.digitalWrite(13, 1);
  Serial.println("TOTO\n");

  lcd.init();
  lcd.setContrast(255);
  lcd.load_custom_character(CHAR_UP, up);
  lcd.load_custom_character(CHAR_DOWN, down);
  lcd.load_custom_character(CHAR_UPDOWN, updown);

  lcd.clear();
  lcd.setCursor(0,0);
  lcd.print("Wire Touch V1");
  delay(1500);
  display_menu(0, ARRAY_SIZE(main_menu), main_menu[0]);   //  Print the first word

}

static byte cur_sel = 0;

void loop()
{
	handle_menu(&cur_sel, main_menu, ARRAY_SIZE(main_menu));
}
