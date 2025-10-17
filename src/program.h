#ifndef PROGRAM_H
#define PROGRAM_H

#ifdef __cplusplus
#define EXTERNC extern "C"
#else
#define EXTERNC
#endif

EXTERNC void run();

#undef EXTERNC

#endif
