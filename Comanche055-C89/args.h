/*
 * args.h -- Command-line argument parsing.
 *
 * Comanche055 (Apollo 11 CM) ANSI C89 port.
 */

#ifndef ARGS_H
#define ARGS_H

#include "dsky_backend.h"

dsky_backend_t*
args_parse_backend(int argc, char* argv[]);

#endif /* ARGS_H */
