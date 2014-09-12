/*
*+
*  Name:
*     datShape

*  Purpose:
*     Enquire object shape

*  Language:
*     Starlink ANSI C

*  Type of Module:
*     Library routine

*  Invocation:
*     datShape( const HDSLoc *locator, int maxdim, hdsdim dims[],
*               int *actdim, int * status );

*  Arguments:
*     locator = const HDSLoc * (Given)
*        Object locator
*     maxdim = int (Given)
*        Allocated size of dims[]
*     dims = hdsdim [] (Returned)
*        Object dimensions.
*     actdim = int * (Returned)
*        Number of dimensions filled in dims[].
*     status = int* (Given and Returned)
*        Pointer to global status.

*  Description:
*     Enquire the shape of an object.

*  Authors:
*     TIMJ: Tim Jenness (Cornell)
*     {enter_new_authors_here}

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

#include "hdf5.h"
#include "hdf5_hl.h"

#include "ems.h"
#include "sae_par.h"

#include "hds1.h"
#include "dat1.h"
#include "hds.h"

#include "dat_err.h"

int
datShape( const HDSLoc *locator, int maxdim, hdsdim dims[],
          int *actdim, int * status ) {

  hsize_t h5dims[DAT__MXDIM];
  int rank = 0;
  hssize_t nblocks = 0;
  hsize_t *blockbuf = NULL;

  if (*status != SAI__OK) return *status;

  /* Special case -- locator overrides file -- short circuit */
  if (locator->vectorized > 0) {
    /* dataspace for dataset could reflect this but groups have no choice */
    *actdim = 1;
    dims[0] = locator->vectorized;
    return *status;
  }

  if (dat1IsStructure( locator, status ) ) {
    int i;
    long long structdims[DAT__MXDIM];

    CALLHDFQ( H5LTget_attribute_int( locator->group_id, ".", "HDSNDIMS", &rank ) );

    if (rank > 0) {

      CALLHDFQ( H5LTget_attribute_long_long( locator->group_id, ".", "HDSDIMS", structdims ) );

      if (rank > maxdim) {
        *status = DAT__DIMIN;
        emsRepf("datshape_1b", "datShape: Dimensions of object exceed maximum allowed size of %d",
                status, maxdim);
        goto CLEANUP;
      }

      for (i=0; i<rank; i++) {
        dims[i] = structdims[i];
      }

    }

  } else {

    CALLHDFE( int,
              rank,
              H5Sget_simple_extent_dims( locator->dataspace_id, h5dims, NULL ),
              DAT__DIMIN,
              emsRep("datshape_1", "datShape: Error obtaining shape of object",
                     status)
              );

    /* If we are using datSlice then there should be one (and only one) hyperslab
       for the dataspace and we need to handle that. Should be same dimensionality
       as above. Negative number indicates there were no hyperslabs. */
    nblocks = H5Sget_select_hyper_nblocks( locator->dataspace_id );

    if (nblocks == 1) {
      herr_t h5err = 0;
      int i;

      blockbuf = MEM_MALLOC( nblocks * rank * 2 * sizeof(*blockbuf) );

      CALLHDF( h5err,
               H5Sget_select_hyper_blocklist( locator->dataspace_id, 0, 1, blockbuf ),
               DAT__DIMIN,
               emsRep("datShape_2", "datShape: Error obtaining shape of slice", status )
               );

      /* We only go through one block. The buffer is returned in form:
         ndim start coordinates, then ndim opposite corner coordinates
         and repeats for each block (if we had more than one block).
      */
      for (i = 0; i<rank; i++) {
        hsize_t start;
        hsize_t opposite;
        start = blockbuf[i];
        opposite = blockbuf[i+rank];
        /* So update the shape to account for the slice */
        h5dims[i] = opposite - start + 1;
      }

    } else if (nblocks > 1) {
      if (*status == SAI__OK) {
        *status = DAT__WEIRD;
        emsRepf("datShape_2", "Unexpectedly got %zd hyperblocks from locator. Expected 1."
                " (possible programming error)", status, (ssize_t)nblocks);
        goto CLEANUP;
      }
    }

    if (rank > maxdim) {
      *status = DAT__DIMIN;
      emsRepf("datshape_1b", "datShape: Dimensions of object exceed maximum allowed size of %d",
              status, maxdim);
      goto CLEANUP;
    }

    dat1ExportDims( rank, h5dims, dims, status );
  }


  *actdim = rank;

 CLEANUP:
  if (blockbuf) MEM_FREE( blockbuf );
  return *status;
}
