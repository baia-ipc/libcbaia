/* (c) 2017 Giorgio Gonnella */

#include <stdbool.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <errno.h>
#include <assert.h>
#include <string.h>
#include <unistd.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <fcntl.h>
#include "alloc.h"
#include "strutils.h"
#include "file.h"

bool xgetline(char **line, size_t *line_size, FILE *file, Logger *logger)
{
  int getline_retval;
  getline_retval = getline(line, line_size, file);
  if (getline_retval == -1)
  {
    LCHECK(feof(file), logger, "Reading line from file failed, %s\n",
        strerror(errno));
    return false;
  }
  return true;
}

bool xgetdelim(char **line, size_t *line_size, int delim, FILE *file,
    Logger *logger)
{
  int getdelim_retval;
  getdelim_retval = getdelim(line, line_size, delim, file);
  if (getdelim_retval == -1)
  {
    LCHECK(feof(file), logger, "Reading from file until delimiter failed, %s\n",
      strerror(errno));
    return false;
  }
  return true;
}

char *compute_nth_filename(const char *basename, const char *sfx, size_t n)
{
  char *fname;
  int reqsize;
  reqsize = (sfx == NULL) ?
    snprintf(NULL, 0, "%s.%lu", basename, n) :
    snprintf(NULL, 0, "%s%s.%lu", basename, sfx, n);
  reqsize++;
  XMALLOC(fname, reqsize);
  if (sfx == NULL)
    snprintf(fname, reqsize, "%s.%lu", basename, n);
  else
    snprintf(fname, reqsize, "%s%s.%lu", basename, sfx, n);
  return fname;
}

#define MODEDESC(MODE) \
  (((MODE)[0] == 'r') ? "for reading" : "for writing")

FILE* __xfopen(const char *fname, const char *sfx, const char *mode,
             const char *description, Logger *logger,
             const char *_file, int _line) {
  FILE* file = NULL;
  char *fname_and_sfx;
  assert(fname != NULL && mode != NULL);
  fname_and_sfx = (sfx != NULL) ? strcat_alloc(fname, sfx) : (char*)fname;
  file = fopen(fname_and_sfx, mode);
  __LCHECK(file != NULL, logger, _file, _line,
      "Opening file '%s' failed, %s\n",
            fname_and_sfx, strerror(errno));
  __lprintf(logger, LOG_DEBUG, _file, _line,
      "Opened %s %s %s (%p)\n", description ?
        description : "file", fname_and_sfx, MODEDESC(mode), file);
  if (sfx != NULL)
    free(fname_and_sfx);
  return file;
}

void* __xfmap(const char *fname, const char *sfx, bool writable,
    size_t *fsize, int *fd, Logger *logger, const char *_file, int _line)
{
  char *fname_and_sfx;
  struct stat sb;
  void *map;
  assert(fname != NULL);
  fname_and_sfx = (sfx != NULL) ? strcat_alloc(fname, sfx) : (char*)fname;
  *fd = open(fname_and_sfx, writable ? O_RDWR : O_RDONLY);
  LCHECK(*fd != -1, logger, "Opening file '%s' failed, %s\n",
              fname_and_sfx, strerror(errno));
  LCHECK(fstat(*fd, &sb) == 0, logger, "fstat failed on file %s\n",
      fname_and_sfx);
  *fsize = sb.st_size;
  map = mmap(0, sb.st_size, PROT_READ | (writable ? PROT_WRITE : 0),
             MAP_SHARED, *fd, 0);
  __lprintf(logger, LOG_DEBUG, _file, _line,
      "Mapped file %s for %s (%p)\n", fname_and_sfx,
      writable ? "writing" : "reading", fd);
  LCHECK(map != MAP_FAILED, logger, "Mapping file '%s' failed, %s\n",
              fname_and_sfx, strerror(errno));
  if (sfx != NULL)
    free(fname_and_sfx);
  return map;
}

void xfunmap(void *map, size_t fsize, int fd, Logger *logger)
{
  LCHECK(munmap(map, fsize) == 0, logger, "Unmapping failed\n");
  LCHECK(close(fd) == 0, logger, "closing file descriptor failed");
}

void __xfclose(FILE** file, Logger *logger, const char *_file, int _line)
{
  assert(file != NULL);
  assert(*file != NULL);
  __LCHECK(fclose(*file) == 0, logger, _file, _line, "closing file failed");
  __lprintf(logger, LOG_DEBUG, _file, _line,
            "closed file... (%p)\n", *file);
  *file = NULL;
}

void xfrename(const char *fname_old, const char *sfx_old,
              const char *fname_new, const char *sfx_new,
              Logger *logger)
{
  char *fname_and_sfx_old, *fname_and_sfx_new;
  assert(fname_old != NULL);
  assert(fname_new != NULL);
  fname_and_sfx_old = (sfx_old != NULL) ?
    strcat_alloc(fname_old, sfx_old) : (char*)fname_old;
  fname_and_sfx_new = (sfx_new != NULL) ?
    strcat_alloc(fname_new, sfx_new) : (char*)fname_new;
  LCHECK(rename(fname_and_sfx_old, fname_and_sfx_new) == 0, logger,
    "Renaming file '%s' to '%s' failed, %s\n", fname_and_sfx_old,
    fname_and_sfx_new, strerror(errno));
  if (sfx_old != NULL)
    free(fname_and_sfx_old);
  if (sfx_new != NULL)
    free(fname_and_sfx_new);
}

void xfunlink(const char *fname, const char *sfx, Logger *logger)
{
  char *fname_and_sfx;
  assert(fname != NULL);
  fname_and_sfx = (sfx != NULL) ? strcat_alloc(fname, sfx) : (char*)fname;
  LCHECK(unlink(fname_and_sfx) == 0, logger, "Deleting file '%s' failed, %s\n",
              fname_and_sfx, strerror(errno));
  if (sfx != NULL)
    free(fname_and_sfx);
}

bool file_exists(const char *fname, const char *sfx)
{
  bool result;
  struct stat buffer;
  char *fname_and_sfx;
  assert(fname != NULL);
  fname_and_sfx = (sfx != NULL) ? strcat_alloc(fname, sfx) : (char*)fname;
  result = stat(fname_and_sfx, &buffer) == 0 ? true : false;
  if (sfx != NULL)
    free(fname_and_sfx);
  return result;
}

size_t file_size(FILE *file)
{
  long fsize, curpos;
  curpos = ftell(file);
  assert(curpos >= 0);
  fseek(file, 0, SEEK_END);
  fsize = ftell(file);
  assert(fsize >= 0);
  fseek(file, curpos, SEEK_SET);
  return (size_t)fsize;
}

void read_file(FILE *file, char **buffer, size_t *bufsize, Logger *logger)
{
  long fsize;
  assert(file != NULL);
  fseek(file, 0, SEEK_END);
  fsize = ftell(file);
  assert(fsize > 0);
  *bufsize = fsize;
  XMALLOC(*buffer, *bufsize);
  fseek(file, 0, SEEK_SET);
  xfread(*buffer, 1, *bufsize, file, logger);
}

#define LONG_LINE_PFXLEN 40

size_t read_lines_in_buffer(FILE *file, char *buffer, const size_t bufsize,
    char ***lines, size_t *linesbufsize, Logger *logger)
{
  size_t nlines;
  size_t nitemsread, i, lastnewline;

  assert(file != NULL);
  assert(buffer != NULL);
  assert(bufsize > 0);
  if (feof(file))
    return 0;
  nitemsread = fread(buffer, 1, bufsize-2, file);
  LCHECK(nitemsread == bufsize-2 || feof(file), logger,
      "IO Error during file read access\n");
  if (nitemsread == 0)
    return 0;
  if (feof(file) && buffer[nitemsread-1] != '\n')
  {
    buffer[nitemsread] = '\n';
    nitemsread++;
  }
  lastnewline = nitemsread;
  nlines = 0;
  XCHECKSPACEINCPERC(*lines, linesbufsize, 2*sizeof(char*), 50);
  (*lines)[0] = buffer;
  for (i = 0; i < nitemsread; i++)
  {
    if (buffer[i] == '\n')
    {
      lastnewline = i;
      XCHECKSPACEINCPERC(*lines, linesbufsize, (nlines+2)*sizeof(char*),
                         50);
      (*lines)[nlines+1] = buffer + i + 1;
      nlines++;
    }
  }
  buffer[lastnewline+1] = 0;
  if (nlines == 0)
  {
    if (bufsize > LONG_LINE_PFXLEN) buffer[LONG_LINE_PFXLEN] = '\0';
    else buffer[bufsize-1] = '\0';
    lprintf(logger, LOG_ERROR, "A line of the input file is larger "
        "than the available buffer size (%lu bytes).\n", bufsize);
    lprintf(logger, LOG_ERROR, "Long line%s: %s\n",
        bufsize > LONG_LINE_PFXLEN ? " starts with" : "", buffer);
    lprintf(logger, LOG_ERROR, "Suggestion: determine the maximal length "
        " of lines in the file "
        "(e.g. awk 'length>m{m=length} END{print m}' filename)"
        "  and re-run the command with a larger value of the buffer "
        "using the buffer size options (see --help)\n");
    exit(EXIT_FAILURE);
  }
  if (!feof(file))
  {
    long rel_offset = ((long)lastnewline - ((long)nitemsread-1));
    fseek(file, rel_offset, SEEK_CUR);
  }
  return nlines;
}

static inline
void wrongsig(const char *fname, const char *sfx, const char *desc,
              const char *helpmsg, Logger *logger)
{
  lprintf(logger, LOG_ERROR, "%s %s%s has a wrong file signature\n",
      desc, fname, sfx);
  if (helpmsg)
    lprintf(logger, LOG_ERROR, "%s\n", helpmsg);
  exit(EXIT_FAILURE);
}

void xchksig(const char *signature, const char *expected,
             const char *fname, const char *sfx,
             const char *desc, const char *helpmsg, Logger *logger)
{
  if (strncmp(signature, expected, strlen(expected)) != 0)
    wrongsig(fname, sfx, desc, helpmsg, logger);
}

FILE* __xfopen_chksig(const char *fname, const char *sfx, const char *mode,
                    const char *description,
                    const char *expected_signature, const char *helpmsg,
                    Logger *logger, const char *_file, int _line)
{
  FILE *result = __xfopen(fname, sfx, mode, description, logger, _file, _line);
  char *signature;
  int len = strlen(expected_signature);
  assert(mode[0] == 'r');
  XMALLOC(signature, len);
  xfread(signature, (size_t)1, len, result, logger);
  xchksig(signature, expected_signature, fname, sfx, description, helpmsg,
      logger);
  free(signature);
  return result;
}

FILE* __xfopen_putsig(const char *fname, const char *sfx, const char *mode,
                      const char *description,
                      const char *signature, Logger *logger,
                      const char *_file, int _line)
{
  FILE *result = __xfopen(fname, sfx, mode, description, logger, _file, _line);
  size_t len = strlen(signature);
  assert(mode[0] == 'w');
  xfwrite(signature, (size_t)1, len, result, logger);
  return result;
}

void *__xfmap_chksig(const char *fname, const char *sfx,
    size_t *fsize, int *fd, const char *expected_signature,
    const char *filedesc, const char *helpmsg, Logger *logger,
    const char *_file, int _line)
{
  void *result = __xfmap(fname, sfx, false, fsize, fd, logger, _file, _line);
  xchksig((char*)result, expected_signature, fname, sfx, filedesc, helpmsg,
      logger);
  return result;
}
