/**************************************************************
   Smart ecoPOT

   1o Γυμνάσιο Ρόδου
   	2ος Πανελλήνιος Διαγωνισμός Ανοιχτών Τεχνολογιών στην Εκπαίδευση https://robotics.ellak.gr/
   	
   v1:


 **************************************************************/



// Είσοδοι / Έξοδοι - Pins
const int redLedPin = 2;      // LED Έλλειψη νερού
const int greenLedPin = 3;    // LED Κανονική υγρασία για το φυτό
const int blueLedPin = 4;     // LED Πότισμα
const int pumpPin = 12;       // Αντλία ποτίσματος
const int waterLevelPin = A3; // Αισθητήρας στάθμης νερού
const int moistureSensorPin = 7; // Αισθητήρας υγρασίας χώματος

// Ρυθμίσεις
long checkInterval = 5000;    // Χρόνος μεταξύ μετρήσεων (ms)
int minWaterLevel = 380;      // Ελάχιστη στάθμη νερού στη δεξαμενή
int minMoistureLevel = 100;   // Ελάχιστη υγρασία χώματος
int amountToPump = 3000;      // Χρόνος λειτουργίας της αντλίας

// Βοηθητικές μεταβλητές 
int sensorWaterLevelValue = 0;
int moistureSensorValue = 0;

/////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////

void setup() {
  Serial.begin(9600);
  delay(10);

  // Ορισμός εισόδων/εξόδων
  pinMode(redLedPin, OUTPUT);
  pinMode(greenLedPin, OUTPUT);
  pinMode(blueLedPin, OUTPUT);
  pinMode(pumpPin, OUTPUT);
  pinMode(moistureSensorPin, INPUT);

  // Ένδειξη λειτουργίας του κώδικα
  for(int i = 0; i < 4; i++) {
    digitalWrite(redLedPin, HIGH);
    delay(300);
    digitalWrite(redLedPin, LOW);
    delay(300);
  }
  delay(2000);
  digitalWrite(redLedPin, HIGH);
}


void loop() {
  // Ανάγνωση αισθητήρων νερού και υγρασίας
  sensorWaterLevelValue = analogRead(waterLevelPin);
  moistureSensorValue = analogRead(moistureSensorPin);

//  // Αποστολή τιμών στον Υπολογιστή μέσω σειριακής θύρας
//  Serial.print("WaterLevel :");
//  Serial.print(sensorWaterLevelValue);
//  Serial.print("  -  moisture :");
//  Serial.println(moistureSensorValue);

  // Αν υπάρχει νερό στη δεξαμενή, έλεγξε την υγρασία του χώματος
  if(sensorWaterLevelValue > minWaterLevel) {

        // Αν υπάρχει ξηρασία στο χώμα, πότισε και τροποποίησε τις ενδείξεις
        if(moistureSensorValue > minMoistureLevel) {
          digitalWrite(redLedPin, LOW);
          digitalWrite(greenLedPin, LOW);          
          digitalWrite(blueLedPin, HIGH);   // Πότισμα
          digitalWrite(pumpPin, HIGH);          
          delay(amountToPump);
          digitalWrite(pumpPin, LOW);          
          digitalWrite(blueLedPin, LOW);          
        }
        // Αλλιώς τροποποίησε τις ενδείξεις
        else {
          digitalWrite(redLedPin, LOW);
          digitalWrite(greenLedPin, HIGH);  // Ικανοποιητική υγρασία
          digitalWrite(blueLedPin, LOW);
        }    
  }
  // Αν δεν υπάρχει νερό στην δεξαμενή, άναψε το ενδεικτικό LED
  else {
    digitalWrite(redLedPin, HIGH);
  }

  // Αναμονή για την επόμενη μέτρηση
  delay(checkInterval);
}


//--------------------------------------------------------------------------------------
