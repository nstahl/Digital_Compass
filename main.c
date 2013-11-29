#include <avr/io.h>
#include <avr/interrupt.h>

#include "config.h"

#include "spi2.h"					// Uses spi

#include <util/delay.h>				// _delay_us() and _delay_ms() functions
#include <util/delay_basic.h>


/* 
set BAUD RATE 
http://www.engineersgarage.com/embedded/avr-microcontroller-projects/serial-communication-atmega16-usart
*/
#define F_CPU 8E6				   // 1MHz
#define USART_BAUDRATE 9600


volatile int16_t loop_count, interrupt_count, cbearing;
volatile unsigned char cdata;

#define swap(a, b) { uint8_t t = a; a = b; b = t; }
/* Interfacing to the Nokia display used on the 5110 phone

This code is copied an modified from Sparkfun's website/code example
to use Brian Jones' spi2 library for driving the LCD using the built in microcontroller CPI interface
(unsigned char)
*/

// Nokia LCD definitions
// These were in the example code. I'm not convinced they were right.
/*
#define PIN_SCE   7 //Pin 3 on LCD
#define PIN_RESET 6 //Pin 4 on LCD
#define PIN_DC    5 //Pin 5 on LCD
#define PIN_SDIN  4 //Pin 6 on LCD
#define PIN_SCLK  3 //Pin 7 on LCD*/


// All on PORTB, these definitions were in the original code, I have modified them 
// to match the pins I used on my sample, but these definitions are never used.
// NOte that PB4 needs to be an output to force SPI Master mode
                        // display pin 1, the square pin, is LED backlight
#define PIN_SCLK  PB7   // display pin 2
#define PIN_SDIN  PB5   // display pin 3
#define PIN_DC    PB2   // display pin 4, DATA OR COMMAND
#define PIN_RESET PB1   // display pin 5
#define PIN_SCE   PB3   // display pin 6
                        // display pin 7 is GND
                        // display pin 8 is 3.3V


//The DC pin tells the LCD if we are sending a command or data
#define LCD_COMMAND 0 
#define LCD_DATA  1

//You may find a different size screen, but this one is 84 by 48 pixels
#define LCD_X     84
#define LCD_Y     48
#define LCD_C_X     LCD_X/2
#define LCD_C_Y     LCD_Y/2

//This table contains the hex values that represent pixels
//for a font that is 5 pixels wide and 8 pixels high
static const uint8_t ASCII[][5] = {
  {0x00, 0x00, 0x00, 0x00, 0x00} // 20  
  ,{0x00, 0x00, 0x5f, 0x00, 0x00} // 21 !
  ,{0x00, 0x07, 0x00, 0x07, 0x00} // 22 "
  ,{0x14, 0x7f, 0x14, 0x7f, 0x14} // 23 #
  ,{0x24, 0x2a, 0x7f, 0x2a, 0x12} // 24 $
  ,{0x23, 0x13, 0x08, 0x64, 0x62} // 25 %
  ,{0x36, 0x49, 0x55, 0x22, 0x50} // 26 &
  ,{0x00, 0x05, 0x03, 0x00, 0x00} // 27 '
  ,{0x00, 0x1c, 0x22, 0x41, 0x00} // 28 (
  ,{0x00, 0x41, 0x22, 0x1c, 0x00} // 29 )
  ,{0x14, 0x08, 0x3e, 0x08, 0x14} // 2a *
  ,{0x08, 0x08, 0x3e, 0x08, 0x08} // 2b +
  ,{0x00, 0x50, 0x30, 0x00, 0x00} // 2c ,
  ,{0x08, 0x08, 0x08, 0x08, 0x08} // 2d -
  ,{0x00, 0x60, 0x60, 0x00, 0x00} // 2e .
  ,{0x20, 0x10, 0x08, 0x04, 0x02} // 2f /
  ,{0x3e, 0x51, 0x49, 0x45, 0x3e} // 30 0
  ,{0x00, 0x42, 0x7f, 0x40, 0x00} // 31 1
  ,{0x42, 0x61, 0x51, 0x49, 0x46} // 32 2
  ,{0x21, 0x41, 0x45, 0x4b, 0x31} // 33 3
  ,{0x18, 0x14, 0x12, 0x7f, 0x10} // 34 4
  ,{0x27, 0x45, 0x45, 0x45, 0x39} // 35 5
  ,{0x3c, 0x4a, 0x49, 0x49, 0x30} // 36 6
  ,{0x01, 0x71, 0x09, 0x05, 0x03} // 37 7
  ,{0x36, 0x49, 0x49, 0x49, 0x36} // 38 8
  ,{0x06, 0x49, 0x49, 0x29, 0x1e} // 39 9
  ,{0x00, 0x36, 0x36, 0x00, 0x00} // 3a :
  ,{0x00, 0x56, 0x36, 0x00, 0x00} // 3b ;
  ,{0x08, 0x14, 0x22, 0x41, 0x00} // 3c <
  ,{0x14, 0x14, 0x14, 0x14, 0x14} // 3d =
  ,{0x00, 0x41, 0x22, 0x14, 0x08} // 3e >
  ,{0x02, 0x01, 0x51, 0x09, 0x06} // 3f ?
  ,{0x32, 0x49, 0x79, 0x41, 0x3e} // 40 @
  ,{0x7e, 0x11, 0x11, 0x11, 0x7e} // 41 A
  ,{0x7f, 0x49, 0x49, 0x49, 0x36} // 42 B
  ,{0x3e, 0x41, 0x41, 0x41, 0x22} // 43 C
  ,{0x7f, 0x41, 0x41, 0x22, 0x1c} // 44 D
  ,{0x7f, 0x49, 0x49, 0x49, 0x41} // 45 E
  ,{0x7f, 0x09, 0x09, 0x09, 0x01} // 46 F
  ,{0x3e, 0x41, 0x49, 0x49, 0x7a} // 47 G
  ,{0x7f, 0x08, 0x08, 0x08, 0x7f} // 48 H
  ,{0x00, 0x41, 0x7f, 0x41, 0x00} // 49 I
  ,{0x20, 0x40, 0x41, 0x3f, 0x01} // 4a J
  ,{0x7f, 0x08, 0x14, 0x22, 0x41} // 4b K
  ,{0x7f, 0x40, 0x40, 0x40, 0x40} // 4c L
  ,{0x7f, 0x02, 0x0c, 0x02, 0x7f} // 4d M
  ,{0x7f, 0x04, 0x08, 0x10, 0x7f} // 4e N
  ,{0x3e, 0x41, 0x41, 0x41, 0x3e} // 4f O
  ,{0x7f, 0x09, 0x09, 0x09, 0x06} // 50 P
  ,{0x3e, 0x41, 0x51, 0x21, 0x5e} // 51 Q
  ,{0x7f, 0x09, 0x19, 0x29, 0x46} // 52 R
  ,{0x46, 0x49, 0x49, 0x49, 0x31} // 53 S
  ,{0x01, 0x01, 0x7f, 0x01, 0x01} // 54 T
  ,{0x3f, 0x40, 0x40, 0x40, 0x3f} // 55 U
  ,{0x1f, 0x20, 0x40, 0x20, 0x1f} // 56 V
  ,{0x3f, 0x40, 0x38, 0x40, 0x3f} // 57 W
  ,{0x63, 0x14, 0x08, 0x14, 0x63} // 58 X
  ,{0x07, 0x08, 0x70, 0x08, 0x07} // 59 Y
  ,{0x61, 0x51, 0x49, 0x45, 0x43} // 5a Z
  ,{0x00, 0x7f, 0x41, 0x41, 0x00} // 5b [
  ,{0x02, 0x04, 0x08, 0x10, 0x20} // 5c  backslash, but line continuation overrides comment
  ,{0x00, 0x41, 0x41, 0x7f, 0x00} // 5d ]
  ,{0x04, 0x02, 0x01, 0x02, 0x04} // 5e ^
  ,{0x40, 0x40, 0x40, 0x40, 0x40} // 5f _
  ,{0x00, 0x01, 0x02, 0x04, 0x00} // 60 `
  ,{0x20, 0x54, 0x54, 0x54, 0x78} // 61 a
  ,{0x7f, 0x48, 0x44, 0x44, 0x38} // 62 b
  ,{0x38, 0x44, 0x44, 0x44, 0x20} // 63 c
  ,{0x38, 0x44, 0x44, 0x48, 0x7f} // 64 d
  ,{0x38, 0x54, 0x54, 0x54, 0x18} // 65 e
  ,{0x08, 0x7e, 0x09, 0x01, 0x02} // 66 f
  ,{0x0c, 0x52, 0x52, 0x52, 0x3e} // 67 g
  ,{0x7f, 0x08, 0x04, 0x04, 0x78} // 68 h
  ,{0x00, 0x44, 0x7d, 0x40, 0x00} // 69 i
  ,{0x20, 0x40, 0x44, 0x3d, 0x00} // 6a j 
  ,{0x7f, 0x10, 0x28, 0x44, 0x00} // 6b k
  ,{0x00, 0x41, 0x7f, 0x40, 0x00} // 6c l
  ,{0x7c, 0x04, 0x18, 0x04, 0x78} // 6d m
  ,{0x7c, 0x08, 0x04, 0x04, 0x78} // 6e n
  ,{0x38, 0x44, 0x44, 0x44, 0x38} // 6f o
  ,{0x7c, 0x14, 0x14, 0x14, 0x08} // 70 p
  ,{0x08, 0x14, 0x14, 0x18, 0x7c} // 71 q
  ,{0x7c, 0x08, 0x04, 0x04, 0x08} // 72 r
  ,{0x48, 0x54, 0x54, 0x54, 0x20} // 73 s
  ,{0x04, 0x3f, 0x44, 0x40, 0x20} // 74 t
  ,{0x3c, 0x40, 0x40, 0x20, 0x7c} // 75 u
  ,{0x1c, 0x20, 0x40, 0x20, 0x1c} // 76 v
  ,{0x3c, 0x40, 0x30, 0x40, 0x3c} // 77 w
  ,{0x44, 0x28, 0x10, 0x28, 0x44} // 78 x
  ,{0x0c, 0x50, 0x50, 0x50, 0x3c} // 79 y
  ,{0x44, 0x64, 0x54, 0x4c, 0x44} // 7a z
  ,{0x00, 0x08, 0x36, 0x41, 0x00} // 7b {
  ,{0x00, 0x00, 0x7f, 0x00, 0x00} // 7c |
  ,{0x00, 0x41, 0x36, 0x08, 0x00} // 7d }
  ,{0x10, 0x08, 0x08, 0x10, 0x08} // 7e ~
  ,{0x78, 0x46, 0x41, 0x46, 0x78} // 7f DEL
};									
									#define swap(a, b) { uint8_t t = a; a = b; b = t; }
									
// Function prototypes.								
void gotoXY(uint8_t x, uint8_t y);					
void LCDBitmap(uint8_t my_array[]);
void LCDCharacter(uint8_t character);
void LCDString(uint8_t *characters);
void LCDClear(void);
void LCDInit(void);
void LCDWrite(uint8_t data_or_command, uint8_t data);



void gotoXY(uint8_t x, uint8_t y) {
	LCDWrite(LCD_COMMAND, 0x80 | x);  // Column.
	LCDWrite(LCD_COMMAND, 0x40 | y);  // Row.  ?
}



//This takes a large array of bits and sends them to the LCD
void LCDBitmap(uint8_t my_array[]){
	uint16_t index;
	for (index = 0 ; index < (LCD_X * LCD_Y / 8) ; index++) {
		LCDWrite(LCD_DATA, my_array[index]);
	}
}

//This function takes in a character, looks it up in the font table/array
//And writes it to the screen
//Each character is 8 bits tall and 5 bits wide. We pad one blank column of
//pixels on each side of the character for readability.
void LCDCharacter(uint8_t character) {

	uint8_t index;

	LCDWrite(LCD_DATA, 0x00); //Blank vertical line padding

	for (index = 0 ; index < 5 ; index++) {
		LCDWrite(LCD_DATA, ASCII[character - 0x20][index]);
	}
    //0x20 is the ASCII character for Space (' '). The font table starts with this character

	LCDWrite(LCD_DATA, 0x00); //Blank vertical line padding
}



//Given a string of characters, one by one is passed to the LCD
void LCDString(uint8_t *characters) {
	while (*characters) {
		LCDCharacter(*characters++);
	}
}



//Clears the LCD by writing zeros to the entire screen
void LCDClear(void) {
	uint16_t index;
	for (index = 0 ; index < (LCD_X * LCD_Y / 8) ; index++) {
		LCDWrite(LCD_DATA, 0x00);
    }
	gotoXY(0, 0); //After we clear the display, return to the home position
}



//This sends the magical commands to the PCD8544
void LCDInit(void) {

//Reset the LCD to a known state
	PORTB &= ~(1<<PB1);
	_delay_us(5);
	PORTB |= (1<<PB1);
	_delay_ms(10);

	LCDWrite(LCD_COMMAND, 0x21); //Tell LCD that extended commands follow
	LCDWrite(LCD_COMMAND, 0xB6); //Set LCD Vop (Contrast): Try 0xB1(good @ 3.3V) or 0xBF if your display is too dark
	LCDWrite(LCD_COMMAND, 0x04); //Set Temp coefficent
	LCDWrite(LCD_COMMAND, 0x14); //LCD bias mode 1:48: Try 0x13 or 0x14

	LCDWrite(LCD_COMMAND, 0x20); //We must send 0x20 before modifying the display control mode
	LCDWrite(LCD_COMMAND, 0x0C); //Set display control, normal mode. 0x0D for inverse
}



//There are two memory banks in the LCD, data/RAM and commands. This 
//function sets the DC pin high or low depending, and then sends
//the data byte
void LCDWrite(uint8_t data_or_command, uint8_t data) {

	if (data_or_command) {
		PORTB |= (1<<PB2) ;					// D/C
	} else {
		PORTB &= ~(1<<PB2) ;
	}
	PORTB &= ~(1<<PB3) ;					// CE
	xmit_spi(data);
	PORTB |= (1<<PB3) ;
}

void drawLine(void)
{
  unsigned char  j;  
   for(j=0; j<84; j++) // top
	{
          gotoXY (j,0);
	  LCDWrite (LCD_DATA,0x01);
  } 	
  for(j=0; j<84; j++) //Bottom
	{
          gotoXY (j,5);
	  LCDWrite (LCD_DATA,0x80);
  } 	

  for(j=0; j<6; j++) // Right
	{
          gotoXY (83,j);
	  LCDWrite (LCD_DATA,0xff);
  } 	
	for(j=0; j<6; j++) // Left
	{
          gotoXY (0,j);
	  LCDWrite (LCD_DATA,0xff);
  }

}
/*
void draw_line(uint8_t x0, uint8_t y0, uint8_t x1, uint8_t y1) {
	uint8_t steep = abs(y1 - y0) > abs(x1 - x0);
	uint8_t dx, dy;
	int8_t err;
	int8_t ystep;
	
	if (steep) {
		swap(x0, y0);
		swap(x1, y1);
	}
	
	if (x0 > x1) {
		swap(x0, x1);
		swap(y0, y1);
	}
	
	dx = x1 - x0;
	dy = abs(y1 - y0);
	
	err = dx / 2;
	
	if (y0 < y1) {
		ystep = 1;
	} else {
		ystep = -1;
	}
	
	for (; x0<=x1; x0++) {
		if (steep) {
      gotoXY (y0,x0);
      LCDWrite (LCD_DATA,0xff);
		} else {
			gotoXY (x0, y0);
      LCDWrite (LCD_DATA,0xff);
		}
		err -= dy;
		if (err < 0) {
			y0 += ystep;
			err += dx;
		}
	}
}
*/

void draw_radius(uint8_t theta) {
int r=0;
for (; r<LCD_C_Y/5; r++) {
  gotoXY (LCD_C_X, LCD_C_Y+r);
  LCDWrite (LCD_DATA,0xff);
}

}


void port_direction_init(void) {

// Note: This is likely to be the ONLY place in the code where you use PORTx= or DDRx=
// everywhere else you would expect to see PORTx |= or PORTx &=  

	DDRB = 0xBF ;
	PORTB = 0x1F ;					// SS, CS, reset inactive	


// PORTC all inputs, no pullups
	DDRC = 0 ;
	PORTC = 0 ;	

// PORTD all inputs, no pullups
	DDRD = 0;
	PORTD = 0 ;														
}





unsigned int usart_getch()
{
  interrupt_count++;
  while ((UCSR0A & (1 << RXC0)) == 0); 
  // Do nothing until data has been received and is ready to be read from UDR
  return(UDR0); // return the byte
}

//transmit to usart
void USART_Transmit(char data)
{
  // Wait for empty transmit buffer 
  while ( !( UCSR0A & (1<<UDRE0)) );
  // Put data into buffer, sends the data 
  UDR0 = data;
}

//usart interrupt
ISR(USART0_RX_vect) {
  cbearing = UDR0;
  PORTB |= (1<<PB0);  
  _delay_ms(20);
  PORTB &= ~(1<<PB0);
  //interrupt_count++;
  //_delay_ms(20);
}

ISR(USART0_TX_vect) {
 interrupt_count++;
}

void USART0_init(void) {
	/*
  UBRR0 = 25;							
	UCSR0A	= 0;								
	UCSR0B	= 0x18;		
  UCSR0C |= 0x17;
  UCSR0A |= (1<<TXC0);		// clear any existing transmits

  UBRR0H = (unsigned char)(USART_BAUDRATE>>8);
  UBRR0L = (unsigned char)USART_BAUDRATE;
  */
  //pg 196
  //send the dividor
  UBRR0 = 51;	
  //set baud
  //UBRR0 = USART_BAUDRATE;
  // Set frame format: 8data, 2stop 
  UCSR0C = (1<<USBS0)|(3<<UCSZ00);
  // Enable receiver and transmitter
  UCSR0B = (1<<RXEN0)|(1<<TXEN0);
  
}

/*

glcd_buffer[x+ (y/8)*GLCD_LCD_WIDTH] &= ~ (1 << (y%8));

*/


int main(void) {
  interrupt_count = 0;
  loop_count = 0;
  // Initialise ports and SPI
	port_direction_init();
	init_spi(3);
	LCDInit();
  // Let everything settle
  PORTB &= ~(1<<PB0);
	_delay_ms(1000);

  // USART
  USART0_init();

  //enable Transmit Complete Interrupt
  UCSR0B |= (1<<TXCIE0);
  //enable Receive Complete Interrupt
  UCSR0B |= (1<<RXCIE0);
	LCDClear();
  char buf[12];
  int data;
  //enable global interrupts
	SREG |= 0x80;
	while (1) {
 
    USART_Transmit(0x12);
    
		sprintf(buf, "L:%2d I:%d", loop_count, cbearing);
    LCDClear();
    LCDString(buf);
    _delay_ms(200);
    LCDClear();
    //gotoXY(24, 42);
    //drawLine();
    //draw_line(0, 0, 50, 50);
    //
    draw_radius(0);
    
		_delay_ms(1000);
    loop_count++;
   
	}
}

