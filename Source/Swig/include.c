/* ----------------------------------------------------------------------------- 
 * include.c
 *
 *     The functions in this file are used to manage files in the SWIG library.
 *     General purpose functions for opening, including, and retrieving pathnames
 *     are provided.
 * 
 * Author(s) : David Beazley (beazley@cs.uchicago.edu)
 *
 * Copyright (C) 1999-2000.  The University of Chicago
 * See the file LICENSE for information on usage and redistribution.	
 * ----------------------------------------------------------------------------- */

static char cvsroot[] = "$Header$";

#include "swig.h"

/* Delimeter used in accessing files and directories */

static List      *directories = 0;        /* List of include directories */
static String    *lastpath = 0;           /* Last file that was included */
static int        bytes_read = 0;         /* Bytes read */
static String    *swiglib = 0;            /* Location of SWIG library */
static String    *lang_config = 0;        /* Language configuration file */

/* This function sets the name of the configuration file */

void Swig_set_config_file(const String_or_char *filename) {
  lang_config = NewString(filename);
}

String *Swig_get_config_file() {
  return lang_config;
}


/* -----------------------------------------------------------------------------
 * Swig_swiglib_set()
 * Swig_swiglib_get()
 *
 * Set the location of the SWIG library.  This isn't really used, by the
 * include mechanism, but rather as a query interface for language modules.
 * ----------------------------------------------------------------------------- */

void
Swig_swiglib_set(const String_or_char *sl) {
  swiglib = NewString(sl);
}

String *
Swig_swiglib_get() {
  return swiglib;
}

/* -----------------------------------------------------------------------------
 * Swig_add_directory()
 *
 * Adds a directory to the SWIG search path.
 * ----------------------------------------------------------------------------- */

void 
Swig_add_directory(const String_or_char *dirname) {
  if (!directories) directories = NewList();
  assert(directories);
  if (!DohIsString(dirname)) {
    dirname = NewString((char *) dirname);
    assert(dirname);
  }
  Append(directories, dirname);
}

/* -----------------------------------------------------------------------------
 * Swig_last_file()
 * 
 * Returns the full pathname of the last file opened. 
 * ----------------------------------------------------------------------------- */

String *
Swig_last_file() {
  assert(lastpath);
  return lastpath;
}

/* -----------------------------------------------------------------------------
 * Swig_search_path() 
 * 
 * Returns a list of the current search paths.
 * ----------------------------------------------------------------------------- */

List *
Swig_search_path() {
  String *filename;
  String *dirname;
  List   *slist;
  int i;

  slist = NewList();
  assert(slist);
  filename = NewString("");
  assert(filename);
#ifdef MACSWIG
  Printf(filename,"%s",SWIG_FILE_DELIMETER);
#else
  Printf(filename,".%s", SWIG_FILE_DELIMETER);
#endif
  Append(slist,filename);
  for (i = 0; i < Len(directories); i++) {
    dirname =  Getitem(directories,i);
    filename = NewString("");
    assert(filename);
    Printf(filename, "%s%s", dirname, SWIG_FILE_DELIMETER);
    Append(slist,filename);
  }
  return slist;
}  

/* -----------------------------------------------------------------------------
 * Swig_open()
 *
 * Looks for a file and open it.  Returns an open  FILE * on success.
 * ----------------------------------------------------------------------------- */

FILE *
Swig_open(const String_or_char *name) {
  FILE        *f;
  String   *filename;
  List     *spath = 0;
  char        *cname;
  int          i;

  if (!directories) directories = NewList();
  assert(directories);

  cname = Char(name);
  filename = NewString(cname);
  assert(filename);
  f = fopen(Char(filename),"r");
  if (!f) {
      spath = Swig_search_path();
      for (i = 0; i < Len(spath); i++) {
	  Clear(filename);
	  Printf(filename,"%s%s", Getitem(spath,i), cname);
	  f = fopen(Char(filename),"r");
	  if (f) break;
      } 
      Delete(spath);
  }
  if (f) {
    Delete(lastpath);
    lastpath = Copy(filename);
  }
  Delete(filename);
  return f;
}

/* -----------------------------------------------------------------------------
 * Swig_read_file()
 * 
 * Reads data from an open FILE * and returns it as a string.
 * ----------------------------------------------------------------------------- */

String *
Swig_read_file(FILE *f) {
  char       buffer[4096];
  String *str = NewString("");

  assert(str);
  while (fgets(buffer,4095,f)) {
    Append(str,buffer);
  }
  Append(str,"\n");
  return str;
}

/* -----------------------------------------------------------------------------
 * Swig_include()
 *
 * Opens a file and returns it as a string.
 * ----------------------------------------------------------------------------- */

static int readbytes = 0;
String *
Swig_include(const String_or_char *name) {
  FILE         *f;
  String    *str;

  f = Swig_open(name);
  if (!f) return 0;
  str = Swig_read_file(f);
  bytes_read = bytes_read + Len(str);
  fclose(f);
  Seek(str,0,SEEK_SET);
  Setfile(str,lastpath);
  Setline(str,1);
  readbytes += Len(str);
  return str;
}

int
Swig_bytes_read() {
  return readbytes;
}

/* -----------------------------------------------------------------------------
 * Swig_insert_file()
 *
 * Copies the contents of a file into another file
 * ----------------------------------------------------------------------------- */

int
Swig_insert_file(const String_or_char *filename, File *outfile) {
  char buffer[4096];
  int  nbytes;
  FILE *f = Swig_open(filename);

  if (!f) return -1;
  while ((nbytes = Read(f,buffer,4096)) > 0) {
    Write(outfile,buffer,nbytes);
  }
  return 0;
}

/* -----------------------------------------------------------------------------
 * Swig_register_filebyname()
 *
 * Register a "named" file with the core.  Named files can become targets
 * for %insert directives and other SWIG operations.  This function takes
 * the place of the f_header, f_wrapper, f_init, and other global variables
 * in SWIG1.1
 * ----------------------------------------------------------------------------- */

static Hash *named_files = 0;

void
Swig_register_filebyname(const String_or_char *filename, File *outfile) {
  if (!named_files) named_files = NewHash();
  Setattr(named_files, filename, outfile);
}

/* -----------------------------------------------------------------------------
 * Swig_filebyname()
 *
 * Get a named file
 * ----------------------------------------------------------------------------- */

File *
Swig_filebyname(const String_or_char *filename) {
  if (!named_files) return 0;
  return Getattr(named_files,filename);
}


