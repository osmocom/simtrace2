LDFLAGS=`pkg-config --libs libusb-1.0 libosmocore` -losmocore
CFLAGS=-Wall -g

APPS=simtrace2-remsim simtrace2-remsim-usb2udp simtrace2-list simtrace2-sniff

all: $(APPS)

simtrace2-remsim: simtrace2-remsim.o apdu_dispatch.o simtrace2-discovery.o libusb_util.o
	$(CC) -o $@ $^ $(LDFLAGS) -losmosim

simtrace2-remsim-usb2udp: usb2udp.o simtrace2-discovery.o
	$(CC) -o $@ $^ $(LDFLAGS)

simtrace2-list: simtrace2_usb.o libusb_util.o
	$(CC) -o $@ $^ $(LDFLAGS)

simtrace2-sniff: simtrace2-sniff.o simtrace2-discovery.o libusb_util.o
	$(CC) -o $@ $^ $(LDFLAGS)

%.o: %.c
	$(CC) $(CFLAGS) `pkg-config --cflags libusb-1.0 libosmocore` -o $@ -c $^

clean:
	@rm -f *.o $(APPS)

install: $(APPS)
	mkdir -p $(DESTDIR)/usr/bin
	cp $(APPS) $(DESTDIR)/usr/bin/
