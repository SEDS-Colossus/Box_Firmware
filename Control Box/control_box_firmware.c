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

/*************************************************************************
 *                                   --PROTOTYPES--
 *************************************************************************/
void uart_init(); //uart initialization
void timer_init(); //Timer 2 in mode 2 for Baud rate for 9600
void uart_tx(unsigned char x); //transmit function to send data from 8051 to other device
unsigned char uart_rx(); //return function to receive data from other device
bit is_good(); //handshake check
bit receive(); //receive data from main_board
bit CRC_check(); //check if no data is lost
void button_check(); //check which buttons are activated
void leds_update(); //set led status to first half on input data
void verify_states(); //set pending led status based on button actions
void CRC_generator(); //generate CRC and append to end of data being sent
void send(); //send data to main_board
void print_errors(); //TODO: put codes into dictionary to print error messages
void delay(unsigned int x); //standard delay function

/*************************************************************************
 *                          --GLOBAL VARIABLES--
 *************************************************************************/
unsigned char input[IN_COUNT]; //stores all recieved inputs
unsigned char output[OUT_COUNT]; //stores all commands to be sent
long DIVISOR = 0x17; //x^4+x^2+x+1
long xor_value = 0x0;
long crc = 0x0;
int remainder = 0;
int i = 0;

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
	P4SW = 0x70; //enable IO for all of P4
	uart_init(); //must be called to initialize communication
	while(!is_good()); //wait for handshake
	
	while(1) { //loop forever
		/* GET NEW DATA FROM main_board */
		if(receive()) { //received good data from main_board
			leds_update(); //update all led status with new data
			print_errors(); //print error messages to oled
		} else {
			//TODO: ask main_board for re-send
			//TODO: PRINT TO OLED "bad connection"
		}
		/* OVERRIDE INACTIVE */
		if(!swtog) { //swtog is OFF, no override
			output[0] = 0; //set "OVERRIDE" element OFF
			send(); //notify main_board
		/* OVERRIDE ACTIVE */
		} else { //swtog is ON, override active
			button_check(); //read button actions, set send[] elements
			output[0] = 1; //set "OVERRIDE" element ON
			verify_states(); //set led status based on input data
			CRC_generator(); //append CRC remainder to output
			send(); //send data to main_board
		} //end if/else
	} //end while
} //end main


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

void button_check() { //change this to what I have in peters board
	/*
    if(p1_3 == 1 && p1_3tog == 0) { //SW1
			p1_3tog = 1;
			send[0] = '1';
		}
	  if else(p1_3 == 1 && p1_3tog == 1) {
			p1_3tog = 0;
			send[0] = '0';
		}
	*/

	
	while(1) {
		unsigned int count10 = 0;
		if(sw10 == 0) { //pressed initially
			while(sw10 == 0) ++count10; //hold
			if(count10 > 10) //held long enough
				sw10B = !sw10B; //switch
		}
		
		
	  if(p3_2 == 1) //SW2
		send[1] = 0x00;
	  if(p3_2 == 0)
		send[1] = 0x5A;
	   if(p3_3 == 1) //SW3
		send[2] = '1';
	   if (p3_3 == 0)
		send[2] = '0';
	   if(p3_4 == 1) //SW4
		send[3] = '1';
	   if (p3_4 == 0)
		send[3] = '0';
	   if(p3_5 == 1) //SW5
		send[4] = '1';
	   if (p3_5 == 0)
		send[4] = '0';
	   if(p3_6 == 1) //SW6
		send[5] = '1';
	   if(p3_6 == 0)
		send[5] = '0';
	   if(p3_7 == 1) //SW7
		send[6] = '1';
	   if(p3_7 == 0)
		send[6] = '0';
	   if(p2_6 == 1) //SW8
		send[7] = '1';
	   if (p2_6 == 0)
		send[7] = '0';
	   if(p2_5 == 1) //SW9
		send[8] = '1';
	   if (p2_5 == 0)
		send[8] = '0';
	   if(p2_4 == 1) //SW10
		send[9] = '1';
	   if(p2_4 == 0)
		send[9] = '0';
	   if(p2_3 == 1) //SW11
		send[10] = '1';
	   if(p2_3 == 0)
		send[10] = '0';
	   if(p2_2 == 1) //SW12
		send[11] = '1';
	   if (p2_2 == 0)
		send[11] = '0';
	   if(p2_1 == 1)  //override switch
		send[12] = '1';
	   if(p2_1 == 0)
		send[12] = '0';
	   if(p1_0 == 1) //potentiometer
		send[13] ='1';
	   if(p1_0 == 0)
		send[13] = '0';
	   if(p1_1 == 1) //potentiometer
		send[14] ='1';
	   if(p1_1 == 0)
		send[14] = '0';
}

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
void uart_init() {
	ES = 1; //Enable UART interrupt
	EA = 1; //Open master interrupt switch
  SCON = 0x50;  //Setting data as 8bit and receive enable
  timer_init(); //baud rate is 9600
}

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
void uart_tx(unsigned char x) {
  SBUF = x;   //Load data to serial buffer register associated to UART
  while(!TI); //transmit flag change when MSB is sent
  TI=0;       //clear the transmit flag
}

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
unsigned char uart_rx() {
  unsigned char z;
  while(!RI); //receive flag gets changed only when 8bits are received in SBUF
  z = SBUF;   //Moving data into z variable
  RI = 0;
  return(z);
}

/*----------------------------
UART interrupt service routine
----------------------------*/
void Uart_Isr() interrupt 4 using 1 {
	if(RI) { //RX flag
		RI = 0; //Clear receive interrupt flag
		//TODO: receive(); //read in expected data
	}
	if(TI) { //TX flag
		TI = 0;             //Clear transmit interrupt flag
		busy = 0;           //Clear transmit busy flag
	}
}
