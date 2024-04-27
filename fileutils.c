/*

fileutils.c - Network protocol for fileflux application

*/

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include <unistd.h>

#include "fileutils.h"

void print_packet(const struct watch_msg *msg) {
	printf("--PACKET--\n");
	
    printf("cmd: %x\ntype: %x\nsize: %d\n",
		msg->action, msg->option, msg->size);

	for (int i = 0; i < msg->len; i++) {
		printf("data %d: %s\n", i, msg->data[i]);
	}

}


/* Constructor for watch_msg struct */
void watch_msg_init(struct watch_msg *msg) {
    msg->action = WATCH_UNSET;
    msg->option = WATCH_UNSET_UNSET;
    msg->size = MIN_PKT_SIZE;

    msg->data = NULL;
    msg->len = 0;

 
}

/* Reset for watch_msg struct */
void watch_msg_reset(struct watch_msg *msg) {
    if (msg->data == NULL) {
        watch_msg_init(msg);
        return;
    }

    for (int i = 0; i < msg->len; ++i) {
        if(msg->data[i] != NULL) {
            free(msg->data[i]);
        }
    }

    free(msg->data);
    watch_msg_init(msg);
}

/* Ensuring action, options, and size are within bounds */
bool verify_msg_bounds(const struct watch_msg *msg) {
    
    if (msg->action > ACTION_SIZE) {
        fprintf(stderr, "Invalid action\n");
        return false; 
    }

    if ((msg->action == WATCH_TRACK) && (msg->option > TRACK_SIZE)) {
        fprintf(stderr, "Invalid track option\n");
        return false; 
    }
    
    if ((msg->action == WATCH_QUIT) && (msg->option > QUIT_SIZE)) {
        fprintf(stderr, "Invalid quit option\n");
        return false; 
    }
    
    if ((msg->action == WATCH_NOTIFY) && (msg->option > NOTIFY_SIZE)) {
        fprintf(stderr, "Invalid notify option\n");
        return false; 
    }

    return true;
    
}

/* Ensuring size of watch_msg struct is within min & max packet size bounds*/
bool verify_msg_size(const struct watch_msg *msg) {
    if (msg->size < MIN_PKT_SIZE) {
        fprintf(stderr, "Packet size to small\n");
        return false; 
    }

    if (msg->size > MAX_PKT_SIZE) {
        fprintf(stderr, "Packet size to large\n");
        return false;
    }

    return true;
}


bool verify_buf_bounds(const uint8_t buf[MAX_PKT_SIZE]) {

    if (buf[0] > ACTION_SIZE || buf[0] < 0) {
        fprintf(stderr, "Invalid Action");
        return false; 
    }

    if ((buf[0] == WATCH_TRACK) && (buf[1] > TRACK_SIZE)) {
        fprintf(stderr, "Invalid track option\n");
        return false; 
    }
    
    if ((buf[0] == WATCH_QUIT) && (buf[1] > QUIT_SIZE)) {
        fprintf(stderr, "Invalid quit option\n");
        return false; 
    }
    
    if ((buf[0] == WATCH_NOTIFY) && (buf[1] > NOTIFY_SIZE)) {
        fprintf(stderr, "Invalid notify option\n");
        return false; 
    }

    return true;

}


bool verify_buf_size(const uint8_t buf[MAX_PKT_SIZE]) {
    
    if ((buf[0] == WATCH_TRACK) && (buf[2] < TRACK_MIN)) {
        fprintf(stderr, "Invalid enough data for track\n");
        return false; 
    }
    
    if ((buf[0] == WATCH_QUIT) && (buf[2] != QUIT_MIN)) {
        fprintf(stderr, "Invalid enough data for quit\n");
        return false; 
    }
    
    if ((buf[0] == WATCH_NOTIFY) && (buf[2] < NOTIFY_MIN)) {
        fprintf(stderr, "Invalid enough data for notify\n");
        return false; 
    }

    return true;


}

/* Serializing watch_msg structure and transfering it to buf */
void serialize(uint8_t buf[MAX_PKT_SIZE], struct watch_msg *msg) {

    int charCount = 0;
    int runningCharCount = 0;

    /* Validating watch_msg struct */
    if ((!verify_msg_bounds(msg)) || (!verify_msg_size(msg))) {
        
        exit(EXIT_FAILURE);
    }


    buf[0] = msg->action;
    buf[1] = msg->option;

    
    for (int i = 0; i < msg->len; ++i) {
        if (msg->data[i] == NULL) {
            fprintf(stderr, "Invalid Data\n");
            exit(EXIT_FAILURE);
        }

        for (int j = 0; j < MAX_PKT_SIZE - MIN_PKT_SIZE - charCount; ++j) {
            runningCharCount += 1; 
            buf[MIN_PKT_SIZE + charCount + j] = msg->data[i][j];

            if (msg->data[i][j] == '\0') {
                break;
            }
        }

        charCount += runningCharCount;
        runningCharCount = 0;
    }


    buf[2] = charCount;
    return;

}

/* Deserializing buf and transferring it to watch_msg structure*/
void deserialize(uint8_t buf[MAX_PKT_SIZE], struct watch_msg *msg) {

    int numberOfStrings = 0;
    int runningStringLength = 0;
    int currString = 0;
    int dataPosition = 0;
    int *stringSizes = NULL;

    /* Validating buffer data */
    if (!verify_buf_bounds(buf)) {
        exit(EXIT_FAILURE);    
    }
    
    msg->action = buf[0];
    msg->option = buf[1];

    if (!verify_buf_size(buf)) {
        exit(EXIT_FAILURE);
    }



    if (buf[2] > MAX_PKT_SIZE - MIN_PKT_SIZE) {
        fprintf(stderr, "Data is too long\n");
        exit(EXIT_FAILURE);
    } 

    
    msg->size = buf[2];

    stringSizes = (int *)malloc(sizeof(int) * msg->size);

    /* Populating watch_msg structure */
    for (int i = MIN_PKT_SIZE; i < msg->size + MIN_PKT_SIZE; ++i) {
        runningStringLength += 1;          
        if (i > MIN_PKT_SIZE + msg->size) {
            free(stringSizes);
            exit(EXIT_FAILURE);
        } 
        
        if (buf[i] == '\0') {
            numberOfStrings += 1;
            stringSizes[currString] = runningStringLength;
            runningStringLength = 0;
            currString += 1;
        }

    }


    msg->data = (char **)malloc(sizeof(char *) * numberOfStrings);

    for (int i = 0; i < numberOfStrings; ++i) {
        msg->data[i] = (char *)malloc(sizeof(char) * stringSizes[i]); 
        if (buf[dataPosition + MIN_PKT_SIZE] < ' ' || buf[dataPosition + MIN_PKT_SIZE] > '~') {
            free(stringSizes); 
            exit(EXIT_FAILURE);
        } 
        //printf("Size of this string is: %d\n", stringSizes[i]); 
        for (int j = 0; j < stringSizes[i]; ++j) {
            msg->data[i][j] = buf[MIN_PKT_SIZE + dataPosition];
            //printf("%c\n", msg->data[i][j]);
            dataPosition += 1;
        }

    }

    free(stringSizes);

    msg->len = numberOfStrings;
    return;


}


