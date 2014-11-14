/*
*+
*  Name:
*     datVec

*  Purpose:
*     Vectorise object

*  Language:
*     Starlink ANSI C

*  Type of Module:
*     Library routine

*  Invocation:
*     datVec( const HDSLoc *locator1, HDSLoc **locator2, int *status );

*  Arguments:
*     locator1 = const HDSLoc * (Given)
*        Array locator
*     locator2 = HDSLoc ** (Returned)
*        Vector locator
*     status = int* (Given and Returned)
*        Pointer to global status.

*  Description:
*     Address an array as if it were a vector.

*  Authors:
*     TIMJ: Tim Jenness (Cornell)
*     {enter_new_authors_here}

*  Notes:
*     - It is an error to vectorize a sliced locator.

*  History:
*     2014-09-08 (TIMJ):
*        Initial version
*     2014-11-13 (TIMJ):
*        Create a 1D dataspace
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
datVec( const HDSLoc *locator1, HDSLoc **locator2, int *status ) {

  size_t nelem;
  *locator2 = NULL;
  if (*status != SAI__OK) return *status;

  if (locator1->isslice) {
    *status = DAT__OBJIN;
    /* It can be done if the slice corresponds to a contiguous
       chunk of memory but for now stay well away */
    emsRep("datVec_1", "datVec: A sliced locator can not (yet) be vectorized",
           status );
    return *status;
  }

  /* Need the number of elements */
  datSize( locator1, &nelem, status );

  /* Clone to a new locator */
  datClone(locator1, locator2, status);

  /* and update the vectorized flag with the number of elements */
  (*locator2)->vectorized = nelem;

  /* Create vectorized dataspace -- we do this to simplify datSlice */
  if ( !dat1IsStructure(*locator2, status) ) {
    hsize_t newsize[1];
    newsize[0] = nelem;
    CALLHDFQ(H5Sset_extent_simple( (*locator2)->dataspace_id, 1, newsize, newsize ));
  }

 CLEANUP:
  if (*status != SAI__OK) {
    if (*locator2 != NULL) datAnnul(locator2, status);
  }
  return *status;
}
