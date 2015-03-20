/*******************************************************************************
*                                                                              *
*                          Digital Voice Recorder                              *
*                                                                              *
*                          File     : Digital Voice Recorder.c                 *
*                          Compiler : IAR EWAAVR 4.12A                         *
*                          Device   : ATMega16                                 *
*                                                                              *
*******************************************************************************/

#include <inavr.h>
#include <ioavr.h>
#include <iom16.h>

//******************************************************************************

// Boolean
#define TRUE                           0xFF
#define FALSE                          0x00

// SPI
#define SPIIF                          0x80

// ADC
#define ADCIF                          0x10
#define ADCSC                          0x40

// Timer1 Overflow
#define T1_OF                          0x04

// Buttons
#define BTN_ERS                        0x01  // PD.0
#define BTN_REC                        0x02  // PD.1
#define BTN_PLY                        0x04  // PD.2
#define BTN_LPB                        0x80  // PD.7

// LEDs
#define LED_PWR_ON                     0xF7  // PB.3
#define LED_ERS_ON                     0xF7  // PD.3
#define LED_REC_ON                     0xDF  // PD.5
#define LED_PLY_ON                     0xBF  // PD.6
#define LED_ALL_ON                     0x97  // PD.3, PD.5, PD.6

// DataFlash
#define DF_READY                       0x80
#define DF_CHIP_SELECT                 0x10
#define BUFFER_1_WRITE                 0x84
#define BUFFER_2_WRITE                 0x87
#define BUFFER_1_READ                  0xD1
#define BUFFER_2_READ                  0xD3
#define READ_STATUS_REGISTER           0xD7
#define READ_Manufacturer_ID           0x9F
#define READ_SEC_PROTECT_REG           0x32
#define READ_SEC_LOCKDWN_REG           0x35
#define MM_PAGE_TO_B1_XFER             0x53
#define MM_PAGE_TO_B2_XFER             0x55
#define B1_TO_MM_PAGE_PROG_WITH_ERASE  0x83
#define B2_TO_MM_PAGE_PROG_WITH_ERASE  0x86
#define CHIP_ERASE_1                   0xC7
#define CHIP_ERASE_2                   0x94
#define CHIP_ERASE_3                   0x80
#define CHIP_ERASE_4                   0x9A
#define BUFFER_SIZE                    528
#define PAGES_COUNT                    4096

//******************************************************************************

volatile unsigned char DF_empty;

//******************************************************************************

void SPI_write(unsigned char data)
{
    SPDR = data;
    while ((SPSR & SPIIF) == 0);             // wait for data transfer to be completed
}

//******************************************************************************

void wait_DF_ready(void)
{ 
    PORTB &= ~DF_CHIP_SELECT;                // enable DataFlash
      
    SPI_write(READ_STATUS_REGISTER);         // Write command read status register

    do
    {
        SPI_write(0xFF);                     // write dummy value to start register shift
    }
    while ((SPDR & DF_READY) == 0);          // wait for DataFlash to be ready
    
    PORTB |= DF_CHIP_SELECT;                 // disable DataFlash
}

//******************************************************************************

void DF_error(void)
{
    PORTD &= LED_ALL_ON;                     // turn all LEDs on
    while (1);	  	   	             // Prevent further use till reset.
}

//******************************************************************************

void check_DataFlash(void)
{
    // Interrupt disabled, SPI port enabled, Master mode, MSB first,  SPI mode 3, f = 500 KHz
    SPCR = 0x5D;                             // enable SPI
    SPSR = 0x00;			     
    
    wait_DF_ready();                         // wait for DataFlash to be ready
    
    PORTB &= ~DF_CHIP_SELECT;                // enable DataFlash
    
    SPI_write(READ_STATUS_REGISTER);	     // Write command read status register

    SPI_write(0xFF);			     // write dummy value to start register shift
    if ((SPDR & 0xBF) != 0xAC)               // an error has occured...?
        DF_error();
    
    PORTB |= DF_CHIP_SELECT;		     // disable DataFlash
    
    SPCR = 0x00;  	 		     // disable SPI
}

//******************************************************************************

void check_Manufacturer_ID(void)
{
    // Interrupt disabled, SPI port enabled, Master mode, MSB first,  SPI mode 3, f = 500 KHz
    SPCR = 0x5D;			     // enable SPI
    SPSR = 0x00;
    
    wait_DF_ready();
    
    PORTB &= ~DF_CHIP_SELECT;                // enable DataFlash
    
    SPI_write(READ_Manufacturer_ID);         // Write command to manufacturer & device ID registers

    SPI_write(0xFF);			     // write dummy value to start register shift
    if (SPDR != 0x1F)                        // an error has occured...?
        DF_error();
    
    SPI_write(0xFF);			     // write dummy value to start register shift	    
    if (SPDR != 0x26)                        // an error has occured...?
        DF_error();
    
    SPI_write(0xFF);			     // write dummy value to start register shift	    
    if (SPDR != 0x00)                        // an error has occured...?
        DF_error();  
    
    SPI_write(0xFF);			     // write dummy value to start register shift	    
    if (SPDR != 0x00)                        // an error has occured...?
        DF_error();
    
    PORTB |= DF_CHIP_SELECT;		     // disable DataFlash
    
    SPCR = 0x00;  	 		     // disable SPI
}

//******************************************************************************

void check_Sector_Protect_Reg(void)
{
    // Interrupt disabled, SPI port enabled, Master mode, MSB first,  SPI mode 3, f = 500 KHz
    SPCR = 0x5D;			     // enable SPI
    SPSR = 0x00;
    
    wait_DF_ready();
    
    PORTB &= ~DF_CHIP_SELECT;                // enable DataFlash
    
    SPI_write(READ_SEC_PROTECT_REG);         // Write command to sector protection registers
    SPI_write(0xFF);			     // write dummy value
    SPI_write(0xFF);			     // write dummy value
    SPI_write(0xFF);			     // write dummy value
    
    //Sectors 0a, 0b & Sectors 1 to 15
    for (int i = 0; i < 16; i++)
    {
        SPI_write(0xFF);		     // write dummy value to start register shift	    
        if (SPDR != 0x00)                    // an error has occured...?
            DF_error();
    }
    
    PORTB |= DF_CHIP_SELECT;		     // disable DataFlash
    
    SPCR = 0x00;  	 		     // disable SPI
}

//******************************************************************************

void check_Sector_Lockdown_Reg(void)
{
    // Interrupt disabled, SPI port enabled, Master mode, MSB first,  SPI mode 3, f = 500 KHz
    SPCR = 0x5D;			     // enable SPI
    SPSR = 0x00;
    
    wait_DF_ready();
    
    PORTB &= ~DF_CHIP_SELECT;                // enable DataFlash
    
    SPI_write(READ_SEC_LOCKDWN_REG);         // Write command to sector lockdown registers
    SPI_write(0xFF);			     // write dummy value
    SPI_write(0xFF);			     // write dummy value
    SPI_write(0xFF);			     // write dummy value
     
    //Sectors 0a, 0b & Sectors 1 to 15
    for (int i = 0; i < 16; i++)
    {
        SPI_write(0xFF);		     // write dummy value to start register shift	    
        if (SPDR != 0x00)                    // an error has occured...?
            DF_error();
    }
    
    PORTB |= DF_CHIP_SELECT;		     // disable DataFlash
    
    SPCR = 0x00;  	 		     // disable SPI
}

//******************************************************************************

void initialize(void)
{
    DDRA  = 0x00;                            // ADC Port initialisation
    PORTA = 0x00;

    DDRB  = 0xBD;                            /* SPI Port initialisation
                                                76543210

                                                1....... = SCK   O

                                                .0...... = MISO  I

                                                ..1..... = MOSI  O

                                                ...1.... = CS    O

                                                ....x... = LED   O

                                                .....1.. = WP    O

                                                ......0. = X     I

                                                .......1 = RST   O

                                                --------------------

                                                10111101 = 0xBD						   

                                             */
    PORTB = 0xFF;                            // all outputs high, inputs have pullups, (LED is off) 
    
    DDRD  = 0x78;                            /* 76543210

                                                0....... = BTN_LPB  I

                                                .1...... = LED_PLY  O

                                                ..1..... = LED_REC  O

                                                ...1.... = OC1B     O

                                                ....1... = LED_ERS  O

                                                .....0.. = BTN_PLY  I

                                                ......0. = BTN_REC  I

                                                .......0 = BTN_ERS  I

                                                --------------------

                                                01111000 = 0x78					   

                                             */
    PORTD = 0xEF;
   
    __enable_interrupt();                    // enable interrupts
    
    DF_empty = TRUE;                         // DataFlash is empty
        
    PORTB &= LED_PWR_ON;                     // turn LED on
  
    check_DataFlash();
    check_Manufacturer_ID();
    check_Sector_Protect_Reg();
    check_Sector_Lockdown_Reg();
}

//******************************************************************************

void erase(void)
{                  
    PORTD &= LED_ERS_ON;                     // turn Yellow LED on  
        
    // Interrupt disabled, SPI port enabled, Master mode, MSB first,  SPI mode 3, f = 2 MHz
    SPCR = 0x5C;			     // enable SPI
    SPSR = 0x00;
    
    wait_DF_ready();
    
    PORTB &= ~DF_CHIP_SELECT;                // enable DataFlash
    
    SPI_write(CHIP_ERASE_1);                 // Write chip erase command 1
    SPI_write(CHIP_ERASE_2);                 // Write chip erase command 2
    SPI_write(CHIP_ERASE_3);                 // Write chip erase command 3
    SPI_write(CHIP_ERASE_4);                 // Write chip erase command 4
    
    PORTB |= DF_CHIP_SELECT;                 // disable DataFlash
    
    wait_DF_ready();
    
    SPCR = 0x00;                             // disable SPI   
    
    PORTD |= ~LED_ERS_ON;                    // turn Yellow LED off
     
    DF_empty = TRUE;                         // DataFlash is empty
    
    while ((PIND & BTN_ERS) == 0);           // wait until erase button is released   
}

//******************************************************************************

void record(void)
{   
    static unsigned int page_counter;
    unsigned char ADC_value;
    unsigned char count;
          
    if (DF_empty == TRUE)                    // is DataFlash empty ?
        page_counter = 0;                    // reset the page counter if new data has to be written
  
    PORTD &= LED_REC_ON;                     // turn Red LED on

    // Interrupt disabled, SPI port enabled, Master mode, MSB first,  SPI mode 3, f = 2 MHz
    SPCR = 0x5C;			     // enable SPI
    SPSR = 0x00;
    
    wait_DF_ready();
        
    ADMUX  = 0x00;                           // ADC Righ Adjust, input pin number = ADC0
    ADCSRA = 0xD5;                           // single A/D conversion, fCK/32, conversion now started 

    while ((page_counter < PAGES_COUNT) && ((PIND & BTN_REC) == 0))
    {    
        for (unsigned int buffer_counter = 0; buffer_counter < BUFFER_SIZE; buffer_counter++)
        {
            while ((ADCSRA & ADCIF) == 0);   // Wait for A/D conversion to finish (ADIF becomes 1)
            ADCSRA |= ADCIF;                 // clear ADIF
            ADC_value = ADC - 0x1D5;
            count = 6;                          
            do                               // Customize this loop to 66 cycles !!!
            {
            } while(--count);                // wait some cycles
            ADCSRA |= ADCSC;                 // start new A/D conversion 
            
            PORTB &= ~DF_CHIP_SELECT;        // enable DataFlash
            SPI_write(BUFFER_1_WRITE);       // Write command buffer 1 write
            SPI_write(0x00);                 // write don't care byte
            SPI_write(buffer_counter >> 8);      
            SPI_write(buffer_counter);   
            SPI_write(ADC_value);            // write data into SPI Data Register
            PORTB |= DF_CHIP_SELECT;         // disable DataFlash
        }
        
        wait_DF_ready();
      
        PORTB &= ~DF_CHIP_SELECT;            // enable DataFlash
        SPI_write(B1_TO_MM_PAGE_PROG_WITH_ERASE);// write data from buffer1 to page
        SPI_write(page_counter >> 6);
        SPI_write(page_counter << 2);
        SPI_write(0x00);                     // write don't care byte
        PORTB |= DF_CHIP_SELECT;             // disable DataFlash
        
        page_counter++;
    }
    
    ADCSRA = 0x00;                           // disable AD converter
    
    SPCR = 0x00;                             // disable SPI        
    
    PORTD |= ~LED_REC_ON;                    // turn Red LED off
    
    DF_empty = FALSE;                        // DataFlash isn't empty
    
    while ((PIND & BTN_REC) == 0);           // wait until record button is released
}

//******************************************************************************

void play(void)
{
    unsigned int page_counter = 0;
        
    PORTD &= LED_PLY_ON;                     // turn Green LED on
    
    // Interrupt disabled, SPI port enabled, Master mode, MSB first,  SPI mode 3, f = 2 MHz
    SPCR = 0x5C;			     // enable SPI
    SPSR = 0x00; 
    
    wait_DF_ready();
        
    TCCR1A = 0x21;                           // 8 bit PWM, using COM1B
    TCCR1B = 0x01;                           // counter1 clock prescale = 1
    TCNT1  = 0x00;                           // set counter1 to zero      
    TIFR   = 0x04;                           // clear counter1 overflow flag 
    TIMSK  = 0x04;                           // enable counter1 overflow interrupt
    OCR1B  = 0x00;                           // set output compare register B to zero
    
    while ((page_counter < PAGES_COUNT) && ((PIND & BTN_PLY) == 0))
    {      
        PORTB &= ~DF_CHIP_SELECT;            // enable DataFlash
        SPI_write(MM_PAGE_TO_B2_XFER);       // Write command page to buffer 1
        SPI_write(page_counter >> 6);
        SPI_write(page_counter << 2);
        SPI_write(0x00);                     // write don't care byte
        PORTB |= DF_CHIP_SELECT;             // disable DataFlash
        
        wait_DF_ready();
            
        for (unsigned int buffer_counter = 0; buffer_counter < BUFFER_SIZE; buffer_counter++)
        {
            while((TIFR & T1_OF) == 0);      // wait for timer1 overflow interrupt
            TIFR |= T1_OF;                   // clear Timer1 overflow flag 
            
            PORTB &= ~DF_CHIP_SELECT;        // enable DataFlash     
            SPI_write(BUFFER_2_READ);        // read from buffer2
            SPI_write(0x00);                 // write don't care byte
            SPI_write(buffer_counter >> 8);  
            SPI_write(buffer_counter);       
            SPI_write(0xFF);                 // write dummy value to start register shift
            OCR1B = SPDR;                    // play data from shift register
            PORTB |= DF_CHIP_SELECT;         // disable DataFlash
        }
        page_counter++;
    }
    
    TIMSK  = 0x00;                           // disable all interrupts
    TCCR1B = 0x00;                           // stop counter1
    
    SPCR = 0x00;                             // disable SPI
    
    PORTD |= ~LED_PLY_ON;                    // turn Green LED off
    
    while ((PIND & BTN_PLY) == 0);           // wait until play button is released
}

//******************************************************************************

void loopback(void)
{
    PORTD &= LED_PLY_ON;                     // turn Green LED on
    
    unsigned char ADC_value;
    unsigned char count;
    
    ADMUX  = 0x00;                           // ADC Righ Adjust, input pin number = ADC0
    ADCSRA = 0xD5;                           // single A/D conversion, fCK/32, conversion now started 
    
    TCCR1A = 0x21;                           // 8 bit PWM, using COM1B
    TCCR1B = 0x01;                           // counter1 clock prescale = 1
    TCNT1  = 0x00;                           // set counter1 to zero      
    TIFR   = 0x04;                           // clear counter1 overflow flag 
    TIMSK  = 0x04;                           // enable counter1 overflow interrupt
    OCR1B  = 0x00;                           // set output compare register B to zero
    
    while ((PIND & BTN_LPB) == 0)            // while play button is pressed
    {
        while ((ADCSRA & ADCIF) == 0);       // Wait for A/D conversion to finish (ADIF becomes 1)
        ADCSRA |= ADCIF;                     // clear ADIF
        ADC_value = ADC - 0x1D5;
        count = 6;                          
        do                                   // Customize this loop to 66 cycles !!
        {
        } while(--count);                    // wait some cycles
        ADCSRA |= ADCSC;                     // start new A/D conversion 
      
        while((TIFR & T1_OF) == 0);          // wait for timer1 overflow interrupt 
        TIFR |= T1_OF;                       // clear Timer1 overflow flag 
        OCR1B = ADC_value;                   // play data from shift register
    }
          
    TIMSK  = 0x00;                           // disable all interrupts
    TCCR1B = 0x00;                           // stop counter1
    
    ADCSRA = 0x00;                           // disable AD converter
    
    PORTD |= ~LED_PLY_ON;                    // turn Green LED off
}

//******************************************************************************

void main(void)
{
    initialize();

    while (1)
    {       
        if ((PIND & BTN_ERS) == 0)           // if erase    button is pressed
            erase();
        
        if ((PIND & BTN_REC) == 0)           // if record   button is pressed
            record();

        if ((PIND & BTN_PLY) == 0)           // if play     button is pressed
            play();
        
        if ((PIND & BTN_LPB) == 0)           // if loopback button is pressed
            loopback();
    }
} 
