/*
*+
*  Name:
*     datSize

*  Purpose:
*     Enquire object size

*  Language:
*     Starlink ANSI C

*  Type of Module:
*     Library routine

*  Invocation:
*     datSize(const HDSLoc *locator, size_t *size,  int * status );

*  Arguments:
*     locator = const HDSLoc * (Given)
*        Object locator
*     size = size_t * (Returned)
*        Object size
*     status = int* (Given and Returned)
*        Pointer to global status.

*  Description:
*     Enquire the size of an object. For an array this will be the product of the
*     dimensions; for a scalar, a value of 1 is returned.

*  Authors:
*     TIMJ: Tim Jenness (Cornell)
*     {enter_new_authors_here}

*  History:
*     2014-08-26 (TIMJ):
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

#include "star/one.h"
#include "ems.h"

#include "hds1.h"
#include "dat1.h"
#include "hds.h"

#include "dat_err.h"
#include "sae_par.h"

int
datSize(const HDSLoc *locator,
        size_t *size,
        int *status ) {

  if (*status != SAI__OK) return *status;

  if (! dat1IsStructure(locator, status)) {
    int rank;

    /* start by looking at the rank */
    CALLHDF( rank,
             H5Sget_simple_extent_ndims( locator->dataspace_id ),
             DAT__HDF5E,
             emsRep("datSize_0", "datSize: Eror determining rank of component", status )
             );

    if (rank == 0) {
      *size = 1;

    } else {
      hssize_t npoints;
      CALLHDFE( hssize_t,
                npoints,
                H5Sget_simple_extent_npoints( locator->dataspace_id ),
                DAT__OBJIN,
                emsRep("datSize_1", "datSize: Error determining size of component", status)
                );

      /* we know that npoints is positive because the negative case is tracked in the macro */
      *size = (size_t)npoints;

    }
  } else {
    /* For now we assume that groups are always scalar */
    *size = 1;
  }

  /* Nothing to clean up but that is fine. We need this for the CALLHDF macro */
 CLEANUP:
  return *status;
}
