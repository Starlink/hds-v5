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

#include "hdf5.h"
#include "hdf5_hl.h"

#include "star/util.h"
#include "ems.h"
#include "sae_par.h"

#include "hds1.h"
#include "dat1.h"
#include "hds.h"

#include "dat_err.h"

#include "f77.h"

int
datMap(HDSLoc *locator, const char *type_str, const char *mode_str, int ndim,
       const hdsdim dims[], void **pntr, int *status) {

  int isprim = 0;
  char normtypestr[DAT__SZTYP+1];
  size_t nbytes = 0;
  hid_t h5type = 0;
  int typcreat = 0;
  int isreg = 0;
  int tries = 0;
  size_t pagesize = 0;
  void *mapped = NULL;
  hdsmode_t accmode = HDSMODE_UNKNOWN;
  void * where = NULL;

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

  /* Now we want the HDSTYPE of the requested type so that we can work out how much
     memory we will need to allocate. */
  CALLHDF(nbytes,
          H5Tget_size( h5type ),
          DAT__HDF5E,
          emsRep("datLen_size", "datMap: Error obtaining size of data type",
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

  /* We need to know the pagesize */
  pagesize = sysconf( _SC_PAGESIZE );

  while (!mapped) {
    isreg = 0;
    tries++;
    if (*status != SAI__OK) goto CLEANUP;

    /* Get some anonymous memory - we always have to map read/write
       because we always have to copy data into this space. */
    mapped = mmap( where, nbytes, PROT_READ|PROT_WRITE, MAP_ANON | MAP_PRIVATE, -1, 0 );
    if (mapped == MAP_FAILED) {
      emsSyser( "MESSAGE", errno );
      *status = DAT__FILMP;
      emsRep("datMap_2", "Error mapping some memory: ^MESSAGE", status );
      mapped = NULL;
      goto CLEANUP;
    }

    /* Must register with CNF so the pointer can be used by Fortran */
    isreg = cnfRegp( mapped );
    if (isreg == -1) {
      /* Serious internal error */
      *status = DAT__FILMP;
      emsRep("datMap_3", "Error registering a pointer for mapped data "
             " - internal CNF error", status );
      goto CLEANUP;
    } else if (isreg == 0) {
      /* Free the memory and try again */
      if ( munmap( mapped, nbytes ) != 0 ) {
        *status = DAT__FILMP;
        emsSyser( "MESSAGE", errno );
        emsRep("datMap_4", "Error unmapping mapped memory following"
               " failed registration: ^MESSAGE", status);
        goto CLEANUP;
      }
      mapped = NULL;
      where += pagesize;
    }

    if (!mapped && tries > 10) {
      *status = DAT__FILMP;
      emsRep("datMap_4b", "Failed to register mapped memory with CNF"
             " after multiple attempts", status );
      goto CLEANUP;
    }

  }

  if (*status != SAI__OK) goto CLEANUP;

  /* Now we have some memory we need to decide if we are filling it
     or note */

  switch (mode_str[0]) {
  case 'R':
  case 'r':
    accmode = HDSMODE_READ;
  case 'U':
  case 'u':
    datGet( locator, type_str, ndim, dims, mapped, status );
    if (accmode == HDSMODE_UNKNOWN) accmode = HDSMODE_UPDATE; /* Prevent bad R case */
    break;
  case 'W':
  case 'w':
    accmode = HDSMODE_WRITE;
    break;
  default:
    *status = DAT__MODIN;
    emsRepf("datMap_6", "Unrecognized mode string '%s' for datMap",
            status, mode_str);
  }

  /* Need to store the mapped information in the locator so that
     we can unmap it later */
  if (*status != SAI__OK) goto CLEANUP;

 CLEANUP:
  /* Cleanups that must happen always */
  if (h5type) H5Tclose(h5type);

  /* cleanups that only happen if status is bad */
  if (*status != SAI__OK) {
    if (mapped) {
      if (isreg == 1) cnfUregp( mapped );
      if ( munmap( mapped, nbytes ) != 0 ) {
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
    locator->bytesmapped = nbytes;
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

  *pntr = locator->pntr;

  return *status;
}
