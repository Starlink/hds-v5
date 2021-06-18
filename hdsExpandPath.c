/*
*+
*  Name:
*     hdsExpandPath

*  Purpose:
*     Expand a supplied file path.

*  Language:
*     Starlink ANSI C

*  Type of Module:
*     Library routine

*  Invocation:
*     int hdsExpandPath( const char *file, char *buffer, size_t buflen,
*                        int * status );

*  Arguments:
*     file = const char * (Given)
*        The file name.
*     buffer = char * (Returned)
*        Pointer to a buffer i which to return the expanded file name.
*     buflen = size_t (Given)
*        The length of the supplied buffer.
*     status = int * (Given and Returned)
*        Pointer to global status.

*  Description:
*     Exapnds any shell characters within the supplied file name and appends
*     ".sdf" if required.

*  Notes:
*     - The file need not exist.
*     - An error is reported if the buffer is too small to hold the fully
*     expanded file path.

*  Returned Value:
*     int = inherited status on exit. This is for compatibility with the
*           original HDS API.

*  Authors:
*     DSB: David S Berry (EAO):
*     {enter_new_authors_here}

*  History:
*     2021-06-18 (DSB):
*        Initial version
*     {enter_further_changes_here}

*  Copyright:
*     Copyright (C) 2021 East Asian Observatory.
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

#include <string.h>
#include "dat1.h"
#include "ems.h"
#include "dat_err.h"
#include "hds.h"
#include "sae_par.h"

int hdsExpandPath( const char *file, char *buffer, size_t buflen,
                   int * status ){

/* Local Variables: */
   size_t nc;
   char *fname;

/* Returns the inherited status for compatibility reasons */
   if( *status != SAI__OK ) return *status;

/* Expand the file name and store in a newly allocated dynamic buffer. */
   fname = dau1CheckFileName( file, status );

/* Copy it into the supplied buffer, if there is room. */
   if( *status == SAI__OK && fname ) {
      nc = strlen( fname );
      if( nc + 1 > buflen ){
         *status = DAT__TRUNC;
         emsRepf( " ", "Expanded file name is too long for the suppied buffer: '%s'",
                  status, fname );
      } else {
         strcpy( buffer, fname );
      }
   }

/* Free allocated resource */
   if (fname) MEM_FREE(fname);

  return *status;
}


