/*
*+
*  Name:
*     hdsGtune

*  Purpose:
*     Obtain tuning parameter value

*  Language:
*     Starlink ANSI C

*  Type of Module:
*     Library routine

*  Invocation:
*     hdsGtune(const char *param_str, int *value, int *status);

*  Arguments:
*     param = const char * (Given)
*        Name of the tuning parameter whose value is required (case insensitive).
*     value = int * (Returned)
*        Current value of the parameter.
*     status = int* (Given and Returned)
*        Pointer to global status.

*  Description:
*     The routine returns the current value of an HDS tuning parameter
*     (normally this will be its default value, or the value last
*     specified using the hdsTune routine).

*  Authors:
*     TIMJ: Tim Jenness (Cornell)
*     {enter_new_authors_here}

*  Notes:
*     - Not Yet Implemented for general case.
*     - Note that the SHELL tuning parameter does not use public
*       constants but declares that (-1=no shell, 0=sh, 2=csh, 3=tcsh).
*       HDF5 implementation currently does not use a shell
*       so always returns -1.
*     - Tuning parameters may be abbreviated to 4 characters.

*  History:
*     2014-10-17 (TIMJ):
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

#include <string.h>
#include "ems.h"
#include "sae_par.h"

#include "hds1.h"
#include "dat1.h"
#include "hds.h"

#include "dat_err.h"

int
hdsGtune(const char *param_str, int *value, int *status) {

  if (*status != SAI__OK) return *status;

  if (strncasecmp(param_str, "SHEL", 4) == 0) {
    *value = -1;
  } else {
    *status = DAT__FATAL;
    emsRep("hdsGtune", "hdsGtune: Not yet implemented for HDF5",
           status);
  }
  return *status;
}
