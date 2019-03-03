#PREFIX is environment variable, but if it is not set, then set default value
ifeq ($(PREFIX),)
    PREFIX := /usr/local
endif

benzl:  	stdlib
				cc -std=c11 -Wall -Ofast -Wno-unknown-pragmas -DNDEBUG -D_DEFAULT_SOURCE src/benz*.c -ledit -o benzl

stdlib:
				xxd -i src/stdlib.benzl src/benzl-stdlib.h

test:   	benzl
				./benzl test/stdlib-tests.benzl

install:	benzl
				install -d $(DESTDIR)$(PREFIX)/bin/
				install -m 755 benzl $(DESTDIR)$(PREFIX)/bin/

clean:
				rm -rf *.o
				rm src/benzl-stdlib.h
