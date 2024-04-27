
#include <stdio.h>
#include <stdlib.h>
#include "fileutils.h"

int main() {
    
    uint8_t buf[MAX_PKT_SIZE] = {
        WATCH_NOTIFY,
        1,
        12,
        'h',
		'e',
		'l',
		'l',
		'o',
		'\0',
		'h',
		'e',
		'l',
		'l',
		'o',
		'\0',
    };


    char *lol = "hey there whats up";
	char *jej = "whatcha doin son";

	char *testData[2] = {
		lol,
		jej
	};

    struct watch_msg * myMsg = (struct watch_msg *)malloc(sizeof(struct watch_msg));
    watch_msg_init(myMsg);



    deserialize(buf, myMsg);
	
    //print_packet(myMsg);

	
    
    
    printf("--START--\n"); 
    
    uint8_t cereal[1024] = {0};
    
    serialize(cereal, myMsg);

    
    for (int i = 0; i < 3 + cereal[3]; i++) {
		printf("%x ", cereal[i]);
	}
    printf("\n");

    myMsg->data = testData;
    myMsg->len = 2;
	serialize(cereal, myMsg);

	for (int i = 0; i < 3 + cereal[3]; i++) {
		printf("%x ", cereal[i]);
	}
    printf("\n");





}


/*

    struct watch_msg *myMsg = malloc(sizeof(struct watch_msg));

    watch_msg_init(myMsg);

    myMsg->action = 2;
    myMsg->option = 2;

    myMsg->len = 3;
    

    myMsg->data = (char **)malloc(sizeof(myMsg->len * sizeof(char) * 5));

    
    for (int i = 0; i < myMsg->len; ++i) {
        myMsg->data[i] = (char *)malloc(sizeof(5 * sizeof(char)));
    }

    myMsg->data[0] = "bye"; 
    myMsg->data[1] = "cat";
    myMsg->data[2] = "dog";

    

    uint8_t buf[255];


    serialize(buf, myMsg);


    // fprintf(stdout, "%d", buf[0]);
    // fprintf(stdout, "%d", buf[1]);
    // fprintf(stdout, "%d", buf[2]);

    for (int i = 0; i < 12; ++i) {
        fprintf(stdout, "%d and %d\n", buf[i + MIN_PKT_SIZE], i);
    
    }
    
    watch_msg_reset(myMsg);

    
    deserialize(buf, myMsg);



    return 0;

*/