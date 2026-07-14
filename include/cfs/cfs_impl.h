/*
 * cfs_impl.h -- convenience header that pulls in the CFS implementation.
 *
 * Include this in EXACTLY ONE translation unit of your project to emit the
 * function bodies, without having to spell out CFS_IMPLEMENTATION yourself:
 *
 *     // cfs_impl.c -- the ONLY file that includes this header.
 *     #include <cfs/cfs_impl.h>
 *
 * Every other file keeps including the plain API header:
 *
 *     #include <cfs/cfs.h>
 *
 * This is purely sugar for `#define CFS_IMPLEMENTATION` followed by
 * `#include <cfs/cfs.h>`; the implementation still has external linkage and is
 * emitted exactly once, so including it in more than one TU produces duplicate
 * symbols at link time -- same rule as defining CFS_IMPLEMENTATION by hand.
 */

#ifndef CFS_IMPLEMENTATION
#define CFS_IMPLEMENTATION
#endif

#include "cfs.h"
