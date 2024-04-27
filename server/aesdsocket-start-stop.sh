#!/bin/sh

# switch the input argument based on if we are starting or stopping
case "$1" in
    start)
        echo "Starting aesdsocket server"
        # make sure to start in daemon mode using -d
        start-stop-daemon -S -n aesdsocket -a /usr/bin/aesdsocket -- -d
        ;;
    stop)
        echo "Stopping aesdsocket server"
        start-stop-daemon -K -n aesdsocket
        ;;
    *)
        echo "Usage: $0 {start|stop}"

    exit 1 # shouldn't get here
esac

exit 0