MASTER_OUTPUT = bin/master.bin
WORKER_OUTPUT = bin/worker.bin

FLAGS = -g -lcrypto -lpthread

SRC_DIR = src
COMMON_DEPENDENCIES = ${SRC_DIR}/common/net/connection/connection.cpp ${SRC_DIR}/common/net/socket/socket.cpp

MASTER_DEPENDENCIES = ${COMMON_DEPENDENCIES} ${SRC_DIR}/master/JobManager/JobManager.cpp ${SRC_DIR}/master/WorkerManager/WorkerManager.cpp ${SRC_DIR}/master/WorkerHandler/WorkerHandler.cpp ${SRC_DIR}/master/JobContext/JobContext.h 
WORKER_DEPENDENCIES = ${COMMON_DEPENDENCIES} ${SRC_DIR}/worker/ChunkHandler/ChunkHandler.cpp

all:
	g++ ${SRC_DIR}/master/main.cpp ${MASTER_DEPENDENCIES}  ${FLAGS} -o ${MASTER_OUTPUT}
	g++ ${SRC_DIR}/worker/main.cpp ${WORKER_DEPENDENCIES} ${FLAGS} -o ${WORKER_OUTPUT}

run:
	./${MASTER_OUTPUT}
	./${WORKER_OUTPUT}
