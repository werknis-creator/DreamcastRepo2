TARGET = game.elf
OBJS   = main.o

all: rm-elf $(TARGET)

include $(KOS_BASE)/Makefile.rules

rm-elf:
	rm -f $(TARGET)

clean:
	rm -f $(TARGET) $(OBJS) romdisk.img
