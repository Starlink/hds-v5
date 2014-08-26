## Process this file with automake to produce Makefile.in

## bin_PROGRAMS = hdsdump
## dist_bin_SCRIPTS = hds_link hds_link_adam
noinst_PROGRAMS = dat_par_f dat_par_h make-hds-types
## starhelp_DATA = hds_machine.txt

AM_CFLAGS = -g -O0

# libhdsf is for fortran interface
lib_LTLIBRARIES = libhdsonhdf5.la

TESTS = hdsTest
check_PROGRAMS = hdsTest

libhdsonhdf5_la_SOURCES = \
	$(PUBLIC_INCLUDES) \
	$(PUBLIC_CINCLUDES) \
	$(include_MESSAGES) \
	$(PRIVATE_INCLUDES) \
	$(C_ROUTINES)

libhdsonhdf5_la_LIBADD = `starmem_link` `ems_link`

##libhdsf_la_SOURCES = \
##	$(F_ROUTINES) \
##	$(F_C_ROUTINES) \
##	$(BLOCK_DATA_FILES)

##libhdsf_la_LIBADD = `ems_link`

# Make all library code position independent. This is handy for creating
# shareable libraries from the static ones (Java JNI libraries).
if !NOPIC
libhdsonhdf5_la_CFLAGS = $(AM_CFLAGS) -prefer-pic
libhdsonhdf5_la_FCFLAGS = $(AM_FCFLAGS) -prefer-pic
libhdsf_la_CFLAGS = $(AM_CFLAGS) -prefer-pic
libhdsf_la_FCFLAGS = $(AM_FCFLAGS) -prefer-pic
endif

## hds_machine_SOURCES = hds_machine.c
## hds_machine_LDADD = libhds.la `ems_link` `cnf_link` `starmem_link`

dat_par_h_SOURCES = dat_par_h.c
dat_par_f_SOURCES = dat_par_f.c
make_hds_types_SOURCES = make-hds-types.c

## hdsdump_SOURCES = hds_dev/hdsdump.c
## hdsdump_LDADD = libhds.la `ems_link` `cnf_link` `starmem_link`

# Try to make sure that hds_types.h is built early
# This does not seem to work right in conjunction with BUILT_SOURCES
dat_par_f_DEPENDENCIES = hds_types.h hds1_types.h
dat_par_h_DEPENDENCIES = hds_types.h hds1_types.h dat1.h

## hds_test_SOURCES = hds_test.f
## hds_test_LDADD = libhdsf.la libhds.la `ems_link` `chr_link` `cnf_link`

hdsTest_SOURCES = hdsTest.c
hdsTest_LDADD = libhdsonhdf5.la `ems_link` `one_link`

## hds_test_prm_SOURCES = hds_test_prm.c
## hds_test_prm_LDADD = libhds.la `ems_link` `cnf_link` `cnf_link`

cincludedir = $(includedir)/star
cinclude_HEADERS = $(PUBLIC_CINCLUDES)
include_HEADERS = $(PUBLIC_INCLUDES)
include_MESSAGES = CMP_ERR DAT_ERR dat_err.h

## stardocs_DATA = @STAR_LATEX_DOCUMENTATION@

# dat_par.h was previously released so must go in the root include dir
PUBLIC_CINCLUDES = hds.h hds_types.h hds_fortran.h
PUBLIC_INCLUDES = DAT_PAR dat_par.h

PRIVATE_INCLUDES = \
	hds1_types.h \
	$(noinst_MESSAGES)

C_ROUTINES = \
	hdsNew.c \
	datNew.c \
	datFind.c \
	datnew0.c \
	datnew1.c \
	datAnnul.c \
	dat1New.c \
	dat1AllocLoc.c \
	dat1FreeLoc.c \
	dat1H5EtoEMS.c \
	dat1InitHDF5.c \
	dau1CheckName.c \
	dau1CheckType.c \
	dat1RetrieveContainer.c

DAT_PAR: dat_par_f$(EXEEXT)
	./dat_par_f > DAT_PAR

dat_par.h: dat_par_h$(EXEEXT)
	./dat_par_h > dat_par.h

hds_types.h: make-hds-types$(EXEEXT)
	./make-hds-types

hds1_types.h: make-hds-types$(EXEEXT)
	./make-hds-types

#hds_machine.txt: hds_machine$(EXEEXT)
#	echo "Machine characteristics, as determined by HDS" >$@
#	echo "" >>$@
#	./hds_machine >>$@

# Have to make sure that hds_types appears before DAT_PAR so that
# the include file generation routines are built in the correct order
BUILT_SOURCES = hds_types.h hds1_types.h \
	$(include_MESSAGES) $(noinst_MESSAGES) DAT_PAR dat_par.h

CLEANFILES = DAT_PAR dat_par.h CMP_ERR DAT_ERR dat_err.h \
	hds_types.h hds1_types.h

## dist_starnews_DATA = hds.news

EXTRA_DIST = cmp_err.msg dat_err.msg

