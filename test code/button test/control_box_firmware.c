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
#define BUTTON_DEBOUNCE_CHECKS 500

/*************************************************************************
 *                            -- VARIABLES --
 *  Each variable is a pin on the MCU that can be read/write 1/0.
 *  Syntax:
 *       varaible-type  variable-name = pin;
 *************************************************************************/
sfr P4SW = 0xBB;

//sbit pot1 = P1^0;
//sbit battlvl = P1^2;
//sbit dc = P1^4;
//sbit din = P1^5;
//sbit cs= P1^6;
//sbit sclk = P1^7;

sbit sw9 = P2^5;
sbit sw10 = P2^4;

sbit swtog = P2^1;
sbit LEDtog = P2^0;

sbit LED9 = P4^6;
sbit LED10 = P4^5;

sbit LED11 = P4^4;

sbit LED12 = P2^7;


sbit LED1 = P0^0;
sbit LED2 = P0^1;
sbit LED3 = P0^2;
sbit LED4 = P0^3;
sbit LED5 = P0^4;
sbit LED6 = P0^5;
sbit LED7 = P0^6;
sbit LED8 = P0^7;

/*************************************************************************
 *                                   --PROTOTYPES--
 *************************************************************************/
//void uart_init(); //uart initialization
void timer_init(); //Timer 2 in mode 2 for Baud rate for 9600
//void uart_tx(unsigned char x); //transmit function to send data from 8051 to other device
//unsigned char uart_rx(); //return function to receive data from other device
//bit is_good(); //handshake check
//bit receive(); //receive data from main_board
//bit CRC_check(); //check if no data is lost
void button_check(); //check which buttons are activated
void leds_update(); //set led status to first half on input data
//void verify_states(); //set pending led status based on button actions
//void CRC_generator(); //generate CRC and append to end of data being sent
//void send(); //send data to main_board
//void print_errors(); //TODO: put codes into dictionary to print error messages
bit ButtonPress(); //debounce code
void delay(unsigned int x);

/*************************************************************************
 *                          --GLOBAL VARIABLES--
 *************************************************************************/
int OUT_COUNT = 19; //1 tog + 12 sw + 2 pot + 4 crc = 19 total send elements
int IN_COUNT = 30; //1 tog + 12 sw + 2 pot + 4 crc = 19 total send elements
unsigned char input[19]; //stores all recieved inputs
unsigned char output[30]; //stores all commands to be sent
long DIVISOR = 0x17; //x^4+x^2+x+1
long xor_value = 0x0;
long crc = 0x0;
int remainder = 0;
int i = 0;
bit sw9B = 0;
bit sw10B = 0;

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
	unsigned char LED10C = 0;
	P4SW = 0x70;
	while(1) {
		unsigned int count = 0;
		if(sw10 == 0) { //pressed initially
			while(sw10 == 0) ++count; //hold
			if(count > 10) //held long enough
				sw10B = !sw10B; //switch
			LED10 = !sw10B; //set led state
		}
		
		delay(50);
		if(sw10B && (LED10C++ > 4)) {
			LED10C = 0;
			LED10 = !LED10;
		}
	/*
	
	while(1) {
		if(sw10 == 0) { //initially pressed
			//TODO: timer starts
			if(sw10 == 1) { //unpressed
				LED10 = 1; //turn off led
			else if(sw10 == 0) { //still pressed
				sw10B = !sw10B;
			}
		}
		
		/*
		while(sw10 == 0) {
			count10++;
			if(count10 > 50) {
				sw10B = !sw10B;
			}
		} */
		
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

/*************************************************************************
 *                          --IS_GOOD--
 *  @Descption:
 *
 *  @PRECONDITION:
 *
 *  @POSTCONDITION: checks if an the connection is good
 *                                 between the two UARTS
 *
 *  @PARAMETER: none
 *
 *  @RETURN: int
 *************************************************************************/
/*
bit is_good() {
	uart_tx('g');
	if(uart_rx() == 'g')
		return 1;
	else
		return 0;
}

bit receive() {
     //the number of bits you add to the input is one less than the divisor
     for(i = 0; i < IN_COUNT; ++i) { //start input loop
          input[i] = uart_rx();
     } //end input loop
     if(CRC_check()) //convert input to hex to save space -- this is stored in the global hex array
          return 1;
     else
          return 0;
}

bit CRC_check(){
     xor_value = 0x0;
     crc = 0x0;
     remainder = 0;
     for(i = 0; i != 30; i++);
          crc = crc + (int)input[i]*pow(2,i);

     xor_value = crc ^ DIVISOR;

     remainder =  remainder + (xor_value % 2);
     xor_value = xor_value/2;
     remainder =  remainder + (xor_value % 2);
     xor_value = xor_value/2;
     remainder =  remainder + (xor_value % 2);
     xor_value = xor_value/2;
     remainder =  remainder + (xor_value % 2);

     if(remainder == 0)
          return 1;
     else
          return 0;
}
*/
void button_check() { //change this to what I have in peters board
	sw10B = ButtonPress();
}

bit ButtonPress()
{
    static unsigned int buttonState = 0;
    static char buttonPressEnabled = 1;

    if(!sw10)
    {
        if(buttonState < BUTTON_DEBOUNCE_CHECKS)
        {
            buttonState++;
        }
        else if(buttonPressEnabled)
        {
            buttonPressEnabled = 0;
            return 1;
        }
    }
    else if(buttonState > 0 )
    {
        buttonState--;
        // alternatively you can set buttonState to 0 here, but I prefer this way
    }
    else
    {
        buttonPressEnabled = 1;
    }

    return 0;
}

void leds_update() {
	if(sw10B == 1) {
		LED10 = 1;
	}
	else {
		LED10 = 0;
	}
}

/*
void verify_states() {
        //solinoids are off -- LEDS are inverted. 1 == off, 0 == on
        if(input[1] == '1' && input[14] == '0') //SW1
          p0_0 = 1;
        if(input[2] == '1' && input[15] == '0') //SW2
          p0_1 = 1;
        if(input[3] == '1' && input[16] == '0') //SW3
          p0_2 = 1;
        if(input[4] == '1' && input[17] == '0') //SW4
          p0_3 = 1;
        if(input[5] == '1' && input[18] == '0') //SW5
          p0_4 = 1;
        if(input[6] == '1' && input[19] == '0') //SW6
          p0_5 = 1;
        if(input[7] == '1' && input[20] == '0') //SW7
          p0_6 = 1;
        if(input[8] == '1' && input[21] == '0') //SW8
          p0_7 = 1;
        if(input[9] == '1' && input[22] == '0') //SW9
          p4_6 = 1;
        if(input[10] == '1' && input[23] == '0') //SW10
          p4_5 = 1;
        if(input[11] == '1' && input[24] == '0') //SW11
          p4_4 = 1;
        if(input[12] == '1' && input[25] == '0') //SW11
          p2_7 = 1;

        //if solinoids are on
        if((input[1] == '1' || input[1] == '0') && input[14] == '1')
          p0_1 = 0;
        if((input[2] == '1' || input[2] == '0') && input[15] == '1')
          p0_2 = 0;
        if((input[3] == '1' || input[3] == '0') && input[16] == '1')
          p0_3 = 0;
        if((input[4] == '1' || input[4] == '0') && input[17] == '1')
          p0_4 = 0;
        if((input[5] == '1' || input[5] == '0') && input[18] == '1')
          p0_5 = 0;
        if((input[6] == '1' || input[6] == '0') && input[19] == '1')
          p0_6 = 0;
        if((input[7] == '1' || input[7] == '0') && input[20] == '1')
          p0_7 = 0;
        if((input[8] == '1' || input[8] == '0') && input[21] == '1')
          p4_6 = 0;
        if((input[9] == '1' || input[9] == '0') && input[22] == '1')
          p4_5 = 0;
        if((input[10] == '1' || input[10] == '0') && input[23] == '1')
          p4_4 = 0;
        if((input[11] == '1' || input[11] == '0') && input[24] == '1')
          p2_7 = 0;
        if((input[12] == '1' || input[12] == '0') && input[25] == '1')
          p2_2 = 0;
}

void CRC_generator() {
     xor_value = 0x0;
     crc = 0x0;
     remainder = 0;
     i = 0;

     for(i = 0; i != 18; i++)
          crc = crc + (int)send[i]*pow(2,i);

     xor_value = crc ^ DIVISOR;

     for(i = 18; i != 15; i--) { // may need to be i != 14. Check through testing
          remainder = xor_value %2;
          xor_value = xor_value/2;
          if (remainder == 1)
               send[i] = '1';
          else
               send[i] = '0';
     }
}

void send() {
     i = 0;
     while (i != 18) {
          uart_tx(send[i]);
          i++;
     }
}

void print_errors() {
	//TODO: print erros to screen
}
*/
/*************************************************************************
 *                             -- UART INITIALIZATION --
 *  @Descption: First thing called from the main function. This will
 *              instantiate the uart
 *
 *  @PRECONDITION: main() is called
 *
 *  @POSTCONDITION: Data is set to 8 bit length and baud rate set to
 *                  9600 baud.
 *
 *  @PARAMETER: none
 *
 *  @RETURN: none
 *************************************************************************/
 /*
void uart_init() {
  SCON = 0x50;  //Setting data as 8bit and receive enable
  timer_init(); //baud rate is 9600
}
*/
/*************************************************************************
 *                       -- TIMER INITIALIZATION --
 *  @Descption: Timers will be set correct for the 8051 architecure. These
 *              timers set the reload mode for filling the buffer with
 *              messages to be sent.
 *
 *  @PRECONDITION: uart_init() is called
 *
 *  @POSTCONDITION: timers are set for correct reading bit lengths
 *
 *  @PARAMETER: none
 *
 *  @RETURN: none
 *************************************************************************/
void timer_init() {
  TMOD = 0x20; //timer 1 in mode 2 i.e. auto reload mode
  TH1 = 0xFD;  //reload value is FD for 9600 baud rate
               //(Found in table in READ_Me file)
  TR1 = 1;     //Timer 1 enable
}

/*************************************************************************
 *                       -- UART TRANSMITION --
 *  @Descption: Sends values to other device using uart transmition.
 *              Sends the single char value to the buffer and sends the
 *              char in 8 bits.
 *
 *  @PRECONDITION: called from main() from within main while loop
 *
 *  @POSTCONDITION: individual message will be sent as one char
 *
 *  @PARAMETER: unsigned char
 *
 *  @RETURN: none
 *************************************************************************/
/*
void uart_tx(unsigned char x) {
  SBUF = x;   //Load data to serial buffer register associated to UART
  while(!TI); //transmit flag change when MSB is sent
  TI=0;       //clear the transmit flag
}
*/
/*************************************************************************
 *                         -- UART RECEIVER --
 *  @Descption: Receives message from other device using the uart
 *              receiver pin. Message is read from the buffer and sent
 *              back to the user
 *
 *  @PRECONDITION: called from main() from within main while loop
 *
 *  @POSTCONDITION: individual 8 bit char read from the buffer and
 *                  sent back to the user
 *
 *  @PARAMETER: none
 *
 *  @RETURN: unsigned char
 *************************************************************************/
/*
unsigned char uart_rx() {
  unsigned char z;
  while(!RI); //receive flag gets changed only when 8bits are received in SBUF
  z = SBUF;   //Moving data into z variable
  RI = 0;
  return(z);
}
*/