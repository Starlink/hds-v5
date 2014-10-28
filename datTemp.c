/*
*+
*  Name:
*     datTemp

*  Purpose:
*     Create temporary object

*  Language:
*     Starlink ANSI C

*  Type of Module:
*     Library routine

*  Invocation:
*     datTemp( const char *type_str, int ndim, const hdsdim dims[],
*              HDSLoc **locator, int *status );

*  Arguments:
*     type_str = const char * (Given)
*        Data type.
*     ndim = int (Given)
*        Number of dimensions.
*     dims = const hdsdim [] (Given)
*        Object dimensions.
*     locator = HDSLoc ** (Returned)
*        Temporary object locator.
*     status = int* (Given and Returned)
*        Pointer to global status.

*  Description:
*     Create an object that exists only for the lifetime of the
*     program run. This may be used to hold temporary objects -
*     including those mapped to obtain scratch space.

*  Authors:
*     TIMJ: Tim Jenness (Cornell)
*     {enter_new_authors_here}

*  Notes:
*     - If type matches one of the primitive type names, a primitive of
*       appropriate type is created; otherwise the object is assumed to
*       be a structure. If the object is a structure array, loc will be
*       associated with the complete array, not the first cell. Thus,
*       new components can only be created through another locator which
*       is explicitly associated with an individual cell (see datCell).

*  History:
*     2014-10-16 (TIMJ):
*        Initial version
*     2014-10-28 (TIMJ):
*        First working version. Reuses the locator so not sure
*        what happens if you create a primitive top level and then
*        ask for a structure. May well need to create an extra layer
*        of hierarchy, store the locator to the top-level but return
*        a locator from a level below with a dynamic name.
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

#include <stdlib.h>
#include <unistd.h>

#include "ems.h"
#include "sae_par.h"
#include "star/one.h"

#include "hds1.h"
#include "dat1.h"
#include "hds.h"
#include "dat_par.h"
#include "dat_err.h"

HDSLoc * tmploc = NULL;

int
datTemp( const char *type_str, int ndim, const hdsdim dims[],
         HDSLoc **locator, int *status ) {

  char * prefix = NULL;
  char fname[256];
  char fname_with_suffix[256+DAT__SZFLX];

  if (*status != SAI__OK) return *status;

  /* If we know that HDS_SCRATCH is not changing then we know
     that the temp file will be fixed for this process. This
     allows us to retain the locator from previous calls and just
     return it directly for each additional call. If we worry that
     HDS_SCRATCH will be modified at run time then we will have
     to store the locator in a hash map like we do for groups. */
  if (tmploc) {
    datClone(tmploc, locator, status );
    return *status;
  }

  /* Probably should use the OS temp file name generation
     system. */
  prefix = getenv( "HDS_SCRATCH" );
  one_snprintf( fname, sizeof(fname), "%s/t%x", status,
                (prefix ? prefix : "."), getpid() );

  /* Open the temp file */
  hdsNew(fname, "DAT_TEMP", type_str, ndim, dims, &tmploc, status );
  datClone(tmploc, locator, status );

  /* Usually at this point you should unlink the file and hope the
     operating system will keep the file handle open whilst deferring the delete.
     This will work on unix systems. On Windows not so well. */
  one_snprintf(fname_with_suffix, sizeof(fname_with_suffix),"%s%s", status,
               fname, DAT__FLEXT);
  retval = unlink(fname_with_suffix);

  return *status;
}
