CFLAGS=-std=c11 -g
SRCS=main.c parse.c tool.c codegen.c  # $(wildcard *.c)
OBJS=$(SRCS:.c=.o)
ASMS=$(SRCS:.c=.s)
GEN2ASMS=$(SRCS:.c=.asm)

mcc: $(OBJS)
		$(CC) -o mcc $(OBJS) $(LDFLAGS)

$(OBJS): mcc.h libc_alternatives.h

test: mcc
		./test.sh

mcc_gen3: $(GEN2ASMS)
		$(CC) -o mcc_gen3 $(ASMS) $(LDFLAGS)

%.asm: %.c mcc_gen2
		./mcc_gen2 $< > $@

mcc_gen2: $(ASMS)
		$(CC) -o mcc_gen2 $(ASMS) $(LDFLAGS)

%.s: %.c mcc $(SRCS)
		./mcc $< > $@

clean:
		rm -f mcc *.o *~ tmp*

.PHONY: test clean
