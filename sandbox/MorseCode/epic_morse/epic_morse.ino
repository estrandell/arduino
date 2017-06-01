
#include <MorseCode.h>

static int ledPin1 = 12;
static int ledPin2 = 13;
static int btnPin1 = 2;
unsigned long button_time = 0;  
unsigned long last_button_time = 0; 

static int dot_ms = 200;
static int dash_ms = 3 * dot_ms;
static int multiplier = 1;

static int l[] = Morse(L);
static int o[] = Morse(O);
static int v[] = Morse(V);
static int e[] = Morse(E);

void setup() {
  Serial.begin(9600);  
  Serial.println("Ellu"); 
  
  pinMode(btnPin1, INPUT);
  digitalWrite(btnPin1, HIGH);
  attachInterrupt(0, OnButtonPressed, FALLING);
  
  pinMode(ledPin1, OUTPUT);
  pinMode(ledPin2, OUTPUT);
  digitalWrite(ledPin1, LOW);
  digitalWrite(ledPin2, LOW);
}

void loop() {
  Write(l);
  delay(dash_ms / multiplier);
  Write(o);
  delay(dash_ms / multiplier);
  Write(v);
  delay(dash_ms / multiplier);
  Write(e);
  delay(6 * dash_ms / multiplier);
}

void Write(int a[]) {
  for (int i = 1; i < a[0]; i++)
  {
    digitalWrite(ledPin1, HIGH);
    if (a[i] == SHORT)
      delay(dot_ms / multiplier);
    else
      delay(dash_ms / multiplier);
    digitalWrite(ledPin1, LOW);
    delay(dot_ms / multiplier);
  }
}

void OnButtonPressed() {  
  button_time = millis();
  if (button_time - last_button_time > 100) {
    if (multiplier == 1) {
      digitalWrite(ledPin2, HIGH);
      multiplier = 4;
    }
    else {
      digitalWrite(ledPin2, LOW);
      multiplier = 1;
    }

    last_button_time = button_time;
  }
}



