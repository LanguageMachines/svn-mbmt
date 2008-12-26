/* make-trigrams-from-giza */

#include<stdio.h>
#include<stdlib.h>
#include<string.h>

#define LEFT 1
#define RIGHT 1

int main(int argc, char *argv[])
{
  FILE *bron;
  char line[65536];
  char *part;
  char words[1024][1024];
  int  i,nrwords;
  
  fprintf(stderr,"make-trigrams-from-txt, Antal, Jun 2008\n");

  setbuf(stdout,NULL);

  strcpy(line,"");
  
  bron=fopen(argv[1],"r");
  fgets(line,65536,bron);
  while (!feof(bron))
    {
      nrwords=0;
      part=strtok(line," \n");
      while (part!=NULL)
	{
	  strcpy(words[nrwords],part);
	  nrwords++;
	  part=strtok(NULL," \n");
	}
      for (i=0; i<nrwords; i++)
	{
	  // left context
	  if ((i-1)<0)
	    fprintf(stdout,"== ");
	  else
	    {
	      if (strlen(words[i-1])>0)
		fprintf(stdout,"%s ",
			words[i-1]);
	      else
		fprintf(stdout,"DUMMY ");
	    }
	  
	  
	  // focus
	  if (strlen(words[i])>0)
	    fprintf(stdout,"%s ",
		    words[i]);
	  else
	    fprintf(stdout,"DUMMY ");

	  // right context
	  if ((i+1)>=nrwords)
	    fprintf(stdout,"== ");
	  else
	    {
	      if (strlen(words[i+1])>0)
		fprintf(stdout,"%s ",
			words[i+1]);
	      else
		fprintf(stdout,"DUMMY ");
	    }

	  // class
	  fprintf(stdout,"?\n");
	}
	
      fgets(line,65536,bron);
    }      
  fclose(bron);

  return(0);
}
