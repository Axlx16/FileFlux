/*

fileutils.h - Network protocol for fileflux application

*/

#include <stdint.h>
#include <stdbool.h>


/* Defining actions for filewatch to send to filedog */
#define WATCH_TRACK                  0
#define WATCH_QUIT                   1
#define WATCH_NOTIFY                 2
#define WATCH_UNSET                  255 

/* Defining options for each action above */
#define TRACK_ADD                    0
#define TRACK_REMOVE                 1

#define QUIT_USER                    0
#define QUIT_ERROR                   1

#define NOTIFY_REPLY_CREATE          0
#define NOTIFY_REPLY_DELETE          1
#define NOTIFY_REPLY_ACCESS          2
#define NOTIFY_REPLY_CLOSE_WRITE     3
#define NOTIFY_REPLY_MODIFY          4
#define NOTIFY_REPLY_MOVE_SELF       5
#define NOTIFY_REPLY_OTHER           6

#define WATCH_UNSET_UNSET            255

/* Setting minimum and maximum packet sizes */
#define MIN_PKT_SIZE                 3
#define MAX_PKT_SIZE                 255

/* Bounds of action and options sizes */
#define ACTION_SIZE                  2
#define TRACK_SIZE                   1
#define QUIT_SIZE                    1
#define NOTIFY_SIZE                  6

/* Minimum sizes for data in buffer */
#define TRACK_MIN                    1
#define QUIT_MIN                     0
#define NOTIFY_MIN                   2


/* Packet structure that will be sent between filewatch and filedog */
struct watch_msg {
    uint8_t action;
    uint8_t option;
    uint8_t size;

    char **data;
    int len;
};

void print_packet(const struct watch_msg *msg);

void watch_msg_init(struct watch_msg *msg);
void watch_msg_reset(struct watch_msg *msg);

bool verify_msg_bounds(const struct watch_msg *msg);
bool verify_msg_size(const struct watch_msg *msg);
bool verify_buf_bounds(const uint8_t buf[MAX_PKT_SIZE]);
bool verify_buf_size(const uint8_t buf[MAX_PKT_SIZE]);

void serialize(uint8_t buf[MAX_PKT_SIZE], struct watch_msg *msg);
void deserialize(uint8_t buf[MAX_PKT_SIZE], struct watch_msg *msg);


