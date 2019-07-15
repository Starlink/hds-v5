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
*        If zero, only unlock the supplied object itself. If non-zero,
*        also unlock any component objects contained within the supplied
*        object.
*     status = int* (Given and Returned)
*        Pointer to global status.

*  Description:
*     This function ensures that the current thread does not have a lock
*     on the supplied HDS object. Note, no error is reported if the object
*     is not locked by the current thread.
*
*     The object must be locked again, using datLock, before it can be
*     used by any other HDS function. All objects are initially
*     locked by the current thread when they are created.

*  Notes:
*     - The majority of HDS functions will report an error if the object
*     supplied to the function has not been locked for use by the calling
*     thread. The exceptions are the functions datLock and datLocked.

*  Authors:
*     DSB: David S Berry (DSB)
*     {enter_new_authors_here}

*  History:
*     10-JUL-2017 (DSB):
*        Initial version
*     15-JUL-2019 (DSB):
*        Changed so that no error is reported if the object is already
*        unlocked on entry. This is because there can be multiple
*        locators for the same object - unlocking an object using one
*        such locator should not cause an error if a later attempt is
*        made to unlock it using one of the othjer locators. For
*        instance, in ARY the "loc" and "dloc" locators may sometimes
*        refer to the same HDS object. Before this change aryUnlock
*        reported an error in such cases because unlocking "loc" also
*        implicitly caused "dloc" to be unlocked, so that the subsequent
*        attempt to unlock "dloc" explicitly caused an error to be
*        reported. There is no HDS function to tell if two locators refer
*        to the same object, so the alternative approach ofnot attempting
*        to unlock "dloc" if it refers to the same object as "loc", cannot
*        be used.
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

#include "sae_par.h"
#include "dat1.h"
#include "ems.h"
#include "hds.h"
#include "dat_err.h"

int datUnlock( HDSLoc *locator, int recurs, int *status ) {

/* Local variables; */
   Handle *error_handle = NULL;
   int lstat;
   const char *phrase;

/* Check inherited status. */
   if (*status != SAI__OK) return *status;

/* Validate input locator. */
   dat1ValidateLocator( "datUnlock", 0, locator, 0, status );

/* Check we can de-reference "locator" safely. */
   if( *status == SAI__OK ) {

/* Attempt to unlock the specified object, plus all its components if
   required. Note, no error is reported if the object is not currently
   locked by the current thread. */
      error_handle = dat1HandleLock( locator->handle, 3, recurs, 0, &lstat,
                                     status );
   }

   return *status;
}

