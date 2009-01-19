#SUBDIRS = m4

AUTOMAKE_OPTIONS = subdir-objects

#ACLOCAL_AMFLAGS = -I m4

bin_PROGRAMS = mbmt-create-test mbmt-create-training mbmt-tar-from-A3 mbmt-decode

mbmt_create_test_SOURCES = \
	src/mbmt-create-test.c

mbmt_create_training_SOURCES = \
	src/mbmt-create-training.c

mbmt_tar_from_A3_SOURCES = \
	src/mbmt-tar-from-A3.c

mbmt_decode_SOURCES = \
	src/sockhelp.c \
	src/sockhelp.h \
	src/mbmt-decode.c

#man_MANS = man/peter.1 man/pcmd.1 man/otpcmd.1

#EXTRA_DIST = \
#	$(man_MANS) 

