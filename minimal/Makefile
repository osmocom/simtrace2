

default: 
	arm-none-eabi-gcc -c -mcpu=cortex-m3 -mthumb -g test.c -o test.o
	arm-none-eabi-ld -T test.ld test.o startup.o pio.o pmc.o -o test.elf
	arm-none-eabi-as -mcpu=cortex-m3 -g startup.s -o startup.o
	arm-none-eabi-objcopy -O binary test.elf test.bin
