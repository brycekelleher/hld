OBJECTS	= hldc2.o
CXX = clang
CXXFLAGS = -ggdb -Wall
LDFLAGS = -ggdb -lGL -lglut -lm

#ifeq ($(APPLE),1)
CXXFLAGS += -I/usr/X11R6/include -DGL_GLEXT_PROTOTYPES
LDFLAGS = -L/usr/X11R6/lib
LDLIBS  = -ggdb -lGL -lglut -lm
#endif

hldc2: hldc2.o

clean:
	rm -rf hldc2 *.o
