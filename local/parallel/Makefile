OUTPUT = bin/dcbc.bin

FLAGS = -g -lcrypto -lpthread
SRC_DIR = src

DEPENDENCIES = ${SRC_DIR}/dcbc/dcbc.cpp ${SRC_DIR}/ChunkHandler/ChunkHandler.cpp ${SRC_DIR}/SaltsIndex/SaltsIndex.cpp ${SRC_DIR}/SaltsIndex/SaltsIndexEntry/SaltsIndexEntry.cpp

all:
	g++ -std=gnu++11 ${SRC_DIR}/main.cpp ${DEPENDENCIES}  ${FLAGS} -o ${OUTPUT}
