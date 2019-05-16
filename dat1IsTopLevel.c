/*
*+
*  Name:
*     dat1IsTopLevel

*  Purpose:
*     Test if a locator is for a top level object.

*  Language:
*     Starlink ANSI C

*  Type of Module:
*     Library routine

*  Invocation:
*     int dat1IsTopLevel( const HDSLoc *loc, int *status );

*  Arguments:
*     loc = const HDSLoc * (Given)
*        Pointer to a locator for the HDF object that is to be tested.
*     status = int* (Given and Returned)
*        Pointer to global status.

*  Returned function value:
*     Non-zero if the locator is for a top-levbel object, and zero
*     otherwise.

*  Description:
*     A non-zero value is returned if the supplied locator is for a
*     top-level object.

*  Notes:
*     - Zero will be returned if an error has already occurred, or if
*     this function fails for any reason.

*  Authors:
*     DSB: David S Berry (EAO)
*     {enter_new_authors_here}

*  History:
*     2-MAY-2019 (DSB):
*        Initial version
*     16-MAY-2019 (DSB):
*        If the container file name has more than DAT__SZNAM (15) characters,
*        the name stored in the handle associated with the top level object
*        will be truncated and so will not equal the name of the container
*        file. So only use the first DAT__SZNAM characters when comparing 
*        the handle name with the container file name.
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
#include <strings.h>
#include <pthread.h>

#include "ems.h"
#include "sae_par.h"
#include "dat1.h"
#include "dat_err.h"

int dat1IsTopLevel( const HDSLoc *loc, int *status ){

/* Local Variables; */
   int result;
   Handle *parent;

/* Initialise */
   result = 0;

/* Return immediately if an error has already occurred. */
   if( *status != SAI__OK ) return result;

/* If the locator's handle has no parent, it is a top level locator. */
   parent = loc->handle->parent;
   if( !parent ) {
      result = 1;

/* Otherwise, if the parent handle has no parent, and the locator's
   handle and the parent handle are for the same object (allowing for
   truncation of the object name to DAT__SZNAM characters), it is a
   top level locator. */
   } else if( !parent->parent && loc->handle->name && parent->name ) {
      result = !strncasecmp( loc->handle->name, parent->name, DAT__SZNAM );
   }

/* Return the result. */
   return result;
}
