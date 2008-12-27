# Uncomment the following line for Solaris
# C_LINK = -lsocket -lnsl

# Uncomment this for SCO.  (Note, this has only been reported to work with
# Revision 3.2.4 with the "SCO TCP/IP Development System" package installed.
# Please let me know if you have any other SCO success stories.
# C_LINK = -lsocket

# Comment the following line if you are not using the gnu c compiler
C_ARGS = -O3 -Wall

# You might have to change this if your c compiler is not cc
CC = gcc

# You shouldn't need to make any more changes below this line.

all:	mbmt-decode mbmt-create-training mbmt-create-test

sockhelp.o:	sockhelp.c
	$(CC) $(C_ARGS) $(C_LINK) -c sockhelp.c

mbmt-decode.o:	mbmt-decode.c
	$(CC) $(C_ARGS) -c $<

mbmt-decode:	mbmt-decode.o sockhelp.o
	$(CC) $(C_ARGS) -o $@ $^ $(C_LINK)

mbmt-create-training:
	$(CC) $(C_ARGS) -o $@ mbmt-create-training.c

mbmt-create-test:
	$(CC) $(C_ARGS) -o $@ mbmt-create-test.c

clean:
	rm -rf *.o mbmt-decode mbmt-create-training mbmt-create-test
