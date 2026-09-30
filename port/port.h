/*
 * port.h -- glue so that the 1979 ISP listing builds with a modern
 * C compiler on a 32/64-bit host.  Included at the end of isp.h.
 *
 * The original relied on implicit declarations; that is fatal on a
 * 64-bit machine for functions returning pointers or chars, so they
 * are declared here (old-style, no parameter lists, to match the
 * K&R definitions).
 */
#include <stdio.h>
#include <stdlib.h>

/* avoid clashing with the C library's ldiv() */
#define ldiv isp_ldiv

struct listelt *uttread(), *allocit(), *freeit(), *adpp();
char chin(), jgetchar(), getdr();
int isp_ldiv();
int mfps();
void mtps();
void runsynth();

/*
 * A listelt holds a pointer, so on a 64-bit host it is bigger than
 * 1+NCONTENT ints.  allocit() uses these to carve listelts out of
 * space[] with correct size and alignment.
 */
#define LISTELT_INTS ((sizeof(struct listelt) + sizeof(int) - 1) / sizeof(int))
#define ALIGN_PTR(p) ((int *)(((size_t)(p) + sizeof(void *) - 1) & ~(sizeof(void *) - 1)))

/* original main() is renamed; port.c supplies the real one */
#define main isp_main
