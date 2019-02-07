/*
*+
*  Name:
*     datDsame

*  Purpose:
*     Check if two primitive objects use the same data representation.

*  Language:
*     Starlink ANSI C

*  Type of Module:
*     Library routine

*  Invocation:
*     datDsame( const HDSLoc *loc1, const HDSLoc *loc2, hdsbool_t *same,
*               int *status);

*  Arguments:
*     loc1 = const HDSLoc * (Given)
*        First primitive object locator.
*     loc2 = const HDSLoc * (Given)
*        Second primitive object locator.
*     same = hdsbool_t * (Returned)
*        Set to true if and only if the two supplied objects use
*        the same data representation.
*     status = int* (Given and Returned)
*        Pointer to global status.

*  Description:
*     The routine returns a flag indicating if the two supplied primitive
*     objects use the same data representation. See also datDrep.

*  Authors:
*     DSB: David S. Berry (EAO):
*     {enter_new_authors_here}

*  History:
*     2019-01-08 (DSB):
*        Initial version
*     {enter_further_changes_here}

*  Copyright:
*     Copyright (C) 2019 East Asian Observatory
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

#include "ems.h"
#include "sae_par.h"

#include "hds1.h"
#include "dat1.h"
#include "hds.h"

#include "dat_err.h"

int
datDsame(const HDSLoc *loc1, const HDSLoc *loc2, hdsbool_t *same, int *status) {

/* Local Variables: */
  hdsbool_t prim;
  hid_t h5type1 = 0;
  hid_t h5type2 = 0;

/* Initialise. */
  *same = HDS_FALSE;

/* Check inherited status */
  if (*status != SAI__OK) return *status;

/* Validate input locators. */
  dat1ValidateLocator( "datDsame", 1, loc1, 1, status );
  dat1ValidateLocator( "datDsame", 1, loc2, 1, status );

/* Check they are both primitive. */
  datPrim( loc1, &prim, status );
  if( !prim && *status == SAI__OK ) {
    *status = DAT__NOTPR;
    datMsg( "O", loc1 );
    emsRepf( " ", "datDsame: The first supplied HDS object ('^O') is "
             "not primitive (programming error).", status );
  }

  datPrim( loc2, &prim, status );
  if( !prim && *status == SAI__OK ) {
    *status = DAT__NOTPR;
    datMsg( "O", loc2 );
    emsRepf( " ", "datDsame: The second supplied HDS object ('^O') is "
             "not primitive (programming error).", status );
  }

/* Get the two HDF5 data type objects. */
  CALLHDFE( hid_t, h5type1,
           H5Dget_type( loc1->dataset_id ),
           DAT__HDF5E,
           emsRep("datDsame_type", "datDsame: Error obtaining data type "
                  "of first dataset", status) );

  CALLHDFE( hid_t, h5type2,
           H5Dget_type( loc2->dataset_id ),
           DAT__HDF5E,
           emsRep("datDsame_type", "datDsame: Error obtaining data type "
                  "of second dataset", status) );

/* See if they are the same. */
  if( *status == SAI__OK && H5Tequal( h5type1, h5type2 ) ) {
     *same = HDS_TRUE;
  }

/* Free resources */
  H5Tclose( h5type1 );
  H5Tclose( h5type2 );

 CLEANUP:
  return *status;
}
