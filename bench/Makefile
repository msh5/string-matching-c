CFLAGS = -DDEBUG -I../src -O3 -Wall -std=gnu99 
#CFLAGS = -DDEBUG -I../src -g -O0 -Wall -std=gnu99 
GLIB_CFLAGS = -I/var/service/iguazu/pkg/include/glib-2.0 -I/var/service/iguazu/pkg/lib/glib-2.0/include
GLIB_LIBS = -L/var/service/iguazu/pkg/lib -lglib-2.0
MATCHER_SOURCES = \
  ../src/ahocorasickunicode.c ../src/commentzwalter.c ../src/commentzwalterunicode.c \
  ../src/boyermoore.c ../src/boyermooreunicode.c ../src/naiveunicode.c

default: bench
	./bench 100 10 1

bench: $(MATCHER_SOURCES) bench.c
	gcc -o bench $(GLIB_LIBS) $(GLIB_CFLAGS) $(CFLAGS) $(MATCHER_SOURCES) bench.c

