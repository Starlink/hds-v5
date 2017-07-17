/*
*+
*  Name:
*     datLock

*  Purpose:
*     Lock an object for exclusive use by the current thread.

*  Language:
*     Starlink ANSI C

*  Type of Module:
*     Library routine

*  Invocation:
*     datLock( HDSLoc *locator, int recurs, int *status );

*  Arguments:
*     locator = HDSLoc * (Given)
*        Locator to the object that is to be locked.
*     recurs = int (Given)
*        If the supplied object is locked successfully, and "recurs" is
*        non-zero, then an attempt is made to lock any component objects
*        contained within the supplied object. No error is reported if
*        any components cannot be locked due to being locked already by
*        a different thread - such components are left unchanged. This
*        operation is recursive - any children of the child components
*        are also locked, etc.
*     status = int* (Given and Returned)
*        Pointer to global status.

*  Description:
*     This function locks an HDS object for exclusive use by the current
*     thread. An error will then be reported if any other thread
*     subsequently attempts to use the object for any purpose, whether
*     through the supplied locator or any other locator.
*
*     If the object is a structure, each component object will have its
*     own lock, which is independent of the lock on the parent object. A
*     component object and its parent can be locked by different threads.
*     However, as a convenience function this function allows all
*     component objects to be locked in addition to the supplied object
*     (see "recurs").
*
*     The current thread must unlock the object using datUnlock before it
*     can be locked for use by another thread. All objects are initially
*     locked by the current thread when they are created.

*  Notes:
*     - An error will be reported if the supplied object is currently
*     locked by another thread, but no error is reported if any component
*     objects contained within the supplied object are locked by other
*     threads (such objects are left unchanged).
*     - The majority of HDS functions will report an error if the object
*     supplied to the function has not been locked for use by the calling
*     thread. The exceptions are the functions that manage these locks -
*     datLock, datUnlock and datLocked.
*     - Attempting to lock an object that is already locked by the
*     current thread has no effect.


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

#include "ems.h"
#include "sae_par.h"
#include "dat1.h"
#include "hds.h"
#include "dat_err.h"

int datLock( HDSLoc *locator, int recurs, int *status ) {

/* Check inherited status. */
   if (*status != SAI__OK) return *status;

/* Validate input locator. */
   dat1ValidateLocator( 0, locator, status );

/* Check we can de-reference "locator" safely. */
   if( *status == SAI__OK ) {

/* Attemp to lock the specified object, plus all its components. If the
   object could not be locked because it was already locked by another
   thread, report an error. */
      if( !dat1HandleLock( locator->handle, 2, recurs, status ) ) {
         if( *status == SAI__OK ) {
            *status = DAT__THREAD;
            datMsg( "O", locator );
            emsRep( " ", "datLock: Cannot lock HDS object '^O' for use by "
                    "the current thread:", status );
            emsRep( " ", "It is already locked by another thread.", status );
         }
      }
   }

   return *status;
}

