/*
  Copyright (c) 2006 - 2009
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

#define MAXWORDS 4096
#define MAXLEN 2048
#define DEBUG 0
#define LEFT 1
#define RIGHT 1
#define NUMBERLIMIT 5

int main(int argc, char *argv[])
{
  FILE *bron;
  char **swords;
  char **twords;
  int  **sindex;
  int  *snumber;
  char *part;
  char line[65536];
  char dummy1[64];
  char dummy2[64];
  int  i,j,k,l,sl,tl;
  long double as;
  
  // fprintf(stderr,"make-trigrams-from-giza, Antal, Aug 2007\n");

  setbuf(stdout,NULL);

  strcpy(line,"");
  
  swords=malloc(MAXWORDS*sizeof(char*));
  twords=malloc(MAXWORDS*sizeof(char*));
  sindex=malloc(MAXWORDS*sizeof(int*));
  snumber=malloc(MAXWORDS*sizeof(int));

  for (i=0; i<MAXWORDS; i++)
    {
      swords[i]=malloc(MAXLEN*sizeof(char));
      twords[i]=malloc(MAXLEN*sizeof(char));
      sindex[i]=malloc(MAXLEN*sizeof(int));
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
	    fprintf(stderr,"alignment score %Le\n",
		    as);
	  
	  // the target sentence
	  for (i=0; i<tl; i++)
	    fscanf(bron,"%s ",
		   twords[i]);
	  if (DEBUG)
	    {
	      fprintf(stderr,"target:");
	      for (i=0; i<tl; i++)
		fprintf(stderr," %s",
			twords[i]);
	      fprintf(stderr,"\n");
	    }
	  
	  // the source sentence
	  for (i=0; i<sl; i++)
	    {
	      fscanf(bron,"%s %s ",
		     swords[i],dummy1);
	      fscanf(bron,"%s ",dummy2);
	      snumber[i]=0;
	      while (strcmp(dummy2,"})")!=0)
		{
		  sscanf(dummy2,"%d",&sindex[i][snumber[i]]);
		  sindex[i][snumber[i]]--;
		  snumber[i]++;
		  fscanf(bron,"%s",dummy2);
		}
	    }
	  if (DEBUG)
	    {
	      fprintf(stderr,"source:\n");
	      for (i=0; i<sl; i++)
		{
		  fprintf(stderr," %s: %d --",
			  swords[i],snumber[i]);
		  for (j=0; j<snumber[i]; j++)
		    fprintf(stderr," %d (%s)",
			    sindex[i][j],twords[sindex[i][j]]);
		  fprintf(stderr,"\n");   
		}
	      fprintf(stderr,"\n");
	    }
	  
	  // generate translation examples
	  for (i=0; i<tl; i++)
	    for (j=1; j<sl; j++)
	      for (l=0; l<snumber[j]; l++)
		if (sindex[j][l]==i)
		  {		      
		    // left context
		    for (k=0; k<LEFT; k++)
		      {
			if ((i-(LEFT-k))<0)
			  {
			    fprintf(stdout,"== ");
			    if (DEBUG)
			      fprintf(stderr,"== ");
			  }
			else
			  {
			    fprintf(stdout,"%s ",
				    twords[i-(LEFT-k)]);
			    if (DEBUG)
			      fprintf(stderr,"%s ",
				      twords[i-(LEFT-k)]);
			  }
		      }
		    
		    // focus word
		    fprintf(stdout,"%s ",
			    twords[i]);
		    if (DEBUG)
		      fprintf(stderr,"%s ",
			      twords[i]);
		    
		    // right context
		    for (k=0; k<RIGHT; k++)
		      {
			if ((i+k+1)>=tl)
			  {
			    fprintf(stdout,"== ");
			    if (DEBUG)
			      fprintf(stderr,"== ");
			  }
			else
			  {
			    fprintf(stdout,"%s ",
				    twords[i+k+1]);
			    if (DEBUG)
			      fprintf(stderr,"%s ",
				      twords[i+k+1]);
			  }
		      }
		    
		    // trigram class
		    if (j<2)
		      {
			fprintf(stdout,"==^");
			if (DEBUG)
			  fprintf(stderr,"==^");
		      }
		    else
		      {
			fprintf(stdout,"%s^",
				swords[j-1]);
			if (DEBUG)
			  fprintf(stderr,"%s^",
				  swords[j-1]);
		      }
		    fprintf(stdout,"%s^",
			    swords[j]);
		    if (DEBUG)
		      fprintf(stderr,"%s^",
			      swords[j]);
		    if ((j+1)>=sl)
		      {
			fprintf(stdout,"==\n");
			if (DEBUG)
			  fprintf(stderr,"==\n");
		      }
		    else
		      {
			fprintf(stdout,"%s\n",
				swords[j+1]);
			if (DEBUG)
			  fprintf(stderr,"%s\n",
				  swords[j+1]);
		      }
		  }	  
	  if (!feof(bron))
	    fgets(line,32768,bron);
	}
    }
  fclose(bron);
  return(0);
}
