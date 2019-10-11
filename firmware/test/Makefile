LIBOSMOCORE_CFLAGS=`pkg-config --cflags libosmocore`
LIBOSMOCORE_LIBS=`pkg-config --libs libosmocore`

CFLAGS=-g -Wall $(LIBOSMOCORE_CFLAGS) \
	-I../src_simtrace \
	-I../atmel_softpack_libraries/libchip_sam3s \
	-I../atmel_softpack_libraries/libchip_sam3s/cmsis \
	-I../atmel_softpack_libraries/libchip_sam3s/include \
	-I../atmel_softpack_libraries/usb/include \
	-I../libcommon/include \
	-I../libboard/common/include \
	-I../libboard/simtrace/include \
	-I.
LIBS=$(LIBOSMOCORE_LIBS)

VPATH=../src_simtrace ../libcommon/source

card_emu_test:	card_emu_tests.hobj card_emu.hobj usb_buf.hobj iso7816_fidi.hobj
	$(CC) $(LDFLAGS) -o $@ $^ $(LIBS)

%.hobj: %.c
	$(CC) $(CFLAGS) -o $@ -c $^

clean:
	@rm -f *.hobj
	@rm -f card_emu_test
