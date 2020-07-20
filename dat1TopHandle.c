/*
*+
*  Name:
*     dat1TopHandle

*  Purpose:
*     Return a pointer to the handle at the top of the tree containing a
*     specified handle.

*  Language:
*     Starlink ANSI C

*  Type of Module:
*     Library routine

*  Invocation:
*     Handle *dat1TopHandle( Handle *handle, int *status );

*  Arguments:
*     handle = HDSLoc * (Given)
*        Pointer to the Handle.
*     status = int * (Given and Returned)
*        Pointer to the inherited status value.

*  Returned function value:
*     A NULL pointer is always returned.

*  Description:
*     Navigates up the tree of handles, starting at the supplied handle,
*     until the top of the tree is reached. The handle at the top is returned.

*  Authors:
*     DSB: David S Berry (EAO)
*     {enter_new_authors_here}

*  History:
*     20-JUL-2020 (DSB):
*        Initial version
*     {enter_further_changes_here}

*  Copyright:
*     Copyright (C) 2020 East Asian Observatory
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

#include "dat1.h"

Handle *dat1TopHandle( Handle *handle, int *status ) {

/* Local Variables: */
   Handle *parent = NULL;
   Handle *result = handle;

/* Check inherited status */
   if( !handle ) return result;

/* Validate the supplied handle. */
   if( !dat1ValidateHandle( "dat1TopHandle", handle, status ) ) return result;

/* Move up the tree of parent handles until a handle is found that has no
   parent. */
   parent = result->parent;
   while( parent ) {
      result = parent;
      parent = result->parent;
   }

/* Return the top handle. */
   return result;
}


