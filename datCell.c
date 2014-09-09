/*
*+
*  Name:
*     datCell

*  Purpose:
*     Locate cell

*  Language:
*     Starlink ANSI C

*  Type of Module:
*     Library routine

*  Invocation:
*     datCell(const HDSLoc *locator1, int ndim, const hdsdim subs[],
*             HDSLoc **locator2, int *status );

*  Arguments:
*     locator1 = const HDSLoc * (Given)
*        Array object locator.
*     ndim = int (Given)
*        Number of dimensions.
*     sub = const hdsdim [] (Given)
*        Subscript values locating the cell in the array. 1-based.
*     locator2 = HDSLoc ** (Returned)
*        Cell locator.
*     status = int* (Given and Returned)
*        Pointer to global status.

*  Description:
*     Return a locator to a "cell" (element) of an array object.

*  Authors:
*     TIMJ: Tim Jenness (Cornell)
*     {enter_new_authors_here}

*  Notes:
*     Typically, this is used to locate an element of a structure
*     array for subsequent access to its components, although this
*     does not preclude its use in accessing a single pixel in a 2-D
*     image for example.

*  History:
*     2014-09-06 (TIMJ):
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
datCell(const HDSLoc *locator1, int ndim, const hdsdim subs[],
        HDSLoc **locator2, int *status ) {

  hsize_t h5subs[DAT__MXDIM];
  HDSLoc * thisloc = NULL;

  if (*status != SAI__OK) return *status;

  /* Copy dimensions if appropriate */
  dat1ImportDims( ndim, subs, h5subs, status );

  if (dat1IsStructure( locator1, status)) {
    int groupdims = 0;
    char cellname[128];
    hid_t group_id = 0;

    /* Check the number of dimensions */
    CALLHDFQ( H5LTget_attribute_int( locator1->group_id, ".", "HDSDIMS", &groupdims ) );

    if (groupdims != ndim) {
      if (*status != SAI__OK) {
        *status = DAT__DIMIN;
        emsRepf("datCell_1", "datCell: Arguments have %d axes but locator refers to %d axes",
                status, ndim, groupdims);
      }
      goto CLEANUP;
    }

    if (groupdims == 0) {
      if (*status == SAI__OK) {
        *status = DAT__DIMIN;
        emsRep("datCell_2", "Can not use datCell for scalar group "
               "(possible programming error)", status );
      }
    }

    /* Calculate the relevant group name */
    dat1Coords2CellName( ndim, subs, cellname, sizeof(cellname), status );

    CALLHDF(group_id,
            H5Gopen2( locator1->group_id, cellname, H5P_DEFAULT ),
            DAT__OBJIN,
            emsRepf("datCell_3", "datCell: Error opening component %s", status, cellname)
            );

    /* Create the locator */
    thisloc = dat1AllocLoc( status );

    if (*status == SAI__OK) thisloc->group_id = group_id;


  } else {
    /* Need to select a single element of the primitive */
    /* Use datSlice when it is available */
    if (*status == SAI__OK) {
      *status = DAT__FATAL;
      emsRep("datCell_1", "Can not yet use datCell to find single element in primitive",
             status);
    }
  }

 CLEANUP:
  if (*status != SAI__OK) {
    if (thisloc) datAnnul( &thisloc, status );
  } else {
    *locator2 = thisloc;
  }
  return *status;
}
