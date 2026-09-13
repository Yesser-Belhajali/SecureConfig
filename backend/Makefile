CC = gcc

# ============================================================
# Compilation normale
# ============================================================

CFLAGS = -Wall -g -I/usr/local/include/openscap

LDFLAGS = -L/usr/local/lib \
          -lopenscap \
          -lcjson \
          -lmicrohttpd \
          -lpthread


# ============================================================
# Compilation MEMTEST
# ============================================================

MEMTEST_FLAGS = -Wall -Wextra -Werror \
                -Wno-unused-parameter \
                -g \
                -I/usr/local/include/openscap \
                -fsanitize=address,undefined \
                -fno-omit-frame-pointer

MEMTEST_LDFLAGS = -L/usr/local/lib \
                  -fsanitize=address,undefined \
                  -lopenscap \
                  -lcjson \
                  -lmicrohttpd \
                  -lpthread





# ============================================================
# Sources
# ============================================================

COMMON_SRCS = scap_service.c json_utils.c active_operation.c


# ============================================================
# TEST NORMAL
# ============================================================

TEST_TARGET = ./build/scap-engine
TEST_SRCS = ./Exploration/session_scan.c $(COMMON_SRCS)


# ============================================================
# SERVEUR NORMAL
# ============================================================

SERVER_TARGET = ./build/http_server
SERVER_SRCS = http_server.c scan.c remediate.c $(COMMON_SRCS)


# ============================================================
# SERVEUR MEMTEST
# ============================================================

MEMTEST_TARGET = ./build/http_server_memtest
MEMTEST_SRCS = http_server.c scan.c remediate.c $(COMMON_SRCS)


# ============================================================
# TARGETS
# ============================================================

.PHONY: all server test memtest clean

all: server test

server: $(SERVER_TARGET)

test: $(TEST_TARGET)

memtest: $(MEMTEST_TARGET)


# ============================================================
# SERVEUR NORMAL
# ============================================================

$(SERVER_TARGET): $(SERVER_SRCS) | build
	$(CC) $(CFLAGS) $(SERVER_SRCS) -o $(SERVER_TARGET) $(LDFLAGS)


# ============================================================
# TEST NORMAL
# ============================================================

$(TEST_TARGET): $(TEST_SRCS) | build
	$(CC) $(CFLAGS) $(TEST_SRCS) -o $(TEST_TARGET) $(LDFLAGS)


# ============================================================
# MEMTEST
# ============================================================

$(MEMTEST_TARGET): $(MEMTEST_SRCS) | build
	$(CC) $(MEMTEST_FLAGS) $(MEMTEST_SRCS) -o $(MEMTEST_TARGET) $(MEMTEST_LDFLAGS)


# ============================================================
# BUILD
# ============================================================

build:
	mkdir -p build


# ============================================================
# CLEAN
# ============================================================

clean:
	rm -f \
		$(TEST_TARGET) \
		$(SERVER_TARGET) \
		$(MEMTEST_TARGET)