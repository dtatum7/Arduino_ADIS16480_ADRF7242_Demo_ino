#include <ADF7242.h>
#include <ADIS16480.h>
#include <SPI.h>

//#define DEBUG // Comment out this line to disable DEBUG mode

// Variables
unsigned char roll = 0;
unsigned char pitch = 0;
unsigned char yaw = 0;
unsigned char serialSyncWord = 0xFF; // Used to synchronize serial data received by GUI on PC

ADF7242 Tx(10); // Instantiate ADF7242 Tx(Chip Select)
ADIS16480 IMU(9,8,7); // Instantiate ADIS16480 IMU(Chip Select, Data Ready, HW Reset)

void setup() {

  // For serial communication to the PC via USB
  Serial.begin(9600); // Baud rate was set arbitrarily

  // Reminder in terminal that you're in DEBUG mode
  #ifdef DEBUG
    Serial.println("**********DEBUG MODE**********");
  #endif

  // ADF7242 RFIC configuration
  Tx.configSPI();
  Tx.reset(); // Reset ADF7242 radio controller during cold start up
  Tx.idle(); // Idle ADF7242 radio controller after cold start up
  // Initialize settings for GFSK/FSK and set data rate
  Tx.initFSK(2); // Data rate [ 1=50kbps, 2=62.5kbps, 3=100kbps, 4=125kbps, 5=250kbps, 6=500kbps, 7=1Mbps, 8=2Mbps ]
  Tx.setMode(0x04); // Set operating mode to GFSK/FSK packet mode
  Tx.chFreq(2450); // Set operating frequency in MHz
  Tx.syncWord(0x00, 0x00);  // Set sync word // sync word currently hardcoded
  Tx.cfgPA(15, 1, 7); // Configure power amplifier (power, high power mode, ramp rate)
  Tx.cfgAFC(80); // Writes AFC configuration for GFSK / FSK
  Tx.cfgPB(0x080, 0x000); // Sets Tx/Rx packet buffer pointers
  Tx.cfgCRC(0); // CRC - Disable automatic CRC = 1, else 0
//  Tx.cfgPreamble(0, 0, 0, 1); // FSK preamble configuration
  Tx.PHY_RDY(); // System calibration
  Tx.regWrite(0x081, 0x05); // Set packet length LSB
  Tx.regWrite(0x080, 0x00); // Set packet length MSB

  // ADIS16480 IMU configuration
  IMU.configSPI();
  IMU.reset();// Reset ADIS16480 radio controller during cold start up
}

void loop() {

  // Read ADIS16480 IMU Data
  if(digitalRead(8) == LOW) { // Check data ready pin
    IMU.configSPI();
    roll = (unsigned char)(IMU.regRead(ROLL_C23_OUT) >> 8); // Read roll register and cast to char
    pitch = (unsigned char)(IMU.regRead(PITCH_C31_OUT) >> 8); // Read pitch register and cast to char
    yaw = (unsigned char)(IMU.regRead(YAW_C32_OUT) >> 8); // Read pitch register and cast to char
    // 0xFF is a reserved word used for data synchronization
    if(roll == 0xFF) { // 0xFF represents 360 degrees
      roll = 0; // This makes sense since 0 and 360 degrees are the same place
    }
    if(pitch == 0xFF) { // 0xFF represents 360 degrees
      pitch = 0; // This makes sense since 0 and 360 degrees are the same place
    }
    if(yaw == 0xFF) { // 0xFF represents 360 degrees
      yaw = 0; // This makes sense since 0 and 360 degrees are the same place
    }

    // Transmit IMU data via ADF7242
    Tx.configSPI();
    Tx.regWrite(0x082, roll); // Write roll data to ADF7242 packet buffer
    Tx.regWrite(0x083, pitch); // Write pitch data to ADF7242 packet buffer
    Tx.regWrite(0x084, yaw); // Write yaw data to ADF7242 packet buffer
    Tx.transmit();

    #ifdef DEBUG
      Tx.dumpISB();
      delay(100);
    #endif

    // Write IMU data to serial port
    #ifndef DEBUG
      Serial.write(roll); // Write roll data to serial connection
      Serial.write(pitch); // Write pitch data to serial connection
      Serial.write(yaw); // Write yaw data to serial connection
      Serial.write(serialSyncWord); // Write synchronization word to serial connection
    #endif
  }
}
