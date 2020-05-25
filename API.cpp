class Messwert{
  private:
    float aktuellsterWert;
    float durchschnittsWert;
    int numMessung;
    float* messung;
    int index;
    unsigned long intervall;
    unsigned long rolltime;
  public:
    Messwert();
    Messwert(int, unsigned long);
    void setzeWert(float);
    float gebeWert();
    float gebeDurchschnitt();
    void setzeNumMessung(int);
    void setzteIntervall(unsigned long);
    void initialisiere(float);
};

Messwert::Messwert(){
  aktuellsterWert = 0;
  durchschnittsWert = 0;
  numMessung = 10;
  messung = new float[numMessung];
  index = 0;
  intervall = 5000;
  rolltime = millis() + intervall;
}

Messwert::Messwert(int numMessung, unsigned long intervall){
  aktuellsterWert = 0;
  durchschnittsWert = 0;
  this->numMessung = numMessung;
  messung = new float[numMessung];
  this-> intervall = intervall;
  rolltime = millis() + intervall;
  index = 0;
}

void Messwert::setzeWert(float messwert){
  if((long)(millis() - rolltime) >= 0)
  {
    aktuellsterWert = messwert;
    messung[index] = messwert;
    index++;
    if (index >= numMessung)
    {
      index = 0;
    }
    rolltime += intervall;
  }
}

float Messwert::gebeWert(){
  return aktuellsterWert;
}

float Messwert::gebeDurchschnitt(){
  float summe = 0;
  for(int i = 0; i < numMessung; i++)
  {
    summe += messung[i];
  }
  durchschnittsWert = summe/numMessung;
  return durchschnittsWert;
}

void Messwert::setzeNumMessung(int anzahl){
  if(anzahl == numMessung)
  {
    return;
  }
  else
  {
    float* neu = new float[numMessung];
    for(int i = 0; i<numMessung; i++)
    {
      neu[i] = messung[i];
    }
    delete[] messung;
    messung = new float[anzahl];
    if(anzahl < numMessung)
    {
      for(int i = 0; i < anzahl; i++)
      {
        messung[i] = neu[i];
      }  
    }
    else if(anzahl > numMessung)
    {
      for(int i = 0; i < anzahl; i++)
      {
        if(i > numMessung)
        {
          messung[i] = aktuellsterWert;
        }
        else
        {
          messung[i] = neu[i];
        }
      }
    }
    delete[] neu;
    neu = NULL;
  }
  
}

void Messwert::setzteIntervall(unsigned long intervall){
  this->intervall = intervall;
}

void Messwert::initialisiere(float messwert){
  for(int i = 0; i<numMessung; i++)
  {
    messung[i] = messwert;
  }
}



String menuEintrag[] = {"Temperatur", "Luftfeuchte", "Krit.Temperatur", "Krit.Luftfeuchte", "Jalousie", "Über"};

// Variablen für die Menünavigation mit dem Tastern
int tasterWert;
int savedDistance = 0;

// Menü Variablen
int menuSeite = 0;
int maxMenuSeiten = round(((sizeof(menuEintrag) / sizeof(String)) / 2) + .5);
int cursorPosition = 0;

// Erstellung von 3 Zeichen für das Menü Display
byte pfeilUnten[8] = {
  0b00100, //   *
  0b00100, //   *
  0b00100, //   *
  0b00100, //   *
  0b00100, //   *
  0b10101, // * * *
  0b01110, //  ***
  0b00100  //   *
};

byte pfeilOben[8] = {
  0b00100, //   *
  0b01110, //  ***
  0b10101, // * * *
  0b00100, //   *
  0b00100, //   *
  0b00100, //   *
  0b00100, //   *
  0b00100  //   *
};

byte menuCursor[8] = {
  B01000, //  *
  B00100, //   *
  B00010, //    *
  B00001, //     *
  B00010, //    *
  B00100, //   *
  B01000, //  *
  B00000  //
};

#include <Wire.h>
#include <LiquidCrystal.h>
#include <DHT.h>
#include <DHT_U.h>
#include <Servo.h>
#include <Stepper.h>

// Setzt die LCD-Shield Pins
LiquidCrystal lcd(8, 9, 4, 5, 6, 7);
int helligkeitLCD = 255;

// DHT22-Sensor
#define DHTPIN 28               
#define DHTTYPE DHT22         
DHT dht(DHTPIN, DHTTYPE);  

//Servo Motor
#define SERVOPIN 52
Servo servoJal;
int Winkel=0;

//Schrittmotor
int SPU = 2048;                       //Schritte pro Umdrehung
Stepper motor(SPU, 37, 33, 35, 31);

//MQSENSOR
#define MQANALOG 8
#define MQDIGITAL 32

//LEDs
#define LEDgruenPIN 22
#define LEDgelbPIN 24
#define LEDrotPIN 26
#define LEDwarnPIN 30

//Fotowiderstand
#define FOTOPIN A9
int sensorWert=0;

//Messwert initialisierung
Messwert luftfeuchtigkeit(10, 2000);
Messwert co2(10, 5000);
Messwert temperatur(10, 2000);
Messwert helligkeit(10, 500);

#define tempPindraussen 0;
int Treading, Unterschied;
bool fensterStatus = 0;
bool reglerJal = 1;
float tempCdraussen, tempK, kritischeTemp, kritLuftfeuchte;
bool tempUnits = false; // false für Celsius, true für Kelvin
unsigned long rolltime_fenster = 0;
int intervall_fenster = 300000;      // 5 Minuten

void setup() {

  // Initialisiert die Serielle Kommunikation für Testzwecke
  Serial.begin(9600);

  kritischeTemp = 20;                          //Setze krtische Temperatur beim Start auf 20°C
  kritLuftfeuchte = 70;                        //Setze kritische Luftfeuchtigkeit beim Start auf 70%

  // Initialisiert den LCD Display
  lcd.begin(16, 2);
  lcd.clear();
  analogWrite(10, helligkeitLCD);

  // Erstelle Bytes für die drei erstellten Symbole
  lcd.createChar(0, menuCursor);
  lcd.createChar(1, pfeilOben);
  lcd.createChar(2, pfeilUnten);

  dht.begin();                                  //DHT22 Sensor starten 
  motor.setSpeed(5);                            //Motor für das Fenster starten
  servoJal.attach(SERVOPIN);                    //Servo für die Jalousie starten

  pinMode(MQDIGITAL, INPUT);
  pinMode(LEDwarnPIN, OUTPUT);                      
  pinMode(LEDgruenPIN, OUTPUT);  
  pinMode(LEDgelbPIN, OUTPUT);                 
  pinMode(LEDrotPIN, OUTPUT);

  temperatur.initialisiere(dht.readTemperature());
  luftfeuchtigkeit.initialisiere(dht.readHumidity());
  co2.initialisiere(analogRead(MQANALOG));
  helligkeit.initialisiere(analogRead(FOTOPIN));
  tempCdraussen = 33.15;
}

void loop() {
  erstelleHauptmenu();
  zeigeCursor();
  zeigeHauptmenu();
}

// Diese Funktion generiert zwei Menupunkte, je nachdem wie weit man durch das Menü scrollt.
// Die erstellten Symbole pfeilUnten & pfeilOben zeigen, wo man sich im Menü befindet
void erstelleHauptmenu() {
  Serial.print(menuSeite);
  lcd.clear();
  lcd.setCursor(1, 0);
  lcd.print(menuEintrag[menuSeite]);
  lcd.setCursor(1, 1);
  lcd.print(menuEintrag[menuSeite + 1]);
  if (menuSeite == 0) {
    lcd.setCursor(15, 1);
    lcd.write(byte(2));
  } else if (menuSeite > 0 and menuSeite < maxMenuSeiten) {
    lcd.setCursor(15, 1);
    lcd.write(byte(2));
    lcd.setCursor(15, 0);
    lcd.write(byte(1));
  } else if (menuSeite == maxMenuSeiten) {
    lcd.setCursor(15, 0);
    lcd.write(byte(1));
  }
}

// Der aktuelle Cursor wird gelöscht und der neue an die richtige Stelle gesetzt
void zeigeCursor() {
  for (int x = 0; x < 2; x++) {     // Lösche aktuellen Cursor
    lcd.setCursor(0, x);
    lcd.print(" ");
  }
  // (menuSeite 0 =  Eintrag 1 & Eintrag 2, menuSeite 1 = Eintrag 2 & Eintrag 3, menuSeite 2 = Eintrag 3 & Eintrag 4 ect.)
  // Um die Cursorposition festzulegen muss geprüft werden, ob die Menüseite und die Cursor-Position gerade oder ungerade ist
    if (menuSeite % 2 == 0) {
    if (cursorPosition % 2 == 0) {  // Wenn die Menü-Seite gerade ist und die Cursor-Position gerade, dann kommt der Cursor in die Zeile 1
      lcd.setCursor(0, 0);          
      lcd.write(byte(0));
    }
    if (cursorPosition % 2 != 0) {  // Wenn die Menü-Seite gerade ist und die Cursor-Position ungerade, dann kommt der Cursor in die Zeile 2
      lcd.setCursor(0, 1);          
      lcd.write(byte(0));
    }
  }
  if (menuSeite % 2 != 0) {
    if (cursorPosition % 2 == 0) {  // Wenn die Menü-Seite ungerade ist und die Cursor-Position gerade, dann kommt der Cursor in die Zeile 2
      lcd.setCursor(0, 1);          
      lcd.write(byte(0));
    }
    if (cursorPosition % 2 != 0) {  // Wenn die Menü-Seite ungerade ist und die Cursor-Position ungerade, dann kommt der Cursor in die Zeile 1
      lcd.setCursor(0, 0);          
      lcd.write(byte(0));
    }
  }
}

//Diese Funktion verwaltet das Hauptmenü
void zeigeHauptmenu() {
  int aktiveTaste = 0;
  while (aktiveTaste == 0) {
    int button;
    check();
    tasterWert = analogRead(0);
    if (tasterWert < 790) {
      delay(100);
      tasterWert = analogRead(0);
    }
    button = evaluiereTaste(tasterWert);
    switch (button) {
      case 0:   //Wenn keine Taste gedrückt wurde, dann soll nichts passieren
        break;
      case 1:   //Wenn die Taste 'rechts' gedrückt wird...
        button = 0;
        switch (cursorPosition) { //...wird in das Untermenü gewechselt, auf das der Cursor zeigt
          case 0:
            unterMenu1();
            break;
          case 1:
            unterMenu2();
            break;
          case 2:
            unterMenu3();
            break;
          case 3:
            unterMenu4();
            break;
          case 4:
            unterMenu5();
            break;
        }                       //Wenn das Untermenü verlassen wurde wird das Hauptmenü aktualisiert und der Cursor neu gesetzt
        aktiveTaste = 1;
        erstelleHauptmenu(); 
        zeigeCursor();
        break;
      case 2:         //Wenn die Taste 'oben' gedrückt wird werden Menü-Seite und Cursor-Position aktualisiert
        button = 0;
        if (menuSeite == 0) {
          cursorPosition = cursorPosition - 1;
          cursorPosition = constrain(cursorPosition, 0, ((sizeof(menuEintrag) / sizeof(String)) - 1));
        }
        if (menuSeite % 2 == 0 and cursorPosition % 2 == 0) {
          menuSeite = menuSeite - 1;
          menuSeite = constrain(menuSeite, 0, maxMenuSeiten);
        }

        if (menuSeite % 2 != 0 and cursorPosition % 2 != 0) {
          menuSeite = menuSeite - 1;
          menuSeite = constrain(menuSeite, 0, maxMenuSeiten);
        }

        cursorPosition = cursorPosition - 1;
        cursorPosition = constrain(cursorPosition, 0, ((sizeof(menuEintrag) / sizeof(String)) - 1));

        erstelleHauptmenu();
        zeigeCursor();
        aktiveTaste = 1;
        break;
      case 3:           //Wenn die Taste 'unten' gedrückt wird werden Menü-Seite und Cursor-Position aktualisiert
        button = 0;
        if (menuSeite % 2 == 0 and cursorPosition % 2 != 0) {
          menuSeite = menuSeite + 1;
          menuSeite = constrain(menuSeite, 0, maxMenuSeiten);
        }

        if (menuSeite % 2 != 0 and cursorPosition % 2 == 0) {
          menuSeite = menuSeite + 1;
          menuSeite = constrain(menuSeite, 0, maxMenuSeiten);
        }

        cursorPosition = cursorPosition + 1;
        cursorPosition = constrain(cursorPosition, 0, ((sizeof(menuEintrag) / sizeof(String)) - 1));
        erstelleHauptmenu();
        zeigeCursor();
        aktiveTaste = 1;
        break;
    }
  }
}

/* * \ brief Evaluiert welche Taste gedrückt wurde
* \ param Analoger Wert x
* \ return Ein Wert zwischen 1 und 5, je nachdem welche Taste gedrückt wurde
*/

int evaluiereTaste(int x) {
  int result = 0;
  if (x < 50) {
    result = 1; // Rechts
  } else if (x < 195) {
    result = 2; // Hoch
  } else if (x < 380) {
    result = 3; // Runter
  } else if (x < 555) {
    result = 4; // Links
  } else if (x < 790) {
    result = 5; // Select
  }
  return result;
}



void unterMenu1() {           //Untermenü für die Temperaturanzeige (innen und außen)
  int aktiveTaste = 0;
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Innen:");
  lcd.setCursor(0, 1);
  lcd.print("Außen:");
  
  while (aktiveTaste == 0) {
    int button;
    check();                      
    tasterWert = analogRead(0);
    if (tasterWert < 790) {
      delay(100);
      tasterWert = analogRead(0);
    }
    button = evaluiereTaste(tasterWert);
    switch (button) {
      case 4:   //Wenn die Taste 'links' gedrückt wird, wird das Untermenü beendet
        button = 0;
        aktiveTaste = 1;
        break;
      case 5:   //Wenn die Taste 'select' gedrückt wird, wird die Einheit der Temperatur verändertd
        button = 0;
        tempUnits = !tempUnits;
    }
    
    Serial.print(tempUnits);
    if(tempUnits)     //Wenn tempUnits 1 
    {
      lcd.setCursor(9,0);
      lcd.print(temperatur.gebeDurchschnitt()+273.15);        //dann ist die Einheit Kelvin
      lcd.print("K    ");
      lcd.setCursor(9,1);   
      lcd.print(tempCdraussen +273.15);
      lcd.print("K    ");
    }
    else
    {
      lcd.setCursor(9,0);
      lcd.print(temperatur.gebeDurchschnitt());
      lcd.print((char)223);
      lcd.print("C     ");            //sonst ist die Einheit Celsius 
      lcd.setCursor(9,1);
      lcd.print(tempCdraussen);
      lcd.print((char)223);
      lcd.print("C    ");
    }
  }
}

void unterMenu2() {                 //Untermenü für die Luftfeuchtigkeitanzeige
  int aktiveTaste = 0;

  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Luftfeuchtigkeit");

  while (aktiveTaste == 0) {
    int button;
    check();
    tasterWert = analogRead(0);
    if (tasterWert < 790) {
      delay(100);
      tasterWert = analogRead(0);
    }
    button = evaluiereTaste(tasterWert);
    switch (button) {
      case 4:       //Wenn die Taste 'links' gedrückt wird, wird das Untermenü beendet
        button = 0;
        aktiveTaste = 1;
        break;
    }
    lcd.setCursor(0, 1);
    lcd.print("Innen:");
    lcd.setCursor(7,1);
    lcd.print(luftfeuchtigkeit.gebeDurchschnitt());   //Gebe die Durchschnittsfeuchte aus
    lcd.print("%");
  }
}

void unterMenu3() { //Untermenü für das Auswählen der Schalttemperatur
  int aktiveTaste = 0;

  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Setze Temperatur");
  lcd.setCursor(4, 1);
  lcd.print(kritischeTemp);
  lcd.print((char)223);
  lcd.print("C    "); 

  while (aktiveTaste == 0) {
    int button;
    check();
    tasterWert = analogRead(0);
    if (tasterWert < 790) {
      delay(100);
      tasterWert = analogRead(0);
    }
    
    button = evaluiereTaste(tasterWert);
    switch (button) {
      case 2:       //Wenn die Taste 'oben' gedrückt wurde
      button = 0;
      kritischeTemp += 1;
      if(kritischeTemp > 40 ){ kritischeTemp = 40; }
      lcd.setCursor(0, 1);
      lcd.print("                ");
      lcd.setCursor(4, 1);
      lcd.print(kritischeTemp);
      lcd.print((char)223);
      lcd.print("C    "); 
      break;
      
      case 3:         //Wenn die Taste 'unten' gedrückt wurde
      button = 0;
      kritischeTemp -= 1;
      if(kritischeTemp < 0 ){ kritischeTemp = 0; }
      lcd.setCursor(0, 1);
      lcd.print("                ");
      lcd.setCursor(4, 1);
      lcd.print(kritischeTemp);
      lcd.print((char)223);
      lcd.print("C    "); 
      break;
        
      case 4:  //Wenn die Taste 'links' gedrückt wird, wird das Untermenü beendet
        button = 0;
        aktiveTaste = 1;
        break;       
    }
  }
}

void unterMenu4() { //Untermenü für das Setzen der kritischen Luftfeuchtigkeit
  int aktiveTaste = 0;

  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Setze Luftfeuchtigkeit");
  lcd.setCursor(4, 1);
  lcd.print(kritLuftfeuchte);
  lcd.print("%"); 

  while (aktiveTaste == 0) {
    int button;
    check();
    tasterWert = analogRead(0);
    if (tasterWert < 790) {
      delay(100);
      tasterWert = analogRead(0);
    }
    
    button = evaluiereTaste(tasterWert);
    switch (button) {
      case 2:       //Wenn die Taste 'oben' gedrückt wurde
      button = 0;
      kritLuftfeuchte += 1;
      if(kritLuftfeuchte > 100 ){ kritLuftfeuchte = 100; }
      lcd.setCursor(0, 1);
      lcd.print("                ");
      lcd.setCursor(4, 1);
      lcd.print(kritLuftfeuchte);
      lcd.print("%"); 
      break;
      
      case 3:         //Wenn die Taste 'unten' gedrückt wurde
      button = 0;
      kritLuftfeuchte -= 1;
      if(kritLuftfeuchte < 0 ){ kritLuftfeuchte = 0; }
      lcd.setCursor(0, 1);
      lcd.print("                ");
      lcd.setCursor(4, 1);
      lcd.print(kritLuftfeuchte);
      lcd.print("%"); 
      break;
        
      case 4:   //Wenn die Taste 'links' gedrückt wird, wird das Untermenü beendet
        button = 0;
        aktiveTaste = 1;
        break;    
    }   
  }
}

void unterMenu5() { //Untermenü für das Einstellen der Jalousie
  int aktiveTaste = 0;
  int dimStep = 10;

  lcd.clear();
  lcd.setCursor(0, 0);;
  lcd.print("Jalousie: ");
  switch (reglerJal)
  {
  case true:
    lcd.print("An");
    break;
  
  case false:
    lcd.print("Aus");
    break;
  }

  while (aktiveTaste == 0) {
    int button;
    check();
    tasterWert = analogRead(0);
    if (tasterWert < 790) {
      delay(100);
      tasterWert = analogRead(0);
    }
    button = evaluiereTaste(tasterWert);
    lcd.setCursor(3, 1);
    lcd.print(Winkel);
    lcd.print((char)223);

    switch (button) {
      case 2:       // passiert beim Drücken der Taste 'oben'
      button = 0;
      if(!reglerJal){
      Winkel += 10;
      if(Winkel>= 180) {
        Winkel = 180;
      }
      servoJal.write(Winkel);
      lcd.setCursor(0, 1);
      lcd.print("                ");
      lcd.setCursor(3, 1);
      lcd.print(Winkel);
      lcd.print((char)223);
      }
      break;
      
      case 3:         //passiert beim Drücken der Taste 'unten'
      button = 0;
      if(!reglerJal){
      Winkel -= 10;
      if(Winkel <= 0){
        Winkel = 0;
      }
      servoJal.write(Winkel);
      lcd.setCursor(0, 1);
      lcd.print("                ");
      lcd.setCursor(3, 1);
      lcd.print(Winkel);
      lcd.print((char)223);; 
      }
      break;
      
      case 4:  //Wenn die Taste 'links' gedrückt wird, wird das Untermenü beendet
        button = 0;
        aktiveTaste = 1;
        break;

      case 5: //Wenn 'Select' gedrückt wird, wird die automatische Jalousie an/-bzw ausgeschaltet  
        button = 0;
        reglerJal = !reglerJal;
        lcd.setCursor(10, 0);
        lcd.print("       ");
        lcd.setCursor(10, 0);
        switch (reglerJal) {
        
          case true:
          lcd.print("An");
          break;
  
          case false:
          lcd.print("Aus");
          break;
        }
        break;
    }
  }
}

float getTemperature(int tempPin_draussen,int Treading, double tempK ){
  Treading = analogRead(tempPin_draussen);
 tempK = log(10000.0 *((1024.0/Treading - 1)));
  tempK = 1 / (0.001129148 + (0.000234125 + (0.0000000876741*tempK*tempK))*tempK );
  return (tempK - 273.15);
}             // Funktion für das Auslesen der Temperatur mittels Thermistor


void window_control(bool humidityStatus, int CO2Status, float tempCdrinnen, float tempCdraussen){
      
  if( (CO2Status == 2 || (humidityStatus && CO2Status == 1)) && !fensterStatus)
  {
    motor.step(1000);                             // öffnen des Fensters
    fensterStatus = true;                         // Fensterstatus auf TRUE = OFFEN gesetzt
    rolltime_fenster = millis() + intervall_fenster;
  }
  else if( (tempCdrinnen > tempCdraussen && tempCdrinnen > kritischeTemp) && !fensterStatus)      // Innentemperatur muss höher als definierte Temperatur sein, damit geöffnet wird
  {                                                                   // kritischeTemp = gewünschte Raumtemperatur
    motor.step(1000); fensterStatus = true;
    rolltime_fenster = millis() + intervall_fenster;
  }
  else if( (tempCdrinnen <= kritischeTemp || tempCdrinnen <= tempCdraussen) && fensterStatus )  // Schließen des Fensters
  {                                         
    if(CO2Status == 2 || (CO2Status == 1 && humidityStatus))
    {  
      return; 
    }
    else                                        
    {  
      motor.step(-1000);     
      fensterStatus = false;                    
    }                                 
  }                                                                        
}                                          
 

int CO2ampel(float averageCO2)
{   
  if (averageCO2 >= 250)
  {
    digitalWrite(LEDrotPIN, HIGH);
    digitalWrite(LEDgelbPIN,LOW);
    digitalWrite(LEDgruenPIN, LOW);
    return 2;
  }
  if (averageCO2 > 150 && averageCO2 < 250)
  {
    digitalWrite(LEDrotPIN, LOW);
    digitalWrite(LEDgelbPIN,HIGH);
    digitalWrite(LEDgruenPIN, LOW);
    return 1;
  }
  if (averageCO2 <= 150)
  {
    digitalWrite(LEDrotPIN, LOW);
    digitalWrite(LEDgelbPIN, LOW);
    digitalWrite(LEDgruenPIN, HIGH);
    return 0;
  }
}


bool humidityWarn(float durchschnittsFeuchte)
{   
    if (durchschnittsFeuchte >= kritLuftfeuchte)
    {
        digitalWrite(LEDwarnPIN, HIGH);
        return true;
    }
    else
    {
        digitalWrite(LEDwarnPIN, LOW);
        return false;
    }
}

void check()
{
  Serial.println(co2.gebeDurchschnitt());
  Serial.println(luftfeuchtigkeit.gebeDurchschnitt());
  temperatur.setzeWert(dht.readTemperature());
  luftfeuchtigkeit.setzeWert(dht.readHumidity());
  co2.setzeWert(analogRead(MQANALOG));
  CO2ampel(co2.gebeDurchschnitt());
  humidityWarn(luftfeuchtigkeit.gebeDurchschnitt());

  if((long)(millis() - rolltime_fenster) >= 0)
  {
    window_control(luftfeuchtigkeit.gebeDurchschnitt(), CO2ampel(co2.gebeDurchschnitt()), temperatur.gebeDurchschnitt(), tempCdraussen);
    rolltime_fenster += millis();
  }
  if(reglerJal) 
  {
    lichtIntensitaet();
  }  
}

void lichtIntensitaet()
{
  helligkeit.setzeWert(analogRead(FOTOPIN));
  float sensorWert = helligkeit.gebeDurchschnitt();
  if(sensorWert<200) {
    Winkel = 0;
    servoJal.write(Winkel);
  }
  else if(sensorWert>=200 && sensorWert<400) {
    Winkel = 45;
    servoJal.write(Winkel);
  }
  else if(sensorWert>=400 && sensorWert<600) {
    Winkel = 90;
    servoJal.write(Winkel);
  }
  else if(sensorWert>=600 && sensorWert<900) {
    Winkel = 135;
    servoJal.write(Winkel);
  }
  else if(sensorWert>=900) {
    Winkel = 180;
    servoJal.write(Winkel);
  }
}

