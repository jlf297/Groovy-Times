/*
 * File:        TFT_test_BRL4.c
 * Author:      Bruce Land
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
#include <math.h>
////////////////////////////////////
#define MAX_BALLS 320
#define TIME_LIMIT 70
#define DAC_config_chan_A 0b0011000000000000
#define EnablePullDownA(bits) CNPUACLR=bits; CNPDASET=bits;

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
char buffer[60];
// === thread structures ============================================
// thread control structs
// note that UART input and output are threads
static struct pt pt_timer, pt_color, pt_anim, pt_paddle, pt_sound, pt_reset;

// === the fixed point macros ========================================
typedef signed int fix16 ;
#define multfix16(a,b) ((fix16)(((( signed long long)(a))*(( signed long long)(b)))>>16)) //multiply two fixed 16:16
#define float2fix16(a) ((fix16)((a)*65536.0)) // 2^16
#define fix2float16(a) ((float)(a)/65536.0)
#define fix2int16(a)    ((int)((a)>>16))
#define int2fix16(a)    ((fix16)((a)<<16))
#define divfix16(a,b) ((fix16)((((signed long long)(a)<<16)/(b)))) 
#define sqrtfix16(a) (float2fix16(sqrt(fix2float16(a)))) 
#define absfix16(a) abs(a)

#define sine_table_size_1 256
#define sine_table_size_2 128
#define sine_table_size_3 64
static short sin_table_1[sine_table_size_1];
static short sin_table_2[sine_table_size_2];
static short sin_table_3[sine_table_size_3];
#define dmaChn 0

static int score_flag = 0;
static int miss_flag = 0;
static int end_flag = 0;

//value of ADC
static int adc_val;
//center x coordinate of the paddle
static short center; 
//definition of ball
typedef struct ball{
    fix16 xc;
    fix16 yc;
    fix16 vxc;
    fix16 vyc;
    char on_screen;
    char hit_timeout;
}ball;
// array of ball objects
static ball sack[MAX_BALLS];
//static ball grid[15][10][MAX_BALLS>>3];
//static int grid_count[15][10];

//system time
int sys_time_seconds;
///frame rate
int frame_time=0;
//player score
int score=0;
//amount of balls on screen
int num_balls=0;

//useful constants
fix16 fixed_100 = int2fix16(100);
fix16 fixed_300 = int2fix16(300);
fix16 fixed_2 = int2fix16(2);

inline void reset(ball* b){
    b->xc = fixed_100;
    b->yc = fixed_300;
    b->vyc = fixed_2;
    b->vxc =int2fix16((-(adc_val-511)*3))>>10;
    //b->on_screen = 0; TAKEN OUT WITH NEW GENERATION
    b->hit_timeout = 2;
}

//inline void reset(ball* b){ TAKEN OUT WITH NEW GENERATION
//    b->vxc =int2fix16((-(adc_val-511)*3))>>10;
//}
inline void draw_ball(short x, short y){
  tft_drawPixel(x, y, ILI9340_GREEN);
  tft_drawPixel(x+2, y, ILI9340_GREEN);
  tft_drawPixel(x-2, y, ILI9340_GREEN);
  tft_drawPixel(x, y+2, ILI9340_GREEN);
  tft_drawPixel(x, y-2, ILI9340_GREEN);
}

inline void erase_ball(short x, short y){
  tft_drawPixel(x, y, ILI9340_BLACK);
  tft_drawPixel(x+2, y, ILI9340_BLACK);
  tft_drawPixel(x-2, y, ILI9340_BLACK);
  tft_drawPixel(x, y+2, ILI9340_BLACK);
  tft_drawPixel(x, y-2, ILI9340_BLACK);
}
/** BALL COLLISIONS THREAD **/
static PT_THREAD (protothread_anim(struct pt *pt))
{
    PT_BEGIN(pt);
    
    // Table of denominators for hard ball collision calculation
    static fix16 denoms[] = {float2fix16(0.25f), float2fix16(0.2f), float2fix16(0.167f), float2fix16(0.143f),
                             float2fix16(0.125f), float2fix16(0.111f), float2fix16(0.1f), float2fix16(0.0909f),
                             float2fix16(0.0833f), float2fix16(0.0769f), float2fix16(0.0714f), float2fix16(0.0666f)};
    static fix16 r_ij_x;
    static fix16 r_ij_y;
    static fix16 v_ij_x;
    static fix16 v_ij_y;
    static fix16 r_ij_squared;
    static fix16 delta_v_scale; 
    static fix16 delta_v_i_x;
    static fix16 delta_v_i_y;  
    
    //useful constants
    static fix16 fixed_4 = int2fix16(4);
    static fix16 fixed_16 = int2fix16(16);
    static fix16 fixed_10 = int2fix16(10);
    static fix16 fixed_310 = int2fix16(310);
    static fix16 fixed_230 = int2fix16(230);
    static fix16 fixed_point15 = float2fix16(0.15f);
    static fix16 fixed_22 = int2fix16(22);
    static fix16 fixed_80 =int2fix16(80);
    static fix16 fixed_87 = int2fix16(87);
    static fix16 fixed_182 = int2fix16(182);
    static fix16 fixed_62 = int2fix16(62);
    
    static int begin_time;
    static char loop_counter;
    frame_time = PT_GET_TIME();
      while(1) {
          // yield time begin
        begin_time = PT_GET_TIME();
        
        // Generate new balls 
        if(num_balls<MAX_BALLS){
            if(loop_counter==1 && !sack[num_balls].on_screen){                    
                    sack[num_balls].on_screen = 1;
                    num_balls++;     
                    loop_counter = 0;
            } 
            else loop_counter++;
        }
        //TAKEN OUT WITH NEW GENERATION
//        if(num_balls<MAX_BALLS){ 
//            if(loop_counter==2){
//                int temp;
//                for(temp=0; temp<MAX_BALLS; temp++){
//                    if(!sack[temp].on_screen){
//                        sack[temp].on_screen = 1;
//                        num_balls++;
//                        break;
//                    }
//                }
//                loop_counter = 0;
//            }
//            else loop_counter++;
//        }
        
        register int b=0;
        register int c=0;
        for(b=0; b<MAX_BALLS; b++){
            if(sack[b].on_screen){                               
                //erase ball
                erase_ball(fix2int16(sack[b].xc), fix2int16(sack[b].yc));   
                
                //check paddle collisions
                if(sack[b].yc<fixed_22){
                    if(sack[b].xc<int2fix16(center+32) && sack[b].xc>int2fix16(center-32)) {
                        score++;
                        score_flag = 1;
                    }
                    else{
                        score--;
                        miss_flag=1;
                    }
                        
                    reset(&sack[b]);
                    //set_vx(&sack[b]); TAKEN OUT W NEW GENERATION
                    //num_balls--;
                    continue;
                }
                
                // Check wall collisions
                if (sack[b].xc<fixed_10){ 
                    sack[b].xc = fixed_10;
                    sack[b].vxc = -sack[b].vxc;
                }
                else if(sack[b].xc>fixed_230){
                    sack[b].xc = fixed_230;
                    sack[b].vxc = -sack[b].vxc;    
                }
                else if(sack[b].yc>fixed_310){
                    sack[b].yc = fixed_310;
                    sack[b].vyc = -sack[b].vyc;    
                } 
                
                //Check bumper collisions
                if(sack[b].yc>fixed_80 && sack[b].yc<fixed_87){
                    if(sack[b].xc>fixed_182 || sack[b].xc<fixed_62){
                        sack[b].vxc = -sack[b].vxc;  
                        sack[b].vyc = -sack[b].vyc;  
                        sack[b].yc = fixed_87;
                    }                           
                }                     
                
                //Check collisions with other balls
                if(sack[b].hit_timeout!=0){
                    for(c=b+1; c<MAX_BALLS; c++){
                        if(sack[c].on_screen && sack[c].hit_timeout!=0){
                            r_ij_x = sack[b].xc - sack[c].xc;
                            r_ij_y = sack[b].yc - sack[c].yc;
                            if ((absfix16(r_ij_x) <= fixed_4 && absfix16(r_ij_y) <= fixed_4)){
                                r_ij_squared = multfix16(r_ij_x,r_ij_x)+multfix16(r_ij_y,r_ij_y);
                                // Head On Collision, swap velocities
                                if(r_ij_squared<=fixed_4){
                                    fix16 temp; 
                                    temp = sack[b].vxc;
                                    sack[b].vxc = sack[c].vxc;
                                    sack[c].vxc = temp;

                                    temp = sack[b].vyc;
                                    sack[b].vyc  = sack[c].vyc;
                                    sack[c].vyc = temp;                                 
                                }
                                // Conservation of momentum collision
                                else if (r_ij_squared < fixed_16){
                                    v_ij_x = sack[b].vxc - sack[c].vxc;
                                    v_ij_y = sack[b].vyc - sack[c].vyc;
                                    delta_v_scale = multfix16(multfix16(r_ij_x, v_ij_x)+multfix16(r_ij_y, v_ij_y),denoms[fix2int16(r_ij_squared)-4]); 
                                    delta_v_i_x = multfix16((-r_ij_x),delta_v_scale);
                                    delta_v_i_y = multfix16((-r_ij_y),delta_v_scale);
                                    sack[b].vxc += delta_v_i_x;
                                    sack[b].vyc += delta_v_i_y;
                                    sack[c].vxc -= delta_v_i_x;
                                    sack[c].vyc -= delta_v_i_y;                                     
                                    sack[b].hit_timeout = 5;
                                    break;
                                }
                            }
                        } 
                    }
                }
                else {sack[b].hit_timeout--;}
                
                //update position / drag
                if(absfix16(sack[b].vxc) > fixed_point15){
                    sack[b].vxc -= sack[b].vxc>>10; //division by 2^10
                    sack[b].xc += sack[b].vxc;
                }    
                if(absfix16(sack[b].vyc) > fixed_point15){
                    sack[b].vyc -= sack[b].vyc>>10;
                    sack[b].yc += sack[b].vyc;
                }
                
                draw_ball(fix2int16(sack[b].xc), fix2int16(sack[b].yc));
            }
        }
        //frame_time = PT_GET_TIME() - frame_time;
        frame_time = (PT_GET_TIME() - begin_time);
        PT_YIELD_TIME_msec(67 - frame_time) ;
       // NEVER exit while
     } // END WHILE(1)
 PT_END(pt);
} // animation thread

/** SOUND THREAD **/
static PT_THREAD (protothread_sound(struct pt *pt))
{
   PT_BEGIN(pt);
   static unsigned int score_timer;
   PT_YIELD_UNTIL(pt, (score_flag==1||miss_flag==1||end_flag==1));
    while(1) {
        if(end_flag==1){
             DmaChnDisable(dmaChn);
             DmaChnSetTxfer(dmaChn, sin_table_3, (void*)&SPI2BUF, sine_table_size_3*2, 2, 2); // DMA sends data to SPI2
             PT_YIELD_TIME_msec(7);
             DmaChnEnable(dmaChn);
             PT_YIELD_TIME_msec(3000);
             DmaChnDisable(dmaChn);
             end_flag=0;
             break;
        }
        if(score_flag==1){
             DmaChnDisable(dmaChn);
             WriteTimer3(0x0);
             DmaChnSetTxfer(dmaChn, sin_table_2, (void*)&SPI2BUF, sine_table_size_2*2, 2, 2); // DMA sends data to SPI2
             PT_YIELD_TIME_msec(7);
             DmaChnEnable(dmaChn);
             score_flag = 0;
        }
        if(miss_flag==1){
             DmaChnDisable(dmaChn);
             WriteTimer3(0x0);
             DmaChnSetTxfer(dmaChn, sin_table_1, (void*)&SPI2BUF, sine_table_size_1*2, 2, 2); // DMA sends data to SPI2
             PT_YIELD_TIME_msec(7);
             DmaChnEnable(dmaChn);
             miss_flag = 0;
        }
        
        
        score_timer = ReadTimer3();
        if(score_timer > 50000){ // Set pre-scaler = 64, 1Sec = 625000
             DmaChnDisable(dmaChn); // Stop the tone after 1 second
             PT_RESTART(pt);
        }
        PT_YIELD_TIME_msec(5); // prevent CPU hogging
        //DmaChnDisable(dmaChn);
        //break;

    }
       //if (end_flag==1)PT_END(pt);
       //else 
   PT_END(pt);
}
    

/** PADDLE THREAD **/
static PT_THREAD (protothread_paddle(struct pt *pt))
{
   PT_BEGIN(pt);
    while(1) {
       // yield time 1 second
       PT_YIELD_TIME_msec(60);
       // read the ADC AN11 
       // read the first buffer position
       adc_val = ReadADC10(0);   // read the result of channel 9 conversion from the idle buffer
       AcquireADC10(); // not needed if ADC_AUTO_SAMPLING_ON below

       // draw adc and voltage
       tft_drawLine(center-30, 20, center+30, 20, ILI9340_BLACK); //erase old paddle
       center = (adc_val>>3)+56; //recalculate the value of the center of paddle
       tft_drawLine(center-30, 20, center+30, 20, ILI9340_RED);//draw new paddle
       tft_drawLine(1, 80, 60, 80, ILI9340_BLUE); //draw bottom bumper of playing field
       tft_drawLine(180, 80, 239, 80, ILI9340_BLUE); //draw top bumper of playing field
       //REDRAW BOUNDS
       tft_drawFastVLine(0, 0, 319,ILI9340_BLUE); //draw bottom border
       tft_drawLine(0, 319, 239, 319, ILI9340_BLUE); //draw right border
       tft_drawLine(239, 319, 239, 0,ILI9340_BLUE); //draw top border
       tft_drawLine(239, 0, 0, 0,ILI9340_BLUE); //draw left border
   }
   PT_END(pt);
}

/**  TIMER THREAD  **/
// system 1 second interval tick
static PT_THREAD (protothread_timer(struct pt *pt))
{
   PT_BEGIN(pt);
    tft_setCursor(0, 0);
    tft_setTextColor(ILI9340_WHITE);  tft_setTextSize(1);
    tft_writeString("Time    FPS         score     balls\n");
    while(1) {
        // yield time 1 second
        PT_YIELD_TIME_msec(1000) ;
        sys_time_seconds++ ;

        // draw sys_time
        tft_fillRoundRect(0,12, 240, 12, 1, ILI9340_BLACK);// x,y,w,h,radius,color
        tft_setCursor(0, 10);
        tft_setTextColor(ILI9340_YELLOW); tft_setTextSize(2);
        sprintf(buffer,"%d", sys_time_seconds);
        tft_writeString(buffer);
        
        tft_setCursor(50, 10);
        sprintf(buffer,"%3d", frame_time);
        tft_writeString(buffer);
        
        tft_setCursor(120, 10);
        if(score>-999)sprintf(buffer,"%4d", score);
        else sprintf(buffer,"BAD");
        tft_writeString(buffer);
        
        tft_setCursor(180, 10);
        sprintf(buffer,"%d", num_balls);
        tft_writeString(buffer);
        
        if(sys_time_seconds==TIME_LIMIT){
            tft_setTextSize(5);
            tft_setCursor(80, 120);
            sprintf(buffer,"END");
            tft_writeString(buffer);
            end_flag=1;
        }
        // NEVER exit while
    } // END WHILE(1)
    PT_END(pt);
} // timer thread
volatile int reset_read;
volatile int kill;
/** RESET THREAD **/
static PT_THREAD (protothread_reset(struct pt *pt))
{
   PT_BEGIN(pt);
   
   
    mPORTASetPinsDigitalIn(BIT_4);
    EnablePullDownA(BIT_4);
    
    while(1) {
        PT_YIELD_TIME_msec(10);
        
        reset_read = mPORTAReadBits(BIT_4);
        if (reset_read==16){
            kill = 1/0;
            
        }
        
    } // END WHILE(1)
    PT_END(pt);
} // timer thread


// === Main  ======================================================
void main(void) {
 //SYSTEMConfigPerformance(PBCLK);
  
  ANSELA = 0; ANSELB = 0; 

  // === config threads ==========
  // turns OFF UART support and debugger pin, unless defines are set
  PT_setup();
  
  CloseADC10();	// ensure the ADC is off before setting the configuration

	// define setup parameters for OpenADC10
	// Turn module on | ouput in integer | trigger mode auto | enable autosample
        // ADC_CLK_AUTO -- Internal counter ends sampling and starts conversion (Auto convert)
        // ADC_AUTO_SAMPLING_ON -- Sampling begins immediately after last conversion completes; SAMP bit is automatically set
        // ADC_AUTO_SAMPLING_OFF -- Sampling begins with AcquireADC10();
        #define PARAM1  ADC_FORMAT_INTG16 | ADC_CLK_AUTO | ADC_AUTO_SAMPLING_OFF //

	// define setup parameters for OpenADC10
	// ADC ref external  | disable offset test | disable scan mode | do 1 sample | use single buf | alternate mode off
	#define PARAM2  ADC_VREF_AVDD_AVSS | ADC_OFFSET_CAL_DISABLE | ADC_SCAN_OFF | ADC_SAMPLES_PER_INT_1 | ADC_ALT_BUF_OFF | ADC_ALT_INPUT_OFF
        //
	// Define setup parameters for OpenADC10
        // use peripherial bus clock | set sample time | set ADC clock divider
        // ADC_CONV_CLK_Tcy2 means divide CLK_PB by 2 (max speed)
        // ADC_SAMPLE_TIME_5 seems to work with a source resistance < 1kohm
        #define PARAM3 ADC_CONV_CLK_PB | ADC_SAMPLE_TIME_5 | ADC_CONV_CLK_Tcy2 //ADC_SAMPLE_TIME_15| ADC_CONV_CLK_Tcy2

	// define setup parameters for OpenADC10
	// set AN11 and  as analog inputs
	#define PARAM4	ENABLE_AN11_ANA // pin 24

	// define setup parameters for OpenADC10
	// do not assign channels to scan
	#define PARAM5	SKIP_SCAN_ALL

	// use ground as neg ref for A | use AN11 for input A     
	// configure to sample AN11 
	SetChanADC10( ADC_CH0_NEG_SAMPLEA_NVREF | ADC_CH0_POS_SAMPLEA_AN11 ); // configure to sample AN11 
	OpenADC10( PARAM1, PARAM2, PARAM3, PARAM4, PARAM5 ); // configure ADC using the parameters defined above

	EnableADC10(); // Enable the ADC
    
    
    
    int j;
    for (j = 0; j < sine_table_size_1; j++){
        sin_table_1[j] = (unsigned short)(1023*sin((float)j*6.283/(float)sine_table_size_1)+1024)|DAC_config_chan_A;
    }
    int i;
    for (i = 0; i < sine_table_size_2; i++){
        sin_table_2[i] = (unsigned short)(1023*sin((float)i*6.283/(float)sine_table_size_2)+1024)|DAC_config_chan_A;
    }
    for (i = 0; i < sine_table_size_3; i++){
        sin_table_3[i] = (unsigned short)(1023*sin((float)i*6.283/(float)sine_table_size_3)+1024)|DAC_config_chan_A;
    }
    
    SpiChnOpen(SPI_CHANNEL2, 
    SPI_OPEN_ON | SPI_OPEN_MODE16 | SPI_OPEN_MSTEN | SPI_OPEN_CKE_REV | SPICON_FRMEN | SPICON_FRMPOL, 2);
    PPSOutput(2, RPB5, SDO2);
    PPSOutput(4, RPB10, SS2);
    OpenTimer2(T2_ON | T2_SOURCE_INT | T2_PS_1_1, 909);
    OpenTimer3(T3_ON | T3_SOURCE_INT | T3_PS_1_64, 0xffff);

    // control CS for DAC
    
    
    
    DmaChnOpen(dmaChn, 0, DMA_OPEN_AUTO); // Auto mode sets DMA to play indefinitely
    DmaChnSetTxfer(dmaChn, sin_table_1, (void*)&SPI2BUF, sine_table_size_1*2, 2, 2); // DMA sends data to SPI2
    DmaChnSetEventControl(dmaChn, DMA_EV_START_IRQ(_TIMER_2_IRQ));

  // === setup system wide interrupts  ========
  INTEnableSystemMultiVectoredInt();

  // init the threads
  //PT_INIT(&pt_timer);
  PT_INIT(&pt_color);
  PT_INIT(&pt_anim);
  PT_INIT(&pt_paddle);
  PT_INIT(&pt_sound);
  PT_INIT(&pt_reset);

  // init the display
  tft_init_hw();
  tft_begin();
  tft_fillScreen(ILI9340_BLACK); //erase screen
  //240x320 vertical display
  tft_setRotation(0); // Use tft_setRotation(1) for 320x240
  tft_drawLine(0, 0, 0, 319,ILI9340_BLUE); //draw bottom border
  tft_drawLine(0, 319, 239, 319, ILI9340_BLUE); //draw right border
  tft_drawLine(239, 319, 239, 0,ILI9340_BLUE); //draw top border
  tft_drawLine(239, 0, 0, 0,ILI9340_BLUE); //draw left border
  //base playing field
  
  // seed random color
  srand(1);

  // initially populate ball array
  int iter = 0;
  for(iter; iter<MAX_BALLS; iter++){
        reset(&sack[iter]);
        //set_vx(&sack[iter]); TAKEN OUT WITH NEW GENERATION
  }
  
  // round-robin scheduler for threads
  while (1){
      if(sys_time_seconds<=TIME_LIMIT) PT_SCHEDULE(protothread_anim(&pt_anim));
      PT_SCHEDULE(protothread_timer(&pt_timer));
      PT_SCHEDULE(protothread_paddle(&pt_paddle));
      PT_SCHEDULE(protothread_sound(&pt_sound));
      PT_SCHEDULE(protothread_reset(&pt_reset));
      }
  } // main

// === end  ======================================================

