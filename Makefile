.PHONY:all
.PHONY:clean

all:
	$(MAKE) -C src all
	$(MAKE) -C src/avr243
	$(MAKE) -C src/keytest

clean:
	$(MAKE) -C src clean
	$(MAKE) -C src/avr243 clean
	$(MAKE) -C src/keytest clean

trailing_space_clean:
	find . \( -name "*.m" -or -name "*.c*" -or -name "*.h" \) -exec sed -i 's/[[:space:]]*$$//' {} \;
