### ---------------------------------------------------------------------------
### EXECUTABLES
### ---------------------------------------------------------------------------

CC = g++

### ---------------------------------------------------------------------------
### FILES
### ---------------------------------------------------------------------------

SOURCES = $(wildcard *.cpp)
HEADERS = $(SOURCES:.cpp=.h)
OBJECTS = $(SOURCES:.cpp=.o)

OUTPUT = bin/avr_prog

### ---------------------------------------------------------------------------
### PARAMETERS
### ---------------------------------------------------------------------------

STANDARD = -std=c++11
WARNINGS = -Wall
TUNNING = -O0 -fshort-enums
DEBUGING = -g -fno-omit-frame-pointer
ERRORS = -Werror
FLAGS = $(STANDARD) $(WARNINGS) $(ERRORS) $(TUNNING) $(DEBUGING)

LIBGTK=`pkg-config --cflags --libs gtkmm-3.0`
LIBXML=`pkg-config --cflags --libs libxml++-2.6`
THREADS=`pkg-config --libs gthread-2.0`

LIBS = $(LIBGTK) $(LIBXML) $(THREADS) -rdynamic -lm

### ---------------------------------------------------------------------------
### TARGETS
### ---------------------------------------------------------------------------

bin: $(OBJECTS)
	$(CC) $(FLAGS) $(OBJECTS) -o $(OUTPUT) $(LIBS)

clean:
	-rm -f *.o *.lst *~ ./bin/*~ $(OUTPUT)

### ---------------------------------------------------------------------------
### RECIPES
### ---------------------------------------------------------------------------

%.o: %.cpp
	 $(CC) $(FLAGS) -c -o $@ $^ $(LIBS)
