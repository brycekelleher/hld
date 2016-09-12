OBJECTS	= hld.o
CXX = clang
CXXFLAGS = -ggdb -Wall
LDFLAGS = -ggdb -lGL -lglut

#ifeq ($(APPLE),1)
CFLAGS += -I/usr/X11R6/include -DGL_GLEXT_PROTOTYPES
LDFLAGS = -L/usr/X11R6/lib
LDLIBS  = -lGL -lglut
#endif

hld: $(OBJECTS)

clean:
	rm -rf hld $(OBJECTS)
