
CC      := $(CROSS_HOST)gcc
PKGCONF := $(CROSS_HOST)pkg-config
CFLAGS  := -g -Wall -pthread -O3 $(EXTRA_CFLAGS)
LDFLAGS := -g -lm -pthread $(EXTRA_LDFLAGS)
OBJS    := fl2k_spdif.o spdif.o fl2k.o

CFLAGS  += $(shell $(PKGCONF) --cflags libosmo-fl2k)
LDFLAGS += $(shell $(PKGCONF) --libs libosmo-fl2k)

all: fl2k_spdif

fl2k_spdif: $(OBJS)
	$(CC) -o fl2k_spdif $(OBJS) $(LDFLAGS)

%.o: %.c Makefile
	$(CC) $(CFLAGS) -c $< -o $@
	@$(CC) $(CFLAGS) -MM $< -o $(@:.o=.d)

clean:
	rm -f *.o *.d spdif

-include $(OBJS:.o=.d)

