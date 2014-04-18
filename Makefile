#### RVM Library Makefile

CC = gcc
RM = -rm -rf
AR = ar
CFLAGS = -Wall
targets = librvm.a objects tests

all: $(targets)

clean:
	$(RM) *.o $(targets) 
	$(RM) basic abort multi truncate multi-abort
cleanall: clean
	rm -rf rvm_segments*
	
objects: rvm.c 
	gcc -g -c rvm.c `pkg-config --cflags --libs glib-2.0`

librvm.a: objects
	$(AR) rcs $@ *.o
	$(RM) *.o

tests: librvm.a
	gcc -o basic basic.c librvm.a `pkg-config --libs --cflags glib-2.0`
	gcc -o abort abort.c librvm.a `pkg-config --libs --cflags glib-2.0`
	gcc -o multi multi.c librvm.a `pkg-config --libs --cflags glib-2.0`   
	gcc -o truncate truncate.c librvm.a `pkg-config --libs --cflags glib-2.0`
	gcc -o multi-abort multi-abort.c librvm.a `pkg-config --libs --cflags glib-2.0`