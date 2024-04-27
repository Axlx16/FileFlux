/*
filedog.c - 
*/

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include <unistd.h>

#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/wait.h>
#include <signal.h>
#include <poll.h>

#include <time.h>
#include <sys/inotify.h>

#define PORT "6160"


/* Creating listener socket to handle new connections */
int get_listener_socket(void) {
    int listener;     // Listening socket descriptor
    int yes=1;        // For setsockopt() SO_REUSEADDR, below
    int rv;

    struct addrinfo hints, *ai, *p;

    // Get us a socket and bind it
    memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = AI_PASSIVE;
    if ((rv = getaddrinfo(NULL, PORT, &hints, &ai)) != 0) {
        fprintf(stderr, "selectserver: %s\n", gai_strerror(rv));
        exit(1);
    }
    
    for(p = ai; p != NULL; p = p->ai_next) {
        listener = socket(p->ai_family, p->ai_socktype, p->ai_protocol);
        if (listener < 0) { 
            continue;
        }
        
        // Lose the pesky "address already in use" error message
        setsockopt(listener, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int));

        if (bind(listener, p->ai_addr, p->ai_addrlen) < 0) {
            close(listener);
            continue;
        }

        break;
    }

    // If we got here, it means we didn't get bound
    if (p == NULL) {
        return -1;
    }

    freeaddrinfo(ai); // All done with this

    // Listen
    if (listen(listener, 10) == -1) {
        return -1;
    }

    return listener;
}

/* Adding new connection to list of file descriptors */
void add_to_pfds(struct pollfd *pfds[], int newfd, int *fd_count, int *fd_size) {
    // If we don't have room, add more space in the pfds array
    if (*fd_count == *fd_size) {
        *fd_size *= 2; // Double it

        *pfds = realloc(*pfds, sizeof(**pfds) * (*fd_size));
    }

    (*pfds)[*fd_count].fd = newfd;
    (*pfds)[*fd_count].events = POLLIN; // Check ready-to-read

    (*fd_count)++;
}

/* Deleting new connection to list of file descriptors */
void del_from_pfds(struct pollfd pfds[], int i, int *fd_count)
{
    // Copy the one from the end over this one
    pfds[i] = pfds[*fd_count-1];

    (*fd_count)--;
}


int main (int argc, char **argv) {


    /* Network setup */
    int listener;
    int newfd;

    struct sockaddr_storage remoteaddr;
    socklen_t addrlen;

    char remoteIP[INET6_ADDRSTRLEN];
    
    int fd_count; 
    int fd_size = 5;
    struct pollfd *pfds = malloc(sizeof(struct pollfd *) * fd_size);

    listener = get_listener_socket();

    if (listener == -1) {
        fprintf(stderr, "Error retrieving listener socket?\n");
        exit(EXIT_FAILURE);
    }

    pfds[0].fd = listener;
    pfds[0].events = POLLIN; // Reporting ready to read on oncoming connection

    fd_count = 1; // since we have the listener


    /* Notification setup */
    int fd;
    int *wd;


    char buf[4096];

    const struct inotify_event *event;
    const uint32_t eventFlags = IN_CREATE | IN_DELETE | IN_ACCESS | IN_CLOSE_WRITE | IN_MODIFY | IN_MOVE_SELF;
    int len;

    char *basePath = NULL;
    char *subString = NULL;

    char *logMessage = NULL;

    int readLength = 0;

    time_t t;
    struct tm tm;


    /* At least 1 provided filename upon running program */
    if (argc < 2) {
        fprintf(stderr, "Usage: %s PATH\n", argv[0]);
        exit(EXIT_FAILURE);
    }
    
    wd = calloc(argc, sizeof(int));
    fd = inotify_init();
    
    if (fd == -1) {
        fprintf(stderr, "Error initializing inotify instance\n");
        exit(EXIT_FAILURE);
    }
    
    for (int i = 1; i < argc; ++i) {
        wd[i] = inotify_add_watch(fd, argv[i], eventFlags); // argv[i] being added to fd

        if (wd[i] == -1) {
            fprintf(stderr, "Could not add file %s, to queue\n", argv[i]);
            exit(EXIT_FAILURE);
        }
    }

    

   

    /* Returning basefile path from argv[1] */
    basePath = (char *)malloc(sizeof(char) * (strlen(argv[1]) + 1));
    strcpy(basePath, argv[1]);

    subString = strtok(basePath, "/");
    while (subString != NULL) {
        basePath = subString;
        subString = strtok(NULL, "/");
    }


    // Good luck
    for(;;) {
        int poll_count = poll(pfds, fd_count, -1);

        if (poll_count == -1) {
            perror("poll");
            exit(1);
        }

        // Run through the existing connections looking for data to read
        for(int i = 0; i < fd_count; i++) {

            // Check if someone's ready to read
            /* THIS CONDITION SHOULD BE FINE, IT ONLY ACCEPTS NEW CONNECTIONS */
            if (pfds[i].revents & POLLIN) { // We got one!!

                if (pfds[i].fd == listener) {
                    // If listener is ready to read, handle new connection

                    addrlen = sizeof remoteaddr;
                    newfd = accept(listener,
                        (struct sockaddr *)&remoteaddr,
                        &addrlen);

                    if (newfd == -1) {
                        perror("accept");
                    } else {
                        add_to_pfds(&pfds, newfd, &fd_count, &fd_size);

                        printf("pollserver: new connection from %s on "
                            "socket %d\n",
                            inet_ntop(remoteaddr.ss_family,
                                get_in_addr((struct sockaddr*)&remoteaddr),
                                remoteIP, INET6_ADDRSTRLEN),
                            newfd);
                    }
                } else {
                    // If not the listener, we're just a regular client
                    int nbytes = recv(pfds[i].fd, buf, sizeof buf, 0);

                    int sender_fd = pfds[i].fd;

                    if (nbytes <= 0) {
                        // Got error or connection closed by client
                        if (nbytes == 0) {
                            // Connection closed
                            printf("pollserver: socket %d hung up\n", sender_fd);
                        } else {
                            perror("recv");
                        }

                        close(pfds[i].fd); // Bye!

                        del_from_pfds(pfds, i, &fd_count);

                    } else {
                        // We got some good data from a client

                        for(int j = 0; j < fd_count; j++) {
                            // Send to everyone!
                            int dest_fd = pfds[j].fd;

                            // Except the listener and ourselves
                            if (dest_fd != listener && dest_fd != sender_fd) {
                                if (send(dest_fd, buf, nbytes, 0) == -1) {
                                    perror("send");
                                }
                            }
                        }
                    }
                } // END handle data from client
            } // END got ready-to-read from poll()
        } // END looping through file descriptors
    } // END for(;;)--and you thought it would never end!




    while (true) {

        basePath = NULL;
        subString = NULL;

        readLength = read(fd, buf, sizeof(buf));

        if (readLength == -1) {
            fprintf(stderr, "Error reading file descriptor\n");
            exit(EXIT_FAILURE);
        }

        for (char *ptr = buf; ptr < buf + readLength; 
                ptr += sizeof(struct inotify_event) + event->len) {
            
            logMessage = NULL;
            event = (const struct inotify_event *) ptr;

            if (event->mask & IN_CREATE) {
                logMessage = "File created!\n";    
            }
            
            if (event->mask & IN_DELETE) {
                logMessage = "File deleted!\n";    
            }
            
            if (event->mask & IN_ACCESS) {
                logMessage = "File accesed!\n";    
            }
            
            if (event->mask & IN_CLOSE_WRITE) {
                logMessage = "File written to and closed!\n";    
            }
            
            if (event->mask & IN_MODIFY) {
                logMessage = "File modified!\n";    
            }
            
            if (event->mask & IN_MOVE_SELF) {
                logMessage = "File moved!\n";    
            }

            if (logMessage == NULL) {
                continue;
            }

            for (int i = 0; i < argc; ++i) {
                if (wd[i] == event->wd) {

                    basePath = (char *)malloc(sizeof(char) * (strlen(argv[i]) + 1));
                    strcpy(basePath, argv[i]);

                    subString = strtok(basePath, "/");
                    while (subString != NULL) {
                        basePath = subString;
                        subString = strtok(NULL, "/");
                    }

                }
            }

            time(&t);
            tm = *localtime(&t);
            fprintf(stdout, "[%02d:%02d:%02d] %s: %s", tm.tm_hour, tm.tm_min, tm.tm_sec, basePath, logMessage); 

            free(basePath); 

        } 

    }

}



