/*
*+
*  Name:
*     dat1ValidateLocator

*  Purpose:
*     Check the supplied locator is usable.

*  Language:
*     Starlink ANSI C

*  Type of Module:
*     Library routine

*  Invocation:
*     dat1ValidateLocator( const HDSLoc *loc, int * status );

*  Arguments:
*     loc = HDSLoc * (Given)
*        Locator to validate.
*     status = int* (Given and Returned)
*        Pointer to global status.

*  Description:
*     An error is reported if the supplied locator is not valid. This can
*     occur for instance if the supplied locator is a secondary locator
*     that has been annulled automatically as a result of the file being
*     closed.

*  Authors:
*     DSB: David Berry (EAO)
*     {enter_new_authors_here}

*  History:
*     7-JUL-2017 (DSB):
*        Initial version
*     {enter_further_changes_here}

*  Copyright:
*     Copyright (C) 2017 East Asian Observatory
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

#include "sae_par.h"
#include "ems.h"
#include "hds.h"
#include "dat1.h"
#include "dat_err.h"

int dat1ValidateLocator( const HDSLoc *loc, int * status ) {

/* Local Variables; */
   int valid;

/* If the locator is not valid, report an error. */
   datValid( loc, &valid, status );
   if( !valid && *status == SAI__OK ) {
      *status = DAT__LOCIN;
      emsRep(" ", "The supplied HDS locator is invalid - it may have been "
             "annulled as a result of the associated file being closed.",
             status );
   }

   return *status;

}
