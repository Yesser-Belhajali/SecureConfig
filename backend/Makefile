CC = gcc
CFLAGS = -Wall -g -I/usr/local/include/openscap
LDFLAGS = -L/usr/local/lib -lopenscap -lcjson -lmicrohttpd

COMMON_SRCS = scap_service.c json_utils.c

TEST_TARGET = ./build/scap-engine
TEST_SRCS = ./Exploration/list_profile_rules.c


SERVER_TARGET = ./build/http_server
SERVER_SRCS = http_server.c $(COMMON_SRCS)





server: $(SERVER_TARGET)

test: $(TEST_TARGET)

$(TEST_TARGET): $(TEST_SRCS)
	mkdir -p build
	$(CC) $(CFLAGS) -o $(TEST_TARGET) $(TEST_SRCS) $(LDFLAGS)

$(SERVER_TARGET): $(SERVER_SRCS)
	mkdir -p build
	$(CC) $(CFLAGS) -o $(SERVER_TARGET) $(SERVER_SRCS) $(LDFLAGS)


clean:
	rm -f $(TEST_TARGET) $(SERVER_TARGET)

.PHONY: clean all