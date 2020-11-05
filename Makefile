CFLAGS_ALL=-I../libusbgx/build/include -I../bcm2835-1.68/build/include -L../bcm2835-1.68/build/lib -I../lua-5.4.0/src -L../libusbgx/build/lib -L../libserialport/build/lib -L../lua-5.4.0/src -lpng -lz -lpthread -llua -lm -lbcm2835 -ldl

pi400: CFLAGS+=-static $(CFLAGS_ALL) -lusbgx -lconfig -DPI400_USB
pi400: pi400.c gadget-hid.c
	$(CC)  $^ $(CFLAGS) -o $@


pi400test: CFLAGS+=-static $(CFLAGS_ALL) -lusbgx -lconfig
pi400test: pi400.c gadget-hid.c
	$(CC)  $^ $(CFLAGS) -o $@


clean:
	-rm pi400
	-rm pi400test
