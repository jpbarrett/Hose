CC=gcc
CCFLAGS= -g -Wall -I/home/dbesse/powerdaq-3.6.24/include
LDFLAGS= -L/usr/local/lib
LIBS= networkinglib.o safecalls.o -lpowerdaq32 -lgpibapi -lmx -lmi -lut -lmat

# Note: put MATLAB libraries in /usr/local/lib.  Had to run:
# "sudo ldconfig /usr/local/lib" to tell the loader libraries are there.

target= antest
objects= antest.o

#all: $(target)
all: antest encoderd encrec

$(target): $(objects) 
	$(CC) $(LDFLAGS) -o $@ $(objects) $(LIBS)

encoderd: encoderd.o
	$(CC) -lpowerdaq32 encoderd.o networkinglib.o safecalls.o -o $@

encrec: encrec.o
	$(CC) -L/usr/local/lib -L/home/westford/encoder_software/libmat -lpowerdaq32 encrec.o networkinglib.o safecalls.o -lmi -lmx -lut -lmat -o $@

%.o: %.c
	$(CC) $(CCFLAGS) -c $< -o $@

clean:
	rm -f antest antest.o encoderd encoderd.o
