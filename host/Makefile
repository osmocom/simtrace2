LDFLAGS=`pkg-config --libs libusb-1.0 libosmocore` -losmocore

all: simtrace2-remsim simtrace2-remsim-usb2udp

simtrace2-remsim: simtrace2-remsim.o apdu_dispatch.o
	$(CC) -o $@ $^ $(LDFLAGS) -losmosim

simtrace2-remsim-usb2udp: usb2udp.o
	$(CC) -o $@ $^ $(LDFLAGS)

%.o: %.c
	$(CC) $(CFLAGS) `pkg-config --cflags libusb-1.0 libosmocore` -o $@ -c $^

clean:
	@rm -f simtrace2-remsim simtrace2-remsim-usb2udp *.o
