
#define CHAN 4

// length of header, for every event type
/* structure is : 
    code number (e.g. 0x11111111)
    four character code (e.g. PADC)
    time (seconds since 1970) 
    remaining num. bytes in the event 
        (i.e. not including these four 4 byte items) 
*/
#define HEADER 4


// length of subheader array (array of 4 byte longs.)
// used for adc events
#define SUBHEADER 4


struct header {
  unsigned int code;
  char name[4];
  int time;
  int size; 
};

// structs for the PADC tree
typedef struct {
  int moni_1;  // all of the 16 bit 
  int moni_2;
  int moni_3;
  int proton;
  double t_ms;
  int t_s;      // seconds timestamp
  int t_us;     // microseconds since the second
  int ev;       // event (scan) number
  int t0;       // t-zero number (128 scans in a t0)
  int proton_scaler;
} PADC_EVENT;

// second struct for the PADC tree
typedef struct {
  int mainloop;      // main loop counter from panda
  int underover;     // under/overflow
  int events;        // event buffer value from the PADC
  int buffer_full;   // buffer_full status?
} PADC_STATUS;


// struct for the scaler tree
typedef struct {
  int cts[16];  // scaler counts by channel
  int t_s;      // seconds timestamp
  int t_us;     // microseconds since the second
  int n;        // scaler module (1 or 2)
} SCALER_EVENT;


// struct for the run info tree
typedef struct {
  int run;    //scaler counts by channel
  int time;   // seconds timestamp
  int pedestal[16]; 
  // pedestals, integer value used to go from raw data 
  // to ptree values, by channel 
} RUN_INFO;


