all: shale

install: /usr/local/bin/shale

clean:
	rm -f shale *.o *.so

shale: shale.h shale.o shalelib.h shalelib.o
	g++ -o shale shale.o shalelib.o

shale.o: shale.h shale.cpp shalelib.h
	g++ -c -o shale.o shale.cpp

shalelib.o: shalelib.h shalelib.cpp
	g++ -c -o shalelib.o shalelib.cpp

/usr/local/bin/shale: shale
	sudo rm -f /usr/local/bin/shale
	sudo cp shale /usr/local/bin/shale
