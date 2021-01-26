MASTER_OUTPUT = bin/master.bin

# FLAGS = -lpthread -lcrypto -ldl -static-libgcc -static-libstdc++ -O2
FLAGS = -static -pthread -lrt -Wl,--whole-archive -lpthread -Wl,--no-whole-archive -lcrypto -ldl -static-libgcc -static-libstdc++ -O2
SRC_DIR = src

COMMON_DEPENDENCIES = ${SRC_DIR}/common/net/connection/connection.cpp ${SRC_DIR}/common/net/socket/socket.cpp
MASTER_DEPENDENCIES = ${COMMON_DEPENDENCIES} ${SRC_DIR}/master/JobManager/JobManager.cpp ${SRC_DIR}/master/WorkerManager/WorkerManager.cpp ${SRC_DIR}/master/WorkerHandler/WorkerHandler.cpp ${SRC_DIR}/master/JobContext/JobContext.h ${SRC_DIR}/master/SaltsIndex/SaltsIndex.cpp ${SRC_DIR}/master/SaltsIndex/SaltsIndexEntry/SaltsIndexEntry.cpp

all:
	g++ -std=c++11 -static -pthread -lrt -Wl,--whole-archive ${SRC_DIR}/master/main.cpp ${MASTER_DEPENDENCIES}  ${FLAGS} -o ${MASTER_OUTPUT}
