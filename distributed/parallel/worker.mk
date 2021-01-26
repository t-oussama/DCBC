WORKER_OUTPUT = bin/worker.bin

FLAGS = -static -pthread -lrt -Wl,--whole-archive -lpthread -Wl,--no-whole-archive -lcrypto -ldl -static-libgcc -static-libstdc++ -O2
SRC_DIR = src

COMMON_DEPENDENCIES = ${SRC_DIR}/common/net/connection/connection.cpp ${SRC_DIR}/common/net/socket/socket.cpp
WORKER_DEPENDENCIES = ${COMMON_DEPENDENCIES} ${SRC_DIR}/worker/ChunkHandler/ChunkHandler.cpp

all:
	g++ ${SRC_DIR}/worker/main.cpp ${WORKER_DEPENDENCIES} ${FLAGS} -o ${WORKER_OUTPUT}
