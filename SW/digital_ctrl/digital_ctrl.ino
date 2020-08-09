/*
  Power Electronics Digital Control Lab Firmware
  Omri Nissan, Initial Version
  March 2020

  This firmware is loaded into the Power Electronics Digital Control Laboratory Arduino to
  provide a high level interface to the Power Electronics Digital Control Control Lab. The firmware
  scans the serial port looking for case-insensitive commands:

  PWMA      set high-side FET on H-brdige left branch, range 0 to 100% subject to limit (0-4094 int)
  PWMB      set high-side FET on H-brdige right branch, range 0 to 100% subject to limit (0-4094 int)
  IIN       get Input Current, IIN, returns current in mA as string
  IOUT      get Output Current, IOUT, returns current in mA as string
  IFET      get source of bottom FETs Current, IFET, returns current in mA as string
  VIN       get Input Voltage, VIN, returns voltage in V as string
  VOUT      get Output Voltage, VOUT, returns voltage in V as string
  VER       get firmware version string
  X         stop, enter sleep mode

*/

#include <SPI.h>

// constants
const String vers = "1.01";    // version of this firmware
const int    baud = 9600;      // serial baud rate
const char   sp   = ' ';       // command separator
const char   nl   = '\n';      // command terminator

// pin numbers corresponding to signals on the TC Lab Shield
const int pinIIN        = 0;         // IIN   - Input Current
const int pinIOUT       = 1;         // IOUT  - Output Current
const int pinVOUT       = 2;         // VOUT  - Output Voltage
const int pinIFET       = 3;         // IFET  - Current at tge Source of the low-side FET
const int pinVIN        = 4;         // VIN   - Input Voltage
const int pinPGOOD      = 8;         // PGOOD
const int pinOVP        = 9;         // OVP
const int chipSelectPin = 10;        // SPI Chip Select

// global variables
char      Buffer[64];                         // buffer for parsing serial input
String    cmd;                                // command
float     pv;                                 // pin value
float     pwmA                     = 0;       // value written to xOA FET
float     pwmB                     = 0;       // value written to xOB FET
int       iwrite                   = 0;       // integer value for writing
int       n                        = 1;       // number of samples for each temperature measurement
const int DAC_B_BUFFER             = 0x4;     // Write data to DAC B and BUFFER
const int TO_BUFFER                = 0x5;     // Write data to BUFFER
const int DAC_A_DAC_B_FROM_BUFFER  = 0xC;     // Write data to DAC A and update DAC B with BUFFER content
const int CONTROL                  = 0xD;     // Write data to control register


//Sends a write command to TLV5638MDREP
void writeOutput(int thisOutput,int thisValue) {

  // TLV5638MDREP expects the output address in the upper 4 bits
  // of the 4 byte. So shift the bits left by twelve bits:
  thisOutput = thisOutput << 12;

  // now combine the register address and the command into one byte:
  int dataToSend = thisOutput | thisValue;
  
  // take the chip select low to select the device:
  digitalWrite(chipSelectPin, LOW);
  
//  delay(0.01);
  SPI.transfer16(dataToSend); //Send command and value
//  delay(0.01);

  // take the chip select high to de-select:
  digitalWrite(chipSelectPin, HIGH);
}

// Command left side of H-Brdige High-Side FET
void writexOA(int hexpercent) {
  writeOutput(TO_BUFFER, 0xFFF);
  writeOutput(DAC_A_DAC_B_FROM_BUFFER, hexpercent);
}

// Command right side of H-Brdige High-Side FET
void writexOB(int hexpercent) {
  writeOutput(TO_BUFFER, hexpercent);
  writeOutput(DAC_A_DAC_B_FROM_BUFFER, 0xFFF);
}

// Turn off both High-Side FETs thus discharging inductor through low side FETs
void stop_system() {
  writeOutput(TO_BUFFER, 0x0000);
  writeOutput(DAC_A_DAC_B_FROM_BUFFER, 0x0000);
}

// Parse UART console
void parseSerial(void) {
  int ByteCount = Serial.readBytesUntil(nl, Buffer, sizeof(Buffer));
  String read_ = String(Buffer);
  memset(Buffer, 0, sizeof(Buffer));

  // separate command from associated data
  int idx = read_.indexOf(sp);
  cmd = read_.substring(0, idx);
  cmd.trim();
  cmd.toUpperCase();

  // extract data. toInt() returns 0 on error
  String data = read_.substring(idx + 1);
  data.trim();
  pv = data.toFloat();
}

// Define command options
void dispatchCommand(void) {
  if (cmd == "PWMA") { //Command the FET PWM A
    pwmA = max(0.0, min(100.0, pv));
    iwrite = int(pwmA *  40.94); // 4.095 max
    iwrite = max(0, min(4094, iwrite));
    writexOA(iwrite);
    Serial.println(pwmA);
  }
  else if (cmd == "PWMB") { //Command the FET PWM B
    pwmB = max(0.0, min(100.0, pv));
    iwrite = int(pwmB *  40.94); // 4.095 max
    iwrite = max(0, min(4094, iwrite));
    writexOB(iwrite);
    Serial.println(pwmB);
  }
  else if (cmd == "IIN") { //Read Input Current 
    float mV = 0.0;
    float mA = 0.0;
    for (int i = 0; i < n; i++) {
      mV = (float) analogRead(pinIFET) / 1;
      mA = mV / 0.01;
    }
    mA = mA / float(n);
    Serial.println(mA);
  }
  else if (cmd == "IOUT") { //Read Output Current 
    float mV = 0.0;
    float mA = 0.0;
    for (int i = 0; i < n; i++) {
      mV = (float) analogRead(pinIFET) / 1;
      mA = mV / 0.01;
    }
    mA = mA / float(n);
    Serial.println(mA);
  }
  else if (cmd == "IFET") { //Read Source of bottom side FET Current 
    float mV = 0.0;
    float mA = 0.0;
    for (int i = 0; i < n; i++) {
      mV = (float) analogRead(pinIFET) / 100;
      mA = mV / 0.01;
    }
    mA = mA / float(n);
    Serial.println(mA);
  }
  else if (cmd == "VIN") { //Read Input Voltage 
    float mV = 0.0;
    float degC = 0.0;
    for (int i = 0; i < n; i++) {
      mV = (float) analogRead(pinVIN) * (100000.0 + 3000.0) / (3000.0);
    }
    mV = mV / float(n);
    Serial.println(mV);
  }
  else if (cmd == "VOUT") { //Read Input Voltage 
    float mV = 0.0;
    float degC = 0.0;
    for (int i = 0; i < n; i++) {
      mV = (float) analogRead(pinVOUT) * (100000.0 + 3000.0) / (3000.0);
    }
    mV = mV / float(n);
    Serial.println(mV);
  }
  else if ((cmd == "V") or (cmd == "VER")) { //Read version
    Serial.println("Power Electronics Control Lab Firmware Version " + vers);
  }
  else if (cmd == "X") { //Exit and shutdown system
    stop_system();
    Serial.println("Stop");
  }
}

// check Overvoltage flags and Power-Good signals for safety operation, commantented out for now
void checkFaults(void) {

//  if (digitalRead(pinPGOOD) == 0)  {
//    stop_system();
//    Serial.println("VCC is out of regulation.");
//    // ADD Stop command to stop DAC command
//  }
//
//  if (digitalRead(pinOVP) == 0)  {
//    stop_system();
//    Serial.println("Over voltage on Input!");
//    // ADD Stop command to to stop DAC command
//  }

}

// arduino startup
void setup() {
  analogReference(DEFAULT);
  Serial.begin(baud);
  while (!Serial) {
    ; // wait for serial port to connect.
  }

  // start the SPI library:
  SPI.begin();

  // initalize the  data ready and chip select pins:
  pinMode(chipSelectPin, OUTPUT);

  // Configure TLV5638MDREP for internal refrence of 1.024 V:
  writeOutput(CONTROL, 0x001);

  // give the DAC time to set up:
  delay(100);
}

// arduino main event loop
void loop() {
  parseSerial();
  dispatchCommand();
  checkFaults();
}
