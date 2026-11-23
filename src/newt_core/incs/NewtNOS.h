/**
 * @file	NewtNOS.h
 * @brief	Newton OS 2.1 global functions that NEWT/0 lacks (NewtNOS.c)
 */

#ifndef NEWTNOS_H
#define NEWTNOS_H

#include "NewtType.h"

/// Defines the C functions; NVMInit calls it after the built-in functions.
void NewtNOSInstall(void);

/// The functions written in NewtonScript, compiled into one function that
/// defines them. NVMInit runs it once, the first time the VM starts.
newtRef NewtNOSLibraryFunction(void);

#endif /* NEWTNOS_H */
