/*
*+
*  Name:
*     dat1ValidateHandle

*  Purpose:
*     Check the supplied Handle is usable.

*  Language:
*     Starlink ANSI C

*  Type of Module:
*     Library routine

*  Invocation:
*     dat1ValidateHandle( const char *func, Handle *handle, int * status );

*  Arguments:
*     func = const char * (Given)
*        Name of calling function. Used in error messages.
*     handle = Handle * (Given)
*        Handle to validate.
*     status = int* (Given and Returned)
*        Pointer to global status.

*  Returned Value:
*     Non-zero if the Handle is valid and zero otherwise.

*  Description:
*     An error is reported if the supplied Handle is not valid. This can
*     occur for instance if the supplied Handle has been freed.
*     have an appropriate lock on the supplied object.

*  Authors:
*     DSB: David Berry (EAO)
*     {enter_new_authors_here}

*  History:
*     7-FEB-2019 (DSB):
*        Initial version
*     {enter_further_changes_here}

*  Copyright:
*     Copyright (C) 2019 East Asian Observatory
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
#include "dat1.h"
#include "dat_err.h"

int dat1ValidateHandle( const char *func, Handle *handle, int * status ) {

/* Local Variables; */
   int result = 1;

   if( !HANDLE_VALID(handle) ) {
      result = 0;
      if( *status == SAI__OK ) {
         *status = DAT__FATAL;
         emsRepf( " ", "%s: An invalid HDS Handle encountered (internal HDS "
                  "programming error).", status, func );
      }
   }
   return result;

}
