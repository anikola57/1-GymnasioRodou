/*
     Smart ecoPOT

   1o Γυμνάσιο Ρόδου
     2ος Πανελλήνιος Διαγωνισμός Τεχνολογιών στην Εκπαίδευση https://robotics.ellak.gr/
    GitHub: https://github.com/anikola57/1-GymnasioRodou

  
  Λήψη δεδομένων από Arduino και ενημέρωση ThingSpeak
  https://thingspeak.com/channels/1026147
*/

#include "ThingSpeak.h"
#include "secrets.h"
#include <ESP8266WiFi.h>
//#include <Wire.h>


char ssid[] = SECRET_SSID;   // your network SSID (name) 
char pass[] = SECRET_PASS;   // your network password
int keyIndex = 0;            // your network key Index number (needed only for WEP)
WiFiClient  client;

unsigned long myChannelNumber = SECRET_CH_ID;
const char * myWriteAPIKey = SECRET_WRITE_APIKEY;

// Μεταβλητές δεδομένων
String receivedData = "";
String pumpStatus = "";
String battery = "";
String soilHumidity = "";
String waterLevel = "";
String data1 = "";
String data2 = "";
boolean dataComplete;


//================================================
//            Setup
//================================================
void setup() {
  dataComplete = false;
  Serial.begin(38400);  // Initialize serial
  // Wait for serial to initialize.
  while(!Serial) { }
  Serial.println("\nStarting...");
  
  WiFi.mode(WIFI_STA); 
  ThingSpeak.begin(client);  // Initialize ThingSpeak

  // Αναμονή για λήψη δεδομένων στη σειριακή
  while (!dataComplete) {
    if (Serial.available()) {
       readData();
    }
  }

  // Όταν ληφθούν νέα δεδομένα, ενημέρωσε τις μεταβλητές
  updateData();
  //Σύνδεση στο wifi
  connectWifi();
  //Αποστολή δεδομένων στο Thing Speak
  updateThingSpeak();
  Serial.println("Going to sleep!");
  ESP.deepSleep(0); 
}

void loop() {
}


//===============================================================
// Ρουτίνες επικοινωνίας
//===============================================================

void connectWifi() {
  // Connect or reconnect to WiFi
  if(WiFi.status() != WL_CONNECTED){
    Serial.print("Attempting to connect to SSID: ");
    Serial.println(SECRET_SSID);
    while(WiFi.status() != WL_CONNECTED){
      WiFi.begin(ssid, pass);  // Connect to WPA/WPA2 network. Change this line if using open or WEP network
      Serial.print(".");
      delay(5000);     
    } 
    Serial.println("\nConnected.");
  }
}

// Ενημερώνει τα δεδομένα στο κανάλι του Thing speak
void updateThingSpeak() {
  // set the fields with the values
  ThingSpeak.setField(1, soilHumidity.toInt());
  ThingSpeak.setField(2, waterLevel.toInt());
  ThingSpeak.setField(3, pumpStatus.toInt());
  ThingSpeak.setField(4, battery.toFloat());

  ThingSpeak.setField(5, data1.toFloat());
  ThingSpeak.setField(6, data2.toFloat());
    
  // set the status
//  ThingSpeak.setStatus(pumpStatus);
  
  // write to the ThingSpeak channel
  int x = ThingSpeak.writeFields(SECRET_CH_ID, SECRET_WRITE_APIKEY);    //changes only here***
  if(x == 200){
    Serial.println("Channel update successful.");
  }
  else{
    Serial.println("Problem updating channel. HTTP error code " + String(x));
  }
}

//===============================================================
// Ρουτίνες ανάγνωσης και ενημέρωσης δεδομένων
//===============================================================

// Ενημέρωση μεταβλητών με τα νέα δεδομένα από τη σειριακή επικοινωνία
void updateData() {
  int firstComma = receivedData.indexOf(',');
  waterLevel = receivedData.substring(0, firstComma); // Στάθμη νερού
  int secondComma = receivedData.indexOf(',', firstComma + 1);
  soilHumidity = receivedData.substring(firstComma + 1, secondComma); // 
  int thirdComma = receivedData.indexOf(',', secondComma + 1);
  pumpStatus = receivedData.substring(secondComma + 1, thirdComma); // 

  int nextComma = receivedData.indexOf(',', thirdComma + 1);
  battery = receivedData.substring(thirdComma + 1, nextComma); // 
  secondComma = receivedData.indexOf(',', nextComma + 1);
  data1 = receivedData.substring(nextComma + 1, secondComma); // 
  thirdComma = receivedData.lastIndexOf(',');
  data2 = receivedData.substring(thirdComma + 1, receivedData.length());

  // Εκκαθάριση μεταβλητής για την επόμενη λήψη δεδομένων
  receivedData = "";
}

// Ανάγνωση δεδομένων από τη σειριακή θύρα επικοινωνίας
void readData() {
  while (Serial.available()) {
    // get the new byte:
    char inChar = (char)Serial.read();    
    if (!dataComplete) {
      // add it to the inputString:
      if (inChar != '\n') {
        receivedData += inChar;
      }
      // if the incoming character is a newline, set a flag
      // so the main loop can do something about it:
      if (inChar == '\n') {
        dataComplete = true;
      }
    }
    // Έλεγχος για τυχόν λειτουργίες WiFi
    yield();
  }
}
