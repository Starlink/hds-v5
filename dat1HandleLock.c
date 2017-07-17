/*
*+
*  Name:
*     dat1HandleLock

*  Purpose:
*     Manage the lock on a Handle.

*  Language:
*     Starlink ANSI C

*  Type of Module:
*     Library routine

*  Invocation:
*     int dat1HandleLock( Handle *handle, int oper, int recurs, int *status );

*  Arguments:
*     handle = Handle * (Given)
*        Handle to be checked.
*     oper = int (Given)
*        Operation to be performed:
*
*        1 - Return information about the current lock on the supplied Handle.
*            Returns 0 if the supplied Handle is unlocked; 1 if the supplied
*            Handle is locked by the current thread; 2 if the supplied Handle
*            is locked by a different thread. Any child Handles are always
*            ignored.
*        2 - Lock the handle for use by the current thread. Returns 0 if the
*            handle is already locked by a different thread (in which case
*            the request to lock the supplied Handle is ignored) and +1
*            otherwise. Any child Handles within the supplied Handle are
*            left unchanged unless "recurs" is non-zero.
*        3 - Unlock the handle. Returns 0 if the handle is already locked
*            by a different thread (in which case the request to unlock the
*            supplied Handle is ignored) and +1 otherwise. Any child Handles
*            within the supplied Handle are left unchanged unless "recurs"
*            is non-zero.
*
*     recurs = int (Given)
*        Only used if "oper" is 2 or 3, and the returned value is non-zero
*        (that is, after a successful lock or unlock operation). If "recurs"
*        is zero, the supplied Handle is the only Handle to be modified -
*        any child Handles are left unchanged. If "recurs" is non-zero, any
*        child Handles contained within the supplied Handle are locked or
*        unlocked recursively, in addition to locking or unclocking the
*        supplied Handle. Any child Handles that are already locked by
*        another thread are ignored and left unchanged. The returned value
*        is not affected by the value of "recurs".
*     status = int* (Given and Returned)
*        Pointer to global status.

*  Returned function value:
*     The values returned for each operation are included in the
*     description of the "oper" argument above.

*  Description:
*     This function locks or unlocks the supplied Handle. When locked,
*     the locking thread has exclusive access to the object (but not
*     necessarily any component objects) attached to the supplied handle.

*  Notes:
*     - "oper" values of -2 and -3 are used in place of 2 and 3 when
*     calling this function recursively from within itself.
*     - A value of zero is returned if an error has already ocurred, or
*     if this function fails for any reason.

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


#include <pthread.h>
#include "sae_par.h"
#include "dat1.h"
#include "ems.h"
#include "dat_err.h"

int dat1HandleLock( Handle *handle, int oper, int recurs, int *status ){

/* Local Variables; */
   int ichild;
   int result = 0;
   int top_level;

/* Check inherited status. */
   if( *status != SAI__OK ) return result;

/* To avoid deadlocks, we only lock the Handle mutex for top level
   entries to this function. If "oper" is negative, negate it and set a
   flag indicating we do not need to lock the mutex. */
   if( oper < 0 ) {
      oper = -oper;
      top_level = 0;
   } else {
      top_level = 1;
   }

/* For top-level entries to this function, we need to ensure no other thread
   is modifying the details in the handle, so attempt to lock the handle's
   mutex. */
   if( top_level ) DAT_LOCK_MUTEX2( handle );


/* Return information about the current lock on the supplied Handle.
   Returns 0 if the Handle is unlocked; 1 if the Handle is locked by
   the current thread; 2 if the Handle is locked by a different thread.
   ------------------------------------------------------------------ */
   if( oper == 1 ) {

/* Check the lock on the supplied handle, and set "result" accordingly. */
      if( !handle->locked ) {
         result = 0; /* Supplied Handle unlocked */

      } else if( pthread_equal( handle->locker, pthread_self() )) {
         result = 1; /* Supplied Handle locked by current thread */

      } else {
         result = 2; /* Supplied Handle locked by other thread */
      }




/* Lock the handle for use by the current thread. Returns 0 if the handle
   is already locked by a different thread (in which case the request to lock
   the supplied Handle is ignored) and 1 otherwise.
   ------------------------------------------------------------------ */
   } else if( oper == 2 ) {

/* If the supplied Handle is unlocked, lock it for use by the current
   thread, and return 1. */
      if( !handle->locked ){
         handle->locked = 1;
         handle->locker = pthread_self();
         result = 1;

/* If the supplied Handle is already locked for use by the current
   thread, just return 1. */
      } else if( pthread_equal( handle->locker, pthread_self() )) {
         result = 1;
      }

/* If required, and if the above lock operation was successful, lock any
   child handles that are not currently locked by another thread. */
      if( result && recurs ){
         for( ichild = 0; ichild < handle->nchild; ichild++ ) {
            (void) dat1HandleLock( handle->children[ichild], -2, 1,
                                   status );
         }
      }

/* Unlock the handle. Returns 0 if the handle is already locked by a
   different thread - in  which case the request to unlock the Handle
   is ignored, and 1 otherwise.
   ------------------------------------------------------------------ */
   } else if( oper == 3 ) {

/* If the supplied Handle is already unlocked, just return 1. */
      if( !handle->locked ){
         result = 1;

/* If the supplied Handle is locked for use by the current thread,
   unlock it and return 1. */
      } else if( pthread_equal( handle->locker, pthread_self() )) {
         handle->locked = 0;
         result = 1;
      }

/* If required, and if the above unlock operation was successful, unlock any
   child handles that are not currently locked by another thread. */
      if( result && recurs ){
         for( ichild = 0; ichild < handle->nchild; ichild++ ) {
            (void) dat1HandleLock( handle->children[ichild], -3, 1,
                                   status );
         }
      }



/* Report an error for any other "oper" value. */
   } else if( *status == SAI__OK ) {
      *status = DAT__FATAL;
      emsRepf( " ", "dat1HandleLock: Unknown 'oper' value (%d) supplied - "
               "(internal HDS programming error).", status, oper );
   }

/* If this is a top-level entry, unlock the Handle's mutex so that other
   threads can access the values in the Handle. */
   if( top_level ) DAT_UNLOCK_MUTEX2( handle );

/* Return the result. */
   return result;
}

