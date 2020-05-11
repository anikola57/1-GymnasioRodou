/**************************************************************
   Smart ecoPOT

   1o Γυμνάσιο Ρόδου
   	2ος Πανελλήνιος Διαγωνισμός Τεχνολογιών στην Εκπαίδευση https://robotics.ellak.gr/
   	GitHub: https://github.com/anikola57/1-GymnasioRodou

   v1, 03/12/2019:
      Αναλογικοί αισθητήρες υργασίας - Υλοποίηση βασικής λειτουργίας ποτίσματος
   v2, 09/02/2020:
      Ψηφιακοί αισθητήρες υργασίας - νερού
      Υλοποίηση με functions
   v3, 07/03/2020:
      Επικοινωνία με wifi
   v4, 21/03/2020:
      Αναλογικοί αισθητήρες υγρασίας - στάθμης νερού
      Χρόνος λειτουργίας αντλίας με χρήση της millis()
      Ρύθμιση χρόνου αναμονής
      υπολογισμός μέσης τιμής μετρήσεων
      μέτρηση τάσης μπαταρίας


  (... Στοιχεία της ομάδας ανάπτυξης....)
 **************************************************************/

#include <SoftwareSerial.h>
#include "sleep.h"

// Ορισμός δεύτερης σειριακής θύρας για αποστολή δεδομένων στο WiFi Module
SoftwareSerial mySerial(11, 12); // RX, TX


// Είσοδοι / Έξοδοι - Pins
const int redLedPin = 4;         // LED Έλλειψη νερού
const int greenLedPin = 5;       // LED Κανονική υγρασία για το φυτό
const int yellowLedPin = 3;      // LED Πότισμα
const int pumpPin = 7;           // Αντλία ποτίσματος
const int waterLevelPin = A3;      // Αισθητήρας στάθμης νερού
const int soilSensorPin = A1;  // Αισθητήρας υγρασίας χώματος
const int smSensorPowerPin = A0;     //
const int wlSensorPowerPin = A2;     //
const int wifiResetPin = 13;      // Reset για το wifi module
const int bateryVoltagePin = A5;  //Μπαταρία

// Ρυθμίσεις
int checkInterval = 900;          // Χρόνος αναμονής μεταξύ μετρήσεων 120 λεπτα (X 8 sec)
int minWaterLevel = 20;             // Ελάχιστη στάθμη νερού % στη δεξαμενή
int minMoistureLevel = 70;          // Ελάχιστη υγρασία % χώματος
int amountToPump = 20;               // Χρόνος λειτουργίας της αντλίας (sec)

// Βοηθητικές μεταβλητές
unsigned int waterLevelValue = 0;
unsigned int soilHumidityValue = 0;
float bateryVoltage = 0;
unsigned long startPumpTime;    // Χρόνος έναρξης λειτουργίας της αντλίας
bool pumping = false;        // Λειτουργία αντλίας
bool waterInTank;

/////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////

// Αρχικοποίηση προγράμματος - Ρυθμίσεις που θα τρέχουν μία φορά μετά από κάθε εκκίνηση ή reset
void setup() {
  Serial.begin(9600);
  delay(10);
  mySerial.begin(38400);
  configure_wdt();

  // Ορισμός εισόδων/εξόδων
  pinMode(wifiResetPin, OUTPUT);
  pinMode(redLedPin, OUTPUT);
  pinMode(greenLedPin, OUTPUT);
  pinMode(yellowLedPin, OUTPUT);
  pinMode(pumpPin, OUTPUT);
  pinMode(smSensorPowerPin, OUTPUT);
  pinMode(wlSensorPowerPin, OUTPUT);

  // Απενεργοποίηση αισθητήρων
  digitalWrite(smSensorPowerPin, LOW);
  digitalWrite(wlSensorPowerPin, LOW);


  // Ενεργοποίηση WiFi Module
  digitalWrite(wifiResetPin, HIGH);

  Serial.println("\n\nStarting...");

  // Ένδειξη λειτουργίας του κώδικα
  blinkLeds();

  // Έλεγξε αν υπάρχει νερό στη δεξαμενή
  waterInTank = checkWaterLevel();
  Serial.print("wlsensorValue: ");
  Serial.println(waterLevelValue);

  wdt_reset();
}


// Πρόγραμμα που εκτελείται συνεχώς σε επαναλήψεις
void loop() {
  wdt_reset();

  // Ανάγνωση αισθητήρα υγρασίας
  soilHumidityValue = readSoilHumidity();
  Serial.print("soilHumidityValue: ");
  Serial.println(soilHumidityValue);

  // Μέτρηση τάσης μπαταρίας
  readBatteryValue();

  // Αν υπάρχει νερό, έλεγξε το χώμα και αν χρειάζεται πότισε
    if (waterInTank) {
      pumping = checkSoilAndWater();
    }


  // Αποστολή τιμών στον Υπολογιστή μέσω σειριακής θύρας
  sendMeasurements();

  // Αποστολή τιμών στο wifi μέσω σειριακής θύρας
  sendToWiFi();
  wdt_reset();


  // Αν ποτίζει, κλείσε την αντλία μετά από χρόνο amountToPump
  while (pumping) {
    if (checkTimeToTurnOffPump()) {
      // Έλεγξε αν υπάρχει νερό στη δεξαμενή μετά το πότισμα
      waterInTank = checkWaterLevel();      
    }
    wdt_reset();
  }

  Serial.print("sleeping...");
  delay(100);
  sleep(checkInterval);   //Κοιμήσου μέχρι την επόμενη μέτρηση
  Serial.print("wake up");
}


//####################################################################################################
//####################################################################################################

// Έλεγχος για το χρόνο ποτίσματος και σβήσιμο της αντλίας
//  Επιστρέφει TRUE όταν ολοκληρωθεί ο χρόνος ποτίσματος
bool checkTimeToTurnOffPump() {
  bool retValue = false;
  unsigned long currentMillis = millis();
  if ((unsigned long)(currentMillis - startPumpTime) > amountToPump * 1000) {
    digitalWrite(pumpPin, LOW);
    digitalWrite(yellowLedPin, LOW);
    pumping = false;
    retValue = true;
  }
  return retValue;
}


// Διαβάζει τον αισθητήρα στάθμης νερού
//  Επιστρέφει την στάθμη %
unsigned int readWlsensorValue() {
  unsigned int rawValue = 0;
  
  digitalWrite(wlSensorPowerPin, HIGH);
  delay(5);
  //  Μέσος όρος 4 μετρήσεων υγρασίας
  for (int i = 0; i < 4; i++) {
    rawValue += analogRead(waterLevelPin);
    delay(1);
  }
  digitalWrite(wlSensorPowerPin, LOW);
  rawValue /= 4;

  // Διόρθωση κλίμακας μετρήσεων
  rawValue = constrain(rawValue, 560, 665);
  rawValue = map(rawValue, 560, 665, 0, 100);
  
  return rawValue;
}


// Ανάγνωση αισθητήρα υγρασίας
//  Επιστρέφει την υγρασία %
unsigned int readSoilHumidity() {
  unsigned int rawValue = 0;

  digitalWrite(smSensorPowerPin, HIGH);
  delay(5);
  //  Μέσος όρος 4 μετρήσεων υγρασίας
  for (int i = 0; i < 4; i++) {
    rawValue += analogRead(soilSensorPin);
    delay(1);
  }
  digitalWrite(smSensorPowerPin, LOW);
  rawValue /= 4;

  // Διόρθωση κλίμακας μετρήσεων
//  rawValue = constrain(rawValue, 450, 680);
  rawValue = map(rawValue, 450, 680, 0, 100);

  return rawValue;
}


// Μέτρηση τάσης μπαραρίας
void readBatteryValue(void) {
  //  Μέσος όρος 10 μετρήσεων τάσης μπαταρίας
  bateryVoltage = 0;
  for (int i = 0; i < 10; i++) {
    bateryVoltage += analogRead(bateryVoltagePin);
    delay(10);
    wdt_reset();
  }
  bateryVoltage /= 10;
 
  // Υπολογισμός τιμής Τάσης μπαταρίας
  bateryVoltage = bateryVoltage * 5 / 1024;
}



// Αποστολή τιμών στον Υπολογιστή μέσω σειριακής θύρας
void sendMeasurements() {
  Serial.print("WaterLevel: ");
  Serial.println(waterLevelValue);
  Serial.print("Moisture: ");
  Serial.println(soilHumidityValue);
  Serial.print("Voltage: ");
  Serial.println(bateryVoltage);
}

// Αποστολή μετρήσεων στο WiFi module
void sendToWiFi() {
  // Wake up wifi module
  digitalWrite(wifiResetPin, LOW);
  delay(10);
  digitalWrite(wifiResetPin, HIGH);
  delay(2000);
  mySerial.print(waterLevelValue);
  mySerial.print(",");
  mySerial.print(soilHumidityValue);
  mySerial.print(",");
  if (digitalRead(pumpPin)) {
    mySerial.print("1");
  } else {
    mySerial.print("0");
  }
  mySerial.print(",");
  mySerial.print(bateryVoltage);

  mySerial.print(",");
  mySerial.print("0");
  mySerial.print(",");
  mySerial.print("0");

  mySerial.print("\n");

}


// Έλεγξε αν υπάρχει νερό στη δεξαμενή
bool checkWaterLevel() {
  waterLevelValue = readWlsensorValue();
  if (waterLevelValue > minWaterLevel) {
    digitalWrite(redLedPin, LOW);
    return true;
  }
  digitalWrite(redLedPin, HIGH);
  return false;
}


// Έλεγχος υγρασίας και ποτίσματος
// Επιστρέφει true ή false ανάλογα αν λειτουργει η αντλια
bool checkSoilAndWater() {
  // Αν υπάρχει ξηρασία στο χώμα, πότισε και τροποποίησε τις ενδείξεις
  if (soilHumidityValue < minMoistureLevel) {
    digitalWrite(greenLedPin, LOW);
    digitalWrite(yellowLedPin, HIGH);   // Ένδειξη ποτίσματος
    digitalWrite(pumpPin, HIGH);      // Λειτουργία αντλίας
    startPumpTime = millis();       //Χρόνος έναρξης αντλίας
    return true;
  }
  // Αλλιώς τροποποίησε τις ενδείξεις
  else {
    digitalWrite(greenLedPin, HIGH);  // Ικανοποιητική υγρασία
    digitalWrite(yellowLedPin, LOW);
  }
  return false;
}


void blinkLeds() {
  for (int i = 0; i < 2; i++) {
    digitalWrite(redLedPin, HIGH);
    delay(100);
    digitalWrite(redLedPin, LOW);
    digitalWrite(yellowLedPin, HIGH);
    delay(100);
    digitalWrite(yellowLedPin, LOW);
    digitalWrite(greenLedPin, HIGH);
    delay(100);
    digitalWrite(greenLedPin, LOW);
  }
}
//--------------------------------------------------------------------------------------
