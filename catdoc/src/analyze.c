/*
  Copyright 1998-2003 Victor Wagner
  Copyright 2003 Alex Ott
  This file is released under the GPL.  Details can be
  found in the file COPYING accompanying this distribution.
*/
#ifdef HAVE_CONFIG_H
#include <config.h>
#endif
#include "catdoc.h"
#include <limits.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

extern char ole_sign[], zip_sign[]; /* from ole.c */
char rtf_sign[] = "{\\rtf";
char old_word_sign[] = {0xdb, 0xa5, 0};
char write_sign[] = {0x31, 0xBE, 0};
unsigned char sttbfRMark[] = {0x00, 0x00, 0x07, 0x00, 0x55, 0x00,
                              0x6E, 0x00, 0x6B, 0x00, 0x6E, 0x00,
                              0x6F, 0x00, 0x77, 0x00, 0x6E, 0x00};
int verbose = 0;

unsigned char *read_metadata(unsigned char *buffer, metadata metadata_type) {
  uint32_t offset_0 = getulong(buffer, 44);
  uint32_t num_properties = getulong(buffer, 52);
  for (int i = 0; i < num_properties; i++) {
    uint32_t identifier = getulong(buffer, 56 + 8 * i);
    uint32_t offset = getulong(buffer, 60 + 8 * i);
    if ((identifier == 2 && metadata_type == title) ||
        (identifier == 3 && metadata_type == subject) ||
        (identifier == 4 && metadata_type == author) ||
        (identifier == 5 && metadata_type == keywords) ||
        (identifier == 6 && metadata_type == comments) ||
        (identifier == 8 && metadata_type == last_author)) {
      return buffer + offset_0 + offset + 8;
    }
  }
  return NULL;
}

int find_offset(FILE *f, unsigned char *mark, int mark_length) {
  int offset = 0;
  int block_size = 1024 * 25;
  int pos = 0;
  int bpos = 0;
  unsigned char buf[block_size];
  int block_n = 0;

  while (!feof(f)) {
    long n = fread(buf, 1, block_size, f);
    while (bpos != n) {
      if (buf[bpos] == mark[pos]) {
        pos++;
        bpos++;
        if (pos == mark_length) {
          offset = block_n * block_size + bpos - pos;
          return offset;
        }
      } else {
        bpos -= pos;
        bpos++;
        pos = 0;
      }
    }
    bpos = 0;
    block_n++;
  }
  fprintf(stderr, "stttbfRMark offset is not found");
  exit(1);
}

void read_annotation_authors(FILE *f) {
  int offset = find_offset(f, sttbfRMark, 18) - 4;
  fseek(f, offset, SEEK_SET);

  int block_size = 1024;
  unsigned char buf[2];
  fread(buf, 1, 2, f);
  if (buf[0] != 0xff || buf[1] != 0xff) {
    fprintf(stderr, "stttbfRMark offset is invalid");
    exit(1);
  }
  fread(buf, 1, 2, f);
  unsigned int count = getshort(buf, 0) - 1;
  fseek(f, 18, SEEK_CUR);
  for (int i = 0; i < count; i++) {
    fread(buf, 1, 2, f);
    unsigned int str_len = getshort(buf, 0);
    unsigned short *str = calloc(str_len, 2);
    fread(str, 2, str_len, f);
    for (int j = 0; j < str_len; j++) {
      printf("%lc", str[j]);
    }
    printf("\n");
    free(str);
  }
}

/*********************************************************************
 * Determines format of input file and calls parse_word_header or
 * process_file if
 * it is word processor file or copy_out if it is plain text file
 * return not 0 when error
 ********************************************************************/
int analyze_format(FILE *f, metadata metadata_type) {
  if (metadata_type == annotation_authors) {
    read_annotation_authors(f);
    return 0;
  }
  unsigned char buffer[129];
  long offset = 0;
  FILE *new_file, *ole_file;
  int ret_code = 69;

  if (!signature_check) {
    /* forced parsing */
    /* no autodetect possible. Assume 8-bit if not overriden on
     * command line */
    if (!get_unicode_char)
      get_unicode_char = get_8bit_char;
    return process_file(f, LONG_MAX);
  }
  catdoc_read(buffer, 4, 1, f);
  buffer[4] = 0;
  if (strncmp((char *)&buffer, write_sign, 2) == 0) {
    printf("[Windows Write file. Some garbage expected]\n");
    get_unicode_char = get_8bit_char;
    return process_file(f, LONG_MAX);
  } else if (strncmp((char *)&buffer, rtf_sign, 4) == 0) {
    return parse_rtf(f);
  } else if (strncmp((char *)&buffer, zip_sign, 4) == 0) {
    fprintf(stderr, "This file looks like ZIP archive or Office 2007 "
                    "or later file.\nNot supported by catdoc\n");
    exit(1);
  } else if (strncmp((char *)&buffer, old_word_sign, 2) == 0) {
    fread(buffer + 4, 1, 124, f);
    return parse_word_header(buffer, f, 128, 0);
  }
  fread(buffer + 4, 1, 4, f);
  if (strncmp((char *)&buffer, ole_sign, 8) == 0) {
    if ((new_file = ole_init(f, buffer, 8)) != NULL) {
      set_ole_func();
      while ((ole_file = ole_readdir(new_file)) != NULL) {
        int res = ole_open(ole_file);
        if (res >= 0) {
          if (metadata_type == none &&
              strcmp(((oleEntry *)ole_file)->name, "WordDocument") == 0) {
            offset = catdoc_read(buffer, 1, 128, ole_file);
            ret_code = parse_word_header(buffer, ole_file, -offset, offset);
          }
          if (metadata_type != none &&
              strcmp((((oleEntry *)ole_file)->name + 1),
                     "SummaryInformation") == 0) {
            int n = ((oleEntry *)ole_file)->length;
            unsigned char temp[n + 1];
            offset = catdoc_read(temp, 1, n, ole_file);
            printf("%s\n", read_metadata(temp, metadata_type));
          }
        }
        ole_close(ole_file);
      }
      set_std_func();
      ole_finish();
    } else {
      fprintf(stderr, "Broken OLE file. Try using -b switch\n");
      exit(1);
    }
  } else {

    copy_out(f, (char *)&buffer);
    return 0;
  }

  return ret_code;
}

#define fDot 0x0001
#define fGlsy 0x0002
#define fComplex 0x0004
#define fPictures 0x0008
#define fEncrypted 0x100
#define fReadOnly 0x400
#define fReserved 0x800
#define fExtChar 0x1000

/*******************************************************************/
/* parses word file info block passed in buffer.
 * Determines actual size of text stream and calls process_file
 ********************************************************************/
int parse_word_header(unsigned char *buffer, FILE *f, int offset, long curpos) {

  int flags, charset, ret_code = 0;
  long textstart, textlen, i;
  char buf[2];

  if (verbose) {
    printf("File Info block version %d\n", getshort(buffer, 2));
    printf("Found at file offset %ld (hex %lx)\n", curpos, curpos);
    printf("Written by product version %d\n", getshort(buffer, 4));
    printf("Language %d\n", getshort(buffer, 6));
  }
  flags = getshort(buffer, 10);
  if (verbose) {
    if ((flags & fDot)) {
      printf("This is template (DOT) file\n");
    } else {
      printf("This is document (DOC) file\n");
    }
    if (flags & fGlsy) {
      printf("This is glossary file\n");
    }
  }
  if (flags & fComplex) {
    fprintf(stderr,
            "[This was fast-saved %2d times. Some information is lost]\n",
            (flags & 0xF0) >> 4);
    /*		ret_code=69;*/
  }
  if (verbose) {
    if (flags & fReadOnly) {
      printf("File is meant to be read-only\n");
    }
    if (flags & fReserved) {
      printf("File is write-reserved\n");
    }
  }
  if (flags & fExtChar) {
    if (verbose) {
      printf("File uses extended character set\n");
    }
    if (!get_unicode_char)
      get_unicode_char = get_word8_char;

  } else if (!get_unicode_char)
    get_unicode_char = get_8bit_char;

  if (verbose) {
    if (buffer[18]) {
      printf("File created on Macintosh\n");
    } else {
      printf("File created on Windows\n");
    }
  }
  if (flags & fEncrypted) {
    fprintf(stderr, "[File is encrypted. Encryption key = %08x]\n",
            getlong(buffer, 14));
    return 69;
  }
  if (verbose) {
    charset = getshort(buffer, 20);
    if (charset && charset != 256) {
      printf("Using character set %d\n", charset);
    } else {
      printf("Using default character set\n");
    }
  }
  /* skipping to textstart and computing textend */
  textstart = getlong(buffer, 24);
  textlen = getlong(buffer, 28) - textstart;
  textstart += offset;
  if (verbose) {
    printf("Textstart = %ld (hex %lx)\n", textstart + curpos,
           textstart + curpos);
    printf("Textlen =   %ld (hex %lx)\n", textlen, textlen);
  }
  for (i = 0; i < textstart; i++) {
    catdoc_read(buf, 1, 1, f);
    if (catdoc_eof(f)) {
      fprintf(stderr, "File ended before textstart. Probably it is broken. Try "
                      "-b switch\n");
      exit(1);
    }
  }
  return process_file(f, textlen) || ret_code;
}
