CC = gcc
RM = -rm -rf
AR = ar
CFLAGS = -Wall
targets = librvm.a objects

all: $(targets)

clean:
	$(RM) *.o $(targets) 
	
cleanall: clean
	rm -rf rvm_segments*
	
objects: rvm.c 
	gcc -g -c rvm.c `pkg-config --cflags --libs glib-2.0`

librvm.a: objects
	$(AR) rcs $@ *.o
	$(RM) *.o