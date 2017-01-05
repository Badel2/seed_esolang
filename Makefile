CC=gcc
CFLAGS=-O2 -Wall
LDFLAGS=
SOURCES=random.c pylong.c seed_program.c main.c
OBJECTS=$(SOURCES:.c=.o)
EXECUTABLE=seed_esolang

all: $(SOURCES) $(EXECUTABLE)
	
$(EXECUTABLE): $(OBJECTS)
	$(CC) $(LDFLAGS) $(OBJECTS) -o $@

.cpp.o:
	$(CC) $(CFLAGS) $< -o $@


