/*
*+
*  Name:
*     hdsIsOpen

*  Purpose:
*     Check if a container file is already open in HDS V5.

*  Language:
*     Starlink ANSI C

*  Type of Module:
*     Library routine

*  Invocation:
*     int hdsIsOpen( const char *file_str, int *isopen, int * status );

*  Arguments:
*     file = const char * (Given)
*        Container file name. Use DAT__FLEXT (".h5sdf") if no suffix specified.
*     isopen = int * (Returned)
*        Pointer to a flag that is returned non-zero if the supplied
*        container file is already open within HDS V5.
*     status = int* (Given and Returned)
*        Pointer to global status.

*  Description:
*     Checks if the supplied container file is already open within HDS V5.

*  Returned Value:
*     int = inherited status on exit. This is for compatibility with the
*           original HDS API.

*  Authors:
*     DSB: David S Berry (EAO):
*     {enter_new_authors_here}

*  Notes:
*     - A file extension of DAT__FLEXT (".h5sdf") is the default.

*  History:
*     2020-10-15 (DSB):
*        Initial version
*     {enter_further_changes_here}

*  Copyright:
*     Copyright (C) 2020 East Asian Observatory.
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
#include <string.h>
#include <unistd.h>

#include "hds1.h"
#include "dat1.h"
#include "ems.h"
#include "dat_err.h"
#include "hds.h"
#include "sae_par.h"

#include "star/one.h"

#include "hdf5.h"

int hdsIsOpen(const char *file_str, int *isopen, int *status) {

  char *fname = NULL;

  /* Returns the inherited status for compatibility reasons */
  if (*status != SAI__OK) return *status;

  /* Configure the HDF5 library for our needs as this routine could be called
     before any others. */
  dat1InitHDF5();

  /* Create buffer for file name so that we include the file extension */
  fname = dau1CheckFileName( file_str, status );

  /* Check to see if the file is currently open. */
  *isopen = hds1IsOpen( fname, status );

  /* Free allocated resource */
 CLEANUP:
  if (fname) MEM_FREE(fname);

  return *status;
}


