KEYS=-lrt -lpthread

all: a.c c.c
	gcc a.c -o a.out $(KEYS)
	gcc c.c -o c.out $(KEYS)
	gcc b.c -o b.out $(KEYS)
a: a.c
	gcc a.c -o a.out $(KEYS)
b: b.c
	gcc b.c -o b.out $(KEYS)
c: c.c
	gcc c.c -o c.out $(KEYS)
