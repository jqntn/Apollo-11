/*
 * args.c -- Command-line argument parsing.
 *
 * Comanche055 (Apollo 11 CM) ANSI C89 port.
 */

#include "args.h"
#include "dsky.h"
#include "dsky_backend.h"
#include "dsky_web.h"

#ifdef _WIN32
#include "dsky_gui.h"
#endif

#include <stdio.h>
#include <string.h>

dsky_backend_t*
args_parse_backend(int argc, char* argv[])
{
  if (argc < 2)
    return NULL;

  if (strcmp(argv[1], "console") == 0)
    return &dsky_console_backend;

  if (strcmp(argv[1], "web") == 0)
    return &dsky_web_backend;

#ifdef _WIN32
  if (strcmp(argv[1], "gui") == 0)
    return &dsky_gui_backend;
#endif

  printf("Unknown backend: %s\n", argv[1]);
  return NULL;
}
