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
*     dat1ValidateLocator( int checklock, const HDSLoc *loc, int * status );

*  Arguments:
*     checklock = int (Given)
*        If non-zero, an error is reported if the supplied locator is not
*        locked by the current thread (see datLock). This check is not
*        performed if "checklock" is zero.
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

#include <pthread.h>
#include "sae_par.h"
#include "ems.h"
#include "hds.h"
#include "dat1.h"
#include "dat_err.h"

int dat1ValidateLocator( int checklock, const HDSLoc *loc, int * status ) {

/* Local Variables; */
   int valid;

/* If the locator has been annulled (e.g. due to the container file being
   closed when the last primary locator was annulled), report an error. */
   datValid( loc, &valid, status );
   if( !valid && *status == SAI__OK ) {
      *status = DAT__LOCIN;
      emsRep(" ", "The supplied HDS locator is invalid - it may have been "
             "annulled as a result of the associated file being closed.",
             status );
   }

/* Report an error if there is no handle in the locator. */
   if( loc && !loc->handle && *status == SAI__OK ) {
      *status = DAT__FATAL;
      datMsg( "O", loc );
      emsRep( " ", "The supplied HDS locator for '^O' has no handle (programming error).",
              status );
   }

/* If required, check that the object is locked by the current thread.
   Do not check any child objects as these wil be checked if and when
   accessed. */
   if( checklock && *status == SAI__OK ) {
      if( dat1HandleLock( loc->handle, 1, 0, status ) != 1 &&
          *status == SAI__OK ) {
         *status = DAT__THREAD;
         datMsg( "O", loc );
         emsRep( " ", "The supplied HDS locator for '^O' cannot be used.",
                 status );
         emsRep( " ", "It has not been locked for use by the current thread "
                 "(programming error).", status );
      }
   }

   return *status;

}
