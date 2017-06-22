OBJ := entry.o core.o
OBJ += mmu.o uart.o
OBJ += print.o main.o
OBJ += low_malloc.o frame.o slab.o

ELF = rank.elf
BIN = rank.bin
LD_SCRIPT  = ld.script

CROSS_COMPILE = arm-linux-gnueabihf-

AS = $(CROSS_COMPILE)gcc -D__ASSEMBLY__ -g -fno-builtin -Wall -Wstrict-prototypes -Wshadow -Wundef  -mtune=cortex-a53 -march=armv8-a  -mfloat-abi=soft -pipe
CC = $(CROSS_COMPILE)gcc -g  -fno-builtin -Wall -Wstrict-prototypes -Wshadow -Wundef -Wno-unused-function -mtune=cortex-a53 -march=armv8-a -mfloat-abi=soft -nostdlib -pipe 
LD = $(CROSS_COMPILE)ld -g -nostartfiles -nodefaultlibs
OBJCOPY = $(CROSS_COMPILE)objcopy

all: $(BIN)

$(BIN): $(OBJ) $(LD_SCRIPT)
	$(LD) -o $(ELF) $(OBJ) --script=$(LD_SCRIPT) -lgcc -L /home/lujie/workdir/toolchain-rpi/tools/arm-bcm2708/gcc-linaro-arm-linux-gnueabihf-raspbian-x64/lib/gcc/arm-linux-gnueabihf/4.8.3
	$(OBJCOPY) $(ELF) -O binary $(BIN)
	mv $(ELF) ../bin/ -f
	mv $(BIN) ../bin/ -f
	
entry.o:  entry.S
	$(AS) -c -o $@ $<

core.o:  core.S
	$(AS) -c -o $@ $<
	
%.o:%.c 
	$(CC) -c -o $@ $<
	
clean:
	rm -f ../bin/$(ELF)../bin/$(BIN) *.o

