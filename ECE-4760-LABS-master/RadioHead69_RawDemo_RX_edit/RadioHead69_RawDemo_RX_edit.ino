#include <SPI.h>
#include <RH_RF69.h>

#if defined (__AVR_ATmega32U4__) // Feather 32u4 w/Radio
  #define RFM69_CS      8
  #define RFM69_INT     7
  #define RFM69_RST     4
  #define LED           13
#endif

#if defined(ADAFRUIT_FEATHER_M0) // Feather M0 w/Radio
  #define RFM69_CS      8
  #define RFM69_INT     3
  #define RFM69_RST     4
  #define LED           13
#endif

#if defined (__AVR_ATmega328P__)  // Feather 328P w/wing //UNO
  #define RFM69_INT     3  // 
  #define RFM69_CS      4  //
  #define RFM69_RST     2  // "A"
  #define LED           13
#endif

#if defined(ESP8266)    // ESP8266 feather w/wing
  #define RFM69_CS      2    // "E"
  #define RFM69_IRQ     15   // "B"
  #define RFM69_RST     16   // "D"
  #define LED           0
#endif

#if defined(ESP32)    // ESP32 feather w/wing
  #define RFM69_RST     13   // same as LED
  #define RFM69_CS      33   // "B"
  #define RFM69_INT     27   // "A"
  #define LED           13
#endif

#define BIT_5_PIN A0
#define BIT_4_PIN 9
#define BIT_3_PIN 8
#define BIT_2_PIN 7
#define BIT_1_PIN 6
#define BIT_0_PIN 5

// Singleton instance of the radio driver
RH_RF69 rf69(RFM69_CS, RFM69_INT);

int16_t packetnum = 0;  // packet counter, we increment per xmission
int received = 0;
int bit4 = 0;
int bit3 = 0;
int bit2 = 0;
int bit1 = 0;
int bit0 = 0;
uint8_t last_buf;

void setup() 
{
  Serial.begin(115200);

  pinMode(LED, OUTPUT);     
  pinMode(RFM69_RST, OUTPUT);

  pinMode(BIT_0_PIN, OUTPUT);
  pinMode(BIT_1_PIN, OUTPUT);
  pinMode(BIT_2_PIN, OUTPUT);
  pinMode(BIT_3_PIN, OUTPUT);
  pinMode(BIT_4_PIN, OUTPUT);
  pinMode(BIT_5_PIN, OUTPUT);
  
  digitalWrite(RFM69_RST, LOW);

  Serial.println("Feather RFM69 RX Test!");
  Serial.println();

  // manual reset
  digitalWrite(RFM69_RST, HIGH);
  delay(10);
  digitalWrite(RFM69_RST, LOW);
  delay(10);
  
  if (!rf69.init()) {
    Serial.println("RFM69 radio init failed");
    while (1);
  }
  Serial.println("RFM69 radio init OK!");
  
  // Defaults after init are 434.0MHz, modulation GFSK_Rb250Fd250, +13dbM (for low power module)
  // No encryption
  if (!rf69.setFrequency(RF69_FREQ)) {
    Serial.println("setFrequency failed");
  }

  // If you are using a high power RF69 eg RFM69HW, you *must* set a Tx power with the
  // ishighpowermodule flag set like this:
  rf69.setTxPower(20, true);  // range from 14-20 for power, 2nd arg must be true for 69HCW

  // The encryption key has to be the same as the one in the server
  uint8_t key[] = { 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08,
                    0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08};
  rf69.setEncryptionKey(key);
  
  pinMode(LED, OUTPUT);

  Serial.print("RFM69 radio @");  Serial.print((int)RF69_FREQ);  Serial.println(" MHz");
}

void loop() {
 if (rf69.available()) {
    // Should be a message for us now   
    //do we want to have the max length be consistently set 
    //uint8_t buf[RH_RF69_MAX_MESSAGE_LEN];
    uint8_t buf[1];
    uint8_t len = sizeof(buf);
    if (rf69.recv(buf, &len)) {
      //if (!len) return;
      buf[len] = 0;
      Serial.print("Received [");
      Serial.print(len);
      Serial.print("]: ");
      Serial.println((char*)buf);
      Serial.print("RSSI: ");
      Serial.println(rf69.lastRssi(), DEC);

      if (1) {
        // Send a reply!
        uint8_t data[] = "Received";
        rf69.send(data, sizeof(data));
        rf69.waitPacketSent();
        received = 1;
        Serial.println("Sent a reply");        
        Blink(LED, 40, 3); //blink LED 3 times, 40ms between blinks
      }
    } else {
      Serial.println("Receive failed");
      received = 0;
    }   

    //LUT to write len to pins
    //LUT chosen over algorithmic methods for speed

    int read_data = ((int) buf[0]) - 48;
    Serial.print("buf: "); Serial.println(read_data);
    
    digitalWrite(BIT_5_PIN, HIGH);
    digitalWrite(BIT_4_PIN, HIGH);
    digitalWrite(BIT_3_PIN, HIGH);
    digitalWrite(BIT_2_PIN, HIGH);
    digitalWrite(BIT_1_PIN, HIGH);
    digitalWrite(BIT_0_PIN, HIGH);
    
    //case statements for case conversion
     switch(read_data){
        case(0):
          digitalWrite(BIT_5_PIN, LOW);
          digitalWrite(BIT_4_PIN, LOW);
          digitalWrite(BIT_3_PIN, LOW);
          digitalWrite(BIT_2_PIN, LOW);
          digitalWrite(BIT_1_PIN, LOW);
          digitalWrite(BIT_0_PIN, LOW);
          break;
        case(1):
          digitalWrite(BIT_5_PIN, LOW);
          digitalWrite(BIT_4_PIN, LOW);
          digitalWrite(BIT_3_PIN, LOW);
          digitalWrite(BIT_2_PIN, LOW);
          digitalWrite(BIT_1_PIN, LOW);
          digitalWrite(BIT_0_PIN, HIGH);
          break;
        case(2):
          digitalWrite(BIT_5_PIN, LOW);
          digitalWrite(BIT_4_PIN, LOW);
          digitalWrite(BIT_3_PIN, LOW);
          digitalWrite(BIT_2_PIN, LOW);
          digitalWrite(BIT_1_PIN, HIGH);
          digitalWrite(BIT_0_PIN, LOW);
          break;
        case(3):
          digitalWrite(BIT_5_PIN, LOW);
          digitalWrite(BIT_4_PIN, LOW);
          digitalWrite(BIT_3_PIN, LOW);
          digitalWrite(BIT_2_PIN, LOW);
          digitalWrite(BIT_1_PIN, HIGH);
          digitalWrite(BIT_0_PIN, HIGH);
          break;
        case(4):
          digitalWrite(BIT_5_PIN, LOW);
          digitalWrite(BIT_4_PIN, LOW);
          digitalWrite(BIT_3_PIN, LOW);
          digitalWrite(BIT_2_PIN, HIGH);
          digitalWrite(BIT_1_PIN, LOW);
          digitalWrite(BIT_0_PIN, LOW);
          break;
        case(5):
          digitalWrite(BIT_4_PIN, LOW);
          digitalWrite(BIT_3_PIN, LOW);
          digitalWrite(BIT_2_PIN, HIGH);
          digitalWrite(BIT_1_PIN, LOW);
          digitalWrite(BIT_0_PIN, HIGH);
          break;
     }
}


void Blink(byte PIN, byte DELAY_MS, byte loops) {
  for (byte i=0; i<loops; i++)  {
    digitalWrite(PIN,HIGH);
    delay(DELAY_MS);
    digitalWrite(PIN,LOW);
    delay(DELAY_MS);
  }
}
