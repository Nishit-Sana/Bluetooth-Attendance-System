#include <LiquidCrystal.h>
#include <SoftwareSerial.h>
#include <SD.h>
#include <SPI.h>

const int SD_CS = 4;               // SD chip select
SoftwareSerial BT(2, 3);           // Bluetooth RX, TX
LiquidCrystal lcd(5, 6, 7, 8, 9, 10); // RS, E, D4, D5, D6, D7

String message = "";
void setup() {
  lcd.begin(16, 2);
  lcd.print("Initializing...");
  delay(1000);
  lcd.clear();

  Serial.begin(115200);
  BT.begin(115200); 

  if (!SD.begin(SD_CS)) {
    lcd.print("SD init failed!");
    Serial.println("SD init failed!");
    while (1);
  }
  if (!SD.exists("text.txt")) {
    Serial.println("text.txt not found, copying from test.txt...");
    File src = SD.open("test.txt", FILE_READ);
    File dst = SD.open("text.txt", FILE_WRITE);
    while (src.available()) {
        dst.write(src.read());
      }
      src.close();
      dst.close();
      Serial.println("Database copied successfully to text.txt");
 
  }

  lcd.print("Ready for input");
  Serial.println("System ready. Send Roll No (e.g. 23EC01040;)");
}
void loop() {
  while (BT.available()) {
    char c = BT.read();

    if (c == ';' || c == '\n' || c == '\r') {
      if (message.length() > 0) {
        Serial.print("Received Roll: ");
        Serial.println(message);
        checkAndMark(message);
        message = "";
      }
    } 
    else if (isPrintable(c)) {
      message += c;
    }
  }
}
void checkAndMark(String rollRaw) {
  rollRaw.trim();
  bool found = false;

  File file = SD.open("text.txt", FILE_READ);
  if (!file) {
    lcd.clear();
    lcd.print("File open failed");
    Serial.println("Error opening test.txt");
    return;
  }

  uint32_t lineStartPos = 0;

  while (file.available()) {
    lineStartPos = file.position();
    String line = file.readStringUntil('\n');
    line.trim();
    if (line.length() == 0) continue;

    int c1 = line.indexOf(',');
    int c2 = line.indexOf(',', c1 + 1);
    if (c1 < 0 || c2 < 0) continue;

    String roll = line.substring(0, c1);
    String name = line.substring(c1 + 1, c2);
    String marked = line.substring(c2 + 1);

    if (roll.equalsIgnoreCase(rollRaw)) {
      found = true;
      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.print(name.substring(0, 16));

      if (marked == "0") {
        lcd.setCursor(0, 1);
        lcd.print("Marked Present");
        Serial.println(name + " marked present");

        file.close();
        markAttendance(lineStartPos, line, roll, name);
      } else {
        lcd.setCursor(0, 1);
        lcd.print("Already Marked");
        Serial.println(name + " already marked");
        file.close();
      }
      return;
    }
  }

  file.close();


  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("An outsider came");
  lcd.setCursor(0, 1);
  lcd.print("to class");
  Serial.println("Unknown roll: " + rollRaw);
}
void markAttendance(uint32_t pos, String line, String roll, String name) {
  File file = SD.open("text.txt", O_RDWR);
  if (!file) {
    lcd.clear();
    lcd.print("Write failed");
    Serial.println("Failed to reopen file for edit");
    return;
  }

  file.seek(pos);

  String updatedLine = roll + "," + name + ",1";
  while (updatedLine.length() < line.length()) updatedLine += ' ';
  updatedLine += "\n";

  file.print(updatedLine);
  file.close();

  Serial.println("Attendance updated successfully");
}
