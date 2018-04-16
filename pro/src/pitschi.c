/* 
  	pitschi.c  version 0.1
    -----------------------------------------------------------------------------
	written by Andi Klein
	Feb 2008

	$Date: 2008/03/25 21:57:22 $
	$Id: pitschi.c,v 1.25 2008/03/25 21:57:22 daqdev Exp $
	
	
	
	$Log: pitschi.c,v $
	Revision 1.25  2008/03/25 21:57:22  daqdev
	 path definition through enviro

	Revision 1.24  2008/03/25 20:38:23  daqdev
	 canges in camac_stack

	Revision 1.23  2008/03/25 20:35:02  daqdev
	 debug version

	Revision 1.22  2008/03/19 22:48:33  daqdev
	 debugged version

	Revision 1.21  2008/03/13 22:06:40  daqdev
	writing data file

	Revision 1.20  2008/03/12 21:51:38  daqdev
	 changed option for camac stack

	Revision 1.19  2008/03/11 21:10:35  daqdev
	first working version

	Revision 1.18  2008/03/07 23:23:45  daqdev
	 stll read out problems in stack

	Revision 1.17  2008/03/06 22:26:52  daqdev
	 added module test

	Revision 1.16  2008/03/06 21:42:10  daqdev
	maintenance

	Revision 1.15  2008/03/05 20:27:26  daqdev
	routine

	Revision 1.14  2008/03/04 23:09:27  daqdev
	 added start/stop daq register write and stop DAQ in initializing

	Revision 1.13  2008/02/27 00:02:03  daqdev
	 added main event loop

	Revision 1.12  2008/02/26 22:24:47  daqdev
	 included module setup

	Revision 1.11  2008/02/26 22:01:59  daqdev
	 scaler stack added

	Revision 1.10  2008/02/26 20:25:02  daqdev
	 debug

	Revision 1.9  2008/02/26 16:45:26  daqdev
	succesful writing the stack

	Revision 1.8  2008/02/26 00:19:48  daqdev
	 stack construction

	Revision 1.7  2008/02/25 15:56:21  daqdev
	 register setting implemented

	Revision 1.6  2008/02/21 22:07:37  daqdev
	data file opening and correct error handling





    -----------------------------------------------------------------------------
*/

/*#include <stdlib.h>*/
#include <stdio.h>
#include <stdint.h>
#include <stdarg.h>
#include <string.h>
#include <sys/time.h>
#include <time.h>
#include <unistd.h>   /*for usleep function*/
#include <math.h>
#include <cstdlib>
#include <usb.h>
#include <signal.h>

/* includes from disribution */
#include "libxxusb.h"
#include "../include/pitschi.h"


#define MYDEBUG 3
#define MYTESTS 1
#define STACKTEST 1

#define DATA "/online/data"	/* default path name for data*/
#define REGISTER_PATH "/home/daqdev/online"    /* location for camac control registers */
#define RUN "/online/runfile.in"  /*this file keeps track of the run number"*/
#define REGISTER "/online/camac_register.in"  /*this file sets the register values*/
#define STACK "/online/camac_stack.in"  /*this file sets the register values*/

#define COMMENT "/online/control/comment.text"
#define RUN_LENGTH 5 /* default run length in 5 seconds*/
#define HEADER 4	/* for data file */
#define SUBHEADER 4


/* time and date variabbles */
struct timeval tv_start;
struct timeval tv1;

/*Global variables */
/* variables for DAQ loop */
int delta_t=0;
int delta_t_us=0;  /*micro seconds*/
int delta_t_ms=0;  /*milli seconds*/
int main_loop_counter =0;
int event_counter=0;
int continue_run=1;
int run_length , requested_events;
int error_code;
int TEST_MODULE=0; /* this flag is set from command line if you want to test a single module*/

char *path_help;


int header[HEADER];
int subheader[SUBHEADER];



/* camamc specific globals */
    int CamN, CamA, CamF;
    long CamD;
    int CamQ, CamX;
    usb_dev_handle *udev;       // Device Handle 

	int err_num=0; //* error code for pitschi*/
FILE *outfile;
char path[100];

// block for different path assignments
char DATA_PATH[80];
char RUN_FILE[80];
char REGISTER_FILE[80];
char STACK_FILE[80];
char COMMENT_FILE[80];


/*
    -----------------------------------------------------------------------------

      Main program

    -----------------------------------------------------------------------------
*/


int main(int argc, char *argv[]) 

{

    xxusb_device_type devices[100]; 
    struct usb_device *dev;

	short status;	//status gives code numbers back from calls
			// sucess is 1, if anything else it is an error or
			//warning




  // length of run in seconds, +-1 due to rounding of microseconds
  // default is the RUN_LENGTH value
  run_length = RUN_LENGTH;

  // if requested_events > 0, 
  // then terminate when get to that many PADC events
  requested_events = 0;

  if (MYDEBUG==1)
    printf("PISTCHI argc: %d \n", argc);

  if (argv[1]) {
  int ii;
    for (ii = 1; ii < argc; ii++) {
      if (strncmp(argv[ii], "-t", 2) == 0) {
        run_length = atoi(argv[ii + 1]);
      }
      if (strncmp(argv[ii], "-e", 2) == 0) {
        requested_events = atoi(argv[ii + 1]);
      }
      if (strncmp(argv[ii], "-z", 2) == 0) {
        TEST_MODULE=1;
      }

    }
  }

  if (run_length < 0 || run_length > 1e6) {
    run_length = RUN_LENGTH;
  }
  if (requested_events < 0 || requested_events > 1e10) {
    requested_events = 0;
  }

  if (MYDEBUG==1)
    printf("PISTCHI run length %d seconds\n", run_length);
  if (MYDEBUG==1)
    printf("PITSCHI requested events? (>0) %d\n", requested_events);


/********************** Open up USB device *******************************/

    //Find XX_USB devices and open the first one found
    xxusb_devices_find(devices);
    dev = devices[0].usbdev;
    printf("device %x \n ",*dev);
    udev = xxusb_device_open(dev); 
    
    // Make sure CC_USB opened OK
    if(!udev) 
    {
	     printf ("\n\nFailedto Open CC_USB \n\n");
	     return 0;
    }
    

	// In case the controller hung in DAQ mode. we want to stop it first
	
    		status=xxusb_register_write(udev,1,0);

/***********************************************************************/
// Setup all the path variables
//	get PATH from environment
	path_help=(getenv("PITSCHI"));
	if(path_help !=NULL) 
	{
	strncpy(DATA_PATH,path_help,sizeof(DATA_PATH)-sizeof(DATA));
	strcat(DATA_PATH,DATA);

	strncpy(RUN_FILE,path_help,sizeof(RUN_FILE)-sizeof(RUN));
	strcat(RUN_FILE,RUN);
	
	strncpy(REGISTER_FILE,path_help,sizeof(REGISTER_FILE)-sizeof(REGISTER));
	strcat(REGISTER_FILE,REGISTER);

	strncpy(STACK_FILE,path_help,sizeof(STACK_FILE)-sizeof(STACK));
	strcat(STACK_FILE,STACK);

	strncpy(COMMENT_FILE,path_help,sizeof(COMMENT_FILE)-sizeof(COMMENT));
	strcat(COMMENT_FILE,COMMENT);
	}
	else
	{
	pitschi_error(udev,9);
	}



	printf(" Home %s \n",path_help);
	
	  
 

/*****************************Initialize Runfiles and data files********/
char runfil[100];
strcpy(runfil,RUN_FILE);
/* the runfile has the number of the last run in */

int run_number; /*run number*/
int run_number_new; /* new run number*/
FILE* run;
	if( (run=fopen(runfil,"r"))== NULL)
	{
	status =pitschi_error(udev,2);
	}
	else
	{
	fscanf(run,"%d4",&run_number);
	fclose(run);
	run=fopen(runfil,"w"); /*overwrite run number file*/
	run_number_new=run_number+1;
	fprintf(run,"%4d",run_number_new);
	fclose(run);
	}
		


char out_file[100];	/*output file name */

strcpy(path,DATA_PATH);
sprintf(out_file,"%s/run-%d.dat",path,run_number);
printf(" new run %d \n ",run_number);
status = pitschi_init_out(out_file);







/**********************************Initialize and Clear Camac***********/

	status=pitschi_init(udev);

	int dummy=0;


/***********************we only want to test a module********************/
	if(TEST_MODULE){
	pitschi_test_module(dummy);
	pitschi_close(udev);
	}
	





/**********************************Set Camac Registers***********/




	status=pitschi_set_register( dummy);


/**********************************Create Camac Command stack***********/
	status=pitschi_create_stack(dummy);

	if(STACKTEST)
		{ 
		status = pitschi_test_stack( dummy);
		}


/***************************** We are done with setting up the system *************/
	
		pitschi_write_header(1);	// here we write beginning of run info and all the
						// stack and register information






/***************************** here we loop over the USB buffer ******************/
  char mydate[100];

    gettimeofday(&tv_start, NULL);
    strcpy(mydate, ctime(&tv_start.tv_sec));
    mydate[24] = '\0';          // get rid of newline at the end
    mydate[19] = '\0';          // and get rid of the year.
      printf("pitschi  start date/time: %s %d \n", mydate,
             (int) tv_start.tv_sec);

	status=pitschi_daq_loop( dummy);









/*******************************Close everything **********************/

	status=pitschi_close(udev);
	
/**********************************************************************/

}







/************************************here start the different calls ****/


short pitschi_init(usb_dev_handle *udev)
{
 	int NOB;
	short status;
	
    CAMAC_Z(udev);
		CAMAC_I(udev, true);
		CAMAC_write(udev, 1, 0, 16, 0xaaaaaa,&CamQ, &CamX);
		CAMAC_read(udev, 1, 0, 0, &CamD,&CamQ, &CamX);
		CAMAC_C(udev);
		CAMAC_I(udev, false);
		CAMAC_Z(udev);

//*  Now print out the firmware version */

      CamD=0;
    NOB=CAMAC_register_read(udev,0  ,&CamD);
    if(NOB<0)
    {
    pitschi_error(udev,1);
    }
    else
    {
    status=1;
  printf(" number of bytes read %i  Firmware version : %lx \n ",status,CamD);
    }
  return(status);
 }

/**************************pitschi_init_out********************/
/* 	opens data file, needs to be root file					   */
/***********************************************************/

short pitschi_init_out(char *fname)
{
#include <stdio.h>

/* setup the output file
/* fname is the output  file name */
	
	if(( outfile=fopen(fname,"r"))==NULL)
	
	outfile=fopen(fname,"wb");  /*open binary output file */
	
	else
	
	{
	short status=pitschi_error(udev,3);
	}
return(1);
}







/**************************pitschi_close********************/
/* close usb device and open files			   */
/***********************************************************/

short pitschi_close(usb_dev_handle *udev)
{
  	int errorstat=0;
    // Close the Device

	pitschi_write_header(2);
	fclose(outfile);
	
	    xxusb_device_close(udev);
	    printf("\n\n\n");
    
    
    exit( errorstat);	// leave the program
    return 1;
}


/*************************pitschi_set_register***********/
/* this routine read file camac_register.in 		*/
/*							*/
/* every line consiststs of two hex numbers:		*/
/* the first is the address of the regsister (see page 15)*/
/* while the second one gives the value to be set	*/

short pitschi_set_register(int dummy)
{
FILE* reg_file;
char registerfil[100];
short status;
int reg_addr;	// camac registers address
long reg_value;	//value to be set in registers
long reg_readback;	//value read back from registers

    char nafin[20];
    char comment[20];


	printf("PITSCHI----------- setting registers---------\n");


strcpy(registerfil,REGISTER_FILE);
	if( (reg_file=fopen(registerfil,"r"))== NULL)
	{
	status =pitschi_error(udev,4);
	}
	else
	{


		while(reg_addr !=-1)
	{
			fscanf(reg_file,"%d  %lx ", &reg_addr,&reg_value);

		if(reg_addr==-1)
		{
		fclose(reg_file);
		printf("PITSCHI----------- finished setting registers-----\n");
		return(1);
		}

	printf("reg addr %04x  reg value %08x    ", reg_addr, reg_value);
		
		// leave out reg addr 0 readonly
		if(reg_addr != 0) 
		   {
			status=CAMAC_register_write(udev,reg_addr,reg_value);
		   }
		   
		if(status<0) pitschi_error(udev,5);

		// let's give the controller some time to settle.
		usleep(1000);
		
		// Now read back the value we set and print them out
		reg_readback=0;
		status=CAMAC_register_read(udev,reg_addr,&reg_readback);
		printf("Register address %04x has been read back as %08x \n",reg_addr, reg_readback);
	}

	fclose(reg_file);
}
		
	return(1);
}



/*************************pitschi_create_stack***********/
/* this routine read file camac_stack.in 		*/
/*							*/
/* every line consiststs of three hex numbers:		*/
/* the first is the address of the regsister (see page 15)*/
/* while the second one gives the value to be set	*/
/*	options: BIT1 : long word			*/






short pitschi_create_stack(int dummy)
{
FILE* stack_file;
char stackfil[100];
short status;

//    char *comment;
	char comment[80];
    short stacklines=0;	// Number of stack lines
    short write_line =0 ;	// Number of data lines to write (eiter 1 or 2)
    ushort stack[768];	// the actual stack lines
    short CamF,CamA,CamN; // the CNAF commands
	/*thi is stupid that I hyave to use ints here */
	/* manual is incorrect there again */
    int CamFI,CamAI,CamNI,CamacOptionI; // the CNAF commands
    int QI,XI;

    long StackData[768];	// the final stack
    short Command_word; // The camac stack command word
    long DataIn;	// data to be sent to Camac 
    short Data_1;	// lower 16 bits of DataIn
    short Data_2;	// most significant Bit of DataIn
    short stack_check;
   ushort temp;
    short StackAddr;	// the stack address 2: Camac  Data Stack, 3: Camac Scaler stack
    short CamacOption;  // Options like 24 bit word etc.. See manual p27
    short LongWord=0;
    char title[80];
    char line1[80];
    char line2[80];
    char line3[80];
     char line4[80];
     char module[20];
   
	strcpy(stackfil,STACK_FILE);
	if( (stack_file=fopen(stackfil,"r"))== NULL)
	{
	  status =pitschi_error(udev,6);
	}
	else
	  {
	   // read title and 4comment lines:
	   fgets(title,80,stack_file);
	   fgets(line1,80,stack_file);
	   fgets(line2,80,stack_file);
	   fgets(line3,80,stack_file);
	   fgets(line4,80,stack_file);
	
	   	   	   



	/*************************here we set the different camac modules up according to the list**/
	   for(int loop=0;loop<1000;loop++)
	   {
	   	fscanf(stack_file,"%d  %d %d %lx %x %8c %[a-zA-Z0-9_ ] ", &CamNI,&CamAI,&CamFI,&DataIn, &CamacOptionI,&module, &comment);
	   	if(CamNI<0) break;
//	   	status=CAMAC_write(udev,CamNI,CamAI,CamFI,DataIn,&QI,&XI);
	   	status=CAMAC_read(udev,CamNI,CamAI,CamFI,&DataIn,&QI,&XI);
		printf(" F %d  Q response: %d , X response %d \n ",CamFI,QI,XI); 
	    }


	/***************************finished setting up the modules **********************************/

	   fgets(line1,80,stack_file);
	   fgets(line2,80,stack_file);

	/* here we read CAMAC stack block ***************/
	   for(int loop=0;loop<1000;loop++)
	   {
		LongWord=0;
//	   fscanf(stack_file,"%hd  %hd %hd %lx %hx %8s %a[a-zA-Z0-9_ ]  ", &CamN,&CamA,&CamF,&DataIn, &CamacOption,&module, &comment);
	   fscanf(stack_file,"%hd  %hd %hd %lx %hx %8c %[a-zA-Z0-9_ ]  ", &CamN,&CamA,&CamF,&DataIn, &CamacOption,&module, &comment);
	   if(CamN<0) break;
	   printf ("%hd  %hd %hd %lx %hx %8s %s \n", CamN,CamA,CamF,DataIn, CamacOption, module, comment );
	   stacklines=stacklines+1;
	   if(CamacOption & BIT_14) LongWord=1;
	   Command_word=CamF+32*CamA+512*CamN+LongWord*16384;
	   stack[stacklines]=Command_word;

	   /* here we have to handle the different conditions */
	   if(CamF == 16 || CamF ==17 || CamF == 18 || CamF == 19) /* we deal with write commands */
	   {
	     if(CamacOption & BIT_15)  // need to put longword into two shorts
	       {  
		printf("  long word write commands not implemented yet \n");
		pitschi_close(udev);
		   Data_1=(ushort)DataIn;
		   Data_2=DataIn >> 16;
		   Command_word=Command_word;	// this will tell the stack that there are two data lines
	           stack[stacklines]=Command_word;
	   	   stacklines=stacklines+1;
		   stack[stacklines]=Data_1;
	   	   stacklines=stacklines+1;
		   stack[stacklines]=Data_2;
		   
		   
		   					// coming
	       }
	     else
	       {
	           Data_1=(ushort)DataIn;
	   	   stacklines=stacklines+1;
		   stack[stacklines]=Data_1;
	       }
	       
	     }  
	       
	       
	       
	       
	   }   // end of for loop



	  }
	  
	  /*	Now we write the endmarker (n(0), a(0), f(16)
	  /* and the actual endmarker 0xffff */
	  /* add these two lines to the total number of stack lines */
	  stacklines=stacklines+1;
	  stack[stacklines]=0x0010;
	  stacklines=stacklines+1;
	  stack[stacklines]=0xffff;
	  /* and now tell the stack how many lines to expect */
	  stack[0]=stacklines;
	  
	/* now load it */
		for(int il=0; il<stack[0]+1;il=il+1)  // this way the last line gets written too
		{
		StackData[il]=stack[il];
		printf(" Stack values at %d  %04x %04x \n",il,*(StackData +il),stack[il]);
		}

		printf("PITSCHI Loading the CAMAC stack \n");

		  stack_check=xxusb_stack_write(udev,2,StackData);
		  /* check if stack write was sucessful: */
		  /* the stack_cehck value should be 2*stack[0]+4 */
		  /*see page 36	(the manula is wrong there				  */
		  
		  if(stack_check-(2*stack[0]+4) !=0)
		  {
		  printf(" PITSCHI error: stack check error in writing: %d %d \n",stack_check ,(2*stack[0]+4));
		  printf(" check manual and your stack file, the two numbers should be equal \n" );
		  pitschi_error(udev,7);
			fclose(stack_file);	 
		  }

	/************************************end of camac data stack***********************************************/
	
	
	
	
	/***********************************Start Scaler stack ****************************************************/

	   // read 1comment line :
		stacklines=0;
		
	   fgets(line1,80,stack_file);
	   	   
	   for(int loop=0;loop<1000;loop++)
	   {
	   LongWord=0;
	   fscanf(stack_file,"%hd  %hd %hd %lx %hx %8c %[a-zA-Z0-9_ ]  ", &CamN,&CamA,&CamF,&DataIn, &CamacOption,&module, &comment);
	   if(CamN<0) break;
	   printf ("%hd  %hd %hd %lx %hx %8s %s \n", CamN,CamA,CamF,DataIn, CamacOption,module, comment );
	   stacklines=stacklines+1;
	   if(CamacOption & BIT_14) LongWord=1;
	   
	   Command_word=CamF+32*CamA+512*CamN+16384*LongWord;
	   stack[stacklines]=Command_word;

	   /* here we have to handle the different conditions */
	   if(CamF == 16 || CamF ==17 || CamF == 18 || CamF == 19) /* we deal with write commands */
	   {
	     if(CamacOption & BIT_15)  // need to put longword into two shorts
	       {  
		printf("  long word write commands not implemented yet \n");
		pitschi_close(udev);
		   Data_1=(ushort)DataIn;
		   Data_2=DataIn >> 16;
	           stack[stacklines]=Command_word;
	   	   stacklines=stacklines+1;
		   stack[stacklines]=Data_1;
	   	   stacklines=stacklines+1;
		   stack[stacklines]=Data_2;
		   
		   
		   					// coming
	       }
	     else
	       {
	           Data_1=(ushort)DataIn;
	   	   stacklines=stacklines+1;
		   stack[stacklines]=Data_1;
	       }
	       
	     }  
	       
	       
	       
	       
	   }   // end of for loop



	/* **************************now write Scaler stack ***********************************************/

	  /*	Now we write the endmarker (n(0), a(0), f(16)
	  /* and the actual endmarker 0xffff */
	  /* add these tow lines to the tola number of stack lines */
	  stacklines=stacklines+1;
	  stack[stacklines]=0x0010;
	  stacklines=stacklines+1;
	  stack[stacklines]=0xffff;
	  /* and now tell the stack how many lines to expect */
	  stack[0]=stacklines;
	  
	/* now load it */
		for(int il=0; il<stack[0]+1;il=il+1)
		{
		StackData[il]=stack[il];
		printf(" scaler Stack values at %d  %04x %04x \n",il,*(StackData +il),stack[il]);
		}

		printf("PITSCHI Loading the scaler stack \n");

		  stack_check=xxusb_stack_write(udev,3,StackData);
		  /* check if stack write was sucessful: */
		  /* the stack_cehck value should be 2*stack[0]+4 */
		  /*see page 36	(the manula is wrong there				  */
		  
		  if(stack_check-(2*stack[0]+4) !=0)
		  {
		  printf(" PITSCHI error: scaler stack check error in writing: %d %d \n",stack_check ,(2*stack[0]+4));
		  printf(" check manual and your stack file, the two numbers should be equal \n" );
		  pitschi_error(udev,8);
			fclose(stack_file);	 
		  }


	fclose(stack_file);	 

	  
	  return(1) ;
}


/*************************pitschi_test_stack********************/
/* 	test the stack execution and reads stack alues back					   */
/***********************************************************/


short pitschi_test_stack(int dummy)
{
	
    	long StackData[768];	// the stack read back
	short StackAddr =2;	// defaultvalue CAMAC stack
	short stack_check;	//returns length of stack in bytes plu 4 extra bytes for header and end
	int stl;


	stack_check=xxusb_stack_read(udev,StackAddr,StackData);
	stl=(stack_check-2)/2;

	printf("PITSCHI_CHECK : stack read back test \n");

	 for(int lo=0;lo<stl;lo=lo+1)
	   {
			printf(" Stack values at %d  %04x \n",lo,*(StackData +lo));
	    }
	    
	stack_check=xxusb_stack_read(udev,3,StackData);
	stl=(stack_check-2)/2;

	printf("PITSCHI_CHECK : scaler read back test \n");

	 for(int lo=0;lo<stl;lo=lo+1)
	   {
			printf(" Stack values at %d  %04x \n",lo,*(StackData +lo));
	    }
	    


	 return(1);
}
	

/**************************pitschi_daq_loop********************/
/* 	This is it, the real McCoy				   */
/***********************************************************/
short pitschi_daq_loop(int dummy)
{ 
	int mod_counter=0; /* to coumnt events and print out a status every so often*/
	int status1;
//	char DATAB[4096];	//array of data
	ushort DATAB[2048];
	int DataLength=2048;	// Number of bytes to read
	int Timeout=2000;		// timeout of read operation in milliseconds
	/* initialize start time */
	
	  gettimeofday(&tv1, NULL);
  		delta_t = (tv1.tv_sec - tv_start.tv_sec);
  		delta_t_us = tv1.tv_usec - tv_start.tv_usec;
  		delta_t_ms = (int) ((delta_t_us + 1e6 * delta_t) / 1000.);


		CAMAC_I(udev, true);  // inhibit modules
		CAMAC_C(udev); 	// clear everything



	/*first make sure everything is cleared */



	// here we set the usb register to start DAQ mode

	CAMAC_I(udev, false);  // enable modules

	status1=xxusb_register_write(udev,1,1); // 17 means also sacler readout


 	 continue_run = 1;
	






  // main event loop -- go until the time is expired.  
  while (continue_run == 1) 
  {				/* main loop */

	// check for interrupt
	signal(SIGINT,pitschi_terminator);
	
	status1=xxusb_bulk_read(udev,&DATAB,DataLength,Timeout);
//	if(status1>0 && status1 !=6) // status1=6 is a watchdog event
	if(status1>0)
	{
	printf("status of read %d \n ", status1);
	// Check if scaler event: (
//	  status1=(status1-2)/2;
	status1=status1/2;
	  header[0] = 0x11111111;

 	 header[1] = 0;
  	 header[1] = ('P' << 0) + ('I' << 8) + ('T' << 16) + ('S' << 24);

  	header[2] = (int) tv1.tv_sec;
  	header[3] = status1;  /*add 2 words for the scaler info */
  	subheader[0]= delta_t_ms;
  	subheader[1]=1; /*dummy*/
  	subheader[2]=1;/*dummy*/
  	subheader[3]=main_loop_counter;
	fwrite(header,4,4,outfile);  /*write header */
	fwrite(subheader,4,4,outfile); /*write suheader */


	fwrite(DATAB,2,status1,outfile);

		if(MYDEBUG==3)
		{
	  	for(int jl=0;jl<status1;jl++)
	  	{
	  	printf("data %hd %x \n ",*(DATAB+jl),*(DATAB+jl) );
	  	}	
	 	}
	
	
	main_loop_counter=main_loop_counter+1;
	/*we got a buffer */
	}
   	if (MYDEBUG && (main_loop_counter % 500 == 0)) 
    		{
      			printf("Main loop elapsed time: %d seconds, or %d ms\n", delta_t,
             			delta_t_ms);
    		}






    gettimeofday(&tv1, NULL);
    delta_t = (tv1.tv_sec - tv_start.tv_sec);
    delta_t_us = tv1.tv_usec - tv_start.tv_usec;
    delta_t_ms = (int) ((delta_t_us + 1e6 * delta_t) / 1000.);





    if (requested_events > 0) {
      if (event_counter >= requested_events) {
        continue_run = 0;
      }
    } else if (delta_t > run_length) {
      continue_run = 0;
    }

  }

	// we are done running, stop daq
	// there is a problem with ctrl-y
	// if we are ctrl-y out of it the controller keeps on running. need to find
	// a way to catch that.
		/* stop DAQ system*/
		status1=xxusb_register_write(udev,1,0);

		int k=0;
		while(status1>0)
		{
		status1=xxusb_bulk_read(udev,&DATAB,DataLength,100); // a last read
		printf("status of last read %d \n ", status1);
		k++;
		if(k>100)status1=0;
		}

			CAMAC_I(udev, true);  // inhibit modules


return(1);
}
void pitschi_terminator(int signum)
{
	ushort DATAB[2048];
	int DataLength=2048;
	int k=0;
	int status1=0;


		printf("***************** terminating after CTRL/Y****************\n");



		status1=xxusb_register_write(udev,1,0);  //make sure the DAQ is stopped


		while(status1>0)
		{
		status1=xxusb_bulk_read(udev,&DATAB,DataLength,100); // a last read
		printf("status of last read %d \n ", status1);
		k++;
		if(k>100)status1=0;
		}




			CAMAC_I(udev, true);  // inhibit modules


pitschi_close(udev);
return;
}

/**************************pitschi_test_error********************/
/* 	this allow to test single modules with CNAF commands  */
/* from a code which came from Wiener			      */
/**************************************************************/

void pitschi_test_module(int dummy)
{

    int CamN, CamA, CamF;
    long CamD;
    int CamQ, CamX;
    char nafin[20];
    char nafinx[20];
    int WriteMode;
    short status;

    int ret, i;





// set controller and initialize it

   CAMAC_Z(udev);
		CAMAC_I(udev, true);
		CAMAC_write(udev, 1, 0, 16, 0xaaaaaa,&CamQ, &CamX);
		CAMAC_read(udev, 1, 0, 0, &CamD,&CamQ, &CamX);
		CAMAC_C(udev);
		CAMAC_I(udev, false);
		CAMAC_Z(udev);
//

    CamD=0;

    strcpy(nafin,"5,5,0");
		sscanf(nafin,"%i,%i,%i",&CamN,&CamA,&CamF);
		while (CamN>0)
		{
			printf("    N,A,F (Comma-separated; x for exit; p for NAF=%i,%i,%i) -> ",CamN, CamA, CamF);
			fflush(stdin);
			scanf("%s",&nafinx);
			if (nafinx[0]=='X' || nafinx[0]=='x') 
			{
				break;
			}
			if (strlen(nafinx)>4)
				strcpy(&nafin[0], &nafinx[0]);
			sscanf(nafin,"%i,%i,%i",&CamN,&CamA,&CamF);
			fflush(stdin);
			if (CamF < 8)	
			{
				ret = CAMAC_read(udev, CamN, CamA, CamF, &CamD,&CamQ, &CamX);
				if (ret < 0)
					printf("Read Operation Failed\n");
				else
					printf("\n       X = %i, Q = %i, D = %lx\n\n",CamX, CamQ, CamD);
			}
			if ((CamF > 7) && (CamF < 16))
			{
				ret = CAMAC_read(udev, CamN, CamA, CamF, &CamD,&CamQ, &CamX);
				if (ret < 0)
					printf("Write Operation Failed\n");
				else
					printf("\n       X = %i, Q = %i\n\n",CamX, CamQ);
				
			}
			if ((CamF > 15) && (CamF < 24))
			{	
				WriteMode=1;
				printf("     D (Use 0x Prefix for Hexadecimal)-> ");
				scanf("%li", &CamD);
				fflush(stdin);
				CAMAC_write(udev, CamN, CamA, CamF, CamD,&CamQ, &CamX);
			}	
			if ((CamF > 23))
			{
				ret = CAMAC_read(udev, CamN, CamA, CamF, &CamD,&CamQ, &CamX);
				if (ret < 0)
					printf("Write Operation Failed\n");
				else
					printf("\n       X = %i, Q = %i\n\n",CamX, CamQ);
			}
		}


return;
}

/**************************************************************/
/*								*/
/* pitschi_write_header	does data file beginning and end of file */
/*	arg=1: begin of run; 2 end of run			*/
/****************************************************************/
void pitschi_write_header(int arg)
{
FILE* stack_file;
char stackfil[100];
short status;
  char mydate[100];
  uint32_t code = 0;

  if (arg == 1) {
    		code = 0x00000000;
    		// get the time that the program starts
    		gettimeofday(&tv_start, NULL);
    		strcpy(mydate, ctime(&tv_start.tv_sec));
    		mydate[24] = '\0';          // get rid of newline at the end
    		mydate[19] = '\0';          // and get rid of the year.
      		printf("pitschi   start date/time: %s %d \n", mydate, (int) tv_start.tv_sec);
  		
	// here we write the stack info again but in line format

		strcpy(stackfil,STACK_FILE);
		if( (stack_file=fopen(stackfil,"r"))== NULL)
		{
	 	 status =pitschi_error(udev,6);
		}
		
		
		fclose(stack_file);


		}
	
	
	
	
	
	
	
  if (arg == 2) {
    		code = 0xffffffff;
    		// note that tv1 time comes from main loop 
    		// other main loop variables are globals
    		strcpy(mydate, ctime(&tv1.tv_sec));
    		mydate[24] = '\0';          // get rid of newline at the end
    		mydate[19] = '\0';          // and get rid of the year.

     		 printf("pitschi  finish date/time: %s %d %d %d\n",mydate, (int) tv1.tv_sec, delta_t, main_loop_counter);
  		}

  		gettimeofday(&tv1, NULL);

  		header[0] = code;
  		header[1] = ('P' << 0) + ('I' << 8) + ('T' << 16) + ('S' << 24);
  		header[2] = (int) tv1.tv_sec;
  		header[3] = 0x0;

 /* //note that header[3] is a 
  //placeholder for size in bytes, add at end of readout
  // by setting dataB[3]=...
  // but no data here, just header, so leave the value 0. */

	
	fwrite(header,4,4,outfile);

  return;
}



/**************************pitschi_error********************/
/* 	handles error					   */
/***********************************************************/


short pitschi_error(usb_dev_handle *udev,int err_num)
{
	switch (err_num)
	{
	case 0:
	printf(" PITSCHI error %3d  erroro opening device",err_num);
	pitschi_close(udev);	// leave with properly closing
	
	case 1:
	printf(" PITSCHI error %3d  reading firmware register failed",err_num);
	pitschi_close(udev);	// leave with properly closing
	case 2:
	printf(" PITSCHI error %3d  can't open run number file",err_num);
	pitschi_close(udev);	// leave with properly closing

	case 3:
	printf(" PITSCHI error %3d  can't open data file",err_num);
	pitschi_close(udev);	// leave with properly closing
	
	case 4:
	printf(" PITSCHI error %3d  can't open register file",err_num);
	pitschi_close(udev);	// leave with properly closing
	
	case 5:
	printf(" PITSCHI error %3d  error setting camac register",err_num);
	pitschi_close(udev);	// leave with properly closing

	case 6:
	printf(" PITSCHI error %3d  error openeing stack file",err_num);
	pitschi_close(udev);	// leave with properly closing

	case 7:
	printf(" PITSCHI error %3d  error writing CAMAC stack",err_num);
	pitschi_close(udev);	// leave with properly closing
	case 8:
	printf(" PITSCHI error %3d  error writing scaler stack ",err_num);
	pitschi_close(udev);	// leave with properly closing

	case 9:
	printf(" PITSCHI error %3d  , environment variable PITSCHI not defined \n",err_num);
	exit(0);	// leave with properly closing
	

	default:
	pitschi_close(udev);
	}
}


