CFLAGS=-g -Wall -I../src_simtrace -I../libcommon/include -I.

VPATH=../src_simtrace ../libcommon/source

card_emu_test:	card_emu_tests.hobj card_emu.hobj req_ctx.hobj iso7816_fidi.hobj
	$(CC) $(LDFLAGS) -o $@ $^

%.hobj: %.c
	$(CC) $(CFLAGS) -o $@ -c $^

clean:
	@rm -f *.hobj
	@rm -f card_emu_test
