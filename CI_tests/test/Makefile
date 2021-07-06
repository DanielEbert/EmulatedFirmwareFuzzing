
all:
	arduino-cli compile -b arduino:avr:mega:cpu=atmega2560 src --output-dir . --build-property build.extra_flags="-O0 -fno-inline"

clean:
	rm src.ino.eep  src.ino.elf  src.ino.hex  src.ino.with_bootloader.bin  src.ino.with_bootloader.hex
