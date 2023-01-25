#define  F_CPU 1000000UL
#include <avr/io.h>
#include <avr/interrupt.h>
#include <util/delay.h>
#include <avr/eeprom.h>


#define ON  1
#define OFF 0
#define None 11  
#define active_direction_load_spi		  1
#define sc signed char
#define uc (unsigned char)


enum Spi_digits {
	BLINK_FIRST_SPI, 
	BLINK_SECOND_SPI,
	RESERV_SPI,
	SIGNAL_SPI,
	CONVEER_SPI,
	BLINK_BUTTON_SPI
	};



_Bool whole_sek			     = 0;
volatile  _Bool  timer_run   = OFF;
volatile   int count = 0;
volatile   int vcount = 0;
#define DIGITS_MAX             6
sc digits_numbers [DIGITS_MAX] = {0,0,0,0,0,0};  
sc  sek		 = 0;
sc  min		 = 0;
sc  hour	 = 0;
sc setup	 = 0;						 // COUNT BUTTON SETUP
_Bool  voltage_f     = 1;						 // voltage on the circuit
_Bool signal_allowed = 0;
volatile char byte = 0;

#define RESPONSE      600						 // delay response button
enum Button_press {
	UNPRESS,
	PRESS_STOP,             
	PRESS_START,
	PRESS_SETTING
};
enum Menu_item {
	READY,
	READ_SETUP,
	EDITING_SEC,
	EDITING_MIN,
	EDITING_HOUR,
	WRITE_SETUP
};

sc active_button = UNPRESS;                    // state button press


#define	 buton_stop		(!(PINC&(1<<2)))
#define  buton_start    (!(PINC&(1<<3)))
#define  buton_set      (!(PINC&(1<<4)))
#define  voltage_state  (!(PINC&(1<<5)))


int	timing				= 0;
_Bool conveer           = OFF;  
_Bool signale			= OFF;
_Bool blink				= OFF;


void get_digits_numbers (void);
void read_m (void);
void direction(sc button);
sc  get_button(void);
void port_ini (void);
void timer_init (void);
void SPI (void);
void segchar (sc seg);



	int main (void){
		port_ini ();
		timer_init ();
		read_m ();
		sei();

																		
	while (1)
	{	
		direction(get_button());											
		get_digits_numbers();
		SPI();
		
	}																		
}




	
	

void segchar (sc seg) {
	switch(seg)
	{
		case 1: byte = 0b000000110; break;
		case 2: byte = 0b01011011; break;
		case 3: byte = 0b01001111; break;
		case 4: byte = 0b01100110; break;
		case 5: byte = 0b01101101; break;
		case 6: byte = 0b01111101; break;
		case 7: byte = 0b00000111; break;
		case 8: byte = 0b01111111; break;
		case 9: byte = 0b01101111; break;
		case 0: byte = 0b00111111; break;
		default: byte = 0; break;
	}
}





void SPI (void) {
	
	cli ();
	for (int digit = 0; digit<DIGITS_MAX; digit++) {
		if (voltage_f) {
			segchar(digits_numbers[digit]);
			if (timing == 0 &&((digit == 0 && (min || hour)) || (digit == 1 && hour))){   // point blink
					byte|=(1<<7);
			}
			else if (conveer == ON && digit == CONVEER_SPI){
					byte|=(1<<7);	
			}
			else if (signale == ON && digit == SIGNAL_SPI){
					byte|=(1<<7);
			}
			else if (blink == ON && digit == BLINK_BUTTON_SPI){
						byte|=(1<<7);
			} 
		}
		else {
			 if (digit == 3) byte = 0X3F;          // WRITE "OFF"
			 else if (digit == 2) byte =  0X71;
			 else if (digit == 1) byte = 0X71;	 
		}
			for (int c=0; c<8; c++)					  //SPI function
			{
				if (byte&0x80)
				{
					PORTD|=(1<<6);
				}
				else
				{
					PORTD&=~(1<<6);
				}
				byte = (byte<<1);
				PORTB|=(1<<0);
				PORTB&=~(1<<0);
			}
	}
	
	PORTD|=(1<<7); 
	PORTD&=~(1<<7);
	sei();
}
	


void get_digits_numbers(void){

		if (setup == EDITING_SEC && whole_sek){
			digits_numbers[0]=None;               // == ''
			digits_numbers[1]=None;
		} 
		else {
			digits_numbers[0]= sek%10 ;
			digits_numbers[1]= sek/10 ;
		}
		if (setup == EDITING_MIN && whole_sek){
			digits_numbers[2]=None; 
			digits_numbers[3]=None;
		}
		else {
			digits_numbers[2]= min%10 ;
			digits_numbers[3]= min/10 ;	
		}
		if (setup == EDITING_HOUR && whole_sek){
			digits_numbers[4]=None; 
			digits_numbers[5]=None;
		}	
		else{
			digits_numbers[4]= hour%10 ;
			digits_numbers[5]= hour/10 ;
		}
	
	if (timer_run){
		for (int digit = 5; digit; digit--){   // delete zero
			if (digits_numbers[digit] == 0){
				digits_numbers[digit] = None;     
			}
			else {
				break;
			}
		}
	}
}




void EEPROM_WRITE (unsigned int uiAddress, sc ucData)
{
	while (EECR&(1<<EEWE));
	EEAR = uiAddress;
	EEDR = ucData;
	EECR |= (1<<EEMWE);
	EECR |= (1<<EEWE);
}


signed char EEPROM_read(unsigned int uiAddress)
{
	while(EECR & (1<<EEWE));
	EEAR = uiAddress;
	EECR |= (1<<EERE);
	return EEDR;
}

void read_m (void){
	sek = EEPROM_read(0x01);
	min = EEPROM_read(0x02);
	hour = EEPROM_read(0x03);
	if (min || hour) signal_allowed = 1;
	else signal_allowed = 0;
}





void timer_init (void){
	TCCR2 = 0x00; //tick 1/2 sek
	TCCR2 |=(1<<CS22);
	ASSR|=(1<<AS2);
	TIMSK |=(1<<TOIE2);
}



void port_ini (void){
	//////////////////// emulation MOSI
	DDRD|=(1<<6); //DS
	PORTD&=~(1<<6); //
	DDRB|=(1<<0); //clk
	PORTB&=~(1<<0); //
	DDRD|=(1<<7); // ST vuvid na ekran
	PORTD&=~(1<<7); //
	DDRB|=(1<<1); //MR registriv skudanie z -
	PORTB|=(1<<1); // +
	DDRD|=(1<<5); //OE dlia dozvolu robotu z -
	PORTD&=~(1<<5); // -

	//////////////// init buttons
	DDRC&=~(1<<2); //button SET
	DDRC&=~(1<<3); //button  start
	DDRC&=~(1<<4); //button pause
	DDRC&=~(1<<5); //voltage
	
	PORTC|=(1<<2); //button SET -
	PORTC|=(1<<3); //button  start
	PORTC|=(1<<4); //button  start
	PORTC|=(1<<5); //voltage 
	
	for (int x=0; x<50; x++)  // clean screen and output - load
	{
		PORTB|=(1<<0);
		PORTB&=~(1<<0);
	}
	PORTD |= (1<<7); 
	PORTD &= ~(1<<7);

}



ISR (TIMER2_OVF_vect){
	if (timer_run){
		if (voltage_f) timing++;
		if (min==0 && hour==0 && sek==10 && signal_allowed) signale = ON;
		if (signale == ON && min==0 && hour==0 && sek<6) signale = OFF;
		if (min == 0 && hour == 0 && sek == 0){
			 if (timing<3)conveer = ON;
			 else if (timing < 44){
				if (conveer == ON) conveer = OFF;
			}
			 else {
				read_m();
				timing = 0;
			}
		}
		else {
			if (timing>1) {
				timing = 0;
				sek--;
				if (sek<0) {
					min--; sek=59;
						if (min<0) {
							hour--; min=59;
						}
				}
			}
		}
	}
	else {
		if (timing) timing = 0;
		if (signale) signale = OFF;
		if (blink) blink = OFF;
		if (conveer) conveer = OFF;
		if (whole_sek) whole_sek = 0;
		else whole_sek = 1;
	}
}
		
			
			
	
	
			
			
sc get_button () {
	sc result = 0;
	if ( voltage_f != voltage_state ){
		if (vcount<RESPONSE*2) vcount++;
		else {
			voltage_f = voltage_state; 
			vcount = 0;
		}
	}
	else if (vcount > 0) vcount--;

	if (active_button == PRESS_SETTING || active_button == UNPRESS){
		if (buton_set) {
			if (active_button == UNPRESS) active_button = PRESS_SETTING;
			if (count<RESPONSE) count++;
		}
		else  if (count > 0) count--;
	}
	
	 if (active_button == PRESS_START || active_button == UNPRESS){
		if (buton_start) {
			if (active_button == UNPRESS) active_button = PRESS_START;
			if (count<RESPONSE) count++;
		}
		else if (count>0) count--;
	}
	
	 if (active_button == PRESS_STOP || active_button == UNPRESS){					
		if (buton_stop) {
			if (active_button == UNPRESS) active_button = PRESS_STOP;
			if (count<RESPONSE) count++;
		}
		else if (count > 0) count--;
	}

	if (count >= RESPONSE){
		count = 0;
		result = active_button;
	}
	if(count <= 0) active_button = UNPRESS;
	return result;
			
}
									
void 	direction(sc but) {
	if (timer_run) {						
		if (but == PRESS_STOP)timer_run = OFF;
	}
	else {
		if  (but == PRESS_STOP) {
			if(setup == EDITING_SEC) {
				sek--;
				if (sek<0) sek=59;
			}											
			else if(setup == EDITING_MIN){
				min--;
				if (min<0) min=59;
			}										
			else if(setup == EDITING_HOUR){
				hour--;
				if (hour<0)hour=24;
			}
		}													
		else if (but == PRESS_START){
			if (setup == READY){
				timer_run = ON;
				blink = ON;
			}														
			else if (setup == EDITING_SEC){
			sek++;
				if (sek>59)sek = 0;
			}													
			else if (setup==EDITING_MIN) {
			min++;
				if (min>59)min = 0;
			}														
			else if(setup==EDITING_HOUR) {
				hour++;
				if (hour>24)hour = 0;
			}
	}															
		if (but == PRESS_SETTING){
			setup++;
			if (setup == READ_SETUP){
				read_m();
				setup = EDITING_SEC;
			}
			else if(setup == WRITE_SETUP){
				cli();
				if (min > 1 || hour > 0) signal_allowed = 1;
				else signal_allowed = 0;
				EEPROM_WRITE(0x01, sek);
				EEPROM_WRITE(0x02, min);
				EEPROM_WRITE(0x03, hour);
				setup = READY;
				sei();
			}
		}
	}																		
}
