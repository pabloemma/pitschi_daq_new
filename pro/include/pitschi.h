/* pitschi include file */
/* written ak Feb 2008


=======
$Date: 2008/03/13 22:06:52 $
$Id: pitschi.h,v 1.9 2008/03/13 22:06:52 daqdev Exp $
$Revision: 1.9 $



*/

/* Define different bit masks */

#define BIT_0   0x1
#define BIT_1   0x2
#define BIT_2   0x4
#define BIT_3   0x8
#define BIT_4   0x10
#define BIT_5   0x20
#define BIT_6   0x40
#define BIT_7   0x80
#define BIT_8   0x100
#define BIT_9   0x200
#define BIT_10  0x400
#define BIT_11  0x800
#define BIT_12  0x1000
#define BIT_13  0x2000
#define BIT_14  0x4000
#define BIT_15  0x8000




/************************* function prototypes **************************/

short pitschi_init( usb_dev_handle *udev);	// this opens camac and clears it
short pitschi_init_out(char *);			// opens files
short pitschi_error(usb_dev_handle *udev, int);	// handles errors
short pitschi_close( usb_dev_handle *udev);   // closes usb and all files open
short pitschi_set_register(int );	      //reads and sets registers in CAMAC
short pitschi_create_stack(int );		//reads the Camac stack file and loads it onto controller
short pitschi_test_stack(int );		//reads the Camac stack and executes a stack_execute
short pitschi_daq_loop(int );		// main DAQ loop
void pitschi_terminator(int );		// handles  terminations
void pitschi_test_module(int);		// separate branch to just test modules
void pitschi_write_header(int);		// this writes beginning header and end of run

typedef unsigned short ushort;