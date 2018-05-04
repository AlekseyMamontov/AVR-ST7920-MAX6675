
////  Работа с МАХ6675 и термопарой типа К //////////////////////

.equ max6675_pin_SO = PB1       ; Чтение данных <d9> ардуино нано
.equ max6675_port_SO = PINB
.equ max6675_ddr_SO = DDRB

.equ max6675_pin_SCK = PB0		; Cтроб _/ \_  <d8> ардуино нано
.equ max6675_port_SCK = PORTB
.equ max6675_ddr_SCK = DDRB

.equ max6675_pin_CS = PD6		; Выбор 1 микросхемы   
.equ max6675_port_CS = PORTD
.equ max6675_ddr_CS = DDRD

.equ max6675_pin_CS2 = PD5	; Выбор 2 микросхемы 
.equ max6675_port_CS2 = PORTD
.equ max6675_ddr_CS2 = DDRD


.def max6675_temp = R16
.def max6675_temp1 = R17
.def max6675_temp2 = R18
.def max6675_temp3 = R19

/*
.dseg
temperature_sensor1_bin: .byte 2    в ОЗУ температура в бинарном представление
temperature_sensor1_bcd: .byte 2    в ОЗУ температура в двоично-десятичном формате
temperature_sensor1_ASCII: .byte 4  в ОЗУ температура в текстовом формате

temperature_sensor2_bin: .byte 2
temperature_sensor2_bcd: .byte 2
temperature_sensor2_ASCII: .byte 4

*/


//////////////////////////////////////////////////////////////////////

MAX6675_init:             cbi max6675_ddr_SO,max6675_pin_SO     ; на чтение
			  sbi max6675_ddr_SCK,max6675_pin_SCK	; на выход
			  sbi max6675_ddr_CS,max6675_pin_CS	; на выход
			  sbi max6675_ddr_CS,max6675_pin_CS2	; на выход	
			  nop
			  cbi max6675_port_SCK,max6675_pin_SCK  ; выставить LOW
			  sbi max6675_port_CS,max6675_pin_CS	; выставить HIGH (отключен обмен)
			  sbi max6675_port_CS,max6675_pin_CS2	; выставить HIGH (отключен обмен)
			  ret
///////////////////////////////////////////////////////////////////

MAX6675_load_temperature: 		
			push max6675_temp1
			push max6675_temp2
			push max6675_temp3

			sbrs max6675_temp,0		         ;если 0 в 1 бите то выставить LOW
			cbi max6675_port_CS,max6675_pin_CS	 ;на первой MAX6675 и начать обмен
		        sbrc max6675_temp,0		         ;если 1 в 1 бите то выставить LOW
			cbi max6675_port_CS,max6675_pin_CS2;на второй MAX6675 и начать обмен. 

			rcall MAX6675_load_bin			 ;считать температуру 12 bit + 1 bit включена ли термопара

			sbi max6675_port_CS,max6675_pin_CS	 ;выставить HIGH (отключить обмен)
			sbi max6675_port_CS,max6675_pin_CS2      ;выставить HIGH (отключить обмен)

			lsr max6675_temp1
			ror max6675_temp
				brcs MAX6675_no_connect	        ; если 1 то термопара не подключена

			st  X+, max6675_temp	; сохранить в памяти bin представление числа
			st  X+, max6675_temp1
			
			lsr max6675_temp1	; после запятой число игнорируем))
			ror max6675_temp	; если понадобится то
			lsr max6675_temp1	; 00 - 00, 01 - 0.25 , 10 - 0.50, 11 - 0.75
			ror max6675_temp

			rcall MAX6675_bin_bcd ; Перевести температуру 
								  ; в двоично-десятичное представление
MAX6675_write_ram:

			st X+, max6675_temp2	; записать в память LSB BINBСD
			st X+, max6675_temp3	; записать в память MSB BINBСD

MAX6675_pop:            pop max6675_temp3
			pop max6675_temp2
			pop max6675_temp1
			ret	
				
MAX6675_no_connect:						; если термопара не подключена

			ldi max6675_temp2,$ff		; пишем во все регистры $ff
			mov max6675_temp3,max6675_temp2
			st  X+, max6675_temp2	
			st  X+, max6675_temp2
			rjmp MAX6675_write_ram

///////////////////////////////////////////////////////////////

MAX6675_load_bin:
				clr max6675_temp
				clr max6675_temp1
				rcall MAX6675_load_bit ; чтение бита маркер _0_							

				ldi max6675_temp2,13 	; задать считать 13 бит
MAX6675_cycle:	rcall MAX6675_load_bit
				rol max6675_temp	; записать регистр max6675_temp1 MSB - max6675_temp LSB бит 
				rol max6675_temp1	; R17 << C << R16 << C								  
				dec max6675_temp2
				brne MAX6675_cycle
								
				rcall MAX6675_load_bit				 ; считать два ненужных бита
MAX6675_load_bit:
				sbi max6675_port_SCK,max6675_pin_SCK ; дать фронт _/ на чтение
				nop									 ; выдержка согласно даташит 100 ns (на 16 Мгц,такт 64ns)
				clc									 ; флаг C=0 ; nop+clc 2 такта 128 ns
				sbic max6675_port_SO,max6675_pin_SO  ; если 0 пропустить следующую команду
				sec									 ; флаг C=1 
				cbi max6675_port_SCK,max6675_pin_SCK ; дать фронт \_  					
				ret

//////// перевод 12 битного числа (0-1023) в двоично десятичное представление ////////////////// 

MAX6675_bin_bcd:clr max6675_temp3
				clr max6675_temp2	 ;max6675_temp1-max6675_temp двоичное число 12bit (0-1023)			
				cpi max6675_temp1,00 ;если число меньше 256
				breq MAX6675_bcd1	 ;то начать с max6675_temp  	

				subi max6675_temp, $e8 ;иначе вычесть 1000
				sbci max6675_temp1,$03
				ldi max6675_temp3,$10  ; установить 1 верхней тетраде max6675_temp3
				brcc MAX6675_bcd3	   ; если число было больше 1000  перейти.

				clr max6675_temp3	   ; если меньше 1000
				subi max6675_temp, $18 ; вернуть все назад
				sbci max6675_temp1,$FC 

MAX6675_bcd1:		subi max6675_temp,100	; посчитаем сотни
					sbci max6675_temp1,0
					brcs MAX6675_bcd2
					inc	max6675_temp3
					rjmp MAX6675_bcd1

MAX6675_bcd2:		subi max6675_temp,-100		   

MAX6675_bcd3:		subi max6675_temp,10	; посчитаем десятки
					brcs MAX6675_bcd4
					inc	max6675_temp2
					rjmp MAX6675_bcd3

MAX6675_bcd4:	subi max6675_temp,-10	
				swap max6675_temp2			  ; max6675_temp2 младщие цифры в формате BSD
				or max6675_temp2,max6675_temp ; max6675_temp3 старшие цифры в формате BSD
				ret
/////////////////////////////////////////////////////////////////////////////////

MAX6675_bcd_to_ASCII:
				push st7920_temp1
				push st7920_temp2
				push st7920_temp3
				push st7920_temp
				rcall MAX6675_to_ASCII
				pop st7920_temp
				rjmp MAX6675_pop
MAX6675_to_ASCII:
				ldi st7920_temp3,$10		; если отнять $10 от '0'($30) будет пробел ($20)
				ldi st7920_temp2,'0'		; символное представления 0
				ld st7920_temp, X+			; младшие 2 цифры (0-99)
				ld st7920_temp1,X+			; старшие 2 цифры (0-10)
				
					push st7920_temp		
					rcall MAX6675_asc_1		;формируем старшие цифры
					pop st7920_temp1		;а потом младшие

MAX6675_asc_1:	mov st7920_temp, st7920_temp1
					swap st7920_temp1		; сначала верхняя тетрада
					rcall MAX6675_asc_2
				mov st7920_temp1,st7920_temp
MAX6675_asc_2:		andi st7920_temp1,$0f	; затем младшая
				breq MAX6675_asc_3
					clr st7920_temp3			  ; если цифра >0 (1-9) то обнулить _temp3
MAX6675_asc_3:		add st7920_temp1,st7920_temp2 ; сформировать символ
					sub st7920_temp1,st7920_temp3 ; если первые нули то поставить пробел
				st X+,st7920_temp1			      ; записать в озу
				ret

				
				 
