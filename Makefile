all: shale maths.so array.so

install: /usr/local/bin/shale /usr/local/lib/shale/maths.so /usr/local/lib/shale/array.so

clean:
	rm -f shale *.o *.so

shale: shale.h shale.o shalelib.h shalelib.o
	g++ -o shale -rdynamic shale.o shalelib.o -ldl

shale.o: shale.h shale.cpp shalelib.h
	g++ -c -o shale.o shale.cpp

shalelib.o: shalelib.h shalelib.cpp
	g++ -c -o shalelib.o shalelib.cpp

maths.o: maths.cpp shalelib.h
	g++ -fPIC -c -o maths.o maths.cpp

maths.so: maths.o
	g++ -shared -o maths.so maths.o

array.o: array.cpp shalelib.h
	g++ -fPIC -c -o array.o array.cpp

array.so: array.o
	g++ -shared -o array.so array.o

/usr/local/bin/shale: shale
	sudo rm -f /usr/local/bin/shale
	sudo cp shale /usr/local/bin/shale

/usr/local/lib/shale/maths.so: maths.so
	[ -d /usr/local/lib/shale ] || sudo mkdir /usr/local/lib/shale
	sudo rm -f /usr/local/lib/shale/maths.so
	sudo cp maths.so /usr/local/lib/shale/maths.so

/usr/local/lib/shale/array.so: array.so
	[ -d /usr/local/lib/shale ] || sudo mkdir /usr/local/lib/shale
	sudo rm -f /usr/local/lib/shale/array.so
	sudo cp array.so /usr/local/lib/shale/array.so
