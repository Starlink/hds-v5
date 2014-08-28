/*
*+
*  Name:
*     datPut

*  Purpose:
*     Write primitive

*  Language:
*     Starlink ANSI C

*  Type of Module:
*     Library routine

*  Invocation:
*     int datPut( const HDSLoc *locator, const char *type_str, int ndim, const hdsdim dims[],
*                 const void *values, int *status);

*  Arguments:
*     locator = const HDSLoc * (Given)
*        Primitive locator.
*     type = const char * (Given)
*        Data type to be stored (not necessarily the data type of the locator).
*     ndim = int (Given)
*        Number of dimensions.
*     dims = const hdsdim[] (Given)
*        Object dimensions.
*     values = const void * (Given)
*        Object value of type "type" and dimensionality "dims".
*     status = int* (Given and Returned)
*        Pointer to global status.

*  Description:
*       Write a primitive (type specified by a parameter).

*  Authors:
*     TIMJ: Tim Jenness (Cornell)
*     {enter_new_authors_here}

*  Notes:
*     - Character strings are given as a single character buffer and not as char **.
*       The type string indicates how many characters are expected per element
*       and the buffer is assumed to be space padded.

*  History:
*     2014-08-27 (TIMJ):
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

#include "hdf5.h"
#include "hdf5_hl.h"

#include "ems.h"
#include "sae_par.h"

#include "hds1.h"
#include "dat1.h"
#include "hds.h"
#include "dat_err.h"

int
datPut( const HDSLoc *locator, const char *type_str, int ndim, const hdsdim dims[],
        const void *values, int *status) {

  int isprim;
  int typcreat;
  hid_t h5type = 0;
  char normtypestr[DAT__SZTYP+1];
  hsize_t h5dims[DAT__MXDIM];
  hid_t mem_dataspace_id = 0;

  if (*status != SAI__OK) return *status;

  /* Ensure that this locator is associated with a primitive type */
  if (locator->dataset_id <= 0) {
    *status = DAT__OBJIN;
    emsRep("", "datPut: Can not put data into non-primitive location",
           status);
    return *status;
  }

  /* Ensure that we have a primitive type supplied */
  isprim = dau1CheckType( type_str, &h5type, normtypestr,
                          sizeof(normtypestr), &typcreat, status );

  if (!isprim) {
    if (*status == SAI__OK) {
      *status = DAT__TYPIN;
      emsRepf("datPut_1", "datPut: Data type must be a primitive type and not '%s'",
              status, normtypestr);
    }
    goto CLEANUP;
  }

  if (*status != SAI__OK) goto CLEANUP;

  /* Copy dimensions if appropriate */
  if (ndim > 0 ) { /* Consider enforcing hdsdim type == hsize_t */
    size_t i;
    for (i=0; i<(size_t)ndim; i++) {
      h5dims[i] = dims[i];
    }
  }

  /* Create a memory dataspace for the incoming data */
  CALLHDF( mem_dataspace_id,
           H5Screate_simple( ndim, h5dims, NULL),
           DAT__HDF5E,
           emsRep("datPut_2", "Error allocating in-memory dataspace", status )
           );

  CALLHDFQ( H5Dwrite( locator->dataset_id, h5type, mem_dataspace_id,
                      locator->dataspace_id, H5P_DEFAULT, values ) );


 CLEANUP:
  if (h5type && typcreat) H5Tclose(h5type);
  if (mem_dataspace_id > 0) H5Sclose(mem_dataspace_id);
  return *status;
}
