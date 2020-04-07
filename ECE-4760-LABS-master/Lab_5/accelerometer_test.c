/*
 * File:        TFT_test_BRL4.c
 * Author:      Bruce Land
 * For use with Sean Carroll's Big Board
 * Adapted from:
 *              main.c by
 * Author:      Syed Tahmid Mahbub
 * Target PIC:  PIC32MX250F128B
 */

////////////////////////////////////
// clock AND protoThreads configure!
// You MUST check this file!
#include "config.h"
// threading library
#include "pt_cornell_1_2_1.h"

////////////////////////////////////
// graphics libraries
#include "tft_master.h"
#include "tft_gfx.h"
// need for rand function
#include <stdlib.h>
////////////////////////////////////
#include "i2c_helper.h"

#define EnablePullDownA(bits) CNPUACLR=bits; CNPDASET=bits;
#define EnablePullDownB(bits) CNPUBCLR=bits; CNPDBSET=bits;


/* Demo code for interfacing TFT (ILI9340 controller) to PIC32
 * The library has been modified from a similar Adafruit library
 */
// Adafruit data:
/***************************************************
  This is an example sketch for the Adafruit 2.2" SPI display.
  This library works with the Adafruit 2.2" TFT Breakout w/SD card
  ----> http://www.adafruit.com/products/1480

  Check out the links above for our tutorials and wiring diagrams
  These displays use SPI to communicate, 4 or 5 pins are required to
  interface (RST is optional)
  Adafruit invests time and resources providing this open source code,
  please support Adafruit and open-source hardware by purchasing
  products from Adafruit!

  Written by Limor Fried/Ladyada for Adafruit Industries.
  MIT license, all text above must be included in any redistribution
 ****************************************************/

// string buffer
char buffer[100];

// DAC ISR
// A-channel, 1x, active
#define DAC_config_chan_A 0b0011000000000000
//== Timer 2 interrupt handler ===========================================
volatile unsigned int DAC_data ;// output value
volatile SpiChannel spiChn = SPI_CHANNEL2 ;	// the SPI channel to use
volatile int spiClkDiv = 2 ; // 20 MHz max speed for this DAC

void __ISR(_TIMER_2_VECTOR, ipl2) Timer2Handler(void)
{
    mT2ClearIntFlag();
    
    // generate  ramp
    // at 100 ksample/sec, 2^12 samples (4096) on one ramp up
    // yields 100000/4096 = 24.4 Hz.
     DAC_data = (DAC_data + 1) & 0xfff ; // for testing
    
    // CS low to start transaction
     mPORTBClearBits(BIT_4); // start transaction
    // test for ready
     while (TxBufFullSPI2());
     // write to spi2
     WriteSPI2(DAC_config_chan_A | DAC_data);
    // test for done
    while (SPI2STATbits.SPIBUSY); // wait for end of transaction
     // CS high
     mPORTBSetBits(BIT_4); // end transaction
}




// === thread structures ============================================
// thread control structs
// note that UART input and output are threads
static struct pt pt_timer, pt_i2c, pt_direction, pt_calibrate, pt_wait;

// system 1 second interval tick
int sys_time_seconds ;

static signed int xAccelPort, yAccelPort, zAccelPort;
static signed int xAccelStar, yAccelStar, zAccelStar;
static char xAccelMSB, yAccelMSB, zAccelMSB;
//static char xAccelLSB, yAccelLSB, zAccelLSB;

unsigned char getDataPort(unsigned char axis) {
    unsigned char ddata;
    StartI2C1(); //Start
    IdleI2C1();
    MasterWriteI2C1(0x3A); //Send Device Address (Write)
    IdleI2C1();
    while (I2C1STATbits.ACKSTAT); //wait for slave acknowledge
    MasterWriteI2C1(axis); //Send Register Address
    IdleI2C1();
    while (I2C1STATbits.ACKSTAT); //wait for slave acknowledge
    RestartI2C1(); //Restart
    IdleI2C1();
    MasterWriteI2C1(0x3B); //Send Device Address (Read)
    IdleI2C1();
    while (I2C1STATbits.ACKSTAT); //wait for slave acknowledge
    ddata = MasterReadI2C1();
    IdleI2C1();
    StopI2C1();
    IdleI2C1();
    return ddata;
}

unsigned char getDataStar(unsigned char axis) {
    unsigned char ddata;
    StartI2C1(); //Start
    IdleI2C1();
    MasterWriteI2C1(0x38); //Send Device Address (Write)
    IdleI2C1();
    while (I2C1STATbits.ACKSTAT); //wait for slave acknowledge
    MasterWriteI2C1(axis); //Send Register Address
    IdleI2C1();
    while (I2C1STATbits.ACKSTAT); //wait for slave acknowledge
    RestartI2C1(); //Restart
    IdleI2C1();
    MasterWriteI2C1(0x39); //Send Device Address (Read)
    IdleI2C1();
    while (I2C1STATbits.ACKSTAT); //wait for slave acknowledge
    ddata = MasterReadI2C1();
    IdleI2C1();
    StopI2C1();
    IdleI2C1();
    return ddata;
}

// === Timer Thread =================================================
// update a 1 second tick counter
static PT_THREAD (protothread_i2c(struct pt *pt))
{
    PT_BEGIN(pt);
      while(1) {
        //float values[3];
        //readImuValues(values);
        
        xAccelMSB = (getDataPort(0x01));
        //xAccelLSB = (getData(0x02));
        yAccelMSB = (getDataPort(0x03));
        //yAccelLSB = (getData(0x04));
        zAccelMSB = (getDataPort(0x05));
        //zAccelLSB = (getData(0x06));
        
        xAccelPort = (signed int)((xAccelMSB));
        yAccelPort = (signed int)((yAccelMSB));
        zAccelPort = (signed int)((zAccelMSB));
        
        xAccelMSB = (getDataStar(0x01));
        //xAccelLSB = (getData(0x02));
        yAccelMSB = (getDataStar(0x03));
        //yAccelLSB = (getData(0x04));
        zAccelMSB = (getDataStar(0x05));
        //zAccelLSB = (getData(0x06));
        
        xAccelStar = (signed int)((xAccelMSB));
        yAccelStar = (signed int)((yAccelMSB));
        zAccelStar = (signed int)((zAccelMSB));
        
        PT_YIELD_TIME_msec(100);
        
        // NEVER exit while
      } // END WHILE(1)
  PT_END(pt);
} // timer thread

static int x_up, x_down, x_left, x_right;
static int y_up, y_down, y_left, y_right;
static int z_up, z_down, z_left, z_right;
static int direction_port, direction_star;
static enum direction{UP,DOWN,LEFT,RIGHT,NONE};

static int flag = 0;
static int i;
static int flag2, time;

// === Timer Thread =================================================
// update a 1 second tick counter
static PT_THREAD (protothread_direction(struct pt *pt))
{
    PT_BEGIN(pt);
    x_up = 0;
    y_up = -64;
    z_up = 0;
    
    x_down = 0;
    y_down = 64;
    z_down = 0;
    
    x_left = 60;
    y_left = 0;
    z_left = 0;
    
    x_right = -65;
    y_right = 5;
    z_right = -5;
    
    int thresh = 30;
    
    int x_up_cond_port, y_up_cond_port, z_up_cond_port;
    int x_down_cond_port, y_down_cond_port, z_down_cond_port;
    int x_left_cond_port, y_left_cond_port, z_left_cond_port;
    int x_right_cond_port, y_right_cond_port, z_right_cond_port;
    
    int x_up_cond_star, y_up_cond_star, z_up_cond_star;
    int x_down_cond_star, y_down_cond_star, z_down_cond_star;
    int x_left_cond_star, y_left_cond_star, z_left_cond_star;
    int x_right_cond_star, y_right_cond_star, z_right_cond_star;
    
    mPORTASetPinsDigitalOut(BIT_3 | BIT_4); 
    mPORTASetBits(BIT_3 | BIT_4);
    
      while(1) {
        
        //UP conditionals
        x_up_cond_port = (xAccelPort > (x_up-thresh))&(xAccelPort < (x_up+thresh));
        y_up_cond_port = (yAccelPort > (y_up-thresh))&(yAccelPort < (y_up+thresh));
        z_up_cond_port = (zAccelPort > (z_up-thresh))&(zAccelPort < (z_up+thresh));
        
        //DOWN conditionals
        x_down_cond_port = (xAccelPort > (x_down-thresh))&(xAccelPort < (x_down+thresh));
        y_down_cond_port = (yAccelPort > (y_down-thresh))&(yAccelPort < (y_down+thresh));
        z_down_cond_port = (zAccelPort > (z_down-thresh))&(zAccelPort < (z_down+thresh));
        
        //LEFT conditionals
        x_left_cond_port = (xAccelPort > (x_left-thresh))&(xAccelPort < (x_left+thresh));
        y_left_cond_port = (yAccelPort > (y_left-thresh))&(yAccelPort < (y_left+thresh));
        z_left_cond_port = (zAccelPort > (z_left-thresh))&(zAccelPort < (z_left+thresh));
        
        //RIGHT conditionals
        x_right_cond_port = (xAccelPort > (x_right-thresh))&(xAccelPort < (x_right+thresh));
        y_right_cond_port = (yAccelPort > (y_right-thresh))&(yAccelPort < (y_right+thresh));
        z_right_cond_port = (zAccelPort > (z_right-thresh))&(zAccelPort < (z_right+thresh));
        
        //UP conditionals
        x_up_cond_star = (xAccelStar > (x_up-thresh))&(xAccelStar < (x_up+thresh));
        y_up_cond_star = (yAccelStar > (y_up-thresh))&(yAccelStar < (y_up+thresh));
        z_up_cond_star = (zAccelStar > (z_up-thresh))&(zAccelStar < (z_up+thresh));
        
        //DOWN conditionals
        x_down_cond_star = (xAccelStar > (x_down-thresh))&(xAccelStar < (x_down+thresh));
        y_down_cond_star = (yAccelStar > (y_down-thresh))&(yAccelStar < (y_down+thresh));
        z_down_cond_star = (zAccelStar > (z_down-thresh))&(zAccelStar < (z_down+thresh));
        
        //LEFT conditionals
        x_left_cond_star = (xAccelStar > (x_left-thresh))&(xAccelStar < (x_left+thresh));
        y_left_cond_star = (yAccelStar > (y_left-thresh))&(yAccelStar < (y_left+thresh));
        z_left_cond_star = (zAccelStar > (z_left-thresh))&(zAccelStar < (z_left+thresh));
        
        //RIGHT conditionals
        x_right_cond_star = (xAccelStar > (x_right-thresh))&(xAccelStar < (x_right+thresh));
        y_right_cond_star = (yAccelStar > (y_right-thresh))&(yAccelStar < (y_right+thresh));
        z_right_cond_star = (zAccelStar > (z_right-thresh))&(zAccelStar < (z_right+thresh));
        
        
        if(x_up_cond_port & y_up_cond_port & z_up_cond_port){
            direction_port = UP; 
            mPORTASetBits(BIT_3 | BIT_4);
        }
        else if(x_down_cond_port & y_down_cond_port & z_down_cond_port){
            direction_port = DOWN; 
            mPORTAClearBits(BIT_3);
            mPORTASetBits(BIT_4);
        }
        else if(x_left_cond_port & y_left_cond_port & z_left_cond_port){
            direction_port = LEFT; 
            mPORTAClearBits(BIT_4);
            mPORTASetBits(BIT_3);
        }
        else if(x_right_cond_port & y_right_cond_port & z_right_cond_port){
            direction_port = RIGHT;
            mPORTAClearBits(BIT_3 | BIT_4);
        }
        else {
            direction_port = NONE;
            mPORTASetBits(BIT_3 | BIT_4);
        }
        
        if(x_up_cond_star & y_up_cond_star & z_up_cond_star){
            direction_star = UP; 
            //mPORTASetBits(BIT_3 | BIT_4);
        }
        else if(x_down_cond_star & y_down_cond_star & z_down_cond_star){
            direction_star = DOWN; 
            //mPORTAClearBits(BIT_3);
            //mPORTASetBits(BIT_4);
        }
        else if(x_left_cond_star & y_left_cond_star & z_left_cond_star){
            direction_star = LEFT; 
            //mPORTAClearBits(BIT_4);
            //mPORTASetBits(BIT_3);
        }
        else if(x_right_cond_star & y_right_cond_star & z_right_cond_star){
            direction_star = RIGHT;
            //mPORTAClearBits(BIT_3 | BIT_4);
        }
        else {
            direction_star = NONE;
            //mPORTASetBits(BIT_3 | BIT_4);
        }
        flag = mPORTBReadBits(BIT_10);
        
        PT_YIELD_TIME_msec(50);
        
        // NEVER exit while
      } // END WHILE(1)
  PT_END(pt);
} // timer thread

static PT_THREAD (protothread_timer(struct pt *pt))
{
    PT_BEGIN(pt);
     tft_setCursor(0, 0);
     tft_setTextColor(ILI9340_WHITE);  tft_setTextSize(1);
     tft_writeString("Time in seconds since boot\n");
     // set up LED to blink
     mPORTASetBits(BIT_0 );	//Clear bits to ensure light is off.
     mPORTASetPinsDigitalOut(BIT_0 );    //Set port as output
     char UP_str[] = "UP";
     char DOWN_str[] = "DOWN";
     char LEFT_str[] = "LEFT";
     char RIGHT_str[] = "RIGHT";
     char NONE_str[] = "NONE";
      while(1) {
        // yield time 1 second
        PT_YIELD_TIME_msec(100) ;
        sys_time_seconds++ ;
        time = ReadTimer2();
        // toggle the LED on the big board
        mPORTAToggleBits(BIT_0);
        // draw sys_time
        tft_fillRoundRect(0,10, 100, 14, 1, ILI9340_BLACK);// x,y,w,h,radius,color
        tft_setCursor(0, 10);
        tft_setTextColor(ILI9340_YELLOW); tft_setTextSize(2);
        sprintf(buffer,"%d | %d", sys_time_seconds, i);
        tft_writeString(buffer);
        
        tft_fillRoundRect(0,30, 200, 14, 1, ILI9340_BLACK);// x,y,w,h,radius,color
        tft_setCursor(0, 30);
        tft_setTextColor(ILI9340_YELLOW); tft_setTextSize(2);
        sprintf(buffer,"X: %d", xAccelPort);
        tft_writeString(buffer);
        tft_fillRoundRect(0,50, 200, 14, 1, ILI9340_BLACK);// x,y,w,h,radius,color
        tft_setCursor(0, 50);
        sprintf(buffer,"Y: %d", yAccelPort);
        tft_writeString(buffer);
        tft_fillRoundRect(0,70, 200, 14, 1, ILI9340_BLACK);// x,y,w,h,radius,color
        tft_setCursor(0, 70);
        sprintf(buffer,"Z: %d", zAccelPort);
        tft_writeString(buffer);
        
        tft_fillRoundRect(0,90, 300, 14, 1, ILI9340_BLACK);// x,y,w,h,radius,color
        tft_setCursor(0, 90);
        if(direction_port==UP) sprintf(buffer,"Direction Port: UP");
        else if(direction_port==DOWN) sprintf(buffer,"Direction Port: DOWN");
        else if(direction_port==LEFT) sprintf(buffer,"Direction Port: LEFT");
        else if(direction_port==RIGHT) sprintf(buffer,"Direction Port: RIGHT");
        else sprintf(buffer,"Direction Port: NONE");
        tft_writeString(buffer);
        
        tft_fillRoundRect(0,110, 200, 14, 1, ILI9340_BLACK);// x,y,w,h,radius,color
        tft_setCursor(0, 110);
        tft_setTextColor(ILI9340_YELLOW); tft_setTextSize(2);
        sprintf(buffer,"X: %d", xAccelStar);
        tft_writeString(buffer);
        tft_fillRoundRect(0,130, 200, 14, 1, ILI9340_BLACK);// x,y,w,h,radius,color
        tft_setCursor(0, 130);
        sprintf(buffer,"Y: %d", yAccelStar);
        tft_writeString(buffer);
        tft_fillRoundRect(0,150, 200, 14, 1, ILI9340_BLACK);// x,y,w,h,radius,color
        tft_setCursor(0, 150);
        sprintf(buffer,"Z: %d", zAccelStar);
        tft_writeString(buffer);
        
        tft_fillRoundRect(0,170, 300, 14, 1, ILI9340_BLACK);// x,y,w,h,radius,color
        tft_setCursor(0, 170);
        if(direction_star==UP) sprintf(buffer,"Direction Star: UP");
        else if(direction_star==DOWN) sprintf(buffer,"Direction Star: DOWN");
        else if(direction_star==LEFT) sprintf(buffer,"Direction Star: LEFT");
        else if(direction_star==RIGHT) sprintf(buffer,"Direction Star: RIGHT");
        else sprintf(buffer,"Direction Star: NONE");
        tft_writeString(buffer);

        // NEVER exit while
      } // END WHILE(1)
  PT_END(pt);
} // timer thread

static enum state{release, maybe_push, push, maybe_release};
static enum state keypad_state = release;

static PT_THREAD (protothread_calibrate(struct pt *pt))
{
    PT_BEGIN(pt);
    flag = 0;
    int next;
    
      while(1) {
        //PT_YIELD_UNTIL(pt,flag!=0);
        flag2 = 1;
        //UP
        
        tft_setCursor(0, 30);
        sprintf(buffer,"Press BUTTON");
        tft_writeString(buffer);
        next = 0;
        PT_YIELD_TIME_msec(30);
        next = mPORTBReadBits(BIT_10);
        switch(keypad_state){
            case release:
                if (next == 0){
                    keypad_state = release;
                }
                else{
                    keypad_state = maybe_push;
                }
               
                break;
                
            case maybe_push:
                if (next == 0){
                    keypad_state = release;    
                }
                else{
                    keypad_state = push;
                    i+=1;
                    tft_fillRoundRect(0,10, 300, 14, 1, ILI9340_BLACK);// x,y,w,h,radius,color
                }
                break;
                
            case push:
                //put in play tone logic
                if (next == 0){
                    keypad_state = maybe_release;
                }
                else{
                    keypad_state = push;
                }
                break;
                
            case maybe_release:
                if (next == 0){
                    keypad_state = release;                    
                }
                else{
                    keypad_state = push;
                }
                break;
        }
        if(i==0){
            tft_fillRoundRect(0,0, 300, 300, 1, ILI9340_BLACK);// x,y,w,h,radius,color
            tft_fillRoundRect(0,10, 300, 14, 1, ILI9340_BLACK);// x,y,w,h,radius,color
            i=1;
        }
        else if(i==1){
            x_up = xAccelPort;
            y_up = yAccelPort;
            z_up = zAccelPort;
            tft_setCursor(0, 10);
            tft_setTextColor(ILI9340_YELLOW); tft_setTextSize(2);
            sprintf(buffer,"Move UP");
            tft_writeString(buffer);
        }
        else if(i==2){
            x_down = xAccelPort;
            y_down = yAccelPort;
            z_down = zAccelPort;
            
            tft_setCursor(0, 10);
            sprintf(buffer,"Move DOWN");
            tft_writeString(buffer);
        }
        else if(i==3){
            x_left = xAccelPort;
            y_left = yAccelPort;
            z_left = zAccelPort;
            tft_setCursor(0, 10);
            sprintf(buffer,"Move LEFT");
            tft_writeString(buffer);
        }
        else if(i==4){
            x_right = xAccelPort;
            y_right = yAccelPort;
            z_right = zAccelPort;
            tft_setCursor(0, 10);
            sprintf(buffer,"Move RIGHT");
            tft_writeString(buffer);        
        }
        else{
            tft_fillRoundRect(0,10, 200, 14, 1, ILI9340_BLACK);// x,y,w,h,radius,color
            tft_fillRoundRect(0,30, 200, 14, 1, ILI9340_BLACK);// x,y,w,h,radius,color
            tft_setCursor(0, 120);
            sprintf(buffer,"GROOVY!");
            tft_writeString(buffer);
            flag = 0;
        }
        
        
        flag2 = 0;
        
        // NEVER exit while
      } // END WHILE(1)
  PT_END(pt);
} // timer thread

static PT_THREAD (protothread_wait(struct pt *pt))
{
    PT_BEGIN(pt);
    int dummy;
    dummy = 0;
      while(1) {
        PT_YIELD_UNTIL(pt,flag2!=0);
        TMR2 = 0x0;
        while(dummy<2000) dummy = ReadTimer2();
        PT_YIELD_TIME_msec(100);
        // NEVER exit while
      } // END WHILE(1)
    
    PT_END(pt);
} // timer thread

// === Main  ======================================================
void main(void) {
 //SYSTEMConfigPerformance(PBCLK);
  
  ANSELA = 0; ANSELB = 0; 

  // set up DAC on big board
  // timer interrupt //////////////////////////
    // Set up timer2 on,  interrupts, internal clock, prescalar 1, toggle rate
    // at 30 MHz PB clock 60 counts is two microsec
    // 400 is 100 ksamples/sec
    // 2000 is 20 ksamp/sec
    OpenTimer2(T2_ON | T2_SOURCE_INT | T2_PS_1_16, 5000);

    // set up the timer interrupt with a priority of 2
    ConfigIntTimer2(T2_INT_ON | T2_INT_PRIOR_2);
    mT2ClearIntFlag(); // and clear the interrupt flag

    // SCK2 is pin 26 
    // SDO2 (MOSI) is in PPS output group 2, could be connected to RB5 which is pin 14
    PPSOutput(2, RPB5, SDO2);

    // control CS for DAC
    mPORTBSetPinsDigitalOut(BIT_4);
    mPORTBSetBits(BIT_4);
    mPORTBSetPinsDigitalIn(BIT_10);
    EnablePullDownB(BIT_10);
    
    mPORTASetPinsDigitalOut(BIT_3 | BIT_4);
    mPORTASetBits(BIT_3);
    mPORTAClearBits(BIT_4);  

    // divide Fpb by 2, configure the I/O ports. Not using SS in this example
    // 16 bit transfer CKP=1 CKE=1
    // possibles SPI_OPEN_CKP_HIGH;   SPI_OPEN_SMP_END;  SPI_OPEN_CKE_REV
    // For any given peripherial, you will need to match these
    // clk divider set to 2 for 20 MHz
    SpiChnOpen(SPI_CHANNEL2, SPI_OPEN_ON | SPI_OPEN_MODE16 | SPI_OPEN_MSTEN | SPI_OPEN_CKE_REV , 2);
  // end DAC setup
    
  OpenI2C1(I2C_ON, 48);
  IdleI2C1();
  
  StartI2C1(); //Start
  IdleI2C1();
  
  MasterWriteI2C1(0x3A); //Send Device Address (Write)
  IdleI2C1();
  while (I2C1STATbits.ACKSTAT); //wait for slave acknowledge
  
  MasterWriteI2C1(0x2A); //Send Register Address
  IdleI2C1();
  while (I2C1STATbits.ACKSTAT); //wait for slave acknowledge
  
  MasterWriteI2C1(0x03); //Send Write Data
  IdleI2C1();
  while (I2C1STATbits.ACKSTAT); //wait for slave acknowledge
  StopI2C1();
    
  // === config threads ==========
  // turns OFF UART support and debugger pin, unless defines are set
  PT_setup();

  // === setup system wide interrupts  ========
  INTEnableSystemMultiVectoredInt();

  // init the threads
  PT_INIT(&pt_timer);
  PT_INIT(&pt_i2c);
  PT_INIT(&pt_direction);
  PT_INIT(&pt_calibrate);
  //PT_INIT(&pt_wait);

  // init the display
  tft_init_hw();
  tft_begin();
  tft_fillScreen(ILI9340_BLACK);
  //240x320 vertical display
  tft_setRotation(0); // Use tft_setRotation(1) for 320x240

  // seed random color
  srand(1);
  
  // round-robin scheduler for threads
  while (1){
        if(flag==0){
            PT_SCHEDULE(protothread_timer(&pt_timer));
            PT_SCHEDULE(protothread_direction(&pt_direction));
            
        }
        else{
            PT_SCHEDULE(protothread_calibrate(&pt_calibrate));
        }
        
        
        PT_SCHEDULE(protothread_i2c(&pt_i2c));
     //if(flag2==1) PT_SCHEDULE(protothread_wait(&pt_wait));
  } // main
}
// === end  ======================================================
