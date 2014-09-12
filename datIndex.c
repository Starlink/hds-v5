/*
*+
*  Name:
*     datIndex

*  Purpose:
*     Index into component list

*  Language:
*     Starlink ANSI C

*  Type of Module:
*     Library routine

*  Invocation:
*     datIndex(const HDSLoc *locator1, int index, HDSLoc **locator2, int *status );

*  Arguments:
*     locator1 = const HDSLoc * (Given)
*        Structure locator
*     index = int (Given)
*        List position
*     locator2 = HDSLoc ** (Returned)
*        Component locator.
*     status = int* (Given and Returned)
*        Pointer to global status.

*  Description:
*     Index into a structure's component list and return a locator to the object
*     at the given position.

*  Authors:
*     TIMJ: Tim Jenness (Cornell)
*     {enter_new_authors_here}

*  History:
*     2014-09-12 (TIMJ):
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
datIndex(const HDSLoc *locator1, int index, HDSLoc **locator2, int *status ) {
  char namestr[2 * DAT__SZNAM + 1];
  char groupnam[DAT__SZNAM+1];
  ssize_t lenstr = 0;

  if (*status != SAI__OK) return *status;

  datName( locator1, groupnam, status );

  /* HDF5 is 0-based */
  index--;
  CALLHDFE( ssize_t,
            lenstr,
            H5Lget_name_by_idx( locator1->group_id, ".", H5_INDEX_NAME, H5_ITER_INC,
                                index, namestr, sizeof(namestr), H5P_DEFAULT ),
            DAT__HDF5E,
            emsRepf("datIndex_1", "datIndex: Error obtaining name of component %d from group %s",
                    status, index, groupnam )
            );

  datFind( locator1, namestr, locator2, status );

 CLEANUP:
  return *status;
}
