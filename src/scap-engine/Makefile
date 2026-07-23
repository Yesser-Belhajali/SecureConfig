CC = gcc
CFLAGS = -Wall -g -I/usr/local/include/openscap
LDFLAGS = -L/usr/local/lib -lopenscap
TARGET = scap-engine

$(TARGET): main.c
	$(CC) $(CFLAGS) -o $(TARGET) main.c $(LDFLAGS)

clean:
	rm -f $(TARGET)

.PHONY: clean