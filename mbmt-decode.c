/** decode-tribl - Antal, Feb 2008

    a rewrite of older decode code from Summer of 2006 and onwards

    syntax: decode-mbmt <TRIBL2 +vdb+di tri output>

*/

#include<stdio.h>
#include<math.h>
#include<stdlib.h>
#include<string.h>
#include "sockhelp.h"
#include<unistd.h>
#include<time.h>

#define BEAM 1 // the beam on the beam. Best: 1, or higher with sparser data
#define DEBUG 1
#define DEBUG2 0
#define DEBUG3 0
#define FILEWOPR 0
#define MAXSENT 2048
#define WORDLEN 256
#define FEATANDCLASS 4
#define CORRECTFORLENGTHDIFF 0 // best: 0; correct for average length diff in English/Dutch sentences
#define BUFFERSIZE 65536
#define MAXOVERLAP 32768
#define DIFFTHRESHOLD 2 // best: 2
#define MAXNRFOLLOWERS 1 // best: 1; to limit non-overlapping nrs of follower trigrams
#define LIMIT 10 // 10 - patience
#define LIMIT2 1000 // 1000 - nr attempts
#define LIMIT3 100 // 100

#define MACHINE "localhost"
#define PORT      "1982"

// OK, one global variable. For fast buffering. May I?
char  *buffer;

int main(int argc, char *argv[])
{
  float wopr(char *woprstring, int woprsock);
  void  bigwopr(char *woprstring, int woprsock);
  void  timer(void);

  FILE  *bron,*woprfile;
  char  **superout;
  char  features[32][2048];
  char  ****classes;
  char  ****overlap;
  char  **attempted;
  int   *attemptedlength;
  char  *sentence;
  char  *woprsentence;
  char  *memsentence;
  char  *altsentence;
  char  *bestsentence;
  int	**overtab;
  int   *starters;
  int   *followers;
  float *followerscore;
  float *followerstart;
  float *followerend;
  char  *done;
  int   *notdone;
  float *overlog;
  float **score;
  int   *nralt;
  char  hulp[WORDLEN];
  char  *inputstring;
  char  ***outputstring;
  char  ***beststring;
  char  **towopr;
  char  class[2048];
  char  prevclass[2048];
  char  bigwoprcommand[1024];
  char  *rest,*rest2,*part;
  int   i,j,k,l,nrinst=0,trigger=0,sock=0;
  int   thisnroverlap,nroverlap,counter,counter2,counter3,nrfollowers,chosen,nrattempted;
  int   diff=0,outputlen,bestlen,supercount=0,thislength,nrtowopr;
  float followertotal,ddice,thisperplex;
  char  ready=0,found,seen;
  float perplex,bestperplex;

  srand48(24101997);
  setbuf(stdout,NULL);
  
  strcpy(prevclass,"");

  rest=malloc(100000000*sizeof(char));
  rest2=malloc(100000000*sizeof(char));

  buffer=malloc(BUFFERSIZE*sizeof(char));
  sentence=malloc(BUFFERSIZE*sizeof(char));
  woprsentence=malloc(BUFFERSIZE*sizeof(char));
  altsentence=malloc(BUFFERSIZE*sizeof(char));
  memsentence=malloc(BUFFERSIZE*sizeof(char));
  bestsentence=malloc(BUFFERSIZE*sizeof(char));

  classes=malloc(MAXSENT*sizeof(char***)); // max number of alternatives
  outputstring=malloc(MAXSENT*sizeof(char**));
  beststring=malloc(MAXSENT*sizeof(char**));
  towopr=malloc(MAXSENT*sizeof(char*));
  overlap=malloc(MAXSENT*sizeof(char***));
  score=malloc(MAXSENT*sizeof(float*));
  nralt=malloc(MAXSENT*sizeof(int));

  superout=malloc(MAXSENT*sizeof(char*));

  starters=malloc(10000*sizeof(int));
  followers=malloc(10000*sizeof(int));
  followerscore=malloc(10000*sizeof(float));
  followerstart=malloc(10000*sizeof(float));
  followerend=malloc(10000*sizeof(float));
  done=malloc(10000*sizeof(char));
  notdone=malloc(10000*sizeof(int));
  
  inputstring=malloc(100000*sizeof(char));
  
  overtab=malloc(MAXOVERLAP*sizeof(int*));
  overlog=malloc(MAXOVERLAP*sizeof(float));
  for (i=0; i<MAXOVERLAP; i++)
    overtab[i]=malloc(4*sizeof(int));

  attempted=malloc(LIMIT2*sizeof(char*));
  attemptedlength=malloc(LIMIT2*sizeof(int));
  for (i=0; i<LIMIT2; i++)
    attempted[i]=malloc(1024*sizeof(char));

  for (i=0; i<MAXSENT; i++)
    {
      classes[i]=malloc(BEAM*sizeof(char**)); //max sentence length
      overlap[i]=malloc(BEAM*sizeof(char**));
      score[i]=malloc(BEAM*sizeof(float));
      for (j=0; j<BEAM; j++)
        {
          classes[i][j]=malloc(3*sizeof(char*)); // three
          for (k=0; k<3; k++)
            {
              classes[i][j][k]=malloc(WORDLEN*sizeof(char)); // word length
            }
          overlap[i][j]=malloc(MAXSENT*sizeof(char*));
          for (k=0; k<MAXSENT; k++)
            {
              overlap[i][j][k]=malloc(BEAM*sizeof(char));
            }
        }

      superout[i]=malloc(WORDLEN*sizeof(char));

      outputstring[i]=malloc(3*sizeof(char*));
      beststring[i]=malloc(3*sizeof(char*));
      towopr[i]=malloc(MAXSENT*sizeof(char));
      for (j=0; j<3; j++)
        {
          outputstring[i][j]=malloc(WORDLEN*sizeof(char));
          beststring[i][j]=malloc(WORDLEN*sizeof(char));
        }
    }

  /* start up communications with the WOPR server */
  ignore_pipe();
  sock=make_connection(PORT,SOCK_STREAM,MACHINE);

  // open trigram predictions
  bron=fopen(argv[1],"r");

  strcpy(inputstring,"");
  while (!feof(bron))
    {

      // read from the trigram file
      for (i=0; ((i<FEATANDCLASS)&&(!feof(bron))); i++)
        fscanf(bron,"%s ",
          features[i]);
      if (!feof(bron))
        fscanf(bron,"%s ",
          class);
      if (!feof(bron))
        fgets(rest,100000000,bron);

      if (trigger>0)
        if ((strcmp(features[((FEATANDCLASS-1)/2)-1],"==")==0)||
            (feof(bron)))
          {
            for (i=0; i<nrinst; i++)
	      for (j=0; j<nralt[i]; j++)
		for (k=0; k<nrinst; k++)
		  for (l=0; l<nralt[k]; l++)
		    overlap[i][j][k][l]=0;

	    nrattempted=0;

            if (DEBUG)
              {
                fprintf(stderr,"\n-----------------------------------------------\n");
                fprintf(stderr,"%d words in sentence\n",
                  nrinst);
              }
              
            if (DEBUG)
              fprintf(stderr,"input: %s\n",
                inputstring);
            strcpy(inputstring,"");

	    if (DEBUG2)
	      {
		for (i=0; i<nrinst; i++)
		  {
		    fprintf(stderr,"%3d. ",
			    i);
		    for (j=0; j<nralt[i]; j++)
		      {
			fprintf(stderr,"%2d. [%s]-[%s]-[%s] ",
				j,classes[i][j][0],classes[i][j][1],classes[i][j][2]);
		      }
		    fprintf(stderr,"\n");
		  }
	      }
            
            // figure it all out

            // compute overlap, exhaustively
            nroverlap=0;
            for (i=0; i<nrinst; i++)
	      {
		thisnroverlap=0;
		for (j=0; j<nralt[i]; j++)
		  {
		    for (k=0; k<nrinst; k++)
		      for (l=0; l<nralt[k]; l++)
			{
			  //if (i!=k)
			    {
			      diff=0;
			      if (i>k)
				diff=i-k;
			      else
				diff=k-i;

			      // special overlap
			      if ((strcmp(classes[i][j][2],classes[k][l][0])==0)&&
				  (k==(i+1)))
				{
				  overlap[i][j][k][l]++;
				}
			      
			      // left-to-right overlap, immediate
			      if (strcmp(classes[i][j][2],classes[k][l][1])==0)
				{
				  overlap[i][j][k][l]++;
				}

			      if (strcmp(classes[i][j][1],classes[k][l][0])==0)
				{
				  overlap[i][j][k][l]++;
				}

			      //fprintf(stderr,"%2d -- %2d >>> %f - %f | %f - %f\n",
			      //      i,k,score[i][j],score[k][l],nndistance[i],nndistance[k]);

			      // rule out weak distant overlaps
			      if ((overlap[i][j][k][l]==1)&&
				  (diff>DIFFTHRESHOLD))
				{
				  overlap[i][j][k][l]=0;
				}
			      
			      if (DEBUG3)
				{
				  if (overlap[i][j][k][l]>0)
				    {
				      if (i>k)
					fprintf(stderr," left overlap: %2d - %2d, diff %2d: [%s][%s][%s] - [%s][%s][%s]\n",
						i,k,i-k,
						classes[i][j][0],classes[i][j][1],classes[i][j][2],
						classes[k][l][0],classes[k][l][1],classes[k][l][2]);
				      else
					fprintf(stderr,"right overlap: %2d - %2d, diff %2d: [%s][%s][%s] - [%s][%s][%s]\n",
						i,k,k-i,
						classes[i][j][0],classes[i][j][1],classes[i][j][2],
						classes[k][l][0],classes[k][l][1],classes[k][l][2]);
				    }
				}
			      
			      if (overlap[i][j][k][l]>0)
				{
				  overtab[nroverlap][0]=i;
				  overtab[nroverlap][1]=j;
				  overtab[nroverlap][2]=k;
				  overtab[nroverlap][3]=l;

				  // just take the product of the scores
				  //overlog[nroverlap]=overlap[i][j][k][l];
				  overlog[nroverlap]=overlap[i][j][k][l]*score[i][j]*score[k][l];
				  //overlog[nroverlap]=score[i][j]+score[k][l];

				  if (DEBUG2)
				    {
				      fprintf(stderr,"%2d. overlap %d,  between %d:%d:[%s-%s-%s] (score %f) and %d:%d:[%s-%s-%s] (score %f)\n",
					      nroverlap,
					      overlap[i][j][k][l],
					      i,
					      j,
					      classes[i][j][0],
					      classes[i][j][1],
					      classes[i][j][2],
					      score[i][j],
					      k,
					      l,
					      classes[k][l][0],
					      classes[k][l][1],
					      classes[k][l][2],
					      score[k][l]);
				    }
				  nroverlap++;
				  thisnroverlap++;
				}
			    }
			}       
		  }
	      }
	    
            if (DEBUG)
              {
                fprintf(stderr,"%d overlaps\n",
                  nroverlap);
              }

	    // generate translations; check if already generated
	    
	    // generate translations with random choices
	    ready=0;
	    counter=counter2=0;
	    bestperplex=999999999999.99;
	    bestlen=0;
	    nrtowopr=0;
	    while (!ready)
	      {
		seen=1;
		outputlen=0;
		counter3=0;
		while ((seen)&&
		       (counter3<LIMIT3)&&
		       (nrattempted<LIMIT2))
		  {
		    for (i=0; i<nrinst; i++)
		      done[i]=0;

		    // determine starter points; default first prediction
		    nrfollowers=0;
		    for (i=0; i<nroverlap; i++)
		      {
			if (strcmp(classes[overtab[i][0]][overtab[i][1]][0],"==")==0)
			  {
			    followers[nrfollowers]=i;
			    followerscore[nrfollowers]=overlog[i];
			    nrfollowers++;
			    done[overtab[i][0]]=1;
			  }
		      }
		    
		    if (nrfollowers==0)
		      {
			followers[0]=0;
			followerscore[0]=overlog[0];
			nrfollowers=1;
			done[overtab[0][0]]=1;
		      }

		    if (DEBUG2)
		      fprintf(stderr,"\nstarting run %d...........................\n",
			      counter);
		    if (DEBUG2)
		      {
			fprintf(stderr,"%d starting points: ",
				nrfollowers);
			for (i=0; i<nrfollowers; i++)
			  fprintf(stderr," %d (score %f) ",
				  followers[i],followerscore[i]);
			fprintf(stderr,"\n");
		      }
		    
		    outputlen=0;
		    chosen=0;
		    
		    while (nrfollowers>0)
		      {
			found=0;
			
			if (DEBUG3)
			  {
			    fprintf(stderr,"%d followers:\n",
				    nrfollowers);
			    for (i=0; i<nrfollowers; i++)
			      {
				fprintf(stderr,"  %d. score %f, distance %d\n",
					i,followerscore[i],overtab[followers[i]][0]-overtab[chosen][0]);
			      }
			  }
			
			if (nrfollowers>1)
			  {
			    // making a random choice, weighted by score
			    followertotal=0.0;
			    for (i=0; i<nrfollowers; i++)
			      {
				followerstart[i]=followertotal;

				if (overtab[i][0]>overtab[chosen][0])
				  diff=overtab[i][0]-overtab[chosen][0];
				else
				  diff=2*(overtab[chosen][0]-overtab[i][0]);

				followertotal+=followerscore[i]/(1.*diff);
				followerend[i]=followertotal;
				
				done[overtab[followers[i]][0]]=1;
			      }
			    ddice=followertotal*drand48();
			    
			    i=0;
			    while (!((ddice>followerstart[i])&&
				     (ddice<=followerend[i])))
			      i++;
			    
			    chosen=followers[i];

			    // making an unweighted random choice
			    /*
			    chosennr=nrfollowers*drand48();
			    chosen=followers[chosennr];
			    */
			   
			    // just take the highest score (works best so far)
			    /*
			    maxscore=0.0;
			    maxnr=0;
			    for (i=0; i<nrfollowers; i++)
			      if (followerscore[i]>maxscore)
				{
				  maxscore=followerscore[i];
				  maxnr=i;
				}
			    chosen=followers[maxnr];
			    */

			  }
			else
			  {
			    chosen=followers[0];
			  }
			
			strcpy(outputstring[outputlen][0],
			       classes[overtab[chosen][0]][overtab[chosen][1]][0]);		    
			strcpy(outputstring[outputlen][1],
			       classes[overtab[chosen][0]][overtab[chosen][1]][1]);
			strcpy(outputstring[outputlen][2],
			       classes[overtab[chosen][0]][overtab[chosen][1]][2]);
			outputlen++;
			
			// mark the position as done
			done[overtab[chosen][0]]=1;
			/*
			for (i=0; i<nroverlap; i++)
			  if ((overtab[i][0]==overtab[chosen][0])&&
			      (overtab[i][1]==overtab[chosen][1]))
			    done[overtab[i][0]]=1;
			*/
			
			// determine candidates to jump to
			nrfollowers=0;
			
			for (j=0; j<nroverlap; j++)
			  {
			    if (!done[overtab[j][0]])
			      {
				if ((overtab[j][0]==overtab[chosen][2])&&
				    (overtab[j][1]==overtab[chosen][3]))
				  {
				    followers[nrfollowers]=j;
				    followerscore[nrfollowers]=overlog[j];
				    nrfollowers++;
				  }
			      }
			  }

			if (DEBUG3)
			  {
			    fprintf(stderr,"%d follower points: ",
				    nrfollowers);
			    for (i=0; i<nrfollowers; i++)
			      fprintf(stderr," %d (distance %d)",
				      followers[i],overtab[followers[i]][0]-overtab[chosen][0]);
			    fprintf(stderr,"\n");
			  }
			
			if (nrfollowers==0)
			  {
			    // generate the overlap that did not have successors anyway
			    
			    if ((!done[overtab[chosen][2]])||
				(outputlen<2))
			      {
				strcpy(outputstring[outputlen][0],
				       classes[overtab[chosen][2]][overtab[chosen][3]][0]);
				strcpy(outputstring[outputlen][1],
				       classes[overtab[chosen][2]][overtab[chosen][3]][1]);
				strcpy(outputstring[outputlen][2],
				       classes[overtab[chosen][2]][overtab[chosen][3]][2]);
				outputlen++;
				
				// mark the position as done
				done[overtab[chosen][2]]=1;
			      }
			    
			    // gather all remaining possible continuations
			    
			    nrfollowers=0;
			    for (i=0; ((nrfollowers<MAXNRFOLLOWERS)&&(i<nroverlap)); i++)
			      if (!((done[overtab[i][0]])||
				    (done[overtab[i][2]])))
				{
				  followers[nrfollowers]=i;
				  followerscore[nrfollowers]=overlog[i];
				  nrfollowers++;
				}
			  }
		      }
		    
		    if (DEBUG2)
		      fprintf(stderr,"current output: ");
		    
		    supercount=0;
		    
		    // first trigram
		    if ((strcmp(outputstring[0][0],"==")!=0)&&
			(strcmp(outputstring[0][1],outputstring[1][0])==0))
		      {
			strcpy(superout[supercount],outputstring[0][0]);
			supercount++;
		      }		  
		    if ((strcmp(outputstring[0][1],outputstring[1][0])==0)||
			(strcmp(outputstring[0][2],outputstring[1][1])==0)||
			(strcmp(outputstring[0][2],outputstring[1][0])==0))
		      {
			strcpy(superout[supercount],outputstring[0][1]);
			supercount++;
			if ((strcmp(outputstring[0][2],outputstring[1][1])==0)||
			    (strcmp(outputstring[0][2],outputstring[1][0])==0))
			  {
			    strcpy(superout[supercount],outputstring[0][2]);
			    supercount++;
			  }
		      }
		    
		    // all middle trigrams
		    for (i=1; i<outputlen-1; i++)
		      {
			// special 2-0 overlap
			if (strcmp(outputstring[i][0],outputstring[i-1][2])==0)
			  {
			    strcpy(superout[supercount],outputstring[i][1]);
			    supercount++;
			    
			    //if (strcmp(outputstring[i][2],outputstring[i+1][1])==0)
			      {
				strcpy(superout[supercount],outputstring[i][2]);
				supercount++;
			      }
			  }
			else
			  {
			    
			    // only right word
			    if (strcmp(outputstring[i][1],outputstring[i-1][2])==0)
			      {
				if ((strcmp(outputstring[i][2],outputstring[i+1][1])==0)||
				    (strcmp(outputstring[i][2],outputstring[i+1][0])==0)||
				    (strcmp(outputstring[i][1],outputstring[i+1][0])!=0))
				  {
				    strcpy(superout[supercount],outputstring[i][2]);
				    supercount++;
				  }
			      }
			    else
			      {
				// middle and right
				if (strcmp(outputstring[i][0],outputstring[i-1][1])==0)
				  {
				    strcpy(superout[supercount],outputstring[i][1]);
				    supercount++;
				    
				    if (((strcmp(outputstring[i][2],outputstring[i+1][1])==0)||
					 (strcmp(outputstring[i][2],outputstring[i+1][0])==0)))
				      {
					strcpy(superout[supercount],outputstring[i][2]);
					supercount++;
				      }
				  }
				else
				  {
				    // all three

				    strcpy(superout[supercount],outputstring[i][0]);
				    supercount++;
				    
				    strcpy(superout[supercount],outputstring[i][1]);
				    supercount++;
				    
				    // if there is overlap, definitely print - or otherwise with a chance
				    if (((strcmp(outputstring[i][2],outputstring[i+1][1])==0)||
					 (strcmp(outputstring[i][2],outputstring[i+1][0])==0)))
				      {
					strcpy(superout[supercount],outputstring[i][2]);
					supercount++;
				      }
				  }
			      }
			  }
		      }
		    
		    // the final trigram  
		    if (strcmp(outputstring[outputlen-1][1],outputstring[outputlen-2][2])==0)
		      {
			strcpy(superout[supercount],outputstring[outputlen-1][2]);
			supercount++;
		      }
		    else
		      {
			if ((strcmp(outputstring[outputlen-1][0],outputstring[outputlen-2][1])==0)||
			    (strcmp(outputstring[outputlen-1][0],outputstring[outputlen-2][2])==0))
			  {
			    strcpy(superout[supercount],outputstring[outputlen-1][1]);
			    supercount++;
			  }
		      }
		    
		    // resolve case when there's nothing yet
		    if (supercount==0)
		      {
			strcpy(superout[0],outputstring[0][1]);
			strcpy(superout[1],outputstring[0][2]);
			supercount=2;
		      }
		    
		    // capitalize non-capitalized first word if it
		    // begins with a low-caps letter
		    if ((superout[0][0]>='a')&&
			(superout[0][0]<='z'))
		      superout[0][0]-=32;


		    // add a period
		    if (!((strcmp(superout[supercount-1],".")==0)||
			  (strcmp(superout[supercount-1],",")==0)||
			  (strcmp(superout[supercount-1],")")==0)||
			  (strcmp(superout[supercount-1],"?")==0)||
			  (strcmp(superout[supercount-1],"!")==0)))
		      {
			strcpy(superout[supercount],".");
			supercount++;
		      }
		    
		    strcpy(sentence,"");
                    for (i=0; i<supercount; i++)
                      {
                        if ((i>1)&&
                            (i<supercount-1)&&
                            (strcmp(superout[i],superout[i-2])==0)&&
                            (strcmp(superout[i+1],superout[i-1])==0))
                          {
                            if (DEBUG2)
                              fprintf(stderr,"[skipped %s %s] ",
                                      superout[i],superout[i+1]);
                            i+=2;
                          }

                        if ((i>0)&&
                            (i<supercount-1)&&
                            (strcmp(superout[i],superout[i-1])==0))
                          {
                            if (DEBUG2)
                              fprintf(stderr,"[skipped %s] ",
                                      superout[i]);
                            i++;
                          }

                        if ((strcmp(superout[i],"==")==0)||
                            (strcmp(superout[i],"???")==0)||
                            ((strcmp(superout[i],".")==0)&&
                             (i<supercount-2))||
                            (strstr(superout[i],"NULL"))||
			    (strstr(superout[i],"###")))
                          {
                            if (DEBUG3)
                              fprintf(stderr,"output %s suppressed at pos %d\n",
                                      superout[i],i);
                          }
                        else
                          {
                            strcat(sentence,superout[i]);
                            strcat(sentence," ");
                            if (DEBUG2)
                              fprintf(stderr,"%s ",
                                      superout[i]);
                          }
		      }
		    strcat(sentence,"\n");
		    
		    // now check if the sentence is already seen
		    
		    if (DEBUG3)
		      fprintf(stderr,"checking if %s is already tried\n",
			      sentence);
		    seen=0;
		    thislength=strlen(sentence);
		    for (i=0; ((i<nrattempted)&&(!seen)); i++)
		      if (thislength==attemptedlength[i])
			if (strcmp(sentence,attempted[i])==0)
			  seen=1;
		    counter3++;		
		    if (DEBUG3)
		      fprintf(stderr,"trying %d: %s",
			      counter3,sentence);
		    if (!seen)
		      {
			strcpy(attempted[nrattempted],sentence);
			attemptedlength[nrattempted]=thislength;
			nrattempted++;
			
			if (FILEWOPR)
			  {
			    // towopr
			    strcpy(towopr[nrtowopr],sentence);
			    nrtowopr++;
			  }
			else
			  {
			    // normal perplex directly to wopr
			    sprintf(woprsentence,"<s> %s </s>\n",
				    sentence);
			    perplex=wopr(woprsentence,sock);
			    
			    if (DEBUG3)
			      fprintf(stderr,"      unseen, with perplexity %f\n",
				      perplex);
			    
			    // English sentences have 15% more words than Dutch sentences
			    if (CORRECTFORLENGTHDIFF)
			      {
				if ((1.15*nrinst)>(1.*supercount))
				  perplex*=((1.15*nrinst)/(1.*supercount));
				else
				  perplex*=((1.*supercount)/(1.15*nrinst));
			      }
			    
			    if (perplex<bestperplex)
			      {
				counter=0;
				bestperplex=perplex;
				strcpy(bestsentence,sentence);
				if (DEBUG)
				  {
				    fprintf(stderr,"current lowest perplexity: %f, length %d, at round %d\n",
					    bestperplex,outputlen,counter2);
				    fprintf(stderr,"       >>> %s",
					    bestsentence);
				  }
				for (i=0; i<outputlen; i++)
				  for (j=0; j<3; j++)
				    strcpy(beststring[i][j],outputstring[i][j]);
				bestlen=outputlen;
				
				if (DEBUG2)
				  {
				    fprintf(stderr,"current best seq [%f]:",
					    bestperplex);
				    for (i=0; i<bestlen; i++)
				      fprintf(stderr," [%s]-[%s]-[%s]",
					      beststring[i][0],beststring[i][1],beststring[i][2]);
				    fprintf(stderr,"\n");
				  }
			      }
			    else
			      counter++;
			  }
		      }
		    else
		      {
			if (DEBUG3)
			  fprintf(stderr,"already seen; counter3 %d, counter2 %d, counter %d\n",
				  counter3,counter2,counter);
		      }
		  }    
		counter2++;
		
		if ((counter>=LIMIT)||
		   (counter3>=LIMIT3))
		  {
		    if (DEBUG2)
		      fprintf(stderr,"counter reached limit count %d\n",
			      LIMIT);
		    ready=1;
		  }
	      }

	    if (FILEWOPR)
	      {
		// write woprfile
		woprfile=fopen("/tmp/wopr-sents","w");
		for (i=0; i<nrtowopr; i++)
		  fprintf(woprfile,"%s",towopr[i]);
		fclose(woprfile);
		
		// send to wopr
		strcpy(bigwoprcommand,"file:/tmp/wopr-sents");
		bigwopr(bigwoprcommand,sock);
		
		// read back
		bestperplex=99999.99;
		woprfile=fopen("/tmp/wopr-sents.wopr","r");
		for (i=0; i<nrtowopr; i++)
		  {
		    fscanf(woprfile,"%f ",
			   &thisperplex);
		    if (thisperplex<bestperplex)
		      {
			bestperplex=thisperplex;
			strcpy(bestsentence,towopr[i]);
		      }
		  }
	      }

	    if (DEBUG)
	      {
		fprintf(stderr,"best seq [perplex %f]: %s\n",
			bestperplex,bestsentence);
	      }
	    
	    fprintf(stdout,"%s",
		    bestsentence);
	    
            nrinst=0;
            if (DEBUG2)
	      getchar();
          }

      // read the rest from the trigram output stored in the "rest" buffer
      if ((nrinst<MAXSENT)&&
	  (rest[0]=='{'))
        {
          nralt[nrinst]=0;
          part=strtok(rest," \n\t");
          part=strtok(NULL," \n\t");
          while ((part!=NULL)&&
                 (strcmp(part,"}")!=0)&&
		 (nralt[nrinst]<BEAM))
            {
	      // parse class

	      if (strcmp(part,"NULL")!=0)
		{
		  strcpy(hulp,"");
		  i=0;
		  while ((i<strlen(part))&&
			 (part[i]!='^'))
		    {
		      strcat(hulp," ");
		      hulp[strlen(hulp)-1]=part[i];
		      i++;
		    }
		  strcpy(classes[nrinst][nralt[nrinst]][0],hulp);
		  i++;
		  strcpy(hulp,"");
		  while ((i<strlen(part))&&
			 (part[i]!='^'))
		    {
		      strcat(hulp," ");
		      hulp[strlen(hulp)-1]=part[i];
		      i++;
		    }
		  strcpy(classes[nrinst][nralt[nrinst]][1],hulp);
		  i++;
		  strcpy(hulp,"");
		  while (i<strlen(part))
		    {
		      strcat(hulp," ");
		      hulp[strlen(hulp)-1]=part[i];
		      i++;
		    }
		  strcpy(classes[nrinst][nralt[nrinst]][2],hulp);
		  
		}

	      // read vote
	      part=strtok(NULL," \n\t");
	      strcpy(hulp,part);
	      if (strstr(hulp,","))
		sscanf(hulp,"%f, ",&score[nrinst][nralt[nrinst]]);
	      else
		sscanf(hulp,"%f ",&score[nrinst][nralt[nrinst]]);

	      //score[nrinst][nralt[nrinst]]=log(score[nrinst][nralt[nrinst]]);

	      nralt[nrinst]++;

	      // read next trigram class
	      part=strtok(NULL," \n\t");
          
            }

          if (DEBUG2)
            fprintf(stderr,"instance %d: %d alternatives;\n",
              nrinst,nralt[nrinst]);

	  /*
          if (strcmp(part,"}")!=0)
            while ((part!=NULL)&&
                   (strcmp(part,"}")!=0))
              part=strtok(NULL," \n\t");
          part=strtok(NULL," \n\t");
          sscanf(part,"%f",&nndistance[nrinst]);

	  for (i=0; i<nralt[nrinst]; i++)
	    {
	      //score[nrinst][i]*=nndistance[nrinst];
	      if (nndistance[nrinst]==0.0)
		score[nrinst][i]*=5.;
	      else
		score[nrinst][i]*=1./nndistance[nrinst];
	    }

          if (DEBUG2)
            {
              fprintf(stderr,"nn distance: %f\n",
                nndistance[nrinst]);
            }
	  */

	  if ((nrinst>1)&&
	      //(nralt[nrinst]==1)&&
	      //(nralt[nrinst-1]==1)&&
	      (strcmp(classes[nrinst][0][0],classes[nrinst-1][0][0])==0)&&
	      (strcmp(classes[nrinst][0][1],classes[nrinst-1][0][1])==0))
	    //(strcmp(classes[nrinst][0][2],classes[nrinst-1][0][2])==0))
	    nrinst--;

	  strcat(inputstring,features[(FEATANDCLASS/2)-1]);
	  strcat(inputstring," ");
	  
	  nrinst++;
	  trigger++;

        }
      strcpy(prevclass,class);

    }

  //fprintf(stdout,"\n");
  fclose(bron);

  close(sock);

  timer();

  return 0;
}

/* timer - understandable time
 */
void timer(void)
{ struct tm *curtime;
  time_t    bintime;

  time(&bintime);
  curtime = localtime(&bintime);
  fprintf(stderr,"current time: %s\n", asctime(curtime));
}

float wopr(char *woprstring, int woprsock)
{
  float perplex=9999.99;
  int   i;
  char  perplexstring[1024];
  
  if (woprsock==-1)
    {
      fprintf(stderr,"WOPR server is not responding; aborting.\n\n");
      exit(1);
    }
  
  if (strlen(woprstring)<2)
    strcpy(woprstring,"dummy \n");

  if (DEBUG3)
    fprintf(stderr,"sending to WOPR: %s\n",
	    woprstring);
			
  /* throw the instance at the socket */
  sock_puts(woprsock,woprstring);

  strcpy(buffer,"");
  /* get the TiMBL server output back */
  sock_gets(woprsock,buffer,BUFFERSIZE);
			
  if (DEBUG2)
    fprintf(stderr,"got back from WOPR: %s\n",
	    buffer);
  if ((strlen(buffer)<2)||
      (woprsock==-1))
    { 
      fprintf(stderr,"The WOPR server is not responding; trying again.\n\n");
      close(woprsock);
      /* start up communications with the WOPR server */
      ignore_pipe();
      woprsock=make_connection(PORT,SOCK_STREAM,MACHINE);

      if (DEBUG3)
	fprintf(stderr,"sending again to WOPR: %s\n",
		woprstring);
      
      /* throw the instance at the socket */
      sock_puts(woprsock,woprstring);
      
      strcpy(buffer,"");
      /* get the TiMBL server output back */
      sock_gets(woprsock,buffer,BUFFERSIZE);
			
      //if ((strlen(buffer)<2)||
      //  (woprsock==-1))
      if (woprsock==-1)
	{ 
	  fprintf(stderr,"WOPR server is not responding; really bailing out now.\n\n");
	  exit(1);
	}
    }

  //sscanf(buffer,"%f",&perplex);

  i=0;
  while ((i<strlen(buffer))&&
	 (buffer[i]!=']'))
    i++;
  if (i<strlen(buffer))
    {
      i+=11;
      strcpy(perplexstring,"");
      while ((i<strlen(buffer))&&
	     (buffer[i]!='\''))
	{
	  strcat(perplexstring," ");
	  perplexstring[strlen(perplexstring)-1]=buffer[i];
	  i++;
	}
      sscanf(perplexstring,"%f",&perplex);
    }

  return perplex;
}

void bigwopr(char *woprstring, int woprsock)
{
  if (woprsock==-1)
    {
      fprintf(stderr,"WOPR server is not responding; aborting.\n\n");
      exit(1);
    }
  
  if (strlen(woprstring)<2)
    strcpy(woprstring,"file:/tmp/woprsents\n");

  if (DEBUG3)
    fprintf(stderr,"sending to WOPR: %s\n",
	    woprstring);
			
  /* throw the instance at the socket */
  sock_puts(woprsock,woprstring);

  strcpy(buffer,"");
  /* get the TiMBL server output back */
  sock_gets(woprsock,buffer,BUFFERSIZE);

  if (DEBUG2)
    fprintf(stderr,"got back from WOPR: %s\n",
	    buffer);
}
