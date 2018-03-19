/*
___________________________
Driver ST7920 arduino nano
Aleksey Mamontov 13/03/2018
___________________________
AC - адресный счетчик

DDRAM - Память символьных данных 
CGRAM -	ОЗУ для пользовательских шрифтов 00 - 3F, символы 4 шт. 16х16
GDRAM - ОЗУ графики 256х64 (по сути экран 128х128, видимая часть 128х64)   
		подпрограмма ST7920_GDRAM_blok - как раз делает преобразование
		в данный формат как на экране 128х128

CGROM - ПЗУ шрифтов 8х16
HCGROM- ПЗУ шрифтов 16х16 , если отправить (ST7920_DATA) в дисплей число  
		от A0 до А9, то второе число будет это номер номер знака из пзу.
		А7- это кириллица, тоесть сначала $А7 а потом $40 то это "А"  
		А1- различные символы
		А2- цифры с точками и т.д.
		А3- английские 16х16
		А4- китайский
		А5- японский
		А6- греческий
		А8- не помню)))
		А9- рамки для таблиц

----- Команды стандартного режима -----

0b00000001 -	очистка экрана знаком пробел и установка курсора
				в начала экрана
0b00000010 -	курсор в начала экрана, данные на экране не меняются
0b00000111 -	курсор сдвигать вправо после вывода, счетчик +1 курсора (не нужные функции)
0b00000101 -	курсор сдвигать влево после вывода, счетчик -1 курсора (не нужные функции)
0b00000110 -    курсор сдвигать вправо , АС+1

0b001100G0 -	передача 8 bit, cтандартный режим, G - 1 включена графика
0b001101G0 -	передача 8 bit, расширенный режим, G - 0 выключена графика
				при инициализации сначало выставить 8 бит 0b00110000
				и только затем можно выставлять расширеный режим и т.д.
				по другому нельзя читай даташит.

0b00001DCB -	D - 1-дисплей включить, 0 - выключить 
				С - 1-курсор виден , 0 - не виден
				B - 1-курсор мигает, 0 - не мигает

0b00010000 -	курсор сдвинуть влево, АС-1
0b00010100 -	курсор сдвинуть вправо, АС+1
0b00011000 -	сдвинуть экран влево, АС=АС
0b00011100 -	сдвинуть экран вправо, АС=АС

0b01nnnnnn -	Set CGRAM address to address counter (AC) 
0b10nnnnnn -	Установить адрес DDRAM на адресный счетчик (AC)

----- Расширенный режим ----

0b00000001 -	перейти в режим ожидания
0b000001nn -	Инвертировать строку, повторно команда возращает все на место.
				их 4 строки 16х256 (бесполезная команда)
0b0000001S -    если, S=0 - использовывать следующую команду
				для выставление адреса GDRAM (как обычном режиме)
				если, S=1 - то следующая команда адрес вертикальной 
				строки которая будет отображена в первой верхней части
				экрана
0b01nnnnnn -    а вот эта команда)) принципе можно устроить скроллинг 
				экрана, но он врядле будет нормальным из за особой организации
				памяти.
				А вот чтоб отображать дополнительный экран самое оно.
				на экране сначало видно 128(х)х64(y) точек, остальные
				128х64 скрыты.
				Чтоб показать второй экран надо дать команду 0b01100000
				вернутся к первому, 0b01000000

0b10nnnnnn -	Выставить вертикальную строку в графической памяти от 0-64
				потом должна идти эта команда и никак по другому
0b1000nnnn -	Выставить горизонтальную cтроку от 0..0F
				После этого обязательно должно идти 2 байта,
				тоесть 8х2 = 16 точек за один раз и никак по другому.				

по экрану

JP2  замыкая средний контакт на "S" или "P"  у вас и будет или SPI или Parallel
JP3  замыкая контакт  - не нужен переменный резистор на контрастность

//////////////////////////переменные в озу ////////////////////////////////

.dseg
st7920_cursor_X:.byte 1	; Горизонтальная позиция курсора
st7920_cursor_Y:.byte 1	; Вертикальная позиция курсора
st7920_X:		.byte 1 ; Начальная точка по горизонтали (графика)
st7920_Y:		.byte 1	; Начальная точка по вертикали (графика)
st7920_height:	.byte 1	; высота точек(графика)
st7920_length:	.byte 1	; длина точек (графика)
st7920_byte_fill:	.byte 1	; каким байтом заполнить область
st7920_addr_buffer:	.byte 2	; адрес откуда читать спрайт/текст
st7920_status_ram:	.byte 1		; 0 бит 0-чтение из flash, 1- озу (текст)
								; 1 бит 0-чтение из flash, 1- озу (графика)
								; 2 бит 0- заполнить байтом, 1 - спрайт
								

////////////////////////////////////////////////////////////////////////////////
*/

.equ st7920_port_E = PORTB	; Управляющий порт к которому подключен ST7920
.equ st7920_port_RW = PORTB
.equ st7920_port_RS = PORTB

.equ st7920_ddr_E = DDRB	; Управляющий порт к которому подключен ST7920
.equ st7920_ddr_RW = DDRB
.equ st7920_ddr_RS = DDRB

.equ _RW = PB3				; RW Шина данных вывод 11 
.equ _E = PB5				; E Строб вывод 13
.equ _RS = PB2				; RS Начало/конец передачи данных вывод 10

.equ ram_or_rom_text = 0
.equ ram_or_rom_graphic = 1
.equ byte_fill = 2



.def st7920_byte = R16		; Регистр используется для записи данных в порт
.def st7920_temp = R17		; Временный регистр
.def st7920_temp1 = R18		; Временный регистр
.def st7920_temp2 = R19		; Временный регистр
.def st7920_temp3 = R20		; Временный регистр
.def st7920_temp4 = R21		; Временный регистр
.def st7920_status = R22    ; регистр хранит состояние вывод из ram или flash

////////////////////////////// Драйвер ST7920 //////////////////////////////
ST7920_COMMAND: 
			push st7920_byte
			push st7920_temp
				call ST7920_OUT_COMMAND
				rjmp ST7920_pop
ST7920_DATA:
			push st7920_byte
			push st7920_temp
				call ST7920_OUT_DATA
				rjmp ST7920_pop

//////////////////////////// Отправка  24 бит //////////////////////////////

ST7920_OUT_COMMAND:							;Отправляет команду
			push st7920_byte				;команда st7920_byte
            ldi st7920_byte, 0b11111000		;первые 8бит маркер команда  
			rjmp ST7920_OUT_24bit
ST7920_OUT_DATA:
			push st7920_byte					;Отправляет данные			
			ldi ST7920_byte, 0b11111010			;данные в ST7920_byte
												;первые байт содержит 0b11111-rw-rs-0  		
ST7920_OUT_24bit:								;начало отправка 24 бит, 3 байта
				sbi st7920_port_RS, _RS			;Выбор дисплея на всякий случай))
				rcall ST7920_OUT_byte			;Отправка байта 111110 - RS(1 данные,0 команда) -0
				pop st7920_byte

				push st7920_byte
				rcall ST7920_OUT_nibble			;Отправка старшего ниббла (7-4) + 0000 
				pop st7920_byte
												;отправка младшего нибла (3-0) + 0000
				swap st7920_byte				;обмен тетрадами (3-0) <-> (7-4)
												 
ST7920_OUT_nibble:	andi st7920_byte, $f0		;Очистка нижнего ниблла 
ST7920_OUT_byte:	ldi st7920_temp,8		; начать цикл , отправка байта.
					//cli					; запретить прерывания
ST7920_OUT_bit:		sbrc st7920_byte,7		; считать 7 бит, пропустить команду если 0. 
					sbi st7920_port_RW, _RW    ; установить 1 в порту данных
					sbrs st7920_byte,7      ; считать 7 бит, пропустить команду если 1. 
					cbi st7920_port_RW, _RW	; установить 0 в порту данных	
					lsl st7920_byte			; сдвинуть регистр данных влево
	
					rcall ST7920_Strob_E    ; Выдать строб _/ на _E 
	
					dec st7920_temp
					brne ST7920_OUT_bit
					//sei					; разрешить прерывания
			cbi st7920_port_E, _E
			ret

;---------------------- строб LOW - 360 us, HiGH - 360 us ------------------
ST7920_strob_E:
			cbi st7920_port_E, _E 
			push r0				   ;Согласно даташита задержка 360 ns
			pop r0				   ;можно меньше, но не меньше 260ns
			nop					   ; 1 такт (16Mhz) 62,5 ns
			sbi st7920_port_E, _E
			push r0
			pop r0
			ret

////////////////////////  1 мкс (1us точная) для 16 Mhz  /////////////////

ST7920_DELAY_us: 
			ldi  st7920_temp,3	   ; 16 тактов с учетом rсall и ret 
ST7920_D0:  dec  st7920_temp	   ; 1 us					 
			brne ST7920_D0
			ret

///////////////////////// Cовместим команду и задержку /////////////////

ST7920_COMMAND_DELAY:
			rcall ST7920_COMMAND

//////////////////задержка 1мкс (1us* N)  от 1-255  не точная ///////////////

ST7920_DELAY_n:
			push st7920_byte
			push st7920_temp
			mov st7920_byte,st7920_temp

ST7920_N0:		call ST7920_DELAY_us
				dec st7920_byte
				brne ST7920_N0
			rjmp ST7920_pop

////////////////////// задержка 1ms * N милисекунда (1-255) /////////////////

ST7920_DELAY_ms:					; В Temp -  количество милисекунд
			push st7920_byte
			push st7920_temp		 
			push st7920_temp1

ST7920_L0:		ldi  st7920_temp1, 21
				ldi  st7920_byte, 199
ST7920_L1:			dec  st7920_byte
					brne ST7920_L1
					dec  st7920_temp1
					brne ST7920_L1
				dec st7920_temp
				brne ST7920_L0

			rjmp ST7920_pop1
			
//////////////////////////// инициализация дисплея //////////////////////

ST7920_INIT:
			sbi st7920_ddr_RW,_RW   ; Выставить порт на выход
			sbi st7920_ddr_E,_E     ; Выставить порт на выход
			sbi st7920_ddr_E,_RS	; Выставить порт на выход
			
			ldi st7920_temp,00
			sts st7920_status_ram,st7920_temp

			sbi st7920_port_RS,_RS		; Выбрать дисплей 
			cbi st7920_port_E,_E		; Линию строба в 0

			ldi st7920_temp,40			 ; cогласно даташита подождать > 40 микросекунд.
			rcall ST7920_DELAY_ms 

			ldi  st7920_byte, 0b00110000  ; Выдать команду на работу в режиме 8bit
			ldi  st7920_temp,100		  ; подождать 100 us, cогласно даташита.
			rcall ST7920_COMMAND_DELAY

			ldi  st7920_byte, 0b00110000 ; Еще раз команда на работу в режиме 8bit
			ldi  st7920_temp,40			 ; подождать 40 us, cогласно даташита.
			rcall ST7920_COMMAND_DELAY


			ldi st7920_byte, 0b00001110  ; DISPLAY STATUS (см. даташит)
			ldi  st7920_temp,100		 ; подождать 100 us, cогласно даташита.
			rcall ST7920_COMMAND_DELAY


			rcall ST7920_CLEAR_DDRAM

			Ldi st7920_byte, 0b00000110	 ; ENTRY MODE SET - установка курсора
			ldi st7920_temp,75

			rjmp ST7920_COMMAND_DELAY

////////// установка  курсора  st7920_cursor_Y	-строка, st7920_cursor_X-позиция в строке ////////////

ST7920_MOVE_CURSOR:
			push st7920_byte				
			push st7920_temp
			lds st7920_temp,st7920_cursor_X
			lds st7920_byte,st7920_cursor_Y			
			rjmp ST7920_CURSOR1
ST7920_CURSOR:								; если войти подпрограмму отсюда 
			push st7920_byte				; позиция в temp, byte.
			push st7920_temp			
ST7920_CURSOR1:
			andi st7920_temp,0b00000111		; В temp --> 16x16 do 7 (/2) / 8x16 do 15 (Горизонтальная) 
			andi st7920_byte,0b00000111		; В byte -- вертикальная строка 0-7 (видно сразу только 4)
				bst st7920_byte,2			; 0.$80-$87 
				bld st7920_byte,5			; 1.90-97
				bst st7920_byte,0			; 2.88-8F	
				bld st7920_byte,4			; 3.98-9F
				bst st7920_byte,1			; 4.A0-А7 эти строки будут видны если 
				bld st7920_byte,3			; 5.B0-B7 сделать скроллинг экрана
				ori st7920_byte,0b10000000  ; 6.A8-AF и 7.B8-BF 
				andi st7920_byte,0b10111000 ; Собираем команду ( базовый режим)
				or st7920_byte,st7920_temp  ; 0b10 - byte (3 bit) - temp (3bit)		
			ldi st7920_temp,5
			rcall ST7920_COMMAND_DELAY
			rjmp st7920_pop

/////////////////// Включение расширенный режим + графика  //////////////////////////

ST7920_Extended_instruction:
			push ST7920_byte					; Выбрано 8 bit 0b001-1(8bit)-0110
			push ST7920_temp
				ldi  ST7920_byte,0b00110110     ; Графика включена 0b001101-(G)-0
				ldi  st7920_temp,70				; Расширенный режим включен 0b00110-(Re)-10 	
				rcall ST7920_COMMAND_DELAY
			rjmp ST7920_pop

/////////////////// Включение базового режима + графика /////////////////////////////

ST7920_Basic_Instruction:
			push ST7920_byte					; Выбрано 8 bit 0b001-1(8bit)-0110
			push ST7920_temp
				ldi  ST7920_byte,0b00110010		; Графика включена 0b001100-(G)-0
				ldi  st7920_temp,70			; Расширенный режим отключен 0b00110-(Re)-10 
				rcall ST7920_COMMAND_DELAY
			rjmp ST7920_pop

///////////////////////////// Очистка символьного экрана /////////////////////////

ST7920_CLEAR_DDRAM :
			push ST7920_byte
			push ST7920_temp	
				ldi st7920_byte, 0b00000001	 ; команда записывает код 20H (пробел) 
				rcall ST7920_COMMAND
				ldi st7920_temp,20
				rcall ST7920_DELAY_ms		 ; согласно даташита ждем 20ms
			rjmp ST7920_pop

////////////////////////// Чтение из памяти текста и графики///////////////////

ST7920_TEXT:
			push st7920_byte
			push st7920_temp
			push st7920_temp1			
			push st7920_temp2
			push st7920_temp3
			lds st7920_temp3,st7920_status_ram	

ST7920_TXT:	rcall ST7920_TXT_ram_or_rom
				tst st7920_byte					; если ноль то 
					brne ST7920_TXT_sensor		; последующие 2 байта 

				rcall ST7920_TXT_ram_or_rom		; вертикальная строка
				mov st7920_temp,st7920_byte 
				rcall ST7920_TXT_ram_or_rom		; горизонтальная строка
				rcall ST7920_CURSOR
				rjmp ST7920_TXT

ST7920_TXT_sensor:	cpi st7920_byte,$01			; если 1 то Вывод значений датчиков
		  			brne ST7920_TXT_16x16		; 
					rcall ST7920_TXT_ram_or_rom	; считать длину значения датчика
					mov st7920_temp2,st7920_byte
					rcall ST7920_TXT_ram_or_rom	; считать номер датчика в таблице
					push ZH						 
					push ZL						
test:					ldi ZL,low(table_sensor*2)	; table_sensor - Хранятся адреса
						ldi ZH,high(table_sensor*2)	; значений датчиков  в ASCII формате
						lsl st7920_byte				; x2 
						add ZL,st7920_byte			;
						ldi st7920_byte,0			;
						adc ZH,st7920_byte
							rcall ST7920_TXT_ram_or_rom
							mov st7920_temp,st7920_byte		; младший байт адреса
							rcall ST7920_TXT_ram_or_rom
							mov st7920_temp1,st7920_byte	; старший байт адрес
							mov ZL,st7920_temp		; загрузить адрес в Z
							mov ZH,st7920_temp1		;

ST7920_TXT_sensor1:				ld st7920_byte,Z+		; считать заданое
								rcall ST7920_DATA		; количество символов
								dec st7920_temp2
								brne ST7920_TXT_sensor1
					pop ZL
					pop ZH
					rjmp ST7920_TXT

ST7920_TXT_16x16:	cpi st7920_byte,$A1			; если A1-A7 то считываем  еще один байт 
						brlo ST7920_TXT_STOP	; это расширенные символы из памяти
					cpi st7920_byte,$AA
						brsh ST7920_TXT_STOP
					rcall  ST7920_DATA			; Шрифт 16х16, A7:20 Русские шрифты
					rcall ST7920_TXT_ram_or_rom
					rjmp ST7920_TXT_LCD

ST7920_TXT_STOP:	cpi st7920_byte,$FF
						breq ST7920_POP3			; Закончить $FF
			
ST7920_TXT_LCD:	rcall ST7920_DATA
				rjmp ST7920_TXT

ST7920_TXT_ram_or_rom :	sbrc st7920_temp3,ram_or_rom_text
						rjmp ST7920_TXT_ram_or_rom1
						lpm st7920_byte,Z+
						ret
ST7920_TXT_ram_or_rom1:	ld st7920_byte,Z+
						ret



///////////////// Вывод заполненого блока или спрайта ///////////////////

ST7920_GRAPHIC:		
			push st7920_byte		    
			push st7920_temp			
			push st7920_temp1				
			push st7920_temp2				
			push st7920_temp3			
			push st7920_temp4						
			push ZH
			push ZL
			push st7920_status

		ldi ZL,low(st7920_X)
		ldi ZH,high(st7920_X)
			ld st7920_temp2,Z+			; X строка 128 точек (8x2x8)
			ld st7920_temp1,Z+			; Y строка	128 строк (0-7F)
			ld st7920_temp3,Z+			; h высота до 128 строк
			ld st7920_temp4,Z+			; d длина --> 8х2байта (0-0F)
			ld st7920_byte,Z+			; код заполнения области
		lds st7920_status,st7920_status_ram			
		sbrc st7920_status,byte_fill	;если 2бит=0, то заполнить байтом
		rjmp ST7920_grf1

				ld st7920_byte,Z+		; иначе загрузить где находится
				ld st7920_temp,Z+		; спрайт в памяти
				mov ZL, st7920_byte
				mov ZH, st7920_temp
ST7920_grf1:	rcall ST7920_GDRAM_blok

			pop st7920_status
			pop ZL
			pop ZH
ST7920_pop4:pop st7920_temp4		; Для экономии памяти
ST7920_pop3:pop st7920_temp3
ST7920_pop2:pop st7920_temp2
ST7920_pop1:pop st7920_temp1
ST7920_pop:	pop st7920_temp
			pop st7920_byte
			ret

/////////////////////////////////////////////////////////////////////

ST7920_GDRAM_blok:
				rcall ST7920_Extended_instruction				
				ldi st7920_temp, 5			; задержка после команды
ST7920_CG1:		push st7920_byte 
					mov st7920_byte,st7920_temp1
					andi st7920_byte,0b00011111
					bst st7920_temp1,6
					bld st7920_byte,5 
					ori st7920_byte,0b10000000 ;1 команда, вертикальная строка
					call ST7920_COMMAND_DELAY

					mov st7920_byte,st7920_temp2
					andi st7920_byte,0b00000111
					bst st7920_temp1,5
					bld st7920_byte,3 
					ori st7920_byte,0b10000000 ;2 команда, горизонтальную строка
					call ST7920_COMMAND_DELAY
				pop st7920_byte

				push st7920_temp4
ST7920_CG2:		rcall ST7920_CG3				; запись в строку 2 байт ST7920_byte											
					dec st7920_temp4			; Согласно ST7920_temp4 0-7
					brne ST7920_CG2
				pop st7920_temp4

				inc st7920_temp1
				dec st7920_temp3
				brne ST7920_CG1

				rjmp ST7920_Basic_instruction

ST7920_CG3:	sbrc st7920_status,byte_fill
			rjmp ST7920_CG4
			rcall ST7920_DATA
			rjmp ST7920_DATA
ST7920_CG4: sbrs st7920_status,ram_or_rom_graphic
			rjmp St7920_CG5
			ld st7920_byte, Z+
			rcall ST7920_DATA
			ld st7920_byte, Z+
			rjmp ST7920_DATA			
ST7920_CG5: lpm st7920_byte, Z+
			rcall ST7920_DATA
			lpm st7920_byte, Z+
			rjmp ST7920_DATA

/////////////////////////   Очистка графичеcкого экрана //////////////////////

ST7920_CLEAR_GDRAM:	
			rcall ST7920_Extended_instruction		; Реальный размер экрана 256х64
			push st7920_byte						
			push st7920_temp			
				ldi st7920_byte,$80					; Установить Y=0,X=0
ST7920_CL1:		push st7920_byte					; 				;											;				
					rcall ST7920_COMMAND			; командой установить Y
					ldi st7920_byte,$80				;
					rcall ST7920_COMMAND			; командой установить Х
					ldi st7920_byte,0
						ldi st7920_temp,33					
ST7920_CL2:				rcall ST7920_DATA			; заполнить строку 0-15
						dec st7920_temp				; 8x2x16 = 256 точек
						brne ST7920_CL2
				pop st7920_byte
				inc st7920_byte						; Увеличить строку 0-63
				cpi st7920_byte,$C0					; 64 строки
				brne ST7920_CL1
			rcall ST7920_Basic_Instruction		
			rjmp ST7920_pop

///////////////////////////////////////////////////////////////////////



/*
////////// перевод из HEX-BCD  st7920_byte (0-ff(255)) -> st7920_temp1(0-2),st7920_temp(99)///////////////

st7920_bin_bcd:		clr st7920_temp				
					clr st7920_temp1
st7920_bin_bcd1:		cpi	st7920_byte,100
						brlo st7920_bin_bcd2
						subi st7920_byte,100
						inc	st7920_temp1
						rjmp st7920_bin_bcd1
st7920_bin_bcd2:			subi st7920_byte,10
							brcs st7920_bin_bcd3
							inc	st7920_temp
							rjmp st7920_bin_bcd2
st7920_bin_bcd3:	subi st7920_byte,-10	
					swap st7920_temp
					or st7920_temp,st7920_byte	
					ret					;r18-r17 - BCD число

//////////////////////  перевод BCD-HEX st7920_temp1,st7920_temp->st7920_byte ///////////////////////

st7920_bcd_bin: mov st7920_byte,r17			;st7920_temp1,st7920_temp - BCD число
				andi st7920_byte,$0f
				swap st7920_temp
				andi st7920_temp,$0f
				breq st7920_bcd_bin2
st7920_bcd_bin1:			subi st7920_byte,-10
							dec st7920_temp
							brne st7920_bcd_bin1
st7920_bcd_bin2:		andi st7920_temp1,$03
						breq st7920_bcd_bin4
st7920_bcd_bin3:	subi st7920_byte,-100
					dec st7920_temp1
					brne st7920_bcd_bin3
st7920_bcd_bin4: ret				 ;r16  двоичное число 0-ff		

///////////////// вывод бинарного 8 bit - r16 /////////////////

Bin_to_LCD: push r16
			push r17
			ldi r17, 8
Bin_to_LCD1:	lsl r16				
			 	push r16
					ldi r16 , '0'	
					BRCC Bin_to_LCD2
					ldi r16 , '1'					
Bin_to_LCD2:		rcall ST7920_DATA
				pop r16
				dec r17
				brne Bin_to_LCD1				
			pop r17
			pop r16
			ret
				
///////////////////////////////////////////////////////////////////

*/
