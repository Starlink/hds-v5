/*
*+
*  Name:
*     

*  Purpose:
*     

*  Language:
*     Starlink ANSI C

*  Type of Module:
*     Library routine

*  Invocation:
*     (  int * status );

*  Arguments:
*     status = int* (Given and Returned)
*        Pointer to global status.

*  Description:


*  Authors:
*     TIMJ: Tim Jenness (Cornell)
*     {enter_new_authors_here}

*  Notes:
*     

*  History:
*     2014-08-26 (TIMJ):
*        Initial version
*     {enter_further_changes_here}

*  Copyright:
*     Copyright (C) 2014 Cornell University
*     All Rights Reserved.

*  Licence:
*     This program is free software; you can redistribute it and/or
*     modify it under the terms of the GNU General Public License as
*     published by the Free Software Foundation; either version 3 of
*     the License, or (at your option) any later version.
*
*     This program is distributed in the hope that it will be
*     useful, but WITHOUT ANY WARRANTY; without even the implied
*     warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
*     PURPOSE. See the GNU General Public License for more details.
*
*     You should have received a copy of the GNU General Public License
*     along with this program; if not, write to the Free Software
*     Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston,
*     MA 02110-1301, USA.

*  Bugs:
*     {note_any_bugs_here}
*-
*/

#include <stdio.h>

#include "hdf5.h"
#include "hdf5_hl.h"

#include "star/one.h"
#include "ems.h"

#include "hds1.h"
#include "dat1.h"
#include "hds.h"

#include "dat_err.h"
#include "sae_par.h"

void dat1DumpLoc( const HDSLoc* locator, int * status ) {
  if (*status != SAI__OK) return;

  printf("File: %d; Group %d; Dataspace: %d; Dataset: %d; Data Type: %d\n",
         locator->file_id, locator->group_id, locator->dataspace_id,
         locator->dataset_id, locator->dtype);
  return;
}
