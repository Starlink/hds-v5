/*
*+
*  Name:
*     hdsErase

*  Purpose:
*     Erase container file.

*  Language:
*     Starlink ANSI C

*  Type of Module:
*     Library routine

*  Invocation:
*     hdsErase(HDSLoc **locator, int *status);

*  Arguments:
*     locator = HDSLoc ** (Given and Returned)
*        Locator to the container file's top-level object. Will be annuled
*        on exit.
*     status = int* (Given and Returned)
*        Pointer to global status.

*  Description:
*     Mark a container file for deletion and annul the locator
*     associated with the top-level object. The container file will
*     not be physically deleted if other primary locators are still
*     associated with the file - this is only done when the reference
*     count drops to zero.

*  Authors:
*     TIMJ: Tim Jenness (Cornell)
*     DSB: David S Berry (EAO)
*     {enter_new_authors_here}

*  Notes:
*     - Must be a top-level object.
*     - Does not attempt execute if status is bad on entry.
*     - calls unlink(2) to remove the file from the file system.

*  History:
*     2014-09-18 (TIMJ):
*        Initial version
*     2019-05-02 (DSB):
*        Re-written to defer the file deletion until the file is closed,
*        as described in the prologue description. The old version
*        deleted it immediately.
*     2019-07-09 (DSB):
*        Store the deferred file deletion flag in the handle rather than
*        the locator. The old system did not work if there were other
*        locators refering to the same file, because the flag got deleted
*        along with the locator and so had no effect when later locators
*        referring to the same file were annulled. All locators for a
*        single file use the same Handle object, so putting the deletion
*        flag in the Handle allows the flag to be honoured no matter
*        which locator ends up being annilled last.
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

#include <unistd.h>
#include <errno.h>

#include "hdf5.h"

#include "ems.h"
#include "sae_par.h"

#include "hds1.h"
#include "dat1.h"
#include "hds.h"

#include "dat_err.h"

int
hdsErase(HDSLoc **locator, int *status) {

  if (*status != SAI__OK) return *status;

  if ( !dat1IsTopLevel( *locator, status ) ) {
    *status = DAT__LOCIN;
    emsRep("hdsErase_1", "Must supply a top level locator to hdsErase",
           status );
    return *status;
  }

  /* Flag that the file should be erased when it is closed. */
  if( (*locator)->handle ) (*locator)->handle->erase = 1;

  /* Annul the locator. This will close the file if the file has no other
     active primary locators, causing it to be erased. */
  datAnnul( locator, status );

  return *status;
}
