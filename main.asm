;____________________________________________________________________
;A
;Created: 15.03.2018 18:26:42
;Author : AlekseyMamontov
;
;_____________________________________________________________________



jmp RESET	;  Старт программы
jmp nop_	; EXT_INT0 ; IRQ0 Handler
jmp nop_	; EXT_INT1 ; IRQ1 Handler
jmp nop_	;PCINT0 ; PCINT0 Handler
jmp	nop_	;PCINT1 ; PCINT1 Handler
jmp	nop_	;PCINT2 ; PCINT2 Handler
jmp	nop_	;WDT ; Watchdog Timer Handler
jmp	nop_	;TIM2_COMPA ; Timer2 Compare A Handler
jmp	nop_	;TIM2_COMPB ; Timer2 Compare B Handler
jmp	nop_	;TIM2_OVF ; Timer2 Overflow Handler
jmp	nop_	;TIM1_CAPT ; Timer1 Capture Handler
jmp	nop_	;TIM1_COMPA ; Timer1 Compare A Handler
jmp	nop_	;TIM1_COMPB ; Timer1 Compare B Handler
jmp	nop_	;TIM1_OVF ; Timer1 Overflow Handler
jmp	nop_	;TIM0_COMPA ; Timer0 Compare A Handler
jmp	nop_	;TIM0_COMPB ; Timer0 Compare B Handler
jmp	nop_	;TIM0_OVF ; Timer0 Overflow Handler
jmp	nop_	;SPI_STC ; SPI Transfer Complete Handler
jmp	nop_	;USART_RXC ; USART, RX Complete Handler
jmp	nop_	;USART_UDRE ; USART, UDR Empty Handler
jmp	nop_	;USART_TXC ; USART, TX Complete Handler
jmp	nop_	;_ADC_ ; ADC Conversion Complete Handler
jmp	nop_	;EE_RDY ; EEPROM Ready Handler
jmp	nop_	;ANA_COMP ; Analog Comparator Handler
jmp	nop_	;TWI ; 2-wire Serial Interface Handler
jmp	nop_	;SPM_RDY ; Store Program Memory Ready Handler
nop_:reti


RESET:	



			cli
			ldi R16,Low(RAMEND)		; Инициализация стека
			out SPL,R16				; Обязательно!!!
			ldi R16,High(RAMEND)
			out SPH,R16     

		
			rcall MAX6675_init  
 			rcall ST7920_INIT
			rcall ST7920_CLEAR_GDRAM
			
			



			
			
endparty1:	
			
						
			

			ldi ZL,low(test5*2)
			ldi ZH,high(test5*2)
			
			ldi st7920_temp1, 0		; Y
			ldi st7920_temp2, $0			; X
			ldi st7920_temp3, 14			; h
			ldi st7920_temp4, $1
			ldi st7920_status,$04			; dх2
			rcall ST7920_GDRAM_blok		

			ldi ZL,low(test6*2)
			ldi ZH,high(test6*2)
			
			ldi st7920_temp1, 0		; Y
			ldi st7920_temp2, $4			; X
			ldi st7920_temp3, 14			; h
			ldi st7920_temp4, $1
			ldi st7920_status,$04			; dх2
			rcall ST7920_GDRAM_blok		



endpartyuy :

			ldi max6675_temp, 00
			ldi XL,low(temperature_sensor1_bin)
			ldi XH,high(temperature_sensor1_bin)
			rcall MAX6675_load_temperature
			ldi XL,low(temperature_sensor1_bcd)
			ldi XH,high(temperature_sensor1_bcd)
			rcall MAX6675_bcd_to_ASCII
			
			ldi max6675_temp, 01
			ldi XL,low(temperature_sensor2_bin)
			ldi XH,high(temperature_sensor2_bin)
			rcall MAX6675_load_temperature
			ldi XL,low(temperature_sensor2_bcd)
			ldi XH,high(temperature_sensor2_bcd)
			rcall MAX6675_bcd_to_ASCII



/*
			ld st7920_byte,X+
			rcall ST7920_DATA
			ld st7920_byte,X+
			rcall ST7920_DATA
			ld st7920_byte,X+
			rcall ST7920_DATA
			ld st7920_byte,X+
			rcall ST7920_DATA
*/
	

			ldi st7920_temp,255
			rcall ST7920_DELAY_ms
			ldi st7920_temp,255
			rcall ST7920_DELAY_ms
			
			

			ldi st7920_temp,255
			rcall ST7920_DELAY_ms
			ldi st7920_temp,255
			rcall ST7920_DELAY_ms

			ldi ZL,low(test2*2)
			ldi ZH,high(test2*2)
			rcall ST7920_TEXT	

			
//pausa: jmp pausa
 rjmp endpartyuy



.include "LCD ST7920.inc"
.include "MAX6675.inc"


///////////////////////////  Тексты //////////////////////////////////

test2: .DB 0,1,0,1,4,0,0,6,0,1,4,1,0,0,1,'1','9','0','-','2','0','0',' ','|','1','9','0','-','2','0','0',$FF

test1: .DB 'A','l','e','k','s','e','y',$FF

table_sensor:.Dw temperature_sensor1_ASCII,temperature_sensor2_ASCII

test5:  .DB 0b00000000,0b00000010
		.DB 0b00111110,0b00000110
		.DB 0b01000011,0b11000010
		.DB 0b01000001,0b00000010
		.DB 0b01000011,0b11000111
		.DB 0b01011101,0b00000000
		.DB 0b01011111,0b11000000
		.DB 0b01011101,0b00000000
		.DB 0b01011101,0b01100000
		.DB 0b01011101,0b01101111
		.DB 0b11011101,0b10001000
		.DB 0b11000001,0b10001000
		.DB 0b00111111,0b00001111
		.DB 0b00000000,0b00000000
test6:  .DB 0b00000000,0b00001111
		.DB 0b00111110,0b00000001
		.DB 0b01000011,0b11001111
		.DB 0b01000001,0b00001000
		.DB 0b01000011,0b11001111
		.DB 0b01011101,0b00000000
		.DB 0b01011111,0b11000000
		.DB 0b01011101,0b00000000
		.DB 0b01011101,0b01100000
		.DB 0b01011101,0b01101111
		.DB 0b01011101,0b00001000
		.DB 0b01000001,0b00001000
		.DB 0b00111110,0b00001111
		.DB 0b00000000,0b00000000

///////////////////////////  SRAM (ОЗУ) //////////////////////////


.dseg
temperature_sensor1_bin: .byte 2    ;в ОЗУ температура в бинарном представление
temperature_sensor1_bcd: .byte 2    ;в ОЗУ температура в двоично-десятичном формате
temperature_sensor1_ASCII: .byte 4  ;в ОЗУ температура в текстовом формате
temperature_sensor2_bin: .byte 2    ;в ОЗУ температура в бинарном представление
temperature_sensor2_bcd: .byte 2    ;в ОЗУ температура в двоично-десятичном формате
temperature_sensor2_ASCII: .byte 4  ;в ОЗУ температура в текстовом формате
st7920_cursor_X:.byte 1	; Горизонтальная позиция курсора
st7920_cursor_Y:.byte 1	; Вертикальная позиция курсора
st7920_X:		.byte 1 ; Начальная точка по горизонтали (графика)
st7920_Y:		.byte 1	; Начальная точка по вертикали (графика)
st7920_height:	.byte 1	; Начальная точка по вертикали (графика)
st7920_length:	.byte 1	; Начальная точка по вертикали (графика)
st7920_byte_fill:	 .byte 1	; каким байтом заполнить область
st7920_addr_buffer:	 .byte 2	; адрес откуда читать спрайт/текст
st7920_status_ram: .byte 1		; 0 бит 0-чтение из памяти, 1- озу (текст)
								; 1 бит 0-чтение из памяти, 1- озу (графика)
								; 2 бит 0- заполнить байтом,1 - спрайт
							








//////////////////////////  Вывод шрифтов  /////////////////////////////////////
 /*
 ;----------------- проверка шрифтов--------------		
blok:		rcall ST7920_Basic_Instruction
			clr st7920_byte
			clr st7920_temp  
			call ST7920_CURSOR
		    
			
			ldi r24,$a1
blok1:		ldi r22,$20

blok3:			
			clr st7920_byte
			clr st7920_temp 
			call ST7920_CURSOR

			ldi r21,16
			mov r25,r22

blok2:   	mov st7920_byte,r24
			call ST7920_DATA
			mov st7920_byte,r25
			call ST7920_DATA	
			inc r25		
			dec r21
			brne blok2
			ldi  st7920_temp,255
			call  ST7920_DELAY_ms
			ldi  st7920_temp,255
			call  ST7920_DELAY_ms
ldi  st7920_temp,255
			call  ST7920_DELAY_ms
			inc r22
			cpi r22,$7f
			brne blok3
			 
		
			ldi r23,$1
pause:	 call ST7920_DELAY_ms
			dec r23
			brne pause		

			

			inc r24
		    cpi r24,$aa
			brne blok1


			
blok4	:	rjmp blok
///////////////////////////////////////////////////////////////////////////

скроллинг экрана    


 
		ldi r22,$41
			ldi r23,00
srty:		push r22 
			ldi st7920_byte, 0b01000000
			or st7920_byte,r23
			ldi st7920_temp, 200
			rcall ST7920_COMMAND_DELAY
			ldi  st7920_temp,70
			call  ST7920_DELAY_ms
			inc r23
			pop r22
			dec r22
			brne srty


rcall ST7920_Extended_instruction
			ldi st7920_byte, 0b00000011
			rcall ST7920_COMMAND

			ldi r22,$41
			ldi r23,00
srty:		push r22 
			ldi st7920_byte, 0b01000000
			or st7920_byte,r23
			ldi st7920_temp, 200
			rcall ST7920_COMMAND_DELAY
			ldi  st7920_temp,70
			call  ST7920_DELAY_ms
			inc r23
			pop r22
			dec r22
			brne srty


			rcall ST7920_Extended_instruction
			ldi st7920_byte, 0b00000010
			rcall ST7920_COMMAND
			rcall ST7920_Basic_Instruction	
			ldi r22,16
			ldi r23,00

			ldi st7920_byte, 0b01000000
			rcall ST7920_COMMAND_delay
	
			ldi st7920_byte,$ff
			rcall st7920_data
			rcall st7920_data
			rcall st7920_data
			rcall st7920_data
			rcall st7920_data
			rcall st7920_data
			rcall st7920_data
			rcall st7920_data
			rcall st7920_data
			rcall st7920_data
			rcall st7920_data
			rcall st7920_data
			rcall st7920_data
			rcall st7920_data
			rcall st7920_data
			rcall st7920_data
			rcall st7920_data
			rcall st7920_data
			rcall st7920_data
			rcall st7920_data
			rcall st7920_data
			rcall st7920_data
			rcall st7920_data
			rcall st7920_data
			rcall st7920_data
			rcall st7920_data
			rcall st7920_data
			rcall st7920_data
			rcall st7920_data
			rcall st7920_data
			rcall st7920_data
			rcall st7920_data











			ldi st7920_byte,01
			rcall st7920_data
			rcall st7920_data




















//////////////////////////////////////////////////////////////////////////





*/

