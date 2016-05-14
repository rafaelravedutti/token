#define MACHINE_TAG             'A'
#define MACHINE_HOST            "dalmore"
#define MACHINE_DEST_HOST       "talisker"
#define MACHINE_SOURCE_PORT     63921
#define MACHINE_DEST_PORT       63922
#define MAX_MSG_LENGTH          1024
#define MAX_MSG_PER_TIME        20
#define BAT_TIME                0.5
#define AVAIABLE_TAGS           "ABCD"

/* Inicia o bastão */
#define START_BAT

#include "ring.c"
