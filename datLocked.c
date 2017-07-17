/*
*+
*  Name:
*     datLocked

*  Purpose:
*     See if an object is locked.

*  Language:
*     Starlink ANSI C

*  Type of Module:
*     Library routine

*  Invocation:
*     locked = datLocked( const HDSLoc *locator, int *status );

*  Arguments:
*     locator = const HDSLoc * (Given)
*        A locator for the object to be checked.
*     status = int* (Given and Returned)
*        Pointer to global status.

*  Returned function value:
*     A value indicating the status of the supplied Object:
*
*     0: the supplied object is unlocked. This is the condition that must
*        be met for the current thread to be able to lock the supplied
*        object using function datLock. This condition can be achieved
*        by calling datUnlock.
*
*     1: the supplied object is locked by the current thread. This is the
*        condition that must be met for the current thread to be able to
*        use the supplied object in any other HDS function (except for the
*        locking and unlocking functions - see below). This condition can
*        be achieved by calling datLock.
*
*     2: the supplied object is locked for use by a different thread. An
*        error will be reported if the current thread attempts to use the
*        object in any other HDS function.

*  Description:
*     This function returns a value that indicates if the object
*     specified by the supplied locator has been locked for exclusive
*     use by a call to datLock.

*  Notes:
*     - The locking performed by datLock, datUnlock and datLocked is
*     based on POSIX threads, and has no connection with the locking
*     referred to in hdsLock and hdsFree.
*     - Zero is returned as the function value if an error has already
*     occurred, or if an error occurs in this function.

*  Authors:
*     DSB: David S Berry (DSB)
*     {enter_new_authors_here}

*  History:
*     7-JUL-2017 (DSB):
*        Initial version
*     {enter_further_changes_here}

*  Copyright:
*     Copyright (C) 2017 East Asian Observatory.
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


#include <pthread.h>
#include "sae_par.h"
#include "dat1.h"

int datLocked( const HDSLoc *locator, int *status ) {

/* Local Variables; */
   int result = 0;

/* Check inherited status. */
   if (*status != SAI__OK) return result;

/* Validate input locator, but do not include the usual check that the
   object is locked by the current thread since we'll be performing that
   test as part of this function. */
   dat1ValidateLocator( 0, locator, status );

/* Get the value to return. Test status first so that we know it is safe
   to deference "locator". */
   if( *status == SAI__OK ) {
      result = dat1HandleLock( locator->handle, 1, 0, status );
   }

/* Return the result. */
   return result;
}

