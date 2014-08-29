/*
*+
*  Name:
*     dau1CheckFileName

*  Purpose:
*     Return checked version of file name with appropriate extension

*  Language:
*     Starlink ANSI C

*  Type of Module:
*     Library routine

*  Invocation:
*     fname = dau1CheckFileName( const char * file_str,  int * status );

*  Arguments:
*     file_str = const char * (Given)
*        File name supplied by user.
*     status = int* (Given and Returned)
*        Pointer to global status.

*  Returned Value:
*     fname = char *
*        Updated filename with extension if appropriate. Should be freed
*        by calling MEM_FREE.

*  Description:
*     Validate the supplied file name and add the standard extension if
*     required.

*  Authors:
*     TIMJ: Tim Jenness (Cornell)
*     {enter_new_authors_here}

*  Notes:
*     - Returned pointer must be freed with MEM_FREE

*  History:
*     2014-08-29 (TIMJ):
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

#include <stdlib.h>
#include <string.h>

#include "hdf5.h"
#include "hdf5_hl.h"

#include "star/one.h"
#include "ems.h"
#include "sae_par.h"

#include "hds1.h"
#include "dat1.h"
#include "hds.h"

#include "dat_err.h"

char *
dau1CheckFileName( const char * file_str, int * status ) {

  size_t lenstr;
  char *fname = NULL;
  int needext = 0;
  size_t i;

  if (*status != SAI__OK) return NULL;

  /* Check to see if we need to add a file extension. If we find no dot before the
     final slash we add an extension later */
  lenstr = strlen(file_str);
  needext = 1; /* Assume we will need to add the suffix */
  for (i=0; i < lenstr; i++) {
    /* start from the end */
    size_t iposn = lenstr - (i + 1);
    if ( file_str[iposn] == '/' ) break;
    if ( file_str[iposn] == '.') {
      needext = 0;
      break;
    }
  }

  /* Create buffer for file name so that we include the file extension */
  if (*status == SAI__OK) {
    size_t outstrlen = lenstr + DAT__SZFLX + 1;
    fname = MEM_MALLOC( outstrlen );
    if (fname) {
      one_strlcpy( fname, file_str, outstrlen, status );
      if (needext) one_strlcat( fname, DAT__FLEXT, outstrlen, status );
    } else {
      *status = DAT__NOMEM;
      emsRep("", "Error in a string malloc. This is not good",
             status );
    }

  }

  return fname;
}
