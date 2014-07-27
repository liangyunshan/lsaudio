#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <stdlib.h>
#include <sys/types.h>
#include <dirent.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>

#include "mp3info.h"


int my_strcmp(char*,char*);
int oggnfo(MPEG_HEAD * mh, off_t size, int fd);

char version[]="lsaudio: sivann 1999-2008, version 1.2";

void qsort1 (register int l, register int r, char *a[])
{
    register int i, j;

    char *w;
    char x[256];

    i = l;
    j = r;
    strncpy (x, a[(l + r) / 2], 255);
    while (i <= j) {
        while (strcmp (a[i], x) < 0)
            i++;
        while (strcmp (a[j], x) > 0)
            j--;
        if (i <= j) {
            w = a[i];
            a[i] = a[j];
            a[j] = w;
            i++;
            j--;
        }
    }
    if (l < j)
        qsort1 (l, j, a);
    if (i < r)
        qsort1 (i, r, a);
}


void
qsort2 (register int l, register int r, char *a[], off_t * siz,MPEG_HEAD * inf)
{
    register int i, j;

    char *w;
    char x[256];

    off_t s;
    MPEG_HEAD mh;

    strncpy (x, a[(l + r) / 2], 254);
    i = l;
    j = r;

    while (i <= j) {

        while (my_strcmp (a[i], x) < 0)
            i++;
        while (my_strcmp (a[j], x) > 0)
            j--;

        if (i <= j) {
            w = a[i];
            a[i] = a[j];
            a[j] = w;

            s = siz[i];
            siz[i] = siz[j];
            siz[j] = s;

            if (inf) {
                mh = inf[i];
                inf[i] = inf[j];
                inf[j] = mh;
            }
            i++;
            j--;
        }
    }
    if (l < j)
        qsort2 (l, j, a, siz, inf);
    if (i < r)
        qsort2 (i, r, a, siz, inf);
} //qsort2


int main(int argc,char **argv)
{
  struct dirent *dp;
  struct stat stp;
  DIR *dirp;

  char dir[512];
  char argdir[512];
  char buf[1024];
  char str[128];
  char *files[2048],*dirs[2048];
  long ffsize=0,ddsize=0, i;
  off_t fsize[2048];
  MPEG_HEAD finfo[2048];
  int audioenc=0,fd=0;

  strcpy(argdir,".");

  if (argc==2)
    strcpy(argdir,argv[1]);
  
  dirp = opendir (argdir);
  if (dirp == 0) {
      fprintf (stderr,"opendir \"%s\": %s\n", argdir, strerror (errno));
      exit (errno);
  }


  ffsize=0;


  /* Read directory contents and mp3 file infos*/
  while ((dp = readdir (dirp)) != NULL) {
      strcpy (dir, argdir);
      strcat (dir, "/");
      strcat (dir, dp->d_name);

      if (strstr (dir, ".mp3"))
	  audioenc = 1;
      else if (strstr (dir, ".ogg"))
	  audioenc = 2;
      else
	  audioenc = 0;

      //printf("DIR:%s\n",dir);
      lstat (dir, &stp);
      finfo[ffsize].fmode = stp.st_mode;
      if (S_ISLNK (finfo[ffsize].fmode)) {
	  stat (dir, &stp);
	  if (S_ISDIR (stp.st_mode))
	      finfo[ffsize].fmode = stp.st_mode;
      }

      if (!S_ISDIR (stp.st_mode)) {
	  files[ffsize] =
	      (char *) realloc ((char *) files[ffsize],
				strlen (dp->d_name) + 2);
	  strcpy (files[ffsize], dp->d_name);
	  fsize[ffsize] = stp.st_size;

	  if (audioenc==1) { //mp3
	      if (!S_ISREG (stp.st_mode)) {
		  finfo[ffsize].valid = 0;
		  fd = -1;
	      }
	      else
		  fd = open (dir, O_RDONLY);
	      if (fd < 0) {
		  finfo[ffsize].valid = 0;
	      }
	      else if (S_ISREG (stp.st_mode)) {
		  get_mp3header (&finfo[ffsize], stp.st_size, fd);
		  close (fd);
	      }
	  }
	  if (audioenc==2) { //ogg
	      if (!S_ISREG (stp.st_mode)) {
		  finfo[ffsize].valid = 0;
		  fd = -1;
	      }
	      else
		  fd = open (dir, O_RDONLY);
	      if (fd < 0) {
		  finfo[ffsize].valid = 0;
	      }
	      else if (S_ISREG (stp.st_mode)) {
		  oggnfo (&finfo[ffsize], stp.st_size, fd);
		  finfo[ffsize].valid = 1;
		  finfo[ffsize].enctype = 2; //ogg
		  close (fd);
	      }
	  }
	  else if (audioenc==0) // non-audio file
	      finfo[ffsize].valid = 0;
	  ffsize++;
      }
      else {
	  dirs[ddsize] =
	      (char *) realloc ((char *) dirs[ddsize],
				strlen (dp->d_name) + 2);
	  strcpy (dirs[ddsize], dp->d_name);
	  ddsize++;
      }
  }
  closedir (dirp);

  /*Sort dirs */
  if ((ddsize - 1) > 1)
      qsort1 (0, ddsize - 1, dirs);

  /*Sort files */
  if ((ffsize - 1) > 0) {
      qsort2 (0, ffsize - 1, files, fsize, finfo);
  }


  //print dirs
  for (i = 0; i < ddsize; i++) {
      printf ("d//////%s\n",dirs[i]);
  }

  //print files
  for (i = 0; i < ffsize; i++) {
    if (strstr (files[i], ".mp3")) {
      if (finfo[i].vbr) {
	strcpy(str,"VBR");
	//printf("I:%ld\n",i);
	if ((finfo[i].mins+finfo[i].secs))
	  finfo[i].bitrate=fsize[i]*8/(60*finfo[i].mins+finfo[i].secs)/1000;
	else
	  finfo[i].bitrate=0;
      }
      else
	strcpy(str,"Kbps");
      sprintf (buf,"fmp3/%08ld/%02d/%02d/%03d/%3s/%s\n",
	   fsize[i],finfo[i].mins,finfo[i].secs, finfo[i].bitrate,str,files[i]);
    }
    else if (strstr (files[i], ".ogg")) {
      //sprintf(buf,"%s\n",files[i]);
	strcpy(str,"VBR");
      sprintf (buf,"fmp3/%08ld/%02d/%02d/%03d/%3s/%s\n",
	   fsize[i],finfo[i].mins,finfo[i].secs, finfo[i].bitrate,str,files[i]);
    }
    else
      sprintf (buf,"f/%ld/////%s\n",fsize[i],files[i]);

    printf("%s",buf);
  }

  return 0;
}
