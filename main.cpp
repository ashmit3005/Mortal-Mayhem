// main.cpp
// Runs on LM4F120/TM4C123
// Jonathan Valvano and Daniel Valvano
// This is a starter project for the ECE319K Lab 10 in C++

// Last Modified: 1/2/2023 
// http://www.spaceinvaders.de/
// sounds at http://www.classicgaming.cc/classics/spaceinvaders/sounds.php
// http://www.classicgaming.cc/classics/spaceinvaders/playguide.php

// ******* Possible Hardware I/O connections*******************
// Slide pot pin 1 connected to ground	
// Slide pot pin 2 connected to PD2/AIN5
// Slide pot pin 3 connected to +3.3V 
// fire button connected to PE0
// special weapon fire button connected to PE1
// 32*R resistor DAC bit 0 on PB0 (least significant bit)
// 16*R resistor DAC bit 1 on PB1 
// 8*R resistor DAC bit 2 on PB2
// 4*R resistor DAC bit 1 on PB3
// 2*R resistor DAC bit 2 on PB4
// 1*R resistor DAC bit 3 on PB5 (most significant bit)
// LED on PB6
// LED on PB7

// Backlight (pin 10) connected to +3.3 V
// MISO (pin 9) unconnected
// SCK (pin 8) connected to PA2 (SSI0Clk)
// MOSI (pin 7) connected to PA5 (SSI0Tx)
// TFT_CS (pin 6) connected to PA3 (SSI0Fss)
// CARD_CS (pin 5) unconnected
// Data/Command (pin 4) connected to PA6 (GPIO), high for data, low for command
// RESET (pin 3) connected to PA7 (GPIO)
// VCC (pin 2) connected to +3.3 V
// Gnd (pin 1) connected to ground

#include <stdint.h>
#include "../inc/tm4c123gh6pm.h"
#include "PLL.h"
#include "ST7735.h"
#include "random.h"
#include "PLL.h"
#include "SlidePot.h"
#include "Images.h"
#include "UART.h"
#include "Timer0.h"
#include "Timer1.h"
#include "Timer2.h"
#include "Sound.h"
//#include "Buttons.h"
extern "C" void DisableInterrupts(void);
extern "C" void EnableInterrupts (void);



SlidePot my(2000,0);

extern "C" void DisableInterrupts(void);
extern "C" void EnableInterrupts(void);
extern "C" void SysTick_Handler(void);


volatile uint32_t flag;
volatile uint32_t langSelected = 0;
volatile uint32_t FallingEdges = 0;
volatile int punchFlag1 = 0;
volatile int punchFlag2 = 0;
volatile uint32_t pressflag = 0;

uint32_t data;
uint32_t ADCdata[2];
SlidePot Sensor1(128,0);
SlidePot Sensor2(128,0);

struct Sprite1 {
	int16_t x;
	int16_t y;
	int16_t health;
}; typedef struct Sprite1 sprite1_t;

struct Sprite2 {
	int16_t x;
	int16_t y;
	int16_t health;
}; typedef struct Sprite2 sprite2_t;

sprite1_t Sprite1 = { 5, 137, 10};
sprite2_t Sprite2 = { 75, 137,  10};

void Clock_Delay(volatile uint32_t ulCount){
  while(ulCount){
    ulCount--;
  }
}

void Delay100ms(uint32_t count){
	uint32_t volatile time;
	while(count>0){
		time = 727240;	// 0.1sec at 80 MHz
		while(time){
			time--;
		}
		count--;
	}
}

typedef enum {English, Spanish} language_t;

const char Fatality_Eng [] = "FATALITY";
const char Fatality_Span [] = "FATALIDAD";
const char Player1_Eng [] = "PLAYER1";
const char Player1_Span [] = "JUGADOR1";
const char Player2_Eng [] = "PLAYER2";
const char Player2_Span [] = "JUGADOR2";
const char gameover_Eng [] = "GAME OVER";
const char gameover_Span [] = "JUEGO TERMINADO";

const char* endmessages[2][2] = {
	{Fatality_Eng, gameover_Eng},
	{Fatality_Span, gameover_Span}
};

// Initialize PE0 1 2 3 for ENGLISH, SPANISH, ATTACKPLAYER1, ATTACKPLAYER2 
void PortE_Init(void){
	volatile int delayPE;
	SYSCTL_RCGCGPIO_R |= 0x10; // start clock for Port E
	delayPE = SYSCTL_RCGCGPIO_R; // 4 clock cycle delay for stabilization
	delayPE = SYSCTL_RCGCGPIO_R; 
	delayPE = SYSCTL_RCGCGPIO_R; 
	delayPE = SYSCTL_RCGCGPIO_R; 
	GPIO_PORTE_DIR_R &= ~0x0F;
	//GPIO_PORTE_DIR_R |= 0x0F;
	//GPIO_PORTE_DEN_R &= 0x0F;
	GPIO_PORTE_DEN_R |= 0x0F;
	GPIO_PORTE_AFSEL_R &= 0x0F;
	GPIO_PORTE_AMSEL_R &= 0x0F;
	GPIO_PORTE_IS_R &= ~0x0C;     // (d) PF4 is edge-sensitive
  GPIO_PORTE_IBE_R &= ~0x0C;    //     PF4 is not both edges
  GPIO_PORTE_IEV_R &= ~0x0C;    //     PF4 falling edge event
  GPIO_PORTE_ICR_R = 0x0C;      // (e) clear flag4
  GPIO_PORTE_IM_R |= 0x0C;      // (f) arm interrupt on PE4 *** No IME bit as mentioned in Book ***
	GPIO_PORTE_PUR_R |= 0x0C;     //     enable weak pull-up on PF4
	NVIC_PRI1_R = (NVIC_PRI7_R&0xFFFFFF0F)|0x000000A0; // (g) priority 5
  NVIC_EN0_R = 0x00000010;      // (h) enable interrupt 30 in NVIC
}


void updatePos1 (void){
	ADC_In89(ADCdata);
	Sensor1.Save(ADCdata[0]);
}

void updatePos2 (void){
	ADC_In89(ADCdata);
	Sensor2.Save(ADCdata[1]);
}


void updatePos3 (void){ 
	// collision detection
	if ((GPIO_PORTE_DATA_R & 0x04) ==0) {}
}

void handler1 (void) {
	updatePos1();
	//updatePos2();
	// collision detection
	if (((GPIO_PORTE_DATA_R & 0x04) == 4) && (punchFlag1 = 0)){
		ST7735_DrawBitmap(Sprite1.x, Sprite1.y, fighter11, 55, 70);
		ST7735_DrawBitmap(Sprite1.x, Sprite1.y, fighter12, 55, 70);
		ST7735_DrawBitmap(Sprite1.x, Sprite1.y, fighter13, 55, 70);
		ST7735_DrawBitmap(Sprite1.x, Sprite1.y, fighter14, 55, 70);
		ST7735_DrawBitmap(Sprite1.x, Sprite1.y, fighter1, 55, 70);		
	}
	punchFlag1 = GPIO_PORTE_DATA_R;
	
	if (((GPIO_PORTE_DATA_R & 0x08) == 8) && (punchFlag2 = 0)){
		ST7735_DrawBitmap(Sprite2.x, Sprite2.y, fighter21, 55, 70);
		ST7735_DrawBitmap(Sprite2.x, Sprite2.y, fighter22, 55, 70);
		ST7735_DrawBitmap(Sprite2.x, Sprite2.y, fighter23, 55, 70);
		ST7735_DrawBitmap(Sprite2.x, Sprite2.y, fighter24, 55, 70);
		ST7735_DrawBitmap(Sprite2.x, Sprite2.y, fighter2, 55, 70);
	}
	punchFlag2 = GPIO_PORTE_DATA_R;
}

/*
void GPIOPortE_Handler(void){
  GPIO_PORTF_ICR_R = 0x0C;      // acknowledge flag4
  FallingEdges = FallingEdges + 1;
	
	if ((GPIO_PORTE_DATA_R & 0x04)==4){ // on releaee
		punchFlag1 = 0;
		ST7735_DrawBitmap(Sprite1.x, Sprite1.y, fighter1, 55, 70);
	}
	else { // on touch
		punchFlag1 = 1;
		ST7735_DrawBitmap(Sprite1.x, Sprite1.y, fighter11, 55, 70);
		ST7735_DrawBitmap(Sprite1.x, Sprite1.y, fighter12, 55, 70);
		ST7735_DrawBitmap(Sprite1.x, Sprite1.y, fighter13, 55, 70);
		ST7735_DrawBitmap(Sprite1.x, Sprite1.y, fighter14, 55, 70);
	}

	if ((GPIO_PORTE_DATA_R & 0x08)==8){ // on release
		punchFlag2 = 0;
		ST7735_DrawBitmap(Sprite2.x, Sprite2.y, fighter2, 55, 70);
	}
	else { // on touch
		punchFlag2 = 1;
		ST7735_DrawBitmap(Sprite2.x, Sprite2.y, fighter21, 55, 70);
		ST7735_DrawBitmap(Sprite2.x, Sprite2.y, fighter22, 55, 70);
		ST7735_DrawBitmap(Sprite2.x, Sprite2.y, fighter23, 55, 70);
		ST7735_DrawBitmap(Sprite2.x, Sprite2.y, fighter24, 55, 70);
	}	
}
*/


void EndScreen(void){
	ST7735_FillScreen(ST7735_BLACK);
	if(langSelected == 0){
		ST7735_SetCursor(6,6);
		ST7735_OutString((char*)"FATALITY");
		Delay100ms(3);
		ST7735_SetCursor(4,8);
		ST7735_OutString((char*)"FATALITY");
		Delay100ms(3);
		ST7735_SetCursor(8,4);
		ST7735_OutString((char*)"FATALITY");
		Delay100ms(10);
		
		ST7735_FillScreen(ST7735_BLACK);
		ST7735_SetCursor(5,5);
		ST7735_OutString((char*)"GAME OVER");
		ST7735_SetCursor(5,6);
		ST7735_OutString((char*)"PLAYER1 WINS");
		
		//ST7735_OutString((char*)"PLAYER2 WINS");
	}
	
	if(langSelected == 1){
		ST7735_SetCursor(6,6);
		ST7735_OutString((char*)"FATALIDAD");
		Delay100ms(3);
		ST7735_SetCursor(5,8);
		ST7735_OutString((char*)"FATALIDAD");
		Delay100ms(3);
		ST7735_SetCursor(7,4);
		ST7735_OutString((char*)"FATALIDAD");
		Delay100ms(10);
		
		ST7735_FillScreen(ST7735_BLACK);
		ST7735_SetCursor(3,5);
		ST7735_OutString((char*)"JUEGO TERMINADO");
		ST7735_SetCursor(3,6);
		ST7735_OutString((char*)"JUGADOR1 GANA");
		//ST7735_OutString((char*)"JUGADOR2 GANA");
	}
}


long StartCritical (void);    // previous I bit, disable interrupts
void EndCritical(long sr);    // restore I bit to previous value
extern "C" void WaitForInterrupt(void);  // low power mode


void drawSprite1 (void) {
	Sprite1.x = Sensor1.Convert(ADCdata[0]);
	ST7735_DrawBitmap(Sprite1.x, Sprite1.y, fighter1, 55, 70);
	Sensor1.Sync();
}

void drawSprite2 (void) {
	Sprite2.x = Sensor1.Convert(ADCdata[1]);
	ST7735_DrawBitmap(Sprite2.x, Sprite2.y, fighter1, 55, 70);
	Sensor2.Sync();
}


void MainMenu (void) {
	ST7735_FillScreen(ST7735_BLACK);
		ST7735_DrawBitmap(15, 137, mklogo, 100, 100);
		ST7735_SetCursor(1, 1);
		ST7735_OutString ((char*) "MORTAL KOMBAT");
		ST7735_SetCursor(1, 2);
		ST7735_OutString ((char*) "loading ...");
		ST7735_SetTextColor(ST7735_RED);
		Clock_Delay(9000000);
		ST7735_FillScreen(ST7735_BLACK);
		ST7735_SetCursor(2, 2);
		ST7735_OutString ((char*) "SELECT A LANGUAGE");
		ST7735_SetCursor(4, 7);
		ST7735_OutString ((char*) "ENGLISH (PE0)");
		ST7735_SetCursor(4, 10);
		ST7735_OutString ((char*) "SPANISH (PE1)");
		while ((GPIO_PORTE_DATA_R & 0x03) == 0) {}
		if ((GPIO_PORTE_DATA_R & 0x01) == 0x01){
			langSelected = 0;
		}
		if ((GPIO_PORTE_DATA_R & 0x02) == 0x02){
			langSelected = 1;
		}
}

void InitFightScreen (void) {
		ST7735_FillScreen(ST7735_BLACK);
		
		if (langSelected == 0) { //English
			ST7735_SetCursor(8,6); 
			ST7735_OutString((char*) "FIGHT!");
			Delay100ms(5);
			ST7735_FillScreen(ST7735_BLACK);
			ST7735_SetCursor(1,1); 
			ST7735_OutString((char*) Player1_Eng);
			ST7735_SetCursor(12,1);
			ST7735_OutString((char*) Player2_Eng);
			ST7735_DrawBitmap(5, 137, fighter1, 55, 70);
			ST7735_DrawBitmap(75, 137, fighter2, 55, 70);
			ST7735_DrawBitmap(5, 23, healthbar1, 45, 6);
			ST7735_DrawBitmap(78, 23, healthbar2, 45, 6);
		}
		else { // Spanish
			ST7735_SetCursor(6,6); 
			ST7735_OutString((char*) "COMBATIR!");
			Delay100ms(5);
			ST7735_FillScreen(ST7735_BLACK);
			ST7735_SetCursor(1,1); 
			ST7735_OutString((char*) Player1_Span);
			ST7735_SetCursor(12,1);
			ST7735_OutString((char*) Player2_Span);
			ST7735_DrawBitmap(5, 137, fighter1, 55, 70);
			ST7735_DrawBitmap(75, 137, fighter2, 55, 70);
			ST7735_DrawBitmap(5, 23, healthbar1, 45, 6);
			ST7735_DrawBitmap(78, 23, healthbar2, 45, 6);
		}
}

  volatile uint32_t delay;                         
	int main(void){
		SYSCTL_RCGCGPIO_R |= 0x00000010;
		delay = SYSCTL_RCGCGPIO_R; 
		delay = SYSCTL_RCGCGPIO_R; 
		delay = SYSCTL_RCGCGPIO_R; 
		while((SYSCTL_PRGPIO_R & 0x00000010) == 0){};
		DisableInterrupts();
		PortE_Init();
		Timer0_Init(&handler1, 8000000);
		//Timer1_Init(&updatePos2, 8000000);
		//Timer2_Init(&updatePos3, 8000000);
		PLL_Init(Bus80MHz);
		ADC_Init89();
		Output_Init();
		Sound_Init();
		//EdgeCounter_Init();
		ST7735_InitR(INITR_REDTAB);
		
		// initializations 
		MainMenu();
		InitFightScreen();
		EndScreen();
		EnableInterrupts();
			
		while (1){
			//WaitForInterrupt();
			drawSprite1();
			drawSprite2();
			/*
			Sensor1.Save(ADCdata[0]); // wait for semaphore
			Sensor2.Save(ADCdata[1]);
			// can call Sensor.ADCsample, Sensor.Distance, Sensor.Convert as needed 
		 Sprite1.x = Sensor1.Convert(ADCdata[0]);
		 Sprite2.x = Sensor2.Convert(ADCdata[1]);
		 ST7735_DrawBitmap(Sprite1.x, Sprite1.y, fighter1, 55, 70);
		 ST7735_DrawBitmap(Sprite2.x, Sprite2.y, fighter2, 55, 70);
		 Sensor1.Sync();
			Sensor2.Sync();	
			*/
		}
}





/*
void EdgeCounter_Init(void){                          
  SYSCTL_RCGCGPIO_R |= 0x00000020; // (a) activate clock for port F
  FallingEdges = 0;             // (b) initialize counter
	GPIO_PORTF_DIR_R &= ~0x10;    // (c) make PF4 in (built-in button)
  GPIO_PORTF_AFSEL_R &= ~0x10;  //     disable alt funct on PF4
  GPIO_PORTF_DEN_R |= 0x10;     //     enable digital I/O on PF4   
  GPIO_PORTF_PCTL_R &= ~0x000F0000; // configure PF4 as GPIO
  GPIO_PORTF_AMSEL_R = 0;       //     disable analog functionality on PF
  GPIO_PORTF_PUR_R |= 0x10;     //     enable weak pull-up on PF4
  GPIO_PORTF_IS_R &= ~0x10;     // (d) PF4 is edge-sensitive
  GPIO_PORTF_IBE_R &= ~0x10;    //     PF4 is not both edges
  GPIO_PORTF_IEV_R &= ~0x10;    //     PF4 falling edge event
  GPIO_PORTF_ICR_R = 0x10;      // (e) clear flag4
  GPIO_PORTF_IM_R |= 0x10;      // (f) arm interrupt on PF4 *** No IME bit as mentioned in Book ***
  NVIC_PRI7_R = (NVIC_PRI7_R&0xFF00FFFF)|0x00A00000; // (g) priority 5
  NVIC_EN0_R = 0x40000000;      // (h) enable interrupt 30 in NVIC
  EnableInterrupts();           // (i) Clears the I bit
}
void GPIOPortF_Handler(void){
  GPIO_PORTF_ICR_R = 0x10;      // acknowledge flag4
  FallingEdges = FallingEdges + 1;
}
*/


