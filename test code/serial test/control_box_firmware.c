//matts board
/*************************************************************************
 *  @File Descption: Read states of manual override buttons of control_box
 *	and bi-directional communication via UART with main_board.
 *	@File: control_box_firmware.c
 *  @author(s):
 *  -Tawfic Rabbani, Embedded Systems Engineer, head of softwar
 *	-Matthew Santos, Electrical and Software Engineer
 *  @Property of SEDS UCSD
 *  @since: 10/2016
 *************************************************************************/
#include "reg60s2.h" //Include reg file for 8051 architecure
#include <math.h> //used for pow function
#define OUT_COUNT 19 //1 tog + 12 sw + 2 pot + 4 crc = 19 total send elements
#define IN_COUNT 30 //TODO: number of elements to receive

/*************************************************************************
 *                            -- VARIABLES --
 *  Each variable is a pin on the MCU that can be read/write 1/0.
 *  Syntax:
 *       varaible-type  variable-name = pin;
 *************************************************************************/
sfr P4SW = 0xBB;

sbit LED1 = P0^0;
sbit LED2 = P0^1;
sbit LED3 = P0^2;
sbit LED4 = P0^3;
sbit LED5 = P0^4;
sbit LED6 = P0^5;
sbit LED7 = P0^6;
sbit LED8 = P0^7;

sbit pot1 = P1^0;
sbit pot2 = P1^1;
sbit battlvl = P1^2;
sbit sw1 = P1^3;
sbit dc = P1^4;
sbit din = P1^5;
sbit cs= P1^6;
sbit sclk = P1^7;

sbit LED12 = P2^7;
sbit sw8 = P2^6;
sbit sw9 = P2^5;
sbit sw10 = P2^4;
sbit sw11 = P2^3;
sbit sw12 = P2^2;
sbit swtog = P2^1;
sbit LEDtog = P2^0;

sbit sw2 = P3^2;
sbit sw3 = P3^3;
sbit sw4 = P3^4;
sbit sw5 = P3^5;
sbit sw6 = P3^6;
sbit sw7 = P3^7;

sbit LED9 = P4^6;
sbit LED10 = P4^5;
sbit LED11 = P4^4;
/*------------------------------------------------------------------*/
/* --- STC MCU Limited ---------------------------------------------*/
/* --- STC12C5Axx Series MCU UART (8-bit/9-bit)Demo ----------------*/
/* --- Mobile: (86)13922805190 -------------------------------------*/
/* --- Fax: 86-0513-55012956,55012947,55012969 ---------------------*/
/* --- Tel: 86-0513-55012928,55012929,55012966----------------------*/
/* --- Web: www.STCMCU.com -----------------------------------------*/
/* --- Web: www.GXWMCU.com -----------------------------------------*/
/* If you want to use the program or the program referenced in the  */
/* article, please specify in which data and procedures from STC    */
/*------------------------------------------------------------------*/

typedef unsigned char BYTE;
typedef unsigned int WORD;

//#define FOSC 11059200L      //System frequency
//#define BAUD 9600           //UART baudrate
#define STARTKEY 's'
#define ENDKEY 'e'

bit busy;

void SendData(BYTE dat);
void SendString(char *s);
void delay(unsigned int x);

unsigned char input[10]; //stores all recieved inputs
int i = 0;
int index = 0;
bit storing = 0;
bit dataChanged = 1;

void main()
{
	for(i = 0; i < 10; ++i) { //initial array is numbers 1-10
		input[i] = i;
	}
	SCON = 0x50;            		//8-bit variable UART
  TMOD = 0x20; 								//timer 1 in mode 2 i.e. auto reload mode
  TH1 = 0xFD;  								//reload value is FD for 9600 baud rate
  TR1 = 1;    					  		//Timer 1 enable
  ES = 1;                 		//Enable UART interrupt
  EA = 1;                 		//Open master interrupt switch
	SendString("STC12C5A60S2\r\nUart Test !\r\n");
  while(1){
		delay(500);
		if(dataChanged) { //data has been changed
			SendString(input); //send the data
			dataChanged = 0; //resest dataB;
		}
	}
}

/*----------------------------
UART interrupt service routine
----------------------------*/
void Uart_Isr() interrupt 4 using 1
{
	unsigned char c;
	if(RI) { //receive flagged
		RI = 0; //reset receive flag
	  c = SBUF; //store buffer in c
		if(c == STARTKEY) { //start key is received
			storing = 1; //set that we are now storing
		} else if(c == ENDKEY) { //end key is received
			storing = 0; //set that we are done storing
			index = 0; //reset index
			dataChanged = 1;
		} else if(storing) { //we are in storing mode
			input[index] = c; //store SBUF in current index of array
			++index; //increment array index
		}
	}
		
    if(TI) { //transmit flagged
			TI = 0; //Clear transmit interrupt flag
			busy = 0; //Clear transmit busy flag
    }
}

/*----------------------------
Send a byte data to UART
Input: dat (data to be sent)
Output:None
----------------------------*/
void SendData(BYTE dat)
{
    while (busy);           	//Wait for the completion of the previous data is sent
    busy = 1;
    SBUF = dat;		            //Send data to UART buffer
}

/*----------------------------
Send a string to UART
Input: s (address of string)
Output:None
----------------------------*/
void SendString(char *s)
{
    while (*s)              //Check the end of the string
    {
        SendData(*s++);     //Send current char and increment string ptr
    }
}

void delay(unsigned int x) {
	unsigned int a;
	unsigned int b;
	for(a=x;a>0;a--) {
		for(b=1000;b>0;b--) {
			continue;
		}
	}
}