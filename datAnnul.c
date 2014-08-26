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

#include "hdf5.h"

#include "hds1.h"
#include "dat1.h"
#include "hds.h"

int datAnnul( HDSLoc **locator, int * status ) {
  /* Attempts to run even if status is bad */
  HDSLoc * thisloc;

  /* Sanity check argument */
  if (!locator) return *status;
  if (! *locator) return *status;

  thisloc = *locator;

  if (thisloc->dtype) H5Tclose(thisloc->dtype);
  if (thisloc->dataspace_id) H5Sclose(thisloc->dataspace_id);
  if (thisloc->dataset_id) H5Dclose(thisloc->dataset_id);
  if (thisloc->group_id) H5Gclose(thisloc->group_id);
  if (thisloc->file_id) H5Fclose(thisloc->file_id);
  *locator = dat1FreeLoc( thisloc, status );

  return *status;
}
