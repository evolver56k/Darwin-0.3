#

CFLAGS = -g

ostats:	ObjcImage.o ObjcProcess.o RegionManager.o ostats.o
	$(CC) -o ostats RegionManager.o ObjcImage.o ObjcProcess.o ostats.o -lsys_p
