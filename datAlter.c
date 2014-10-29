/*
*+
*  Name:
*     datAlter

*  Purpose:
*     Alter object size

*  Language:
*     Starlink ANSI C

*  Type of Module:
*     Library routine

*  Invocation:
*     datAlter( HDSLoc *locator, int ndim, const hdsdim dims[], int *status);

*  Arguments:
*     locator = HDSLoc * (Given)
*        Object locator to alter.
*     ndim = int (Given)
*        Number of dimensions specified in "dim". Must be the number of dimensions
*        in the object itself.
*     dim = const hdsdim [] (Given)
*        New dimensions for object.
*     status = int* (Given and Returned)
*        Pointer to global status.

*  Description:
*     Alter the size of an array by increasing or reducing the last
*     (or only) dimension. If a structure array is to be reduced in
*     size, the operation will fail if any truncated elements contain
*     components.

*  Authors:
*     TIMJ: Tim Jenness (Cornell)
*     {enter_new_authors_here}

*  Notes:
*     - The dimensionality of the object can not differ before and after this
*     routine is called.
*     - Can not be called on a vectorized locator.

*  History:
*     2014-10-14 (TIMJ):
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
datAlter( HDSLoc *locator, int ndim, const hdsdim dims[], int *status) {

  hdsdim curdims[DAT__MXDIM];
  hsize_t h5dims[DAT__MXDIM];
  int curndim;
  int i;

  if (*status != SAI__OK) return *status;

  if (locator->vectorized) {
    *status = DAT__OBJIN;
    emsRep("datAlter_1", "Can not alter the size of a vectorized object",
           status);
    return *status;
  }

  if (locator->pntr) {
    *status = DAT__OBJIN;
    emsRep("datAlter_2", "Can not alter the size of a mapped primitive",
           status);
    return *status;
  }

  if (locator->isslice) {
    *status = DAT__OBJIN;
    emsRep("datAlter_3", "Can not alter the size of a slice",
           status);
    return *status;
  }

  /* Get the current dimensions and validate new ones */
  datShape( locator, DAT__MXDIM, curdims, &curndim, status );

  if (curndim != ndim) {
    if (*status == SAI__OK) {
      *status = DAT__DIMIN;
      emsRepf("datAlter_4", "datAlter can not change the dimensionality (%d != %d)",
              status, curndim, ndim);
      return *status;
    }
  }

  if (*status != SAI__OK) return *status;

  for (i=0; i< (ndim-2); i++) {
    if ( dims[i] != curdims[i] ) {
      *status = DAT__DIMIN;
      emsRepf("datAlter_5", "datAlter: Dimension %d (1-based) does not match (%d != %d)",
              status, i+1, dims[i], curdims[i]);
      return *status;
    }
  }

  /* Copy dimensions and reorder */
  dat1ImportDims( ndim, dims, h5dims, status );

  if ( dat1IsStructure( locator, status) ) {

    /* This is somewhat problematic as we have to determine
       all the array components that we need to keep and how
       many we need to create */
    if (*status == SAI__OK) {
      *status = DAT__OBJIN;
      emsRep("datAlter_2", "Reshaping of structure arrays not yet supported",
             status );
    }

  } else {

    CALLHDFQ( H5Dset_extent( locator->dataset_id, h5dims ) );

    /* We need to get a new dataspace from this modified datset. */
    H5Sclose( locator->dataspace_id );
    locator->dataspace_id = H5Dget_space( locator->dataset_id );

  }

 CLEANUP:
  return *status;
}
