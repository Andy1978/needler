.PHONY:all
.PHONY:clean

all:
	$(MAKE) -C src all

clean:
	$(MAKE) -C src clean

trailing_space_clean:
	find . \( -name "*.m" -or -name "*.c*" -or -name "*.h" \) -exec sed -i 's/[[:space:]]*$$//' {} \;
