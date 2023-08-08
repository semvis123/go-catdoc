/*
  Copyright 1996-2003 Victor Wagner
  Copyright 2003 Alex Ott
  This file is released under the GPL.  Details can be
  found in the file COPYING accompanying this distribution.
*/
#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include "catdoc.h"
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

void help(void);

int signature_check = 1;
int forced_charset = 0; /* Flag which disallow rtf parser override charset*/
int wrap_margin = WRAP_MARGIN;
int (*get_unicode_char)(FILE *f, long *offset, long fileend) = NULL;

char *input_buffer, *output_buffer;
#ifdef __WATCOMC__
/* watcom doesn't provide way to access program args via global variable */
/* so we would hack it ourselves in Borland-compatible way*/
char **_argv;
int _argc;
#endif
/**************************************************************/
/*       Main program                                         */
/*  Processes options, reads charsets  files and substitution */
/*  maps and passes all remaining args to processfile         */
/**************************************************************/
int main(int argc, char **argv) {
  FILE *f;
  int c, i;
  char *tempname;
  short int *tmp_charset;
  int stdin_processed = 0;
#ifdef __WATCOMC__
  _argv = argv;
  _argc = argc;
#endif
  read_config_file(SYSTEMRC);
#ifdef USERRC
  tempname = find_file(strdup(USERRC), getenv("HOME"));
  if (tempname) {
    read_config_file(tempname);
    free(tempname);
  }
#endif
#ifdef HAVE_LANGINFO
  get_locale_charset();
#endif
  metadata metadata_type = none;
  while ((c = getopt(argc, argv, "Vls:d:f:taubxv8wALTSKCUm:")) != -1) {
    switch (c) {
    case 's':
      check_charset(&source_csname, optarg);
      forced_charset = 1;
      break;
    case 'd':
      check_charset(&dest_csname, optarg);
      break;
    case 'f':
      format_name = strdup(optarg);
      break;
    case 't':
      format_name = strdup("tex");
      break;
    case 'a':
      format_name = strdup("ascii");
      break;
    case 'u':
      get_unicode_char = get_word8_char;
      break;
    case '8':
      get_unicode_char = get_8bit_char;
      break;
    case 'v':
      verbose = 1;
      break;
    case 'w':
      wrap_margin = 0; /* No wrap */
      break;
    case 'A':
      metadata_type = author;
      break;
    case 'L':
      metadata_type = last_author;
      break;
    case 'T':
      metadata_type = title;
      break;
    case 'S':
      metadata_type = subject;
      break;
    case 'K':
      metadata_type = keywords;
      break;
    case 'C':
      metadata_type = comments;
      break;
    case 'U':
      metadata_type = annotation_authors;
      break;
    case 'm': {
      char *endptr;
      wrap_margin = (int)strtol(optarg, &endptr, 0);
      if (*endptr) {
        fprintf(stderr, "Invalid wrap margin value `%s'\n", optarg);
        exit(1);
      }
      break;
    }
    case 'l':
      list_charsets();
      exit(0);
    case 'b':
      signature_check = 0;
      break;
    case 'x':
      unknown_as_hex = 1;
      break;
    case 'V':
      printf("Catdoc Version %s\n", CATDOC_VERSION);
      exit(0);
    default:
      help();
      exit(1);
    }
  }
  input_buffer = malloc(FILE_BUFFER);
  if (!input_buffer) {
    fprintf(stderr, "Input buffer not allocated\n");
  }
  source_charset = read_charset(source_csname);
  if (!source_charset)
    exit(1);
  if (strncmp(dest_csname, "utf-8", 6)) {
    tmp_charset = read_charset(dest_csname);
    if (!tmp_charset)
      exit(1);
    target_charset = make_reverse_map(tmp_charset);
    free(tmp_charset);
  } else {
    target_charset = NULL;
  }
  spec_chars = read_substmap(stradd(format_name, SPEC_EXT));
  if (!spec_chars) {
    fprintf(stderr, "Cannot read substitution map %s%s\n", format_name,
            SPEC_EXT);
    exit(1);
  }
  replacements = read_substmap(stradd(format_name, REPL_EXT));
  if (!replacements) {
    fprintf(stderr, "Cannot read replacement map %s%s\n", format_name,
            REPL_EXT);
    exit(1);
  }

  if (LINE_BUF_SIZE - longest_sequence <= wrap_margin) {
    fprintf(stderr, "wrap margin is too large. cannot proceed\n");
    exit(1);
  }
  set_std_func();
  if (optind == argc) {
    if (isatty(fileno(stdin))) {
      help();
      exit(0);
    }
    if (input_buffer)
      setvbuf(stdin, input_buffer, _IOFBF, FILE_BUFFER);
    return analyze_format(stdin, metadata_type);
  }
  c = 0;
  for (i = optind; i < argc; i++) {
    if (!strcmp(argv[i], "-")) {
      if (stdin_processed) {
        fprintf(stderr, "Cannot process stdin twice\n");
        exit(1);
      }
      if (input_buffer)
        setvbuf(stdin, input_buffer, _IOFBF, FILE_BUFFER);
      analyze_format(stdin, metadata_type);
      stdin_processed = 1;
    } else {
      f = fopen(argv[i], "rb");
      if (!f) {
        c = 1;
        perror("catdoc");
        continue;
      }
      if (input_buffer) {
        if (setvbuf(f, input_buffer, _IOFBF, FILE_BUFFER)) {
          perror(argv[i]);
        }
      }
      c = analyze_format(f, metadata_type);
      fclose(f);
    }
  }
  return c;
}
/************************************************************************/
/* Displays  help message                                               */
/************************************************************************/
void help(void) {
  printf("Usage:\n catdoc [-vu8btawxlV] [-m number] [-s charset] "
         "[-d charset] [ -f format] files\n");
}

void get_author() {
  char *args[] = {"", "-A", "/input_file/file.doc"};
  main(3, args);
}

void get_last_author() {
  char *args[] = {"", "-L", "/input_file/file.doc"};
  main(3, args);
}

void get_text() {
  char *args[] = {"", "/input_file/file.doc"};
  main(2, args);
}

void get_version() {
  char *args[] = {"", "-V"};
  main(2, args);
}

void get_title() {
  char *args[] = {"", "-T", "/input_file/file.doc"};
  main(3, args);
}
void get_subject() {
  char *args[] = {"", "-S", "/input_file/file.doc"};
  main(3, args);
}
void get_keywords() {
  char *args[] = {"", "-K", "/input_file/file.doc"};
  main(3, args);
}
void get_comments() {
  char *args[] = {"", "-C", "/input_file/file.doc"};
  main(3, args);
}

void get_annotation_authors() {
  char *args[] = {"", "-U", "/input_file/file.doc"};
  main(3, args);
}
