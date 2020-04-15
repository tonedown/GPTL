#ifndef DEFINES_H
#define DEFINES_H

#ifndef MIN
#define MIN(X,Y) ((X) < (Y) ? (X) : (Y))
#endif

#ifndef MAX
#define MAX(X,Y) ((X) > (Y) ? (X) : (Y))
#endif

#define STRMATCH(X,Y) (strcmp((X),(Y)) == 0)

// Default size of hash table
#define DEFAULT_TABLE_SIZE 1023

// Output counts less than PRTHRESH will be printed as integers
#define PRTHRESH 1000000L

// Maximum allowed callstack depth
#define MAX_STACK 128

// longest timer name allowed (probably safe to just change)
#define MAX_CHARS 63

// Longest allowed symbol name for libunwind
#define MAX_SYMBOL_NAME 255

// max allowable number of PAPI counters, or derived events. For convenience, set to
// max (# derived events, # papi counters required) so "avail" lists all available options.
#define MAX_AUX 9
#endif
