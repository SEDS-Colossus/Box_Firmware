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
//#include <math.h> //used for pow function
#define OUT_COUNT 20 //1 startkey + 1 tog + 12 sw + 2 pot + 4 crc
#define IN_COUNT 20 //1 override + 12 solenoidstates + 3 errormessages + 4 crc
#define STARTKEY 255 //indicates beginning of an array message
#define HOLD_TIME 8 //time threshold for button press
#define PEND_NUM 100 //time threshold for pending led state swap

/*************************************************************************
 *                            -- VARIABLES --
 *  Each variable is a pin on the MCU that can be read/write 1/0.
 *  Syntax:
 *       varaible-type  variable-name = pin;
 *************************************************************************/
sfr P4SW = 0xBB; //special register for using P4 pins as I/O

sbit LED1 = P0^0;
sbit LED2 = P0^1;
sbit LED3 = P0^2;
sbit LED4 = P0^3;
sbit LED5 = P0^4;
sbit LED6 = P0^5;
sbit LED7 = P0^6;
sbit LED8 = P0^7;

sbit pot1 = P1^0; //TODO: potentiometer analog input
sbit pot2 = P1^1; //TODO: potentiometer analog input
sbit battlvl = P1^2; //TODO: battery analog input
sbit sw1 = P1^3;
sbit dc = P1^4; //OLED SPI
sbit din = P1^5; //OLED SPI
sbit cs= P1^6; //OLED SPI
sbit sclk = P1^7; //OLED SPI

sbit LED12 = P2^7;
sbit sw8 = P2^6;
sbit sw9 = P2^5;
sbit sw10 = P2^4;
sbit sw11 = P2^3;
sbit sw12 = P2^2;
sbit swTOG = P2^1; //main override toggle switch
sbit LEDtog = P2^0; //main override indicator

sbit sw2 = P3^2;
sbit sw3 = P3^3;
sbit sw4 = P3^4;
sbit sw5 = P3^5;
sbit sw6 = P3^6;
sbit sw7 = P3^7;

sbit LED9 = P4^6;
sbit LED10 = P4^5;
sbit LED11 = P4^4;

/*************************************************************************
 *                           --PROTOTYPES--
 *************************************************************************/
void uart_init(); //uart initialization
//bit handshake(); //handshake check

void sendByte(unsigned char dat); //uart TX for an unsigned char
void send(); //uart TX of output array

bit CRC_check(); //check if no data is lost
void CRC_generator(); //generate CRC and append to end of data being sent

void button_check(); //check which buttons are activated
void set_leds(); //set led status based on states received
//void print_errors(); //TODO: put codes into dictionary to print error messages

void delay(unsigned int x); //standard delay function
int pow(int base, int exponent); //power function

/*************************************************************************
 *                          --GLOBAL VARIABLES--
 *************************************************************************/
unsigned char input[IN_COUNT]; //stores all recieved inputs
/* [0,override][1-12,solenoidstates][13-15,errormessages][16-19,crc] */
//SOLENOIDSTATES CAN BE 0=INACTIVE, 1=ACTIVE, 2=PENDING
unsigned char output[OUT_COUNT]; //stores all commands to be sent
/* [0,STARTKEY][1-12,buttonpresses][13-14,potvals][15,override][16-19,crc] */

long DIVISOR = 0x17; //x^4+x^2+x+1
long xor_value = 0x0; //for CRC
long crc = 0x0; //for CRC
int remainder = 0; //for CRC

int i = 0; //for loops
bit busy; //boolean for UART TX holding
unsigned int index = 0; //for UART RX array location
bit storing = 0; //1 if currently updating RX array, 0 otherwise
bit inputChanged = 1; //1 if input data has been updated, 0 otherwise
//bit handshake = 0; //1 if handshake successful, 0 otherwise

unsigned char LEDtogC = 0;
unsigned char LED1C = 0;
unsigned char LED2C = 0;
unsigned char LED3C = 0;
unsigned char LED4C = 0;
unsigned char LED5C = 0;
unsigned char LED6C = 0;
unsigned char LED7C = 0;
unsigned char LED8C = 0;
unsigned char LED9C = 0;
unsigned char LED10C = 0;
unsigned char LED11C = 0;
unsigned char LED12C = 0;

/*************************************************************************
 *                          -- MAIN FUNCTION --
 *  @Descption: This function will instatiate the UART connection.
 *               Inside the while(1) is where the main code will be
 *               written.the unsigned char array "input" will be where
 *               commands will be stored when they are sent over and
 *               then evaluated. The first bit of input will indicate
 *               an error message or a state message is being received.
 *               0 is a state message, 1 is an error message. Input loop
 *               will end when '?' is sent. This will be the indicator as
 *               when all informaiton has been sent.
 *
 *  @PRECONDITION: none
 *
 *  @POSTCONDITION: -connenction between two MCU's will be complete
 *                  -commands will be received and delt with
 *                  -commands will be sent to other MCU
 *
 *  @PARAMETER: none
 *
 *  @RETURN: none
 *************************************************************************/
void main() {
	unsigned int counter = 0;
	P4SW = 0x70; //enable IO for all of P4
	uart_init(); //must be called to initialize communication
	//TODO; if no data has been received yet, tell OLED and wait for data
	while(1) { //loop forever
		if(swTOG == 0) { //override toggle is ON
			output[15] = 1; //set override element
			button_check(); //read button presses
			//TODO: pot_check() read pot values
			CRC_generator(); //append crc to end of output
		} else { //override toggle is OFF
			output[15] = 0; //clear override element
			//don't check buttons, don't check pots
			CRC_generator(); //append crc to end of output
		}
		if(counter++ == 100) {
			counter = 0;
			send(); //transmit output
		}
		if(inputChanged /*&& CRC_check()*/) { //new valid data was recieved
			set_leds(); //update leds based on current data
			//TODO: update OLED info
		}
	} //end while
} //end main




int pow(int base, int exponent)
{
	int result;
	while(exponent != 0) {
		result *= base;
		--exponent;
	}
	return result;
}

bit CRC_check(){
	xor_value = 0x0;
	crc = 0x0;
	remainder = 0;
	
	for(i = 0; i != IN_COUNT; ++i);
			crc = crc + (int)input[i]*pow(2,i);

	xor_value = crc ^ DIVISOR;

	remainder =  remainder + (xor_value % 2);
	xor_value = xor_value/2;
	remainder =  remainder + (xor_value % 2);
	xor_value = xor_value/2;
	remainder =  remainder + (xor_value % 2);
	xor_value = xor_value/2;
	remainder =  remainder + (xor_value % 2);

	if(remainder == 0) return 1;
	else return 0;
}

void CRC_generator() {
	xor_value = 0x0;
	crc = 0x0;
	remainder = 0;

	for(i = 0; i < OUT_COUNT - 4; ++i)
		crc = crc + (int)output[i]*pow(2,i);

	xor_value = crc ^ DIVISOR;

	for(i = OUT_COUNT - 1; i != OUT_COUNT - 5; --i) {
		remainder = xor_value % 2;
		xor_value = xor_value/2;
		if (remainder == 1) output[i] = 1;
		else output[i] = 0;
	}
}



void button_check() {
	unsigned int count = 0;

	if(sw1 == 0) { //pressed initially
		while(sw1 == 0) ++count; //hold
		if(count > HOLD_TIME) //held long enough
			output[1] = !output[1]; //switch state
		count = 0; //reset count
	}
	if(sw2 == 0) { //pressed initially
		while(sw2 == 0) ++count; //hold
		if(count > HOLD_TIME) //held long enough
			output[2] = !output[2]; //switch state
		count = 0; //reset count
	}
	if(sw3 == 0) { //pressed initially
		while(sw3 == 0) ++count; //hold
		if(count > HOLD_TIME) //held long enough
			output[3] = !output[3]; //switch state
		count = 0; //reset count
	}
	if(sw4 == 0) { //pressed initially
		while(sw4 == 0) ++count; //hold
		if(count > HOLD_TIME) //held long enough
			output[4] = !output[4]; //switch state
		count = 0; //reset count
	}
	if(sw5 == 0) { //pressed initially
		while(sw5 == 0) ++count; //hold
		if(count > HOLD_TIME) //held long enough
			output[5] = !output[5]; //switch state
		count = 0; //reset count
	}
	if(sw6 == 0) { //pressed initially
		while(sw6 == 0) ++count; //hold
		if(count > HOLD_TIME) //held long enough
			output[6] = !output[6]; //switch state
		count = 0; //reset count
	}
	if(sw7 == 0) { //pressed initially
		while(sw7 == 0) ++count; //hold
		if(count > HOLD_TIME) //held long enough
			output[7] = !output[7]; //switch state
		count = 0; //reset count
	}
	if(sw8 == 0) { //pressed initially
		while(sw8 == 0) ++count; //hold
		if(count > HOLD_TIME) //held long enough
			output[8] = !output[8]; //switch state
		count = 0; //reset count
	}
	if(sw9 == 0) { //pressed initially
		while(sw9 == 0) ++count; //hold
		if(count > HOLD_TIME) //held long enough
			output[9] = !output[9]; //switch state
		count = 0; //reset count
	}
	if(sw10 == 0) { //pressed initially
		while(sw10 == 0) ++count; //hold
		if(count > HOLD_TIME) //held long enough
			output[10] = !output[10]; //switch state
		count = 0; //reset count
	}
	if(sw11 == 0) { //pressed initially
		while(sw11 == 0) ++count; //hold
		if(count > HOLD_TIME) //held long enough
			output[11] = !output[11]; //switch state
		count = 0; //reset count
	}
	if(sw12 == 0) { //pressed initially
		while(sw12 == 0) ++count; //hold
		if(count > HOLD_TIME) //held long enough
			output[12] = !output[12]; //switch state
		count = 0; //reset count
	}
}

void set_leds() {
	//LEDS are inverted. 1 == off, 0 == on
		
	if((input[0] == 2) && (LEDtogC++ > PEND_NUM)) { //in pending status
		LEDtogC = 0; //reset counter
		LEDtog = !LEDtog; //invert led state
	} else if(input[0] == 1) LEDtog = 0; //turn led ON
	else if(input[0] == 0) LEDtog = 1; //turn led OFF
	
	if((input[1] == 2) && (LED1C++ > PEND_NUM)) { //in pending status
		LED1C = 0; //reset counter
		LED1 = !LED1; //invert led state
	} else if(input[1] == 1) LED1 = 0; //turn led ON
	else if(input[1] == 0) LED1 = 1; //turn led OFF
	
	if((input[2] == 2) && (LED2C++ > PEND_NUM)) { //in pending status
		LED2C = 0; //reset counter
		LED2 = !LED2; //invert led state
	} else if(input[2] == 1) LED2 = 0; //turn led ON
	else if(input[2] == 0) LED2 = 1; //turn led OFF

	if((input[3] == 2) && (LED3C++ > PEND_NUM)) { //in pending status
		LED3C = 0; //reset counter
		LED3 = !LED3; //invert led state
	} else if(input[3] == 1) LED3 = 0; //turn led ON
	else if(input[3] == 0) LED3 = 1; //turn led OFF
	
	if((input[4] == 2) && (LED4C++ > PEND_NUM)) { //in pending status
		LED4C = 0; //reset counter
		LED4 = !LED4; //invert led state
	} else if(input[4] == 1) LED4 = 0; //turn led ON
	else if(input[4] == 0) LED4 = 1; //turn led OFF

	if((input[5] == 2) && (LED5C++ > PEND_NUM)) { //in pending status
		LED5C = 0; //reset counter
		LED5 = !LED5; //invert led state
	} else if(input[5] == 1) LED5 = 0; //turn led ON
	else if(input[5] == 0) LED5 = 1; //turn led OFF
	
	if((input[6] == 2) && (LED6C++ > PEND_NUM)) { //in pending status
		LED6C = 0; //reset counter
		LED6 = !LED6; //invert led state
	} else if(input[6] == 1) LED6 = 0; //turn led ON
	else if(input[6] == 0) LED6 = 1; //turn led OFF
	
	if((input[7] == 2) && (LED7C++ > PEND_NUM)) { //in pending status
		LED7C = 0; //reset counter
		LED7 = !LED7; //invert led state
	} else if(input[7] == 1) LED7 = 0; //turn led ON
	else if(input[7] == 0) LED7 = 1; //turn led OFF
	
	if((input[8] == 2) && (LED8C++ > PEND_NUM)) { //in pending status
		LED8C = 0; //reset counter
		LED8 = !LED8; //invert led state
	} else if(input[8] == 1) LED8 = 0; //turn led ON
	else if(input[8] == 0) LED8 = 1; //turn led OFF
	
	if((input[9] == 2) && (LED9C++ > PEND_NUM)) { //in pending status
		LED9C = 0; //reset counter
		LED9 = !LED9; //invert led state
	} else if(input[9] == 1) LED9 = 0; //turn led ON
	else if(input[9] == 0) LED9 = 1; //turn led OFF
	
	if((input[10] == 2) && (LED10C++ > PEND_NUM)) { //in pending status
		LED10C = 0; //reset counter
		LED10 = !LED10; //invert led state
	} else if(input[10] == 1) LED10 = 0; //turn led ON
	else if(input[10] == 0) LED10 = 1; //turn led OFF
	
	if((input[11] == 2) && (LED11C++ > PEND_NUM)) { //in pending status
		LED11C = 0; //reset counter
		LED11 = !LED11; //invert led state
	} else if(input[11] == 1) LED11 = 0; //turn led ON
	else if(input[11] == 0) LED11 = 1; //turn led OFF
	
	if((input[12] == 2) && (LED12C++ > PEND_NUM)) { //in pending status
		LED12C = 0; //reset counter
		LED12 = !LED12; //invert led state
	} else if(input[12] == 1) LED12 = 0; //turn led ON
	else if(input[12] == 0) LED12 = 1; //turn led OFF
}

/*************************************************************************
 *                       -- UART INITIALIZATION --
 *  @Descption: First thing called from the main function. This will
 *              instantiate the uart, timer for baud rate, and uart 
 * 							interrupt
 *
 *  @PRECONDITION: main() is called
 *
 *  @POSTCONDITION: Data buffer is set to 8 bit length, baud rate set
 *                  to 9600 baud, uart interrupt is enabled
 *
 *  @PARAMETER: none
 *
 *  @RETURN: none
 *************************************************************************/
void uart_init() {
	SCON = 0x50; //8-bit variable UART
  TMOD = 0x20; //timer 1 in mode 2 i.e. auto reload mode
  TH1 = 0xFD; //reload value is FD for 9600 baud rate
  TR1 = 1; //Timer 1 enable
  ES = 1; //Enable UART interrupt
  EA = 1; //Open master interrupt switch
	
	for(i = 0; i < OUT_COUNT; ++i) //initialize output array to zeros
		output[i] = 0;
	for(i = 0; i < IN_COUNT; ++i) //initialize input array to zeros
		input[i] = 0;
}

/*************************************************************************
 *                  -- UART interrupt service routine --
 *  @Descption: 
 *
 *  @PRECONDITION: 
 *
 *  @POSTCONDITION: 
 *
 *  @PARAMETER: 
 *
 *  @RETURN: none
 *************************************************************************/
void Uart_Isr() interrupt 4 using 1
{
	unsigned char c;
	if(RI) { //receive flagged
		RI = 0; //reset receive flag
	  c = SBUF; //store buffer in c
		if(c == STARTKEY) { //start key is received
			storing = 1; //set that we are now storing
			index = 0; //start from beginning of array
		} else if(storing) { //we are in storing mode
			input[index] = c; //store SBUF in current index of array
			++index; //increment array index
		}
		if(index >= IN_COUNT) { //read in enough values
			storing = 0; //set that we are done storing
			index = 0; //reset index
			inputChanged = 1; //tell the program that new data has been delivered
		}
	}
		
	if(TI) { //transmit flagged
		TI = 0; //Clear transmit interrupt flag
		busy = 0; //Clear transmit busy flag
	}
}

/*************************************************************************
 *                  -- UART Transmission of a Byte --
 *  @Descption: send a byte of data (unsigned char) to UART
 *
 *  @PRECONDITION: 
 *
 *  @POSTCONDITION: 
 *
 *  @PARAMETER: dat (data to be sent)
 *
 *  @RETURN: none
 *************************************************************************/
void sendByte(unsigned char dat)
{
	while(busy); //Wait for the completion of the previous data is sent
	busy = 1; //set transmit busy flag
	SBUF = dat; //Send data to UART buffer
}

/*************************************************************************
 *                  -- UART Transmission of an Array --
 *  @Descption: send the output array to UART
 *
 *  @PRECONDITION: 
 *
 *  @POSTCONDITION: 
 *
 *  @PARAMETER: none
 *
 *  @RETURN: none
 *************************************************************************/
void send()
{
	for(i = 0; i < OUT_COUNT; ++i) {
		sendByte(output[i]);
	}
}