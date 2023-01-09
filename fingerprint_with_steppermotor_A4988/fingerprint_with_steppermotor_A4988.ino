#include <Adafruit_Fingerprint.h>
#include <AccelStepper.h>

//#if (defined(__AVR__) || defined(ESP8266)) && !defined(__AVR_ATmega2560__)
// For UNO and others without hardware serial, we must use software serial...
// pin #2 is IN from sensor (GREEN wire)
// pin #3 is OUT from arduino  (WHITE wire)
// Set up the serial port to use softwareserial..
SoftwareSerial mySerial(2, 3);

//#else
// On Leonardo/M0/etc, others with hardware serial, use hardware serial!
// #0 is green wire, #1 is white
//#define mySerial Serial1

//#endif

#define pinEnable 13
#define dirPin 4
#define stepPin 5
#define motorInterfaceType 1
AccelStepper myStepper = AccelStepper(motorInterfaceType, stepPin, dirPin);

const int stepsPerRevolution = 250;
unsigned long currentMillis = 0;
int readInterval = 100;
unsigned long previousReadMillis = 0;
bool defaultColor = false;

Adafruit_Fingerprint finger = Adafruit_Fingerprint(&mySerial);

void setup()
{
  Serial.begin(9600);
  while (!Serial);  // For Yun/Leo/Micro/Zero/...
  delay(100);
  Serial.println("\n\nAdafruit finger detect test");

  pinMode( pinEnable, OUTPUT );

  // set the data rate for the sensor serial port
  finger.begin(57600);
  delay(5);
  if (finger.verifyPassword()) {
    Serial.println("Found fingerprint sensor!");
  } else {
    Serial.println("Did not find fingerprint sensor :(");
    while (1) {
      delay(1);
    }
  }

  Serial.println(F("Reading sensor parameters"));
  finger.getParameters();
  Serial.print(F("Status: 0x")); Serial.println(finger.status_reg, HEX);
  Serial.print(F("Sys ID: 0x")); Serial.println(finger.system_id, HEX);
  Serial.print(F("Capacity: ")); Serial.println(finger.capacity);
  Serial.print(F("Security level: ")); Serial.println(finger.security_level);
  Serial.print(F("Device address: ")); Serial.println(finger.device_addr, HEX);
  Serial.print(F("Packet len: ")); Serial.println(finger.packet_len);
  Serial.print(F("Baud rate: ")); Serial.println(finger.baud_rate);
  finger.getTemplateCount();

  if (finger.templateCount == 0) {
    Serial.print("Sensor doesn't contain any fingerprint data. Please run the 'enroll' example.");
  }
  else {
    Serial.println("Waiting for valid finger...");
    Serial.print("Sensor contains "); Serial.print(finger.templateCount); Serial.println(" templates");
  }
  finger.LEDcontrol(FINGERPRINT_LED_BREATHING, 500, FINGERPRINT_LED_BLUE);

  myStepper.setMaxSpeed(1000.0);
  //myStepper.setMaxSpeed(800.0);
  myStepper.setAcceleration(1000.0);
}

void loop()                     // run over and over again
{
  currentMillis = millis();
  getFingerprintID();
  myStepper.run();
}

uint8_t getFingerprintID() {
  if (millis() - previousReadMillis >= readInterval) {
    previousReadMillis += readInterval;
    readInterval = 100;
    //myStepper.disableOutputs();
    digitalWrite( pinEnable, HIGH );
    myStepper.setCurrentPosition(0);
    if (!defaultColor) {
      finger.LEDcontrol(FINGERPRINT_LED_BREATHING, 500, FINGERPRINT_LED_BLUE);
      defaultColor = true;
    }
    uint8_t p = finger.getImage();
    switch (p) {
      case FINGERPRINT_OK:
        Serial.println("Image taken");
        break;
      case FINGERPRINT_NOFINGER:
        Serial.println("No finger detected");
        return p;
      case FINGERPRINT_PACKETRECIEVEERR:
        Serial.println("Communication error");
        return p;
      case FINGERPRINT_IMAGEFAIL:
        Serial.println("Imaging error");
        return p;
      default:
        Serial.println("Unknown error");
        return p;
    }

    // OK success!

    p = finger.image2Tz();
    switch (p) {
      case FINGERPRINT_OK:
        Serial.println("Image converted");
        break;
      case FINGERPRINT_IMAGEMESS:
        Serial.println("Image too messy");
        return p;
      case FINGERPRINT_PACKETRECIEVEERR:
        Serial.println("Communication error");
        return p;
      case FINGERPRINT_FEATUREFAIL:
        Serial.println("Could not find fingerprint features");
        return p;
      case FINGERPRINT_INVALIDIMAGE:
        Serial.println("Could not find fingerprint features");
        return p;
      default:
        Serial.println("Unknown error");
        return p;
    }

    // OK converted!
    p = finger.fingerSearch();
    if (p == FINGERPRINT_OK) {
      Serial.println("Found a print match!");
    } else if (p == FINGERPRINT_PACKETRECIEVEERR) {
      Serial.println("Communication error");
      readInterval = 10000;
      finger.LEDcontrol(FINGERPRINT_LED_FLASHING, 25, FINGERPRINT_LED_RED, 20);
      defaultColor = false;
      return p;
    } else if (p == FINGERPRINT_NOTFOUND) {
      Serial.println("Did not find a match");
      readInterval = 10000;
      finger.LEDcontrol(FINGERPRINT_LED_FLASHING, 25, FINGERPRINT_LED_RED, 10);
      defaultColor = false;
      return p;
    } else {
      Serial.println("Unknown error");
      return p;
    }

    // found a match!
    Serial.print("Found ID #"); Serial.print(finger.fingerID);
    Serial.print(" with confidence of "); Serial.println(finger.confidence);
    finger.LEDcontrol(FINGERPRINT_LED_ON, 0, FINGERPRINT_LED_PURPLE);
    defaultColor = false;

    if (finger.fingerID > 63) {
      digitalWrite( pinEnable, LOW );
      //move to left
      myStepper.moveTo(stepsPerRevolution);
      readInterval = 4000;
    } else {
      digitalWrite( pinEnable, LOW );
      //move to right
      myStepper.moveTo(-stepsPerRevolution);
      readInterval = 4000;
    }

    return finger.fingerID;

  }
}

// returns -1 if failed, otherwise returns ID #
int getFingerprintIDez() {
  uint8_t p = finger.getImage();
  if (p != FINGERPRINT_OK)  return -1;

  p = finger.image2Tz();
  if (p != FINGERPRINT_OK)  return -1;

  p = finger.fingerFastSearch();
  if (p != FINGERPRINT_OK)  return -1;

  // found a match!
  Serial.print("Found ID #"); Serial.print(finger.fingerID);
  Serial.print(" with confidence of "); Serial.println(finger.confidence);
  return finger.fingerID;
}
