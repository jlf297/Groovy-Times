#include <SPI.h>
#include <RH_RF69.h>

/************ Radio Setup ***************/

// Change to 434.0 or other frequency, must match RX's freq!
#define RF69_FREQ 915.0

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

/* Teensy 3.x w/wing
#define RFM69_RST     9   // "A"
#define RFM69_CS      10   // "B"
#define RFM69_IRQ     4    // "C"
#define RFM69_IRQN    digitalPinToInterrupt(RFM69_IRQ )
*/
 
/* WICED Feather w/wing 
#define RFM69_RST     PA4     // "A"
#define RFM69_CS      PB4     // "B"
#define RFM69_IRQ     PA15    // "C"
#define RFM69_IRQN    RFM69_IRQ
*/

// Singleton instance of the radio driver
RH_RF69 rf69(RFM69_CS, RFM69_INT);

int16_t packetnum = 0;  // packet counter, we increment per xmission
int received = 0;
int bit4 = 0;
int bit3 = 0;
int bit2 = 0;
int bit1 = 0;
int bit0 = 0;
uint8_t len_last;

void setup() 
{
  Serial.begin(115200);
  //while (!Serial) { delay(1); } // wait until serial console is open, remove if not tethered to computer

  pinMode(LED, OUTPUT);     
  pinMode(RFM69_RST, OUTPUT);
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
    uint8_t buf[6];
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
    #define BIT_4_PIN 10
    #define BIT_3_PIN 9
    #define BIT_2_PIN 8
    #define BIT_1_PIN 7
    #define BIT_0_PIN 6
    if(len != len_last){
      switch(len){
        case(0):
          digitalWrite(BIT_4_PIN, LOW);
          digitalWrite(BIT_3_PIN, LOW);
          digitalWrite(BIT_2_PIN, LOW);
          digitalWrite(BIT_1_PIN, LOW);
          digitalWrite(BIT_0_PIN, LOW);
          break;
        case(1):
          digitalWrite(BIT_4_PIN, LOW);
          digitalWrite(BIT_3_PIN, LOW);
          digitalWrite(BIT_2_PIN, LOW);
          digitalWrite(BIT_1_PIN, LOW);
          digitalWrite(BIT_0_PIN, HIGH);
          break;
        case(2):
          digitalWrite(BIT_4_PIN, LOW);
          digitalWrite(BIT_3_PIN, LOW);
          digitalWrite(BIT_2_PIN, LOW);
          digitalWrite(BIT_1_PIN, HIGH);
          digitalWrite(BIT_0_PIN, LOW);
          break;
        case(3):
          digitalWrite(BIT_4_PIN, LOW);
          digitalWrite(BIT_3_PIN, LOW);
          digitalWrite(BIT_2_PIN, LOW);
          digitalWrite(BIT_1_PIN, HIGH);
          digitalWrite(BIT_0_PIN, HIGH);
          break;
        case(4):
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
        case(6):
          digitalWrite(BIT_4_PIN, LOW);
          digitalWrite(BIT_3_PIN, LOW);
          digitalWrite(BIT_2_PIN, HIGH);
          digitalWrite(BIT_1_PIN, HIGH);
          digitalWrite(BIT_0_PIN, LOW);
          break;
        case(7):
          digitalWrite(BIT_4_PIN, LOW);
          digitalWrite(BIT_3_PIN, LOW);
          digitalWrite(BIT_2_PIN, HIGH);
          digitalWrite(BIT_1_PIN, HIGH);
          digitalWrite(BIT_0_PIN, HIGH);
          break;
        case(8):
          digitalWrite(BIT_4_PIN, LOW);
          digitalWrite(BIT_3_PIN, HIGH);
          digitalWrite(BIT_2_PIN, LOW);
          digitalWrite(BIT_1_PIN, LOW);
          digitalWrite(BIT_0_PIN, LOW);
          break;
        case(9):
          digitalWrite(BIT_4_PIN, LOW);
          digitalWrite(BIT_3_PIN, HIGH);
          digitalWrite(BIT_2_PIN, LOW);
          digitalWrite(BIT_1_PIN, LOW);
          digitalWrite(BIT_0_PIN, HIGH);
          break;
        case(10):
          digitalWrite(BIT_4_PIN, LOW);
          digitalWrite(BIT_3_PIN, HIGH);
          digitalWrite(BIT_2_PIN, LOW);
          digitalWrite(BIT_1_PIN, HIGH);
          digitalWrite(BIT_0_PIN, LOW);
          break;
        case(11):
          digitalWrite(BIT_4_PIN, LOW);
          digitalWrite(BIT_3_PIN, HIGH);
          digitalWrite(BIT_2_PIN, LOW);
          digitalWrite(BIT_1_PIN, HIGH);
          digitalWrite(BIT_0_PIN, HIGH);
          break;
        case(12):
          digitalWrite(BIT_4_PIN, LOW);
          digitalWrite(BIT_3_PIN, HIGH);
          digitalWrite(BIT_2_PIN, HIGH);
          digitalWrite(BIT_1_PIN, LOW);
          digitalWrite(BIT_0_PIN, LOW);
          break;
        case(13):
          digitalWrite(BIT_4_PIN, LOW);
          digitalWrite(BIT_3_PIN, HIGH);
          digitalWrite(BIT_2_PIN, HIGH);
          digitalWrite(BIT_1_PIN, LOW);
          digitalWrite(BIT_0_PIN, HIGH);
          break;
        case(14):
          digitalWrite(BIT_4_PIN, LOW);
          digitalWrite(BIT_3_PIN, HIGH);
          digitalWrite(BIT_2_PIN, HIGH);
          digitalWrite(BIT_1_PIN, HIGH);
          digitalWrite(BIT_0_PIN, LOW);
          break;
        case(15):
          digitalWrite(BIT_4_PIN, LOW);
          digitalWrite(BIT_3_PIN, HIGH);
          digitalWrite(BIT_2_PIN, HIGH);
          digitalWrite(BIT_1_PIN, HIGH);
          digitalWrite(BIT_0_PIN, HIGH);
          break;
        case(16):
          digitalWrite(BIT_4_PIN, HIGH);
          digitalWrite(BIT_3_PIN, LOW);
          digitalWrite(BIT_2_PIN, LOW);
          digitalWrite(BIT_1_PIN, LOW);
          digitalWrite(BIT_0_PIN, LOW);
          break;
        case(17):
          digitalWrite(BIT_4_PIN, HIGH);
          digitalWrite(BIT_3_PIN, LOW);
          digitalWrite(BIT_2_PIN, LOW);
          digitalWrite(BIT_1_PIN, LOW);
          digitalWrite(BIT_0_PIN, HIGH);
          break;
        case(18):
          digitalWrite(BIT_4_PIN, HIGH);
          digitalWrite(BIT_3_PIN, LOW);
          digitalWrite(BIT_2_PIN, LOW);
          digitalWrite(BIT_1_PIN, HIGH);
          digitalWrite(BIT_0_PIN, LOW);
          break;
        case(19):
          digitalWrite(BIT_4_PIN, HIGH);
          digitalWrite(BIT_3_PIN, LOW);
          digitalWrite(BIT_2_PIN, LOW);
          digitalWrite(BIT_1_PIN, HIGH);
          digitalWrite(BIT_0_PIN, HIGH);
          break;
        case(20):
          digitalWrite(BIT_4_PIN, HIGH);
          digitalWrite(BIT_3_PIN, LOW);
          digitalWrite(BIT_2_PIN, HIGH);
          digitalWrite(BIT_1_PIN, LOW);
          digitalWrite(BIT_0_PIN, LOW);
          break;
        case(21):
          digitalWrite(BIT_4_PIN, HIGH);
          digitalWrite(BIT_3_PIN, LOW);
          digitalWrite(BIT_2_PIN, HIGH);
          digitalWrite(BIT_1_PIN, LOW);
          digitalWrite(BIT_0_PIN, HIGH);
          break;
        case(22):
          digitalWrite(BIT_4_PIN, HIGH);
          digitalWrite(BIT_3_PIN, LOW);
          digitalWrite(BIT_2_PIN, HIGH);
          digitalWrite(BIT_1_PIN, HIGH);
          digitalWrite(BIT_0_PIN, LOW);
          break;
        case(23):
          digitalWrite(BIT_4_PIN, HIGH);
          digitalWrite(BIT_3_PIN, LOW);
          digitalWrite(BIT_2_PIN, HIGH);
          digitalWrite(BIT_1_PIN, HIGH);
          digitalWrite(BIT_0_PIN, HIGH);
          break;
        case(24):
          digitalWrite(BIT_4_PIN, HIGH);
          digitalWrite(BIT_3_PIN, HIGH);
          digitalWrite(BIT_2_PIN, LOW);
          digitalWrite(BIT_1_PIN, LOW);
          digitalWrite(BIT_0_PIN, LOW);
          break;
        default: 
          break;
      }
    }
    len_last = len;
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
