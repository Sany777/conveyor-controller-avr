#include "main.h"

volatile _Bool
		conveer = OFF,
		signale = OFF,
		blink = FALSE,
		timer_run = OFF,
		signal_allowed = FALSE,
		voltage_f = TRUE;
		
uint8_t sec   = 0,
		min   = 0,
		hour  = 0,
		setup = READY;

int main (void)
{
		port_ini ();
		timer_init (HALF_SEC_4M);
		read_m ();
		uint8_t numbers[DIGITS_MAX]={None,None,None,None,None,None};
		sei();

																		
	while (1)
	{
		send_to_SPI(numbers);
		execute(getKey());											
		set_digits_numbers(numbers);
	}																		
}



uint8_t getCharSegment(uint8_t n)
{
	switch(n)
	{
		case 1:  return  0b00000110; break;
		case 2:  return  0b01011011;  break;
		case 3:  return  0b01001111;  break;
		case 4:  return  0b01100110;  break;
		case 5:  return  0b01101101;  break;
		case 6:  return  0b01111101;  break;
		case 7:  return  0b00000111;  break;
		case 8:  return  0b01111111;  break;
		case 9:  return  0b01101111;  break;
		case 0:  return  0b00111111;  break;
		default: return  0;           break;
	}
}

void send_to_SPI (uint8_t *numbers) 
{
	cli ();
	for (uint8_t digit = 0,byte = 0; digit<DIGITS_MAX; digit++) 
	{
		if (voltage_f) 
		{
			byte = getCharSegment(numbers[digit]);
			// ---------------------------------- control load
			switch(digit)
			{
				case BLINK_FIRST_POINTS  : if(timer_run == OFF || blink && (min || hour)) active_Load; break;	
				case BLINK_SECOND_POINTS : if(timer_run == OFF || blink && hour) active_Load; break;
				case CONVEER			 : if(conveer == ON) active_Load; break;
				case SIGNAL				 : if(signale == ON) active_Load; break;
				default                  : break;
			}
		}
		else 
		{
		// ---------------------------------- show "OFF"
			 if (digit == 3)      
					byte = 0X3F;          
			 else if (digit == 2) 
					byte = 0X71;
			 else if (digit == 1) 
					byte = 0X71;
			 else  
					byte = 0;
		}
		//---------------------------------- send to SPI
		for (uint8_t i=0; i<SIZE_BYTE; i++)
		{
			if (byte&0x80) 
				send_1;
			else 
				send_0;
			byte = (byte<<1);
			send_CLK;
		}
	}
	end_Transmision_Spi;
	sei();
}
	


void set_digits_numbers(uint8_t *numbers)
{
	numbers[0]= setup == EDITING_SEC  && blink ? None : sec%10;
	numbers[1]= setup == EDITING_SEC  && blink ? None : sec/10;
	numbers[2]= setup == EDITING_MIN  && blink ? None : min%10;
	numbers[3]= setup == EDITING_MIN  && blink ? None : min/10;
	numbers[4]= setup == EDITING_HOUR && blink ? None : hour%10;
	numbers[5]= setup == EDITING_HOUR && blink ? None : hour/10;
	
	if (timer_run)
	{
		for (int8_t digit=5; digit && numbers[digit]; digit--)
		{
			numbers[digit] = None;     
		}
	}
}


void EEPROM_WRITE (uint16_t uiAddress, uint8_t ucData)
{
	while (EECR&(1<<EEWE));
	EEAR = uiAddress;
	EEDR = ucData;
	EECR |= (1<<EEMWE);
	EECR |= (1<<EEWE);
}


uint8_t EEPROM_read(uint16_t uiAddress)
{
	while(EECR & (1<<EEWE));
	EEAR = uiAddress;
	EECR |= (1<<EERE);
	return EEDR;
}

void read_m (void)
{
	sec  = EEPROM_read(ADDR_SEC);
	min  = EEPROM_read(ADDR_MIN);
	hour = EEPROM_read(ADDR_HOUR);
	if(sec > MAX_MIN_SEC)sec = 0;
	if(min > MAX_MIN_SEC)min = 25;
	if(hour > MAX_MIN_SEC)hour = 0;
	if (min || hour) signal_allowed = TRUE;
	else signal_allowed = FALSE;
}


void timer_init (uint16_t delay)
{
	TCCR1B |= (1<<WGM12)      // CTC mode
	| (1<<CS12) | (1<<CS10); // /1024
	OCR1AH = delay>>SIZE_BYTE;
	OCR1AL = delay<<SIZE_BYTE;
	TIMSK = (1<<TOIE1)       // Timer 1 enable
	| (1<<OCIE1A);           // interupt compare with OCR1A
}

						


void port_ini (void)
{
		
	//---------------------- program SPI : 0-6 bit - show number, 7bit - control load
	DDRD|=(1<<6);     //DS
	PORTD&=~(1<<6);   // set 0
	DDRB|=(1<<0);     //clk
	PORTB&=~(1<<0);   // 
	DDRD|=(1<<7);     // ST 
	PORTD&=~(1<<7);   // 
	DDRB|=(1<<1);     //MR 
	PORTB|=(1<<1);    // +
	DDRD|=(1<<5);     //OE
	PORTD&=~(1<<5);   // OE enable
	
	//----------------------------- port input

	DDRC&=~(1<<2); //button SET
	DDRC&=~(1<<3); //button  start
	DDRC&=~(1<<4); //button pause
	DDRC&=~(1<<5); //voltage
		
	//--------------------------- pin pull up
	PORTC|=(1<<2);
	PORTC|=(1<<3);
	PORTC|=(1<<4);
	PORTC|=(1<<5);
		
	//-------------------------- clear registers
	for (uint8_t i=0; i<SIZE_BYTE*DIGITS_MAX; i++) 
	{
		send_CLK;
	}
	end_Transmision_Spi;

}



ISR (TIMER1_COMPA_vect)
{
	static uint8_t timing=0;
	if (voltage_f)
	{
		blink = !blink;
		if (timer_run)
		{
			if (min==0 && hour==0 && sec == SIGNAL_TO_LOAD_ON && signal_allowed && signale == OFF) signale = ON;
			else if (min==0 && hour==0 && sec<6) signale = OFF;
			if (min == 0 && hour == 0 && sec == 0)
			{
				if (timing == 0)
				{
					conveer = ON;
				}
				else if(timing == 2)
				{
					conveer = OFF;
				}
				else if (timing > 44)
				{
					read_m();
					timing = 0;
					blink = TRUE;
				}
				timing++;
			}
			else if(blink) 
			{
				sec--;
				if (sec<0)
				{
					min--;
					sec=59;
					if (min<0)
					{
						hour--;
						min=59;
					}
				}
			}
		}
	}
	else if(conveer == ON)
	{
		conveer = OFF;
	}
}
		
			
	
			
uint8_t get_button (void) 
{
	static uint8_t active_button = UNPRESS;
	static uint8_t count_volt=0, count=0;
	if (voltage_f != voltage_state)
	{
		count_volt++;
		if (count_volt>RESPONSE)
		{
			voltage_f = voltage_state; 
			count_volt = 0;
		}
	}
	else if (count_volt > 0)
	{
		count_volt--;
	} 
	
	if(count == 0)active_button = UNPRESS;
	if(active_button == UNPRESS)
	{
		if(buton_set)active_button=PRESS_SETTING;
		else if(buton_start)active_button=PRESS_START;
		else if(buton_stop)active_button=PRESS_STOP;
	}

	if((buton_set && active_button==PRESS_SETTING) || (buton_start && active_button==PRESS_START) || (buton_stop && active_button==PRESS_STOP))
	{
		if(count > RESPONSE)
		{
			count = 0;
			return active_button;
		}
		count++;
	}
	else if(count)
	{
		count--;
	}
	return UNPRESS;	
}
			
		
void execute(const uint8_t but) 
{
	if (timer_run) 
	{
		if (but == PRESS_STOP)
		{
			timer_run = OFF;
			if (signale) signale = OFF;
			if (conveer) conveer = OFF;
		}
	}
	else 
	{
		if (but == PRESS_STOP) 
		{
			if(setup == EDITING_SEC) 
			{
				sec--;
				if (sec>MAX_MIN_SEC) sec=MAX_MIN_SEC;
			}											
			else if(setup == EDITING_MIN)
			{
				min--;
				if (min>MAX_MIN_SEC) min=MAX_MIN_SEC;
			}										
			else if(setup == EDITING_HOUR)
			{
				hour--;
				if (hour>MAX_HOUR)hour=MAX_HOUR;
			}
		}													
		else if (but == PRESS_START)
		{
			if (setup == READY)
			{
				timer_run = ON;
			}														
			else if (setup == EDITING_SEC)
			{
				sec++;
				if (sec>MAX_MIN_SEC)sec = 0;
			}													
			else if (setup==EDITING_MIN)
			{
				min++;
				if (min>MAX_MIN_SEC)min = 0;
			}														
			else if(setup==EDITING_HOUR) 
			{
				hour++;
				if (hour>MAX_HOUR)hour = 0;
			}
		}															
		else if (but == PRESS_SETTING)
		{
			setup++;
			if (setup == READ_SETUP)
			{
				read_m();
				setup = EDITING_SEC;
			}
			else if(setup >= WRITE_SETUP)
			{
				cli();
				if (min || hour) signal_allowed = TRUE;
				else signal_allowed = FALSE;
				if(hour == 0 && min == 0 && sec < ALLOW_MINIMUM_DELAY_TIMER)sec = ALLOW_MINIMUM_DELAY_TIMER;
				EEPROM_WRITE(ADDR_SEC, sec);
				EEPROM_WRITE(ADDR_MIN, min);
				EEPROM_WRITE(ADDR_HOUR, hour);
				setup = READY;
				sei();
			}
		}
	}																		
}


uint8_t getKey(void)
{
	
	for(uint8_t i = 0, key=UNPRESS; i<DELAY_BUTTON; i++)
	{
		key = get_button();
		if(key != UNPRESS)return key;
		for(uint8_t ii=0; ii<RESPONSE; ii++);
	}
	return UNPRESS;
}