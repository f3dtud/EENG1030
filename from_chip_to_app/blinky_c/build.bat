arm-none-eabi-gcc -O0 -mthumb -g -mcpu=cortex-m4 *.c -T linker_script.ld -o main.elf   -nostartfiles -Xlinker -Map=output.map
arm-none-eabi-objcopy -g -O ihex main.elf main.hex 
arm-none-eabi-objdump -dS main.elf > objdump.txt
