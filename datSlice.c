/*
*+
*  Name:
*     datSlice

*  Purpose:
*     Locate slice

*  Language:
*     Starlink ANSI C

*  Type of Module:
*     Library routine

*  Invocation:
*     datSlice(const HDSLoc *locator1, int ndim, const hdsdim lower[],
*              const hdsdim upper[], HDSLoc  **locator2, int *status );

*  Arguments:
*     locator1 = const HDSLoc * (Given)
*        Array locator. Currently must be primitive type.
*     ndim = int (Given)
*        Number of dimensions.
*     lower = const hdsdim [] (Given)
*        Lower dimension bounds. 1-based.
*     upper = const hdsdim [] (Given)
*        Upper dimension bounds. 1-based. If any of the upper bounds
*        are zero or negative the full upper dimension is used instead.
*     locator2 = HDSLoc ** (Returned)
*        Slice locator.
*     status = int* (Given and Returned)
*        Pointer to global status.

*  Description:
*     Return a locator to a "slice" of a vector or an array.

*  Authors:
*     TIMJ: Tim Jenness (Cornell)
*     {enter_new_authors_here}

*  History:
*     2014-09-08 (TIMJ):
*        Initial version
*     2014-10-30 (TIMJ):
*        Bypass slicing for vectorized locators if the full size is requested
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
datSlice(const HDSLoc *locator1, int ndim, const hdsdim lower[],
         const hdsdim upper[], HDSLoc  **locator2, int *status ) {
  hdsdim loc1dims[DAT__MXDIM];
  int loc1ndims = 0;
  hsize_t h5lower[DAT__MXDIM];
  hsize_t h5upper[DAT__MXDIM];
  hsize_t h5dims[DAT__MXDIM];
  hsize_t *points = NULL;
  HDSLoc * sliceloc = NULL;
  int i = 0;

  if (*status != SAI__OK) return *status;

  /* We only work with primitives at the moment */
  if (dat1IsStructure( locator1, status ) ) {
    *status = DAT__OBJIN;
    emsRep("datSlice_1", "datSlice only works with primitive datasets",
           status);
    return *status;
  }

  /* Get the shape of the input locator and validate dimensionality */
  datShape( locator1, DAT__MXDIM, loc1dims, &loc1ndims, status );

  if (loc1ndims == 0) {
    if (*status == SAI__OK) {
      *status = DAT__DIMIN;
      emsRep("datSlice_2", "Can not use datSlice for scalar primitive "
             "(possible programming error)", status);
    }
  }

  if (loc1ndims != ndim) {
    if (*status != SAI__OK) {
      *status = DAT__DIMIN;
      emsRepf("datSlice_3", "datSlice: Arguments have %d axes but locator refers to %d axes",
              status, ndim, loc1ndims);
    }
  }

  if (*status != SAI__OK) return *status;

  /* import the bounds */
  dat1ImportDims( ndim, lower, h5lower, status );
  dat1ImportDims( ndim, upper, h5upper, status );
  dat1ImportDims( ndim, loc1dims, h5dims, status );

  /* Check that the upper bounds are greater than the lower
     bounds and within h5dims. Cap at h5dims if zero is given. */
  for (i=0; i<ndim; i++) {
    if (*status != SAI__OK) break;

    if ( h5lower[i] < 1 || h5lower[i] > h5dims[i] ) {
      *status = DAT__DIMIN;
      emsRepf("datSlice_4", "datSlice: lower bound %d is out of bounds 1 <= %llu <= %llu",
              status, i, (unsigned long long)h5lower[i], (unsigned long long)h5dims[i] );
      break;
    }

    if (h5upper[i] <= 0) h5upper[i] = h5dims[i];

    if ( h5upper[i] < h5lower[i] || h5upper[i] > h5dims[i] ) {
      *status = DAT__DIMIN;
      emsRepf("datSlice_4", "datSlice: upper bound %d is out of bounds %llu <= %llu <= %llu",
              status, i, (unsigned long long)h5lower[i],
              (unsigned long long)h5upper[i], (unsigned long long)h5dims[i] );
      break;
    }
  }

  /* Clone the locator and modify its dataspace */
  datClone( locator1, &sliceloc, status );

  if (*status != SAI__OK) goto CLEANUP;

  /* Locator for a slice MUST be secondary - this may test datPrmry
     so might end up having to do the primary demotion in this code. */
  {
    int prmry = HDS_FALSE;
    datPrmry( HDS_TRUE, &sliceloc, &prmry, status );
  }

  /* For a vectorized slice we need to adjust the dataspace
     to reflect the elements requested in the slice. We do that
     by calculating the coordinates of the points and how they
     map to the underlying dataspace. */
  if ( locator1->vectorized > 0 ) {
    size_t coordnum = 0;
    size_t nvecelem;
    int rank = 0;
    size_t loc1size = 0;

    /* Sometimes people call datSlice on a vectorized locator
       even though they are selecting the whole array (I'm looking
       at you ARY) */
    datSize( locator1, &loc1size, status );

    /* Number of elements in the slice */
    nvecelem = upper[0] - lower[0] + 1;

    if ( nvecelem != loc1size ) {
      /* Only need to mess with the dataspace if the sizes differ */

      /* Now need to get the shape of the dataspace so that we know
         how to index within it */
      CALLHDFE( int,
                rank,
                H5Sget_simple_extent_dims( locator1->dataspace_id, h5dims, NULL ),
                DAT__DIMIN,
                emsRep("datshape_1", "datShape: Error obtaining shape of object",
                       status)
                );
      dat1ExportDims( rank, h5dims, loc1dims, status );

      /* Need to allocate some memory for the points */
      points = MEM_MALLOC( rank * nvecelem * sizeof(*points) );

      /* Convert index to coordinates and store coordinates in array of points */
      for (i = lower[0]; i <= upper[0]; i++) {
        int j;
        hdsdim hdscoords[DAT__MXDIM];
        hsize_t h5coords[DAT__MXDIM];
        dat1Index2Coords( i, rank, loc1dims, hdscoords, status );
        dat1ImportDims( rank, hdscoords, h5coords, status );

        /* and insert the points into the array */
        for (j = 0; j<rank; j++) {
          hsize_t posn = coordnum * rank + j;
          points[posn] = h5coords[j] - 1;  /* 0-based HDF5 */
        }
        coordnum++; /* or use i-slicelower[0] */
      }

      CALLHDFQ( H5Sselect_elements( sliceloc->dataspace_id,
                                    H5S_SELECT_SET, nvecelem, points ) );
    }

    /* Update the slice with the correct number of vectorized elements */
    sliceloc->vectorized = nvecelem;

  } else {
    /* For a normal slice that is the same shape as the underlying
       dataspace on disk we can use a hyperslab */
    hsize_t h5count[DAT__MXDIM];
    hsize_t h5blocksize[DAT__MXDIM];

    /* Calculate the number of elements but also remember that
       HDF5 will be using 0-based counting */
    for (i=0; i<ndim; i++) {
      h5blocksize[i] = h5upper[i] - h5lower[i] + 1;
      h5lower[i]--;
      h5count[i] = 1;
    }
    CALLHDFQ( H5Sselect_hyperslab( sliceloc->dataspace_id, H5S_SELECT_SET, h5lower,
                                   NULL, h5count, h5blocksize ) );
  }

  /* Store knowledge of slice in locator -- we have to do this for vectorized
     dataspaces as they result in many different blocks */
  sliceloc->isslice = HDS_TRUE;
  for (i=0; i<ndim; i++) {
    (sliceloc->slicelower)[i] = lower[i];
    (sliceloc->sliceupper)[i] = upper[i];
  }

 CLEANUP:
  if (points) MEM_FREE( points );
  if (*status != SAI__OK) {
    if (sliceloc) datAnnul( &sliceloc, status );
  } else {
    *locator2 = sliceloc;
  }

  return *status;
}
