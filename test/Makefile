GLIB_CFLAGS = -I/var/service/iguazu/pkg/include/glib-2.0 -I/var/service/iguazu/pkg/lib/glib-2.0/include
GLIB_LIBS = -L/var/service/iguazu/pkg/lib -lglib-2.0

default: ahocorasickunicode boyermoore commentzwalter commentzwalterunicode
	./test_ahocorasickunicode
	./test_boyermoore
	./test_commentzwalter
	./test_commentzwalterunicode

ahocorasickunicode:
	gcc -o test_ahocorasickunicode $(GLIB_LIBS) $(GLIB_CFLAGS) -DDEBUG -I../src -g -Wall -std=gnu99 ../src/ahocorasickunicode.c test_ahocorasickunicode.c

boyermoore:
	gcc -o test_boyermoore $(GLIB_LIBS) $(GLIB_CFLAGS) -DDEBUG -I../src -g -Wall -std=gnu99 ../src/boyermoore.c test_boyermoore.c

commentzwalter:
	gcc -o test_commentzwalter $(GLIB_LIBS) $(GLIB_CFLAGS) -DDEBUG -I../src -g -Wall -std=gnu99 ../src/commentzwalter.c test_commentzwalter.c

commentzwalterunicode:
	gcc -o test_commentzwalterunicode $(GLIB_LIBS) $(GLIB_CFLAGS) -DDEBUG -I../src -g -Wall -std=gnu99 ../src/commentzwalterunicode.c test_commentzwalterunicode.c
