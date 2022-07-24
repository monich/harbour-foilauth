/*
 * qmake thinks that gets image.c is included by binarize.c and
 * doesn't generate a rule for it
 */
#include "../zbar/image.c"
