/*
*+
*  Name:
*     datMap

*  Purpose:
*     Map a primitive

*  Language:
*     Starlink ANSI C

*  Type of Module:
*     Library routine

*  Invocation:
*     datMap (HDSLoc *locator, const char *type_str, const char *mode_str, int ndim,
*             const hdsdim dims[], void **pntr, int *status);

*  Arguments:
*     locator = HDSLoc * (Given and Returned)
*        Primitive locator.
*     type_str = const char * (Given)
*        Data type
*     mode_str = const char * (Given)
*        Access mode (READ, UPDATE or WRITE)
*     ndim = int (Given)
*        Number of dimensions
*     dims = const hdsdim [] (Given)
*        Object dimensions.
*     pntr = void ** (Returned)
*        Pointer to be updated with the mapped data.
*        In WRITE mode the buffer will be filled with zeroes by the operating system.
*        In READ or UPDATE mode the buffer will contain the contents of the dataset.
*     status = int* (Given and Returned)
*        Pointer to global status.

*  Description:
*     Map a primitive (access type specified by a parameter).

*  Authors:
*     TIMJ: Tim Jenness (Cornell)
*     {enter_new_authors_here}

*  Notes:
*     - The API differs slightly from the HDS API in that the locator
*       supplied to the routine is no longer marked const.
*     - HDF5 does not support memory mapping directly. This routine
*       therefore emulates memory mapping by allocating memory using
*       mmap() and, for read or update modes, reading the contents from
*       the HDF5 file.
*     - The resources will be freed either by using datUnmap or by
*       calling datAnnul().
*     - In WRITE and UPDATE mode the contents of the buffer will be
*       written to the HDF5 file on datUnmap() or datAnnul().
*     - The resultant pointer can be used from both C and Fortran
*       using CNF.

*  History:
*     2014-08-29 (TIMJ):
*        Initial version
*     2014-11-06 (TIMJ):
*        First attempt at supporting mmap(). Currently
*        disabled as it is not quite working correctly.
*     {enter_further_changes_here}

*  Copyright:
*     Copyright (C) 2014 Cornell University
*     All Rights Reserved.

*  Licence:
*     Redistribution and use in source and binary forms, with or
*     without modification, are permitted provided that the following
*     conditions are met:
*
*     - Redistributions of source code must retain the above copyright
*       notice, this list of conditions and the following disclaimer.
*
*     - Redistributions in binary form must reproduce the above
*       copyright notice, this list of conditions and the following
*       disclaimer in the documentation and/or other materials
*       provided with the distribution.
*
*     - Neither the name of the {organization} nor the names of its
*       contributors may be used to endorse or promote products
*       derived from this software without specific prior written
*       permission.
*
*     THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND
*     CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES,
*     INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
*     MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
*     DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR
*     CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
*     SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
*     LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF
*     USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED
*     AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
*     LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING
*     IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF
*     THE POSSIBILITY OF SUCH DAMAGE.

*  Bugs:
*     {note_any_bugs_here}
*-
*/

#include <unistd.h>
#include <errno.h>
#include <sys/mman.h>
#include <stdio.h>
#include <fcntl.h>
#include <errno.h>

#include "hdf5.h"
#include "hdf5_hl.h"

#include "star/util.h"
#include "star/one.h"
#include "ems.h"
#include "sae_par.h"

#include "hds1.h"
#include "dat1.h"
#include "hds.h"

#include "dat_err.h"

#include "f77.h"

static void *
dat1Mmap( size_t nbytes, int prot, int flags, int fd, off_t offset, int *isreg, void **pntr, size_t * actbytes, int * status );

int
datMap(HDSLoc *locator, const char *type_str, const char *mode_str, int ndim,
       const hdsdim dims[], void **pntr, int *status) {

  int isprim = 0;
  char normtypestr[DAT__SZTYP+1];
  size_t nbytes = 0;
  hid_t h5type = 0;
  int isreg = 0;
  void *regpntr = NULL;
  void *mapped = NULL;
  hdsmode_t accmode = HDSMODE_UNKNOWN;
  haddr_t offset;
  hdsbool_t try_mmap = HDS_FALSE;
  unsigned intent = 0;
  size_t actbytes = 0;

  if (*status != SAI__OK) return *status;

  /* Get the HDF5 type code and confirm this is a primitive type */
  isprim = dau1CheckType( 1, type_str, &h5type, normtypestr,
                          sizeof(normtypestr), status );

  if (!isprim) {
    if (*status == SAI__OK) {
      *status = DAT__TYPIN;
      emsRepf("datMap_1", "datGet: Data type must be a primitive type and not '%s'",
              status, normtypestr);
    }
    goto CLEANUP;
  }

  /* First have to validate the access mode */
  switch (mode_str[0]) {
  case 'R':
  case 'r':
    accmode = HDSMODE_READ;
    break;
  case 'U':
  case 'u':
    accmode = HDSMODE_UPDATE;
    break;
  case 'W':
  case 'w':
    accmode = HDSMODE_WRITE;
    break;
  default:
    *status = DAT__MODIN;
    emsRepf("datMap_6", "Unrecognized mode string '%s' for datMap",
            status, mode_str);
    goto CLEANUP;
  }

  /* How did we open this file? */
  CALLHDFQ( H5Fget_intent( locator->file_id, &intent ));
  if (accmode == HDSMODE_UPDATE || accmode == HDSMODE_WRITE) {
    /* Must check whether the file was opened for write */
    if ( intent == H5F_ACC_RDONLY ) {
      *status = DAT__ACCON;
      emsRepf("datMap_6b", "datMap: Can not map readonly locator in mode '%s'",
             status, mode_str);
      goto CLEANUP;
    }
  }

  /* There is a super-special case for datMap when called with a map
     type of "_CHAR". In that case we need to work out the size ourselves
     and adjust the type size */
  if (strcmp( "_CHAR", normtypestr ) == 0 ) {
    size_t clen = 0;
    char tmpbuff[DAT__SZTYP+1];
    datClen( locator, &clen, status );
    CALLHDFQ( H5Tset_size( h5type, clen ) );
    one_snprintf( tmpbuff, sizeof(tmpbuff), "*%zu",
                  status, clen );
    one_strlcat( normtypestr, tmpbuff, DAT__SZTYP+1, status );
  }

  /* Now we want the HDSTYPE of the requested type so that we can work out how much
     memory we will need to allocate. */
  CALLHDF(nbytes,
          H5Tget_size( h5type ),
          DAT__HDF5E,
          emsRep("datLen_size", "datMap: Error obtaining size of requested data type",
                 status)
          );

  {
    int i;
    if (ndim > 0) {
      for (i = 0; i < ndim; i++) {
        nbytes *= dims[i];
      }
    }
  }


  /* Work out whether memory mapping is possible -- at the moment
     I'm pretty sure the only safe use of mmap is when we are reading
     the data and the file itself was opened readonly. I'm not sure what happens
     if other components are removed or added -- will the offset change? Maybe we just try */
  offset = H5Dget_offset( locator->dataset_id );
  if (offset != HADDR_UNDEF) {
    hid_t dataset_h5type = 0;
    /* In theory we can do a memory map so now compare
       the data types of the request and the low-level dataset. */
    CALLHDF( dataset_h5type,
             H5Dget_type( locator->dataset_id ),
             DAT__HDF5E,
             emsRep("datMap_type", "datType: Error obtaining data type of dataset", status)
             );
    if (H5Tequal( dataset_h5type, h5type )) {
      try_mmap = HDS_TRUE;
    }
    H5Tclose(dataset_h5type);
  }

  /* There seem to be issues doing this on files opened for update/write.
     For now only allow mmap for files opened read only */
  if (intent != H5F_ACC_RDONLY) try_mmap = 0;

  /* mmap() does not work properly yet so disable it */
  try_mmap = 0;

#if DEBUG_HDS
  {
    char *name_str;
    char * file_str;
    const char * reason;
    name_str = dat1GetFullName( locator->dataset_id, 0, NULL, status );
    file_str = dat1GetFullName( locator->dataset_id, 1, NULL, status );
    if (offset != HADDR_UNDEF) {
      reason = "[HAD offset]";
    } else {
      reason = "[no offset]";
    }
    if (!try_mmap) {
      printf("Will NOT attempt %s to mmap %s:%s\n",reason,file_str,name_str);
    } else {
      printf("WILL TRY %s to mmap OFFSET=%zu %s:%s\n", reason, (size_t)offset, file_str, name_str);
    }
    MEM_FREE(name_str);
  }
#endif

  if (try_mmap) {
    /* We have to open the file ourselves! */
    char * fname = NULL;
    int fd = 0;
    int flags = 0;
    int prot = 0;
    fname = dat1GetFullName( locator->dataset_id, 1, NULL, status );
    if ( intent == H5F_ACC_RDONLY || accmode == HDSMODE_READ ) {
      flags |= O_RDONLY;
      prot = PROT_READ;
    } else {
      flags |= O_RDWR;
      prot = PROT_READ | PROT_WRITE;
    }
    if (*status == SAI__OK) {
      fd = open(fname, flags);
      if (fd > 0) {
        /* Set up for memory mapping */
        int mflags = 0;
        mflags = MAP_SHARED | MAP_FILE;
        if (*status == SAI__OK) {
          mapped = dat1Mmap( nbytes, prot, mflags, fd, offset, &isreg, &regpntr, &actbytes, status );
          if (*status == SAI__OK) {
            /* Store the file descriptor in the locator to allow us to close */
            if (mapped) locator->fdmap = fd;
            printf("SUCCEED IN MMAP\n");
          } else {
            /* Not currently fatal -- we can try without the file */
            close(fd);
            emsAnnul(status);
          }
        }
      }
    }
    if (fname) MEM_FREE(fname);
  }

  /* If we have not been able to map anything yet, just mmap some
     memory not associated with a file */
  if (!mapped) {
    mapped = dat1Mmap( nbytes, PROT_READ|PROT_WRITE, MAP_ANON | MAP_PRIVATE, -1, 0,
                       &isreg, &regpntr, &actbytes, status );

    /* Populate the memory */
    if (accmode == HDSMODE_READ || accmode == HDSMODE_UPDATE) {
      hdsbool_t do_get = HDS_TRUE;
      if (accmode == HDSMODE_UPDATE) {
        /* If this is UPDATE mode but the data array has not actually
           been defined yet we do not actually want to call datGet */
        hdsbool_t defined;
        datState( locator, &defined, status );
        if (!defined) do_get = HDS_FALSE;
      }

      if (do_get) datGet( locator, normtypestr, ndim, dims, regpntr, status );
    }
  }

 CLEANUP:
  /* Cleanups that must happen always */
  if (h5type) H5Tclose(h5type);

  /* cleanups that only happen if status is bad */
  if (*status != SAI__OK) {
    if (mapped) {
      if (isreg == 1) cnfUregp( mapped );
      if ( munmap( mapped, actbytes ) != 0 ) {
        emsSyser( "MESSAGE", errno );
        emsRep("datMap_4", "Error unmapping mapped memory: ^MESSAGE", status);
      }
      mapped = NULL;
    }
  }

  /* Update the locator to reflect the mapped status */
  if (*status == SAI__OK) {
    int i;
    locator->pntr = mapped;
    locator->bytesmapped = actbytes;
    locator->accmode = accmode;

    /* In order to copy the data back into the underlying HDF5 dataset
       we need to store additional information about how this was mapped
       to allow us to either call datPut later on or at least a new
       dataspace. For now store the arguments so we can pass them straight
       to datPut */
    locator->ndims = ndim;
    for (i=0; i<ndim; i++) {
      (locator->mapdims)[i] = dims[i];
    }
    star_strlcpy( locator->maptype, normtypestr, sizeof(locator->maptype) );
  }

  /* Note that the returned pointer is not necessarily the same as the
     mapped pointer because of pagesize corrections */
  *pntr = regpntr;

  return *status;
}


static void *
dat1Mmap( size_t nbytes, int prot, int flags, int fd, off_t offset, int *isreg, void **pntr, size_t *actbytes, int * status ) {
  void * mapped = NULL;
  int tries = 0;
  size_t pagesize = 0;
  void * where = NULL;
  off_t off = 0;
  *pntr = NULL;
  *isreg = 0;

  if (*status != SAI__OK) return NULL;

  /* We need to know the pagesize */
  pagesize = sysconf( _SC_PAGESIZE );

  *actbytes = nbytes;
  if (offset > 0) {
    /* Calculate the starting offset into the file and round this down to a     */
    /* multiple of the system page size. Calculate the number of bytes to map,  */
    /* allowing for this rounding.                                              */
    off = offset - ( offset % pagesize );
    *actbytes += ( offset - off );
  }

  while (!mapped) {
    *isreg = 0;
    tries++;
    if (*status != SAI__OK) goto CLEANUP;

    /* Get some anonymous memory - we always have to map read/write
       because we always have to copy data into this space. */
    //printf("mmap(%p, %zu, %d, %d, %d, %zu -> %zu [%d])\n", where, *actbytes, prot, flags, fd, offset, off, pagesize);
    mapped = mmap( where, *actbytes, prot, flags, fd, off );
    if (mapped == MAP_FAILED) {
      emsSyser( "MESSAGE", errno );
      *status = DAT__FILMP;
      emsRep("datMap_2", "Error mapping some memory: ^MESSAGE", status );
      mapped = NULL;
      *pntr = NULL;
      goto CLEANUP;
    }
    /* The pointer we register is the one that has been corrected
       for the shift we applied in the original request */
    *pntr = mapped + (offset - off );

    /* Must register with CNF so the pointer can be used by Fortran */
    *isreg = cnfRegp( *pntr );
    if (*isreg == -1) {
      /* Serious internal error */
      *status = DAT__FILMP;
      emsRep("datMap_3", "Error registering a pointer for mapped data "
             " - internal CNF error", status );
      goto CLEANUP;
    } else if (*isreg == 0) {
      /* Free the memory and try again */
      if ( munmap( mapped, *actbytes ) != 0 ) {
        *status = DAT__FILMP;
        emsSyser( "MESSAGE", errno );
        emsRep("datMap_4", "Error unmapping mapped memory following"
               " failed registration: ^MESSAGE", status);
        goto CLEANUP;
      }
      mapped = NULL;
      *pntr = NULL;
      where += pagesize;
    }

    if (!mapped && tries > 10) {
      *status = DAT__FILMP;
      emsRep("datMap_4b", "Failed to register mapped memory with CNF"
             " after multiple attempts", status );
      goto CLEANUP;
    }

  }
 CLEANUP:
  return mapped;
}
