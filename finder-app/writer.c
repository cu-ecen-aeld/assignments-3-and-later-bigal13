#include <stdio.h>
#include <stdlib.h>
#include <syslog.h>

int main(int argc, char *argv[]) {
    // Check if correct number of arguments is provided
    if (argc != 3) {
        fprintf(stderr, "Usage: %s <filename> <content>\n", argv[0]);
        return 1;
    }

    // Open file for writing
    FILE *file = fopen(argv[1], "w");
    if (file == NULL) {
        syslog(LOG_USER, "Failed to open file '%s' for writing.", argv[1]);
        perror("fopen");
        return 1;
    }

    // Write content to the file
    fprintf(file, "%s", argv[2]);
    fclose(file);

    // Log the operation using syslog
    openlog("writer", LOG_CONS | LOG_PID | LOG_NDELAY, LOG_USER);
    syslog(LOG_DEBUG, "Writing '%s' to '%s'", argv[2], argv[1]);
    closelog();

    return 0;
}