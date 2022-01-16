PORT=53509 
CFLAGS = -g -Wall -std=gnu99

all: auction_server auction_client

auction_server: auction_server.o
	gcc -DPORT=${PORT} ${CFLAGS} -o $@ $^

auction_client: auction_client.o
	gcc ${CFLAGS} -o $@ $^

%.o: %.c
	gcc ${CFLAGS} -c $<

clean:
	rm -f *.o auction_server auction_client
