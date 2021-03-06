/*
*+
*  Name:
*     datValid

*  Purpose:
*     Enquire locator valid

*  Language:
*     Starlink ANSI C

*  Type of Module:
*     Library routine

*  Invocation:
*     datValid(const HDSLoc *locator, hdsbool_t *valid, int *status);

*  Arguments:
*     locator = const HDSLoc * (Given)
*        Object to test.
*     valid = hdsbool_t * (Returned)
*        1 if valid, otherwise 0.
*     status = int* (Given and Returned)
*        Pointer to global status.

*  Description:
*     Enquire if a locator is valid, ie currently associated with an object.

*  Notes:
*     This fuction attempts to execute even if an error status is set on entry.

*  Authors:
*     TIMJ: Tim Jenness (Cornell)
*     {enter_new_authors_here}

*  History:
*     2014-09-16 (TIMJ):
*        Initial version
*     {enter_further_changes_here}

*  Copyright:
*     Copyright (C) 2014 Cornell University
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

#include "hds1.h"
#include "dat1.h"
#include "hds.h"
#include "ems.h"

int
datValid(const HDSLoc *locator, hdsbool_t *valid, int *status) {

/* Initialise the returned value */
   *valid = 0;

/* Check a locator was supplied. */
   if ( !locator ) return *status;

/* Begin a new error reporting context */
   emsBegin( status );

/* Check the validity of the locator */
   if (locator->group_id > 0 || locator->dataset_id > 0 ) {
      if( HANDLE_VALID(locator->handle) ) *valid = 1;
   }

/* End the current error reporting context */
   emsEnd( status );

   return *status;
}
