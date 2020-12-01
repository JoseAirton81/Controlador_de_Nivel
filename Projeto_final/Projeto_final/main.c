/*
 * Projeto_final.c
 *
 * Created: 28/11/2020 08:28:07
 * Author : junio
 */ 

#define F_CPU 16000000UL
#define BAUD 9600
#define MYUBRR F_CPU/16/BAUD-1
#include <avr/io.h>
#include <util/delay.h>
#include <avr/interrupt.h>
#include "nokia5110.h"


char estado, aux2;//Variáveis globais


#define tam_vetor 3
unsigned char leitura_P_string[tam_vetor];
uint16_t leitura_P ;

//------------------------------------------------------------------------------------
//                      ADC
//------------------------------------------------------------------------------------

ISR(ADC_vect)
{
	leitura_P = (0.0978)*ADC;// Porcentagem = (100/1023)*ADC
	OCR0B = ADC/4.02;  // Duty Cycle = ADC/(1024/255), ou seja, de 0% até 100%
}

//------------------------------------------------------------------------------------
//Conversão de inteiro para string
//------------------------------------------------------------------------------------
void int2string(unsigned int valor, unsigned char *disp)
{
	for(uint8_t n=0; n<tam_vetor; n++)
	disp[n] = 0 + 48; //limpa vetor para armazenagem dos digitos
	disp += (tam_vetor-1);
	do
	{
		*disp = (valor%10) + 48; //pega o resto da divisão por 10
		valor /=10; //pega o inteiro da divisão por 10
		disp--;
	}while (valor!=0);
}

//------------------------------------------------------------------------------------
//                      USART
//------------------------------------------------------------------------------------

ISR(USART_RX_vect)
{
	char recebido;
	recebido = UDR0;
	
	if(recebido=='l')
	PORTB = 0b00000001;//BOMBA ON
	
	if(recebido=='d')
	PORTB = 0b00000000;//BOMBA OFF
	nokia_lcd_init(); //Inicia o LCD
	
	estado = recebido;
	Funcao_LCD(estado);
	
	USART_Transmit(recebido);
	
}

// ||Função para inicialização da USART||
void USART_Init(unsigned int ubrr)
{
	UBRR0H = (unsigned char)(ubrr>>8); //Ajusta a taxa de transmissão
	UBRR0L = (unsigned char)ubrr;
	UCSR0B = (1<<RXCIE0)|(1<<RXEN0)|(1<<TXEN0); //Habilita o transmissor e o receptor
	UCSR0C = (1<<USBS0)|(3<<UCSZ00); //Ajusta o formato do frame: 8 bits de dados e 2 de parada
	
	
}

// ||Função para envio de um frame de 5 a 8bits||
void USART_Transmit(unsigned char data)
{
	while(!( UCSR0A & (1<<UDRE0)));//Espera a limpeza do registr. de transmissão
	UDR0 = data; //Coloca o dado no registrador e o envia
}

// ||Função para recepção de um frame de 5 a 8bits||
unsigned char USART_Receive(void)
{
	while(!(UCSR0A & (1<<RXC0))); //Espera o dado ser recebido
	return UDR0; //Lê o dado recebido e retorna
}


ISR (INT0_vect);
ISR (INT1_vect);

void Funcao_LCD(char i);
void Funcao_Auto(uint8_t x);

int main()
{
	//GPIO
	DDRB = 0xFF; //Porta B como saída
	DDRC = 0x00; //Porta C como entrada
	DDRD =	0b00100000; //Porta PD5 como saída
	PORTC = 0xFE; //Desabilita o pullup do PC0
	PORTD = 0b01001100; //Os pull-ups da porta PD2, PD3 e PD6 habilitados
	
	//Configuração das interrupções
	EICRA = 0b00001111;//Interrupção externa INT0 e Int1 na borda de subida
	EIMSK = 0b00000011;//Habilita a interrupção externa INT0 e INT1
	sei();//habilita interrupções globais, ativando o bit I do SREG

	//Configura ADC
	ADMUX = 0b11000000; //Tensão interna de ref (1.1V), canal 0
	ADCSRA = 0b11101111; //habilita o AD, habilita interrupção, modo de conversão contínua, prescaler = 128
	ADCSRB = 0x00; //modo de conversão contínua
	DIDR0 = 0b00111110; //habilita pino PC0 como entrada do ADC0
	
	//Configuração do sinal PWM
	TCCR0A = 0b00100011;//PWM não invertido nos pinos OC0B
	TCCR0B = 0b00000101;//liga TC0, prescaler = 1024, fpwm =f0sc/(256* prescaler ) = 16MHz/(256*1024) = 61,04Hz
	OCR0B = 0 ;//controle do ciclo ativo do PWM 0C0A (PD5), Duty Cycle inicial
	
	//Configuração do USART
	USART_Init(MYUBRR);
	
	// Configuração do Comparador  analógico
	DIDR1 = 0b00000011; //desabilita as entradas digitais nos pinos AIN0 e AIN1
	ACSR = 1<<ACIE; //habilita interrup. por mudança de estado na saída do comparador
	 
	
	nokia_lcd_init(); //Inicia o LCD
	
	while(1)
	{	
	
		Funcao_LCD(estado);
		Funcao_Auto(OCR0B);
	}
	
}

ISR(INT0_vect)//interrupção externa 0, Ligar a Bomba
{
	PORTB = 0b00000001;//BOMBA ON
	estado = 'l';
	nokia_lcd_init(); //Inicia o LCD
	Funcao_LCD(estado);
}

ISR(INT1_vect) //interrupção externa 1, Desligar a Bomba
{
	PORTB = 0b00000000;//BOMBA OFF
	estado = 'd';
	nokia_lcd_init(); //Inicia o LCD
	Funcao_LCD(estado);
}


void Funcao_LCD(char i){
	
	
	nokia_lcd_clear(); //Limpa o LCD
	int2string(leitura_P, leitura_P_string); //converte a leitura do ADC em string
	nokia_lcd_write_string("Reservatorio",1);
	nokia_lcd_set_cursor(0,20);
	nokia_lcd_write_string(leitura_P_string,2); //Escreve a leitura no buffer do LCD
	nokia_lcd_write_string("%",2);
	nokia_lcd_set_cursor(0,40);
	if(i =='l')nokia_lcd_write_string("Bomba: ON", 1);
	else nokia_lcd_write_string("Bomba: OFF", 1);
	nokia_lcd_render(); //Atualiza a tela do display com o conteúdo do buffer
	_delay_ms(10);
	
}
void Funcao_Auto(uint8_t x){
	
		if (x <= 13){
			PORTB = 0b00000001;//BOMBA ON
			estado = 'l';
			nokia_lcd_init(); //Inicia o LCD
			Funcao_LCD(estado);
			_delay_ms(1500);
		}
		if (x >= 250){
			PORTB = 0b00000000;//BOMBA OFF
			estado = 'd';	
			nokia_lcd_init(); //Inicia o LCD
			Funcao_LCD(estado);
			_delay_ms(1500);
		}
}