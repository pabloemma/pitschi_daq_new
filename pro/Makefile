 ########################################################################
#                                                                      
#   Created  :  FEB 2008  
# modified July 210                                         
#                                                                      
#   Auth: A. Klein                                                
#
#
#
#              $Date: 2008/03/25 20:35:02 $                 
#              $Header: /home/cvsroot/daq/pitschi_dev/Makefile,v 1.3 2008/03/25 20:35:02 daqdev Exp $
#		$Id: Makefile,v 1.3 2008/03/25 20:35:02 daqdev Exp $                                               
#   pitschi makefile                                    
#                                                                      
#   Created  :  FEB 2008                                           
#                                                                      
#   Auth: A. Klein                                                
#                                                                      
########################################################################

EXE	=	pitschi 
SRCDIR  = 	./src
VPATH	=	$(SRCDIR)
CC	=	g++ 
#COPTS	=	-fPIC -DLINUX -Wall 
COPTS	=	-g3 -fPIC -DLINUX

FLAGS	=	 -g3
#FLAGS	=	-Wall

DEPLIBS	=       -lxx_usb -lm -lusb-1.0

LIBS	=	
LIBDIRS   =  -L/usr/local/lib 
INCLUDEDIR =	-I../xxusb_3.14/include/

OBJS	=	pitschi.o 

INCLUDES =	

#########################################################################

all	:	$(EXE)

clean	:
		/bin/rm -f $(OBJS) $(EXE)

$(EXE)	:	$(OBJS)
		/bin/rm -f $(EXE)
		$(CC) $(FLAGS) -o $(EXE) $(OBJS) -L/usr/local/lib $(DEPLIBS)

$(OBJS)	:	$(INCLUDES) Makefile

%.o	:	%.c
		$(CC) $(COPTS) $(INCLUDEDIR) -c -o $@ $<

