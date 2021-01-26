#define DECRYPT_CHUNK_FLAG '0'
#define ENCRYPT_CHUNK_FLAG '1' // Used when the master asks a worker to encrypt a chunk
#define SET_CHUNK_FLAG '2'
#define SET_BASE_SALT_FLAG '3' // Used when the master sends salt from worker i-1 to worker i
#define SET_SALT_FLAG '4' // Used wehen the worker needs to send the salt to the master
#define GET_PREV_SALT_FLAG '5' // Get Previous Salt
#define GET_PREV_SALT_RESP_FLAG '6' // Respond to a GET_PREV_SALT_FLAG request
#define SIGNAL_DONE_FLAG '7'
#define RESET_FLAG '8'
#define SIGNAL_RESET_DONE_FLAG '8'
#define SET_OUTPUT_DIR_FLAG '9'
#define GET_CHUNK_FLAG 'A'