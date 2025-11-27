# Makefile for aix for Jikes Parser Generator
CC = gcc -O1

all: jikespg

jikespg: ctabs.o globals.o lpgparse.o lpgutil.o main.o \
     mkfirst.o mkred.o mkstates.o partset.o prntstat.o produce.o \
     ptables.o remsp.o resolve.o spacetab.o tabutil.o timetab.o
	$(CC) -s -o jikespg ctabs.o globals.o lpgparse.o lpgutil.o main.o \
	    mkfirst.o mkred.o mkstates.o partset.o prntstat.o produce.o \
	    ptables.o remsp.o resolve.o spacetab.o tabutil.o timetab.o

lpgact.i: jikespg.g
	  lpg jikespg
	  rm jikespg.l
	  make
	  rm CRASH_THE_INITIAL_MAKE_COMMAND_TO_BREAK_OUT

main.o:	main.c common.h
	$(CC) -c main.c

globals.o: globals.c common.h
	$(CC) -c globals.c

lpgparse.o: lpgprs.h lpgdcl.h lpgdef.h lpgact.h lpgact.i lpgsym.h \
	    common.h lpgparse.h	lpgparse.c
	$(CC) -c lpgparse.c

lpgutil.o: lpgutil.c common.h
	$(CC) -c lpgutil.c

mkfirst.o: mkfirst.c common.h
	$(CC) -c mkfirst.c

mkstates.o: mkstates.c common.h
	$(CC) -c mkstates.c

prntstat.o: prntstat.c common.h
	$(CC) -c prntstat.c

mkred.o: mkred.c common.h reduce.h
	$(CC) -c mkred.c

remsp.o: remsp.c common.h reduce.h
	$(CC) -c remsp.c

resolve.o: resolve.c common.h reduce.h
	$(CC) -c resolve.c

ptables.o: ptables.c common.h
	$(CC) -c ptables.c

spacetab.o: spacetab.c common.h space.h
	$(CC) -c spacetab.c

timetab.o: timetab.c common.h
	$(CC) -c timetab.c

tabutil.o: tabutil.c common.h
	$(CC) -c tabutil.c

produce.o: produce.c common.h
	$(CC) -c produce.c

partset.o: partset.c common.h
	$(CC) -c partset.c

ctabs.o: ctabs.c common.h space.h
	$(CC) -c ctabs.c

clean:
	rm jikespg
	rm ctabs.o 
	rm globals.o
	rm lpgparse.o 
	rm lpgutil.o 
	rm main.o 
	rm mkfirst.o 
	rm mkred.o
	rm mkstates.o 
	rm partset.o 
	rm prntstat.o 
	rm produce.o 
	rm ptables.o 
	rm remsp.o
	rm resolve.o
	rm spacetab.o 
	rm tabutil.o
	rm timetab.o
