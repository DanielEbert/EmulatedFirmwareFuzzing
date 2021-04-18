
all:
ifdef compile
	gcc -shared -fPIC -I.. -Wall -o $(compile).so $(compile)
else
	@echo 'Error: run make like this: "make compile=xyz"'
endif
