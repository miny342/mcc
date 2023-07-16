CFLAGS=-std=c11 -g
SRCS=main.c parse.c tool.c codegen.c gen_ir.c  # $(wildcard *.c)
OBJS=$(SRCS:.c=.o)
ASMS=$(SRCS:.c=.s)
GEN2ASMS=$(SRCS:.c=_2.s)
GEN3ASMS=$(SRCS:.c=_3.s)

# mcc: $(OBJS)
# 		$(CC) -o mcc $(OBJS) $(LDFLAGS)

# $(OBJS): mcc.h libc_alternatives.h

mcc: $(ASMS)
		$(CC) -o mcc $(ASMS) $(LDFLAGS)

%.s: %.c libc_alternatives.h mcc.h
		./mcc_stable $< > $@

test: mcc
		./test.sh

test_self_compile: $(GEN3ASMS)
		./test_self_compile.sh

mcc_gen3: $(GEN3ASMS)
		$(CC) -o mcc_gen3 $(GEN3ASMS) $(LDFLAGS)

%_3.s: %.c mcc_gen2
		./mcc_gen2 $< > $@

mcc_gen2: $(GEN2ASMS)
		$(CC) -o mcc_gen2 $(GEN2ASMS) $(LDFLAGS)

%_2.s: %.c mcc
		./mcc $< > $@

mcc_stable: mcc
		cp mcc mcc_stable

clean:
		rm -f *.o *~ tmp* *.s mcc mcc_gen2 mcc_gen3

.PHONY: test clean test_self_compile
