GCCPARAMS = -g -Iinclude -std=c++11 -lsfml-graphics -lsfml-window -lsfml-system

OBJECTS=obj/main.o \
	obj/chip8.o \

obj/%.o: src/%.cpp
	mkdir -p $(@D)
	g++ $(GCCPARAMS) -c -o $@ $<

all: chip-8

chip-8: $(OBJECTS)
	g++ $(OBJECTS) -o chip-8 $(GCCPARAMS)

clean:
	rm -rf chip-8 obj
