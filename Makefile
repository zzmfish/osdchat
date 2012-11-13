FLAGS=-Wl,--export-dynamic -Wall -O1 -shared -fPIC -lpthread `pkg-config --libs --cflags gtk+-2.0 cairo`
SRCS=osdchat.c osd_window.c config.c

ALL:
	gcc $(FLAGS) $(SRCS) -o osdchat.so

install:
	cp osdchat.so ~/.xchat2/

testwin:
	gcc -DDEBUG -DTEST_WINDOW -ggdb -lpthread `pkg-config --libs --cflags gtk+-2.0 cairo` osd_window.c
