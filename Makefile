CC = gcc
LD = gcc
CFLAGS = -g -O0 -Wall -I/usr/include/libxml2
LDFLAGS = -lreadline -lsqlite3 -lxml2
RM = /bin/rm -f
OBJS = cli.o config.o feed.o http.o main.o storage.o utils.o
RSS = rss

all: $(RSS)

$(RSS): $(OBJS)
	$(LD) $(LDFLAGS) $(OBJS) -o $(RSS)

%.o: %.c
	$(CC) $(CFLAGS) -c $<

clean:  
	$(RM) $(RSS) $(OBJS)

