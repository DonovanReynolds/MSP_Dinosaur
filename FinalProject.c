/*
EEL 4742C - UCF

Code that prints a welcome message to the pixel display.
*/

#include "msp430fr6989.h"
#include "Grlib/grlib/grlib.h"          // Graphics library (grlib)
#include "LcdDriver/lcd_driver.h"       // LCD driver
#include <stdio.h>
extern Graphics_Image TinyDino8BPP_UNCOMP;
extern Graphics_Image Cactus;
extern Graphics_Image Cloud;
extern Graphics_Image CloudSmall;
extern Graphics_Image DinoChar;
extern Graphics_Image Ground;
extern Graphics_Image GroundBump;

#define S1 BIT1
#define S2 BIT2

#define redLED BIT0
#define greenLED BIT7
#define BUT1 BIT1
#define BUT2 BIT2

#define FLAGS UCA1IFG
#define RXFLAG UCRXIFG
#define TXFLAG UCTXIFG
#define TXBUFFER UCA1TXBUF
#define RXBUFFER UCA1RXBUF

#define RESULTREGISTER 0x00
#define CONFIGREGISTER 0x01
#define LOWLIMITREGISTER 0X02
#define HIGHLIMITREGISTER 0X03

#define LIGHTSENSORADDRESS 0x44
#define RESULTADDRESS 0x00
void Initialize_Clock_System();



//ADC
//ADC
void Initialize_ADC() {
// Divert the pins to analog functionality
// X-axis: A10/P9.2, for A10 (P9DIR=x, P9SEL1=1, P9SEL0=1)
    P8SEL1 |= (BIT5|BIT6|BIT4);
    P8SEL0 |= (BIT5|BIT6|BIT4);

// Turn on the ADC module
    ADC12CTL0 |= ADC12ON;
// Turn off ENC (Enable Conversion) bit while modifying the configuration
    ADC12CTL0 &=  ~ADC12ENC;

//*************** ADC12CTL0 ***************
// Set ADC12SHT0 (select the number of cycles that you determined)

    ADC12CTL0  |= ADC12SHT01 | ADC12MSC;

//*************** ADC12CTL1 ***************
// Set ADC12SHS (select ADC12SC bit as the trigger)
// Set ADC12SHP bit
// Set ADC12DIV (select the divider you determined)
// Set ADC12SSEL (select MODOSC)
    ADC12CTL1 |= ADC12SC;
    ADC12CTL1 |= ADC12SHP;
    //ADC12CTL1 |= ADC12DIV0;
    //ADC12CTL1 |= ADC12SSEL0;
    ADC12CTL1 |= ADC12CONSEQ_1;



//*************** ADC12CTL2 ***************
// Set ADC12RES (select 12-bit resolution)
// Set ADC12DF (select unsigned binary format)

    ADC12CTL2 |= ADC12RES1;

//*************** ADC12CTL3 ***************
// Leave all fields at default values
    ADC12CTL3 |= ADC12CSTARTADD_0;

//*************** ADC12MCTL0 ***************
// Set ADC12VRSEL (select VR+=AVCC, VR-=AVSS)
// Set ADC12INCH (select channel A10)
// Turn on ENC (Enable Conversion) bit at the end of the configuration
    ADC12MCTL0 |= ADC12INCH_5;
    ADC12MCTL0 |= ADC12EOS;

    ADC12IER0 |= ADC12IE0;

ADC12CTL0 |= ADC12ENC;
return;
}

//////////////////////////////////////////

// *****************************
void Initialize_Clock_System() {
    // DCO frequency = 8 MHz (default value)
    // MCLK = fDCO/2 = 4 MHz
    // SMCLK = fDCO/1 = 8 MHz
    CSCTL0 = CSKEY;                         // Unlock clock module config registers
    CSCTL3 &= ~(BIT2|BIT1|BIT0);            // DIVM = 000
    CSCTL3 |= BIT0;                         // DIVM = 001 = /2
    CSCTL3 &= ~(BIT6|BIT5|BIT4);            // DIVS = 000 = /1
    CSCTL0_H = 0;                           // Relock clock module config registers

    return;
}

void Initialize_I2C(void)
{
    P4SEL1 |= (BIT1|BIT0);
    P4SEL0 &= ~(BIT1|BIT0);

    UCB1CTLW0 = UCSWRST;

    UCB1CTLW0 |= UCMST| UCMODE_3 | UCSYNC | UCSSEL_3;

    UCB1BRW = 25.6;
    UCB1CTLW0 &= ~UCSWRST;
}

int i2c_read_word(unsigned char i2c_address, unsigned char i2c_reg, unsigned int* data)
{
    unsigned char byte1, byte2;
    // Initialize the bytes to make sure data is received every time
    byte1 = 111;
    byte2 = 111;
    //********** Write Frame #1 ***************************
    UCB1I2CSA = i2c_address; // Set I2C address
    UCB1IFG &= ~UCTXIFG0;
    UCB1CTLW0 |= UCTR; // Master writes (R/W bit = Write)
    UCB1CTLW0 |= UCTXSTT; // Initiate the Start Signal

    while ((UCB1IFG & UCTXIFG0) ==0) {}
    UCB1TXBUF = i2c_reg; // Byte = register address
    while((UCB1CTLW0 & UCTXSTT)!=0) {}
    if(( UCB1IFG & UCNACKIFG )!=0) return -1;
    UCB1CTLW0 &= ~UCTR; // Master reads (R/W bit = Read)
    UCB1CTLW0 |= UCTXSTT; // Initiate a repeated Start Signal
    //****************************************************
    //********** Read Frame #1 ***************************
    while ( (UCB1IFG & UCRXIFG0) == 0) {}
    byte1 = UCB1RXBUF;
    //****************************************************
    //********** Read Frame #2 ***************************
    while((UCB1CTLW0 & UCTXSTT)!=0) {}
    UCB1CTLW0 |= UCTXSTP; // Setup the Stop Signal
    while ( (UCB1IFG & UCRXIFG0) == 0) {}
    byte2 = UCB1RXBUF;
    while ( (UCB1CTLW0 & UCTXSTP) != 0) {}
    //****************************************************
    // Merge the two received bytes
    *data = ( (byte1 << 8) | (byte2 & 0xFF) );
    return 0;

}
// Write a word (2 bytes) to I2C (address, register)
int i2c_write_word(unsigned char i2c_address, unsigned char i2c_reg,
unsigned int data) {
unsigned char byte1, byte2;
byte1 = (data >> 8) & 0xFF; // MSByte
byte2 = data & 0xFF; // LSByte
UCB1I2CSA = i2c_address; // Set I2C address
UCB1CTLW0 |= UCTR; // Master writes (R/W bit = Write)
UCB1CTLW0 |= UCTXSTT; // Initiate the Start Signal
while ((UCB1IFG & UCTXIFG0) ==0) {}
UCB1TXBUF = i2c_reg; // Byte = register address
while((UCB1CTLW0 & UCTXSTT)!=0) {}
// ---- ADD THIS LINE ----
while ((UCB1IFG & UCTXIFG0) ==0) {}
//********** Write Byte #1 ***************************
UCB1TXBUF = byte1;
while ( (UCB1IFG & UCTXIFG0) == 0) {}
//********** Write Byte #2 ***************************
UCB1TXBUF = byte2;
while ( (UCB1IFG & UCTXIFG0) == 0) {}
UCB1CTLW0 |= UCTXSTP;
while ( (UCB1CTLW0 & UCTXSTP) != 0) {}
return 0;
}


// Configures ACLK to 32 KHz crystal
void config_ACLK_TO_32KHz_crystal(void) {
   PJSEL1 &= ~BIT4;
   PJSEL0 |= BIT4;

   CSCTL0 =CSKEY;
   do{
       CSCTL5 &= ~LFXTOFFG;
       SFRIFG1 &= ~OFIFG;
   } while((CSCTL5 & LFXTOFFG) != 0);
   CSCTL0_H = 0;
   return;
}






Graphics_Context g_sContext;
Graphics_Context layerTwo;
Graphics_Context clouds;

Graphics_Rectangle DinoBox;
Graphics_Rectangle CactusBox;

int dinoX,dinoY,cactusX,cactusY;
int imageYDino;

int isJumping;

int imageX;
int imageY;

int goingUp;
int goingDown;

int smallCloudX;
int topSmallCloudX;

int gameOverFlg;

//63 is top

void init()
{


    imageYDino = 100;
    _delay_cycles(50000);
    Graphics_clearDisplay(&g_sContext);
    P1IFG &= ~(BUT1|BUT2);

    smallCloudX = 128;
    topSmallCloudX = 128;

    dinoX = 34;
    dinoY = imageYDino;

    DinoBox.xMin = dinoX+2;
    DinoBox.xMax = dinoX + 21;
    DinoBox.yMin = dinoY;
    DinoBox.yMax = dinoY + 20;

    cactusX = 110;
    cactusY = 100;

    CactusBox.xMin = cactusX;
    CactusBox.xMax = cactusX + 25;
    CactusBox.yMin = cactusY+10;
    CactusBox.yMax = cactusY + 20;



    isJumping = 0;
    imageX = 110;
    imageY = 100;
    goingUp = 0;
    goingDown = 0;
    gameOverFlg = 0;

    Graphics_Rectangle wholeScreen = {0,0,128,128};
    Graphics_Rectangle CloudScreen = {24,0,128,60};
    Graphics_Rectangle cutScreen = {24,0,128,128};
    Graphics_Rectangle cutOffRegion = {0,0,23,128};

    Graphics_setClipRegion(&g_sContext, &wholeScreen);
    Graphics_setForegroundColor(&g_sContext,GRAPHICS_COLOR_PURPLE);
    Graphics_fillRectangle(&g_sContext,&cutOffRegion);



    Graphics_setClipRegion(&g_sContext, &cutScreen);
    Graphics_setClipRegion(&layerTwo, &cutScreen);
    Graphics_setClipRegion(&clouds, &CloudScreen);


    drawScreen();
    TA0CTL |= TASSEL_1 | MC_1 | TACLR;

    Graphics_setForegroundColor(&layerTwo,GRAPHICS_COLOR_PURPLE);




}





void updateHitboxes()
{
    DinoBox.xMin = dinoX+2;
    DinoBox.xMax = dinoX + 21;
    DinoBox.yMin = dinoY;
    DinoBox.yMax = dinoY + 20;

    CactusBox.xMin = cactusX;
    CactusBox.xMax = cactusX + 25;
    CactusBox.yMin = cactusY+5;
    CactusBox.yMax = cactusY + 20;


}



void drawScreen()
{
    Graphics_drawImage(&clouds, &CloudSmall, smallCloudX, 40);
    Graphics_drawImage(&clouds, &CloudSmall, topSmallCloudX, 0);

    Graphics_setForegroundColor(&clouds, GRAPHICS_COLOR_BLACK);
    Graphics_Rectangle clearBotCloud = {smallCloudX+22,45,smallCloudX+24,47};
    Graphics_Rectangle clearTopCloud = {topSmallCloudX+22,5,topSmallCloudX+24,7};
    Graphics_fillRectangle(&clouds,&clearBotCloud);
    Graphics_fillRectangle(&clouds,&clearTopCloud);





    Graphics_drawImage(&layerTwo,&GroundBump,0,119);
    Graphics_drawImage(&g_sContext, &Cactus, imageX, 100);
    Graphics_drawImage(&g_sContext, &DinoChar, 30, imageYDino);


    Graphics_Rectangle clearBot = {32,imageYDino+23,44,imageYDino+29};
    Graphics_Rectangle clearTop = {41,imageYDino-1,51,imageYDino-7};

    if (isJumping)
    {
        Graphics_setForegroundColor(&g_sContext, GRAPHICS_COLOR_BLACK);
        if (goingUp)
            Graphics_fillRectangle(&g_sContext,&clearBot);
        else if (goingDown)
            Graphics_fillRectangle(&g_sContext,&clearTop);
    }
    else
    {
        Graphics_setForegroundColor(&g_sContext,GRAPHICS_COLOR_BLACK);
        Graphics_Rectangle clearTop = {50,imageYDino,56,imageYDino-1};
        Graphics_fillRectangle(&g_sContext,&clearTop);
    }



}




// ****************************************************************************
void main(void) {


    // Configure WDT & GPIO
    WDTCTL = WDTPW | WDTHOLD;
    PM5CTL0 &= ~LOCKLPM5;

    Initialize_Clock_System();

    //initUart();
    //Initialize_I2C();
    Initialize_ADC();
    config_ACLK_TO_32KHz_crystal();

    // Configure LEDs
    P1DIR |= redLED;                P9DIR |= greenLED;
    P1OUT &= ~redLED;               P9OUT &= ~greenLED;

    // Configure buttons
    P1DIR &= ~(S1|S2);
    P1REN |= (S1|S2);
    P1OUT |= (S1|S2);
    P1IFG &= ~(S1|S2);          // Flags are used for latched polling


    P1IES |= (BUT1|BUT2);
    P1IFG &= ~(BUT1|BUT2);
    P1IE |= (BUT1|BUT2);


    // Set the LCD backlight to highest level
    //P2DIR |= BIT6;
    //P2OUT |= BIT6;


    Crystalfontz128x128_Init();         // Initialize the display

        // Set the screen orientation
        Crystalfontz128x128_SetOrientation(0);

        // Initialize the context
        Graphics_initContext(&g_sContext, &g_sCrystalfontz128x128);
        Graphics_initContext(&layerTwo, &g_sCrystalfontz128x128);
        Graphics_initContext(&clouds, &g_sCrystalfontz128x128);



        // Set background and foreground colors


        // Set the default font for strings
        GrContextFontSet(&g_sContext, &g_sFontFixed6x8);

        Graphics_setBackgroundColor(&g_sContext,GRAPHICS_COLOR_BLACK);

        //Graphics_setClipRegion(&g_sContext,&DinoBox);

        TA0CCR0 |= 1092;

    init();


    TA0CCTL0 |= CCIE;
    __enable_interrupt();
    //_low_power_mode_1();
    while(1){}


}

void reset()
{

    isJumping = 0;
    imageX = 10;
    imageY = 80;


    Graphics_clearDisplay(&g_sContext);




}



void gameOver()
{
    TA0CTL = 0;
    gameOverFlg = 1;
}


void moveScreen()
{
    //rightDinoX,rightDinoY,botDinoX,botDinoY,leftCactusX,leftCactusY,rightCactusX,rightCactusY,topCactusX,topCactusY;
    Graphics_setForegroundColor(&g_sContext, GRAPHICS_COLOR_BLACK);
    Graphics_Rectangle temp = {imageX+15,imageY, imageX+17, imageY+28};

    Graphics_fillRectangle(&g_sContext, &temp);

    if (imageX > 6)
        Graphics_setClipRegion(&layerTwo, &temp);



    drawScreen();

    imageX = imageX - 3;

    smallCloudX = smallCloudX - 2;
    topSmallCloudX = topSmallCloudX - 1;

    if (topSmallCloudX < 0)
        topSmallCloudX = 128;

    if (smallCloudX < 0)
        smallCloudX = 128;

    if (imageX < 7)
    {
        imageX = 128;
    }

    cactusX = imageX-1;
    updateHitboxes();

    //Test

    if (Graphics_isRectangleOverlap(&CactusBox,&DinoBox))
    {
        gameOver();
    }

}

int jumpingAnimation[] = {100,92,85,79,74,70,67,65,64,64,63,63,63,63,63,64,64,65,66,67,69,70,72,74,79,83,85,89,92,95,99,100};

int jumpingIndex;


void jumpDino()
{


    if (!isJumping)
    {
        jumpingIndex = 0;
    }
    imageYDino = jumpingAnimation[jumpingIndex++];

    if (jumpingIndex >= 32)
    {
        isJumping = 0;
        jumpingIndex = 0;
    }
    if (jumpingIndex == 16)
    {
        goingUp = 0;
        goingDown = 1;
    }

    if (imageYDino == 85)
    {
        Graphics_drawImage(&g_sContext, &GroundBump,0,119);
    }

    dinoY = imageYDino;
    updateHitboxes();
}

int x,y,z;
void updateDisplay()
{

    moveScreen();

    if (isJumping)
    {
        jumpDino();
    }
    else
        ADC12IER0 |= ADC12IE0;

    ADC12CTL0 |= ADC12SC;

}


#pragma vector=TIMER0_A0_VECTOR
   __interrupt void Timer0_A0 (void) {
       updateDisplay();


}

#pragma vector = PORT1_VECTOR

__interrupt void resetButton(void)
{
    if (gameOverFlg)
        init();
    else
        P1IFG &= ~(BUT1|BUT2);
}


#pragma vector = ADC12_VECTOR

__interrupt void doSomething(void)
{
    int z = ADC12MEM0;
    if (z > 3100 || z < 2600)
    {
        isJumping = 1;
        goingUp = 1;
        ADC12IER0 &= ~ADC12IE0;
    }


}


