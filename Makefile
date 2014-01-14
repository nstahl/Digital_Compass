CPPFLAGS=-I. -I../lib
MCU=atmega644p
VPATH=../lib2
all: main.hex

main.elf:     main.o   spi2.o   

%.o: %.c
	avr-gcc ${CPPFLAGS} -Os -mmcu=${MCU} -o $@ -c $^ -lm

%.elf: %.o
	avr-gcc -Os -mmcu=${MCU} -o $@ $^ -lm

%.hex: %.elf
	avr-objcopy -j .text -j .data -O ihex $^ $@

%.lst: %.elf
	avr-objdump -h -S $^ > $@

clean:
	rm -f *.o *.elf *.hex *.lst


program: main.hex
	avrdude -p m644p -P /dev/ttyUSB0 -c avrusb500 -e -U flash:w:$^

fuses:
	avrdude -p m644p -P /dev/ttyUSB0 -c avrusb500 -e -U hfuse:w:0xD9:m
	avrdude -p m644p -P /dev/ttyUSB0 -c avrusb500 -e -U lfuse:w:0xE7:m
