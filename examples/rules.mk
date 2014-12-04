SOURCES = $(PROG).c ../../fossa.c
CFLAGS = -W -g3 -O0 -Wall -I../.. -pthread -DNS_ENABLE_SSL -DNS_ENABLE_IPV6 -DNS_ENABLE_THREADS -lssl $(CFLAGS_EXTRA) $(MODULE_CFLAGS)

all: $(PROG)

$(PROG): $(SOURCES)
	$(CC) $(SOURCES) -o $@ $(CFLAGS)

$(PROG).exe: $(SOURCES)
	cl $(SOURCES) /I../.. /DNS_ENABLE_SSL /MD /Fe$@

clean:
	rm -rf *.gc* *.dSYM *.exe *.obj *.o a.out $(PROG)
