CC=gcc
CPPFLAGS=-Wall -g -O3 -std=gnu99 -Wno-unused-result -pthread


SRC=image-tagger.o
TARGET=image-tagger

all: $(SRC)
	$(CC) -o $(TARGET) $(SRC) $(CPPFLAGS) -lm

clean:
	rm -f $(TARGET) src/*.o

clean:
	-rm -f *.o
	-rm -f $(TARGET)
