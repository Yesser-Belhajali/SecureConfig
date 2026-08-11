CC = gcc

CFLAGS = -Wall -g -I/usr/local/include/openscap
LDFLAGS = -L/usr/local/lib -lopenscap -lcjson -lmicrohttpd

SANITIZE_FLAGS = -fsanitize=address,undefined -fno-omit-frame-pointer

COMMON_SRCS = scap_service.c json_utils.c


TEST_TARGET = ./build/scap-engine
TEST_SRCS = ./Exploration/list_profiles.c $(COMMON_SRCS)

SERVER_TARGET = ./build/http_server
SERVER_SRCS = http_server.c $(COMMON_SRCS)

MEMTEST_TARGET = ./build/test_memory
MEMTEST_SRCS = ./Exploration/test_memory.c $(COMMON_SRCS)


server: $(SERVER_TARGET)

test: $(TEST_TARGET)

memtest: $(MEMTEST_TARGET)

asan: $(MEMTEST_TARGET)


$(TEST_TARGET): $(TEST_SRCS) | build
	$(CC) $(CFLAGS) -o $(TEST_TARGET) $(TEST_SRCS) $(LDFLAGS)


$(SERVER_TARGET): $(SERVER_SRCS) | build
	$(CC) $(CFLAGS) -o $(SERVER_TARGET) $(SERVER_SRCS) $(LDFLAGS)


$(MEMTEST_TARGET): $(MEMTEST_SRCS) | build
	$(CC) $(CFLAGS) $(SANITIZE_FLAGS) -o $(MEMTEST_TARGET) \
		$(MEMTEST_SRCS) $(LDFLAGS) $(SANITIZE_FLAGS)


build:
	mkdir -p build


clean:
	rm -f $(TEST_TARGET) $(SERVER_TARGET) $(MEMTEST_TARGET)


.PHONY: clean server test asan memtest