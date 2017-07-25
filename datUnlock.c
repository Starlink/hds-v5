/*
*+
*  Name:
*     datUnlock

*  Purpose:
*     Unlock an object so that it can be locked by a different thread.

*  Language:
*     Starlink ANSI C

*  Type of Module:
*     Library routine

*  Invocation:
*     datUnlock( HDSLoc *locator, int recurs, int *status );

*  Arguments:
*     locator = HDSLoc * (Given)
*        Locator to the object that is to be unlocked.
*     recurs = int (Given)
*        If the supplied object is unlocked successfully, and "recurs" is
*        non-zero, then an attempt is made to unlock any component objects
*        contained within the supplied object. No error is reported if
*        any components cannot be unlocked due to being locked already by
*        a different thread - such components are left unchanged. This
*        operation is recursive - any children of the child components
*        are also unlocked, etc.
*     status = int* (Given and Returned)
*        Pointer to global status.

*  Description:
*     This function ensures that the current thread does not have a lock
*     of any type on the supplied HDS object. See datLock.
*
*     The object must be locked again, using datLock, before it can be
*     used by any other HDS function. All objects are initially
*     locked by the current thread when they are created.

*  Notes:
*     - No error is reported if the supplied object, or any child object,
*     is current locked for read-only or read-write access by another thread.
*     - The majority of HDS functions will report an error if the object
*     supplied to the function has not been locked for use by the calling
*     thread. The exceptions are the functions that manage these locks -
*     datLock, datUnlock and datLocked.
*     - Attempting to unlock an object that is not locked by the current
*     thread has no effect, and no error is reported. The datLocked
*     function can be used to determine if the current thread has a lock
*     on the object.

*  Authors:
*     DSB: David S Berry (DSB)
*     {enter_new_authors_here}

*  History:
*     10-JUL-2017 (DSB):
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

#include "hdf5.h"

#include "ems.h"
#include "sae_par.h"

#include "hds1.h"
#include "dat1.h"
#include "hds.h"

#include "dat_err.h"

int datUnlock( HDSLoc *locator, int recurs, int *status ) {

/* Check inherited status. */
   if (*status != SAI__OK) return *status;

/* Validate input locator. */
   dat1ValidateLocator( "datUnlock", 0, locator, 0, status );

/* Check we can de-reference "locator" safely. */
   if( *status == SAI__OK ) {

/* Attemp to unlock the specified object, plus all its components. */
      (void) dat1HandleLock( locator->handle, 3, recurs, 0, status );
   }

   return *status;
}

