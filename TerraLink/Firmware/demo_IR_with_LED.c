#include <avr/io.h>
#include <util/delay.h>

void UART_Init(void)
{
    UBRR0H = 0;
    UBRR0L = 103;

    UCSR0B = (1 << TXEN0);

    UCSR0C = (1 << UCSZ01) | (1 << UCSZ00);
}

void UART_SendChar(char data)
{
    while (!(UCSR0A & (1 << UDRE0)));

    UDR0 = data;
}

void UART_SendString(const char *str)
{
    while (*str)
    {
        UART_SendChar(*str);
        str++;
    }
}

int main(void)
{
    DDRB |= (1 << PB5);

    DDRD &= ~(1 << PD2);

    PORTD |= (1 << PD2);

    UART_Init();

    UART_SendString("IR Sensor System Started\r\n");

    while (1)
    {
        if (!(PIND & (1 << PD2)))
        {
            PORTB |= (1 << PB5);

            UART_SendString("Object Detected\r\n");

            _delay_ms(500);
        }
        else
        {
            PORTB &= ~(1 << PB5);

            UART_SendString("No Object\r\n");

            _delay_ms(500);
        }
    }

    return 0;
}