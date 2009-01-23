/*
  Copyright (c) 2008 - 2009
  Antal van den Bosch
  ILK Research Group, Tilburg centre for Creative Computing
  Tilburg University

  This file is part of Mbmt

  Mbmt is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 3 of the License, or
  (at your option) any later version.

  Mbmt is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program; if not, see <http://www.gnu.org/licenses/>.

  For questions and suggestions, see:
      http://ilk.uvt.nl/mbmt
  or send mail to:
      Timbl@uvt.nl
*/

#include<stdio.h>
#include<stdlib.h>
#include<string.h>

#define MAXWORDS 2048
#define MAXLEN 1024
#define DEBUG 0
#define LEFT 1
#define RIGHT 1
#define NUMBERLIMIT 5

int main(int argc, char *argv[])
{
  FILE *bron,*target;
  char **swords;
  char **twords;
  int  **tindex;
  int  *tnumber;
  char *part;
  char line[32768];
  char dummy1[32];
  char dummy2[32];
  char targetname[1024];
  char ok;
  int  i,j,k,l,sl,tl,foundat,maxfoundat;
  long double as;

  sprintf(targetname,"%s.tar.txt",argv[1]);
  
  target=fopen(targetname,"w");

  // fprintf(stderr,"get-source-target-from-A3, Antal, Mar 2008\n");

  strcpy(line,"");
  
  swords=malloc(MAXWORDS*sizeof(char*));
  twords=malloc(MAXWORDS*sizeof(char*));
  tindex=malloc(MAXWORDS*sizeof(int*));
  tnumber=malloc(MAXWORDS*sizeof(int));
  for (i=0; i<MAXWORDS; i++)
    {
      swords[i]=malloc(MAXLEN*sizeof(char));
      twords[i]=malloc(MAXLEN*sizeof(char));
      tindex[i]=malloc(MAXLEN*sizeof(int));
    }

  if (DEBUG)
    fprintf(stderr,"done allocating\n");

  bron=fopen(argv[1],"r");
  while (!feof(bron))
    {
      // the ID line
      if (line[0]!='#')
        {
          fgets(line,32768,bron);
        }

      if (!feof(bron))
        {
	  part=strtok(line," \n");
	  for (i=0; i<3; i++)
	    part=strtok(NULL," \n");
	  if (DEBUG)
	    fprintf(stderr,"sentence pair %s\n",
		    part);
	  for (i=0; i<3; i++)
	    part=strtok(NULL," \n");
	  sscanf(part,"%d",&sl);
	  sl++;
	  if (DEBUG)
	    fprintf(stderr,"source length %d\n",
		    sl);
	  
	  for (i=0; i<3; i++)
	    part=strtok(NULL," \n");
	  sscanf(part,"%d",&tl);
	  if (DEBUG)
	    fprintf(stderr,"target length %d\n",
		    tl);
	  
	  for (i=0; i<4; i++)
	    part=strtok(NULL," \n");
	  sscanf(part,"%Le",&as);
	  if (DEBUG)
	    fprintf(stderr,"alignment score %g\n",
		    as);

	  // the source sentence
	  for (i=0; i<tl; i++)
	    {
	      fscanf(bron,"%s ",
		     swords[i]);
	    }

	  // the target sentence
	  for (i=0; i<sl; i++)
	    {
	      fscanf(bron,"%s %s ",
		     twords[i],dummy1);
	      if (DEBUG)
		fprintf(stderr,"twords %d: %s\n",
			i,twords[i]);
	      fscanf(bron,"%s ",dummy2);
	      tnumber[i]=0;
	      while (strcmp(dummy2,"})")!=0)
		{
		  sscanf(dummy2,"%d",&tindex[i][tnumber[i]]);
		  tindex[i][tnumber[i]]--;
		  if (DEBUG)
		    fprintf(stderr,"tindex %d %d: %d\n",
			    i,tnumber[i],tindex[i][tnumber[i]]);
		  tnumber[i]++;
		  fscanf(bron,"%s",dummy2);
		}
	      if (i>0)
		fprintf(target,"%s ",
			twords[i]);
	    }
	  fprintf(target,"\n");

	  if (DEBUG)
	    {
	      fprintf(stderr,"target:\n");
	      for (i=0; i<sl; i++)
		{
		  fprintf(stderr," %s: %d --",
			  twords[i],tnumber[i]);
		  for (j=0; j<tnumber[i]; j++)
		    fprintf(stderr," %d (%s)",
			    tindex[i][j],swords[tindex[i][j]]);
		  fprintf(stderr,"\n");   
		}
	      fprintf(stderr,"\n");
	    }
	  
	  if (!feof(bron))
	    fgets(line,32768,bron);
	}
    }
  fclose(bron);
  fclose(target);
}
