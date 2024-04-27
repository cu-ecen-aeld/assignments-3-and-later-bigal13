#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <syslog.h>
#include <errno.h>
#include <signal.h>
#include <fcntl.h>
#include <arpa/inet.h>
#include <sys/socket.h>

#define PORT (9000)
#define MAX_BUFFER (1024)
#define DATA_FILE "/var/tmp/aesdsocketdata"

char **buf;
static volatile int active = 1;
void handle_signal(int sig);

int main(int argc, char *argv[]) {
    remove(DATA_FILE); // start fresh

    // Parse inputs and look for daemon mode flag
    int daemon_flag = 0;
    if (argc > 1 && strcmp(argv[1], "-d") == 0) {
        daemon_flag = 1;
    }

    // Set up buffer for input string
    buf = malloc(sizeof(char *));
    *buf = malloc(MAX_BUFFER);

    // Set up signal handlers
    struct sigaction sig_act;
    memset(&sig_act, 0, sizeof(sig_act));
    sig_act.sa_handler = handle_signal;
    sig_act.sa_flags = 0;
    // Let's initialize an empty signal set and register SIGINT and SIGTERM signals for our function
    if ((sigemptyset(&sig_act.sa_mask) == -1) ||
         sigaction(SIGINT, &sig_act, NULL) == -1 ||
         sigaction(SIGTERM, &sig_act, NULL) == -1) {
            perror("Failed to set signal handler");
            exit(EXIT_FAILURE);
    };

    // Open syslog to start logging
    openlog("aesdsocket", LOG_PID, LOG_USER);
    syslog(LOG_INFO, "Starting aesdsocket");

    // Create a stream socket bound to port 9000
    int sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd == -1) {
        perror("Failed to create socket");
        exit(EXIT_FAILURE);
    }

    // set SO_REUSEADDR
    const int enable = 1;
    if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &enable, sizeof(int)) < 0) {
        perror("Failed to set socket options");
        exit(EXIT_FAILURE);
    }

    // Now bind socket to port; sockaddr maps to a server sockeet location
    struct sockaddr_in server_addr;
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(PORT);

    // call bind
    if (bind(sockfd, (struct sockaddr *)&server_addr, sizeof(server_addr)) == -1) {
        perror("Failed to bind");
        exit(EXIT_FAILURE);
    }

    // listen; allow 10 pending connections before refusing
    if (listen(sockfd, 10) == -1) {
        perror("Failed to listen");
        exit(EXIT_FAILURE);
    }

    // make this a daemon if the flag is set
    if (daemon_flag) {
        syslog(LOG_DEBUG, "Creating Daemon");
        pid_t pid = fork();
        if (pid < 0) {
            perror("Failed to fork");
            exit(EXIT_FAILURE);
        }

        if (pid > 0) {
            // we are in the parent process so exit
            exit(EXIT_SUCCESS);
        }

        // here we are the child, aka the daemon in this case
        // create a new session with no controlling tty
        if (setsid() < 0) {
            perror("Failed to setsid");
            exit(EXIT_FAILURE);
        }

        // change directory to root of file system
        if (chdir("/") != 0) {
            perror("Failed to change directory");
            exit(EXIT_FAILURE);
        }

        // redirect std* to /dev/null
        // first, we need to open /dev/null and get the fd
        int dev_null = open("/dev/null", O_RDWR);

        // stdin; STDIN_FILENO, also 0
        if (dup2(dev_null, STDIN_FILENO) == -1) {
            perror("Failed to redirect stdin to /dev/null");
            exit(EXIT_FAILURE);
        }

        // stdout; STDOUT_FILENO, also 1
        if (dup2(dev_null, STDOUT_FILENO) == -1) {
            perror("Failed to redirect stdout to /dev/null");
            exit(EXIT_FAILURE);
        }

        // stdin; STDERR_FILENO, also 2
        if (dup2(dev_null, STDERR_FILENO) == -1) {
            perror("Failed to redirect stderr to /dev/null");
            exit(EXIT_FAILURE);
        }

        // close the dev null fd
        if (close(dev_null) == -1) {
            perror("Failed to close dev null fd");
            exit(EXIT_FAILURE);
        }

    }

    while (active) {
        // create the client socket and accept a connection
        struct sockaddr client_addr;
        socklen_t client_addr_len = sizeof(client_addr);

        int clientfd = accept(sockfd, &client_addr, &client_addr_len); 
        if (clientfd == -1) {
            perror("Failed to accept");
            continue;
        }

        // get the client ip and log it
        struct sockaddr_in *client_inaddr = (struct sockaddr_in *)&client_addr;
        char *client_ip = inet_ntoa(client_inaddr->sin_addr);
        syslog(LOG_INFO, "Accepted connection from %s", client_ip);

        // open the data file
        int outputfd = open(DATA_FILE, O_RDWR | O_CREAT | O_APPEND, 0644);
        if (outputfd < 0) {
            perror("Failed to open data file");
            close(clientfd);
            continue;
        }

        // move cursor to the end to get ready to write
        if (lseek(outputfd, 0, SEEK_END) == -1) {
            perror("Failed to move cursor");
            close(clientfd);
            continue;
        }

        // here is where we receive and read the clients strings
        char inbuf[MAX_BUFFER];
        int num_bytes;
        *buf[0] = '\0'; // clear the string
        size_t outbuf_len = 0; // this is the length of the buffer we hold the stream in
        while ((num_bytes = recv(clientfd, inbuf, sizeof(inbuf), 0)) > 0) {
            syslog(LOG_DEBUG, "inside while receive loop");
            // we have to wait for the newline to write to the data file; since we assume there are no null characters
            // we can use string handling functions to build up the string before writing it
            syslog(LOG_DEBUG, "Received %d number of bytes\n", num_bytes);
            int newline = 0;
            int i = 0;
            for (; i < num_bytes; i++) {
                if (inbuf[i] == '\n') {
                    newline = 1;
                    syslog(LOG_DEBUG, "newline at position %d of input stream", i+1);
                    break;
                }
            }
            
            // i is equivalent to the position of the newline character in the stream
            // if this index is less than the nuber of bytes received, set the currlen to the number
            // of bytes we have to add to the buf string
            int currlen = num_bytes;
            if (i + 1 < num_bytes) { // if the newline is not at the end of the bytes we received
                currlen = i + 1; // set currlen to the position of the newline character
            }

            // append the received data to the buf string up to currlen
            size_t max_buffer_len = (size_t)MAX_BUFFER;
            if (max_buffer_len < (outbuf_len + currlen + 1)) {
                max_buffer_len += outbuf_len + 2 * currlen;
                *buf = realloc(*buf, max_buffer_len);
            }
            strncat(*buf, inbuf, currlen);

            outbuf_len += currlen; // update the buffer length

            // if we have a newline, write the buffer out to the file
            if (newline) {
                syslog(LOG_DEBUG, "inside newline logic");
                // write the line to the file
                if (write(outputfd, *buf, strlen(*buf)) == -1) {
                    perror("Failed to write to data file");
                    close(clientfd);
                    close(outputfd);
                    continue;
                }

                // reset string and buf len
                *buf[0] = '\0';
                outbuf_len = 0;

                // send the full content of the data file to the client
                char readbuf[MAX_BUFFER];
                ssize_t n;

                // move to start of the file
                if (lseek(outputfd, 0, SEEK_SET) == -1) {
                    syslog(LOG_DEBUG, "inside error of lseek");
                    perror("Failed to move to start of output file");
                    close(clientfd);
                    close(outputfd);
                    continue;
                }

                syslog(LOG_DEBUG, "right above read output file loop");
                // now we read the data into the buffer and send it to the client
                while ((n = read(outputfd, readbuf, sizeof(readbuf))) > 0) {
                    syslog(LOG_DEBUG, "inside read output file loop");
                    if (send(clientfd, readbuf, n, 0) == -1) {
                        syslog(LOG_DEBUG, "inside send client error");
                        perror("Failed to send data to client");
                        close(clientfd);
                        close(outputfd);
                        continue;
                    };
                }

                if (n == 0) {
                    syslog(LOG_DEBUG, "Read to the end of the file successfully");
                } else if (n < 0) {
                    perror("Error reading the data");
                    continue;
                }

                // we're at the end of the file and we can copy any other bytes after the new line
                // location to our buffer
                // size_t max_buffer_len = (size_t)MAX_BUFFER;
                // if (i < num_bytes) {
                //     outbuf_len = num_bytes - i - 1; // buffer length of the remaining bytes
                //     if (max_buffer_len < outbuf_len + outbuf_len + 1) {
                //         max_buffer_len += outbuf_len + 2 * outbuf_len;
                //         *buf = realloc(*buf, max_buffer_len);
                //     }
                //     strncat(*buf, inbuf[i+1], outbuf_len);
                // }
            }
        }

        close(outputfd);
        close(clientfd);

        syslog(LOG_INFO, "Closed connection from %s", client_ip);
    }

    closelog();
    close(sockfd);
    return 0;
}

void handle_signal(int sig) {
    if (sig == SIGINT || sig == SIGTERM) {
        active = 0;
        syslog(LOG_INFO, "Caught signal, exiting");
        exit(EXIT_SUCCESS);
    }
}