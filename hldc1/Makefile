OBJECTS	= hldc1.o
CXX = clang
CXXFLAGS = -ggdb -Wall
LDFLAGS = -ggdb -lGL -lglut -lm

#ifeq ($(APPLE),1)
CFLAGS += -I/usr/X11R6/include -DGL_GLEXT_PROTOTYPES
LDFLAGS = -L/usr/X11R6/lib
LDLIBS  = -lGL -lglut -lm
#endif

hldc1: hldc1.o

clean:
	rm -rf hldc1 *.o
