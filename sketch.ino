#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include <Keypad.h>

LiquidCrystal_I2C lcd(0x27, 16, 2);

const byte ROWS = 4, COLS = 4;
char keys[ROWS][COLS] = {
  {'1','2','3','A'},
  {'4','5','6','B'},
  {'7','8','9','C'},
  {'<','0','=','D'}
};
byte rowPins[ROWS] = {13, 12, 14, 27};
byte colPins[COLS] = {26, 25, 33, 32};
Keypad kpad = Keypad(makeKeymap(keys), rowPins, colPins, ROWS, COLS);

#define LED_RED    2
#define LED_GREEN  4
#define LED_BLUE   5
#define BUZZER     18

enum Role { ROLE_NONE = 0, ROLE_ADMIN, ROLE_OPERATOR, ROLE_VIEWER };

struct User {
  const char* id;
  Role        role;
  const char* name;      // shown on LCD row 0
  const char* welcome;   // shown on LCD row 1
};

const User USERS[] = {
  {"1234", ROLE_ADMIN,    "Alice (Admin)", "Welcome, Boss! "},
  {"5678", ROLE_OPERATOR, "Bob (Operator)", "Welcome, Bob!  "},
  {"9012", ROLE_OPERATOR, "Carol (Oper.) ", "Welcome, Carol!"},
  {"3456", ROLE_VIEWER,   "Dave (Viewer) ", "Welcome, Dave! "},
};
const int USER_COUNT = sizeof(USERS) / sizeof(USERS[0]);

enum State { ST_IDLE, ST_INPUT, ST_GRANTED, ST_DENIED, ST_LOCKOUT };
State state = ST_IDLE;

char          inputBuf[5] = {0};
int           inputLen    = 0;
int           failCount   = 0;
unsigned long lastKeyTime = 0;
unsigned long stateTimer  = 0;

#define TIMEOUT_MS   7000UL
#define LOCKOUT_MS  15000UL
#define FEEDBACK_MS  3000UL  

void allLedsOff() {
  digitalWrite(LED_GREEN, LOW);
  digitalWrite(LED_BLUE,  LOW);
  digitalWrite(LED_RED,   LOW);
}

void blinkLed(int pin, int times, int onMs, int offMs) {
  for (int i = 0; i < times; i++) {
    digitalWrite(pin, HIGH); delay(onMs);
    digitalWrite(pin, LOW);  delay(offMs);
  }
}

/*
 *  Alice  ADMIN    – green rapid triple-blink then solid green
 *  Bob    OPERATOR – blue double slow pulse then solid blue
 *  Carol  OPERATOR – alternating blue/green twice then solid blue
 *  Dave   VIEWER   – single long red flash then solid blue
 */
void successSignal(const User* u) {
  allLedsOff();
  switch (u->role) {
    case ROLE_ADMIN:
      blinkLed(LED_GREEN, 3, 120, 80);
      digitalWrite(LED_GREEN, HIGH);
      tone(BUZZER, 1000, 80); delay(100);
      tone(BUZZER, 1500, 80); delay(100);
      tone(BUZZER, 2000, 200);
      break;

    case ROLE_OPERATOR:
      if (strcmp(u->id, "5678") == 0) {
        blinkLed(LED_BLUE, 2, 250, 150);
        digitalWrite(LED_BLUE, HIGH);
        tone(BUZZER, 1200, 150); delay(200);
        tone(BUZZER, 1200, 150);
      } else {
        for (int i = 0; i < 2; i++) {
          digitalWrite(LED_BLUE,  HIGH); delay(200);
          digitalWrite(LED_BLUE,  LOW);
          digitalWrite(LED_GREEN, HIGH); delay(200);
          digitalWrite(LED_GREEN, LOW);
        }
        digitalWrite(LED_BLUE, HIGH);
        tone(BUZZER, 1100, 100); delay(150);
        tone(BUZZER, 1400, 150);
      }
      break;

    default: 
      digitalWrite(LED_RED, HIGH); delay(400);
      digitalWrite(LED_RED, LOW);  delay(100);
      digitalWrite(LED_BLUE, HIGH);
      tone(BUZZER, 900, 400);
      break;
  }
}

void deniedSignal() {
  digitalWrite(LED_RED, HIGH);
  tone(BUZZER, 300, 600);
}

void lockoutSignal() {
  for (int i = 0; i < 3; i++) {
    digitalWrite(LED_RED, HIGH); delay(200);
    digitalWrite(LED_RED, LOW);  delay(200);
  }
  digitalWrite(LED_RED, HIGH);
  tone(BUZZER, 200, 1000);
}

void lcdRow(int r, const char* msg) {
  lcd.setCursor(0, r);
  lcd.print(msg);
}

void showIdle() {
  lcd.clear();
  lcdRow(0, " SECURE TERMINAL");
  lcdRow(1, "  Enter ID: ____");
}

void showInputMasked() {
  char mask[17] = "  Enter ID: ____";
  for (int i = 0; i < inputLen && i < 4; i++) mask[12 + i] = '*';
  lcd.clear();
  lcdRow(0, " SECURE TERMINAL");
  lcdRow(1, mask);
}

void showGranted(const User* u) {
  lcd.clear();
  lcdRow(0, u->name);
  lcdRow(1, u->welcome);
}

void showDenied() {
  lcd.clear();
  lcdRow(0, "  ACCESS DENIED ");
  char buf[17];
  snprintf(buf, 17, " Attempt %d of 3 ", failCount);
  lcdRow(1, buf);
}

void showLockout(unsigned long remaining) {
  lcd.clear();
  lcdRow(0, "*** LOCKED OUT **");
  char buf[17];
  snprintf(buf, 17, "  Wait %2lus ...  ", remaining / 1000 + 1);
  lcdRow(1, buf);
}

const User* findUser(const char* id) {
  for (int i = 0; i < USER_COUNT; i++)
    if (strcmp(USERS[i].id, id) == 0) return &USERS[i];
  return nullptr;
}

void resetInput() {
  memset(inputBuf, 0, sizeof(inputBuf));
  inputLen    = 0;
  lastKeyTime = 0;
}

void setup() {
  Serial.begin(115200);
  pinMode(LED_RED,   OUTPUT);
  pinMode(LED_GREEN, OUTPUT);
  pinMode(LED_BLUE,  OUTPUT);
  pinMode(BUZZER,    OUTPUT);
  allLedsOff();

  Wire.begin(21, 22);
  lcd.init();
  lcd.backlight();

  showIdle();
  state = ST_IDLE;
  Serial.println("[BOOT] Terminal ready.");
}

void loop() {
  unsigned long now = millis();

  if (state == ST_LOCKOUT) {
    unsigned long elapsed = now - stateTimer;
    if (elapsed >= LOCKOUT_MS) {
      allLedsOff();
      failCount = 0;
      resetInput();
      state = ST_IDLE;
      showIdle();
    } else {
      static unsigned long lastLockUpdate = 0;
      if (now - lastLockUpdate > 500) {
        lastLockUpdate = now;
        showLockout(LOCKOUT_MS - elapsed);
      }
    }
    return;
  }

  if (state == ST_GRANTED || state == ST_DENIED) {
    if (now - stateTimer >= FEEDBACK_MS) {
      allLedsOff();
      noTone(BUZZER);
      resetInput();
      state = ST_IDLE;
      showIdle();
    }
    return;
  }

  if (state == ST_INPUT && inputLen > 0) {
    if (now - lastKeyTime >= TIMEOUT_MS) {
      Serial.println("[TIMEOUT] Input cleared.");
      resetInput();
      state = ST_IDLE;
      showIdle();
      return;
    }
  }

  char key = kpad.getKey();
  if (!key) return;

  tone(BUZZER, 1200, 40);
  lastKeyTime = now;
  Serial.printf("[KEY] '%c'\n", key);

  if (key == '<') {   
    if (inputLen > 0) {
      inputLen--;
      inputBuf[inputLen] = 0;
      if (inputLen == 0) { state = ST_IDLE;  showIdle(); }
      else               { state = ST_INPUT; showInputMasked(); }
    }
    return;
  }

  if (key == '=') {   
    if (inputLen < 4) {
      lcd.clear();
      lcdRow(0, " INCOMPLETE ID  ");
      lcdRow(1, " Need 4 digits  ");
      delay(800);
      showInputMasked();
      return;
    }
    inputBuf[4] = 0;
    const User* u = findUser(inputBuf);
    if (u) {
      failCount = 0;
      successSignal(u);       
      showGranted(u);         
      state      = ST_GRANTED;
      stateTimer = millis();
      Serial.printf("[GRANTED] %s\n", u->name);
    } else {
      failCount++;
      Serial.printf("[DENIED] fail=%d\n", failCount);
      if (failCount >= 3) {
        lockoutSignal();
        state      = ST_LOCKOUT;
        stateTimer = millis();
        showLockout(LOCKOUT_MS);
      } else {
        deniedSignal();
        showDenied();
        state      = ST_DENIED;
        stateTimer = millis();
      }
    }
    return;
  }

  if (inputLen < 4 && key >= '0' && key <= '9') {
    inputBuf[inputLen++] = key;
    inputBuf[inputLen]   = 0;
    state = ST_INPUT;
    showInputMasked();
  }
}