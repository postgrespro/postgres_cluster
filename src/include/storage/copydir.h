/*-------------------------------------------------------------------------
 *
 * copydir.h
 *	  Copy a directory.
 *
 * Portions Copyright (c) 1996-2016, PostgreSQL Global Development Group
 * Portions Copyright (c) 1994, Regents of the University of California
 *
 * src/include/storage/copydir.h
 *
 *-------------------------------------------------------------------------
 */
#ifndef COPYDIR_H
#define COPYDIR_H

extern void copydir(char *fromdir, char *todir, bool recurse);
extern void copy_file(char *fromfile, char *tofile);

extern void copyzipdir(char *fromdir, bool from_compressed, char *todir, bool to_compressed);
extern void copy_zip_file(char *fromfile, bool from_compressed, char *tofile, bool to_compressed);

#endif   /* COPYDIR_H */
