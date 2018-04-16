// Program to analyze the PITSCHI data.
// pitschi_analyze ANdi Klein
// Feb/March 2008
/*
$Date: 2008/03/20 22:23:47 $
$Id: pitschi_analyze.cc,v 1.3 2008/03/20 22:23:47 daqdev Exp $

$Log: pitschi_analyze.cc,v $
Revision 1.3  2008/03/20 22:23:47  daqdev
scalers included

Revision 1.2  2008/03/20 21:30:54  daqdev
 stable

*/

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>

#include <iostream.h>
#include <iomanip.h>
 
#include "TFile.h"
#include "TH1.h"
#include "TH2.h"
#include "TF1.h"
#include "TTree.h"
#include "TProfile.h"
#include "TNtuple.h"
#include "TRandom.h"
#include "TText.h"

#include <time.h>
#include <sys/time.h>

#include "data.h"

#define MYDEBUG 0
#define VERBOSE 0
#define DEBUG_BANKS 1
#define SKIP 0
#define MAXCHAN 200	// max number of CAMAC channels
#define BIT_14 0x2000
#define PATH "/home/daqdev/online"
#define DATA_PATH "/home/daqdev/online/data"
///#define PEDESTAL_FILE "/home/fission/online/pedestals.dat"

// how often to update the root file when 
// analyzing from shared memory (every so many banks)
//prototypes
int analyze(int);

TH1F *htime;
TH1F *hch[CHAN];
TH2F *h2xy;

//TTree *event_tree;  // the tree which holds all the variables
TTree *stree;
TTree *rtree;
int channel[MAXCHAN]; // Right now 200  is the max number of data/event
		  // I foresee
FILE *fp1;
FILE *pfile;

int run,run1,run2;
char path[100];
char fname1[100];

int skip_to_bank=0;
ushort dataB[514]; // buffer for input record, should be big enough.
int dataB1[100]; // buffer for input record, should be big enough.
int dummy[4]; // for subehader

char hname[100]; char htitle[100];
int i,ii;
int NUM_CHAN1=0;
int run_start_time;
		int num_scalers=0;

int main(int argc,char *argv[]){
  
  	printf("********************************************************** \n");
	printf("*                                                        * \n");
	printf("           PITSCHI_ANALYZE Vs 1.0                        * \n");
	printf("*                                                        * \n");
  	printf("********************************************************** \n");


   run1 = atoi(argv[2]);
   run2 = atoi(argv[3]);
   NUM_CHAN1 = atoi(argv[1]);
   
   for(run=run1;run<=run2;run++)
     {
       strcpy(path,DATA_PATH); /* default path */
       if (argc>=5) {strcpy(path,argv[4]);}
       
       //if in debug mode, use this to skip to 
       // start printing stuff after some number of banks.
       if (MYDEBUG){if (argc>=6) {skip_to_bank = atoi(argv[5]);}}
       
       //   sprintf(fname1,"%s/run-%04d.dat",path,run);
       sprintf(fname1,"%s/run-%d.dat",path,run);
       
       fp1 = fopen(fname1,"rb");
       
       if (fp1== NULL) {
	 printf("File can't be opened. Exiting.\n");
	 return(-1);
       }
       
       if (MYDEBUG) {
	 printf("Opened file: %s \n",fname1);
	 printf("Calling analyze function. \n");
       }
       
       analyze(2);  // argument 2 means from file fp1
       
       if (fp1 != NULL){ fclose(fp1); }
     }

  return 0;
}
int analyze(int option){

  header *thisheader;
  struct timeval  tv1;
 

  char mydate[100];
  char hour[3];
  char minute[3];
  char second[3];
  int hour_int,minute_int,second_int,time_int;
  int time_old(0);

  //structs for the trees.
  PADC_EVENT padc_event; 


  TFile *hfile;



/* currently only option 2 implemented */ 
  if (option != 2) {
    printf("analyze() subroutine needs to be called with proper argument.\n");
    return 0;
  }
  if (MYDEBUG) printf("Opening root file.\n");

    char rootfile[100];
    if (option==2) sprintf(rootfile,"%s/root/run-%d.root",PATH,run);

    // if debug mode, don't want to overwrite an existing root file
    // overwrite the "rootfile" filename here...
    if (MYDEBUG) sprintf(rootfile,"%s/root/run-debug.root",PATH);

    hfile = new TFile(rootfile,"RECREATE","ROOT file");

    if (MYDEBUG) printf("Booking histograms and trees.\n");

	int num_events=0;  // number of events in one block
	int num_lines=0;	// number of lines in that block including terminator line
	int num_event_words=0;
	int scaler[100];	// max number of scalers
     int NUM_CHAN=4;
    
     int eventID=0;	// event number;

		if(NUM_CHAN1 !=0)NUM_CHAN=NUM_CHAN1;
     // PADC tree
      TTree event_tree("event_tree"," event data");

//	event_tree.Branch("eventID",&eventID,"eventID/I");
     // create the tree according to the number of channels
    for(int num=0;num<NUM_CHAN;num++)
 //	int num=100;
//	int* channel= new int[num];
     	{
     		event_tree.Branch(Form("channel_%d",num),&channel[num],Form("channel_%d/I",num));
 //   		event_tree.Branch("channel",channel,Form("channel[%d]/I",num));

	}

     
   //done with setting up root stuff.

  //now, intitialize stuff for the main analysis loop


  thisheader = new header;
   
  int mycontinue = 1;
  int analyzed_banks = 0;
  int read_size;

  int total_bytes_read = 0;

  // main analysis loop, exited when mycontinue variable set to 0 
  // inside (for example at an end of run bank)

  while(mycontinue)
  
  
 {	// begin large mycontinue loop
    if (option == 2) 
   {

     read_size = fread(thisheader, 1, sizeof(header), fp1);

     total_bytes_read += read_size;

     if (MYDEBUG && analyzed_banks>=skip_to_bank) 
       printf("\n: %d \n",read_size);

     if (read_size<=0)
     {
       mycontinue=0; 
       if (MYDEBUG) printf("End of file.\n");
       break;
     }
   }


   tv1.tv_sec = thisheader->time;


   if (thisheader->code >= 0)
   {
   
    strcpy(mydate,ctime(&tv1.tv_sec));
//    printf("mydate is %s \n",mydate);
    mydate[24]='\0'; // get rid of newline at the end
    mydate[19]='\0'; // and get rid of the year.

// determine time
    hour[0]=mydate[11]; hour[1]=mydate[12];
    minute[0]=mydate[14]; minute[1]=mydate[15];
    second[0]=mydate[17]; second[1]=mydate[18];
    hour[2]='\0';
    minute[2]='\0';
    second[2]='\0';

    hour_int=atoi(hour);
    minute_int=atoi(minute);
    second_int=atoi(second);  
    time_int=3600*hour_int+60*minute_int+second_int;    
    if(time_int<time_old) time_int=time_int+86400; //if went over midnight, add 24*3600 sec.
      time_old=time_int;

//    printf("\n lets see if this works...%s %s %s %d \n",hour,minute,second,time_int);

    if (MYDEBUG && analyzed_banks>=skip_to_bank) 
    	{

      		//if (thisheader->code==0x0f0f0f0f) break; 

     		printf(" code1: %08x   ",thisheader->code);
     		printf(" name: "); 
        		for (ii=0;ii<4;ii++){printf("%c",thisheader->name[ii]);}
     		printf(" \n");
     		printf(" time: %10d  size: %d\n",thisheader->time,thisheader->size);
     		printf(" PITSCHI bank date/time: %s \n",mydate);
    	}
   }
   if (MYDEBUG && analyzed_banks>=skip_to_bank) 
    {
        if ( ((thisheader->time - run_start_time)%10) == 0 )
	{
         printf("elapsed -%ds-\n",(thisheader->time - run_start_time));
       fflush(stdout);
   	}
    }
    // get the rest of the bank into data buffer.
      if (thisheader->code==0x11111111) 
      {
      	/*got a PITSCHI adc value ; skip over subheader now */
	if(VERBOSE) 
	  {
	    printf(" code: %08x   ",thisheader->code);
	    printf(" name: "); 
	    for (ii=0;ii<4;ii++){printf("%c",thisheader->name[ii]);}
	    printf(" \n");
	    printf(" time: %10d  size: %d\n",thisheader->time,thisheader->size);
	    printf(" PITSCHI bank date/time: %s \n",mydate);
	    printf("got a PITSCHI adc block");
	  }
	read_size = fread(dummy, 4,4,fp1); //skip subheader
	padc_event.t_ms= dummy[0];
	tv1.tv_usec=dummy[0];
	if (VERBOSE) printf(" - time %lf \n",padc_event.t_ms);
			total_bytes_read += read_size;
	read_size = fread(dataB, 2, thisheader->size, fp1);
			total_bytes_read += read_size;
		if(MYDEBUG)
		{
			for(int testr=0;testr<thisheader->size;testr++)
			{
			printf(" data %d \n",dataB[testr]);
			}
		}
      }

      if (thisheader->code==0xeeeeeeee) 
      {
      	/*got a PITSCHI adc value ; skip over subheader now */
        read_size = fread(dataB1, 1, thisheader->size-16, fp1);
	if (MYDEBUG) printf("header size for eeeeeeee %d \n",thisheader->size);

      }
      /* now skip over subheader */
      




    if (thisheader->size > 0 )
  {
      // from file
      if (option==2)
      {
	if (MYDEBUG) printf("header size %d \n",thisheader->size);
//	int iloop=0;
//	for(iloop=0;iloop<512;iloop=iloop+1)
//	{
//	printf("data %d  %d \n", dataB[iloop] ,iloop);
//	if(dataB[iloop]==0){
//	printf("finished %d \n" ,iloop);
//	break;
//	}
//	}

        
	

        		if (MYDEBUG && analyzed_banks>=skip_to_bank) 
          		printf("read size of data: %d  total_bytes_read: %d\n",
          		read_size,total_bytes_read);

       		if (  ((read_size < thisheader->size) &&(thisheader->code!=0xeeeeeeee)) 
	             || 
		      ((read_size+16<thisheader->size)&&(thisheader->code==0xeeeeeeee)) 
		      )
		{

	  		printf("Analysis code stopping, possibly ungraceful end of run.\n");
	  		printf("Error in read. read_size=%d thisheader->size=%d\n",
	        read_size, thisheader->size); 

	  		// print out header info... then exit.
     		printf(" code: %08x   ",thisheader->code);
     		printf(" name: "); 
        		for (ii=0;ii<4;ii++){printf("%c",thisheader->name[ii]);}
     		printf(" \n");
     		printf(" time: %10d  size: %d\n",thisheader->time,thisheader->size);
     		printf(" PITSCHI bank date/time: %s \n",mydate);

     		printf("analyzed banks = %d \n", analyzed_banks);
     		printf("total bytes read = %d \n", total_bytes_read);

	 	//exit(0);

	 		// just exiting isn't so graceful, use mycontinue to 
	 		// exit the while, then save root file.
	 		mycontinue=0; continue;
        	}
        }

    } // if this_header->size > 0 
    
    
    
    
    
    
    analyzed_banks++;
    if (option==2) 
    { //offline
      if ((analyzed_banks % 1000000)==0) 
      {
        printf("%d ",analyzed_banks);
        if ((analyzed_banks % 10000000)==0) printf("\n");
        fflush(stdout);
      }
     
    }


    if (thisheader->code==0x11111111)
    	{
 		// the block length is thisheader->size
		// each event is NUM_CHAN +terminator long
		// the first word in the buffer says how many events
		// the second tells how many lines including a buffer terminator line
		// the curious this are the watchdog events, which are
		// siginfied by Bit_14 + number of events; i.e. x8006 means watchdog + g events
		// following
		// as a consistency check I compare number of dataline/event with
		// number of cannels given
		num_events=dataB[0];
		num_lines=dataB[1];
		int off=1;
		for(int outloop=0; outloop<num_lines;outloop++)
		 {
		     off=off+1;
		     if(dataB[off]==0xffff)off++;	//end of event
		     if(dataB[off]==0xffff){
		     break;
		     } 	//end of buffer
		     
		     if(off==num_lines)break;
		     num_event_words=dataB[off];
		     
			if(num_event_words & BIT_14)
			{	
			num_event_words=num_event_words-BIT_14;
		   		for(int iloop=0;iloop<num_event_words-1;iloop++)
		   		{
				num_scalers=num_event_words-1;
				off=off+1;
				scaler[iloop]=dataB[off];	
				}
			}
			else
			{
			  //check for given number of channel versus words in
			  //events
			  if(NUM_CHAN != num_event_words-1){
			  printf("you have a problem, the number of words/event is not the same \n as you said there are readout channels %d %d \n",NUM_CHAN,num_event_words-1);
			   }
			  for(int iloop=0;iloop<num_event_words-1;iloop++)
		   		{
				off=off+1;
				channel[iloop]=dataB[off];
			if(MYDEBUG)	printf("dataB %d  off %d \n",dataB[off],off);
				}
 				event_tree.Fill();
			
 		       }
 
 		}
				
	}
} /* end of mycontinue */

		   		for(int iloop=0;iloop<num_scalers;iloop++)
		   		{
				printf(" Scaler # %3d : %d \n",iloop,scaler[iloop]);	
				}

    printf("\n Run %d : Writing histograms.  Analyzed banks = %d\n",run,analyzed_banks);

    // Save all objects in the ROOT file
    hfile->Write();
    // Close the file. Note that this is automatically done when you leave
    // the application.
    hfile->Close();



return(1);
}
