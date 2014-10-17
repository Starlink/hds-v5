/*
*+
*  Name:
*     hdsTune

*  Purpose:
*     Set HDS tuning parameter

*  Language:
*     Starlink ANSI C

*  Type of Module:
*     Library routine

*  Invocation:
*     hdsTune(const char *param_str, int  value, int  *status);

*  Arguments:
*     param_str = const char * (Given)
*        Name of the tuning parameter.
*     value = int (Given)
*        New parameter value.
*     status = int* (Given and Returned)
*        Pointer to global status.

*  Description:
*     Alter an HDS control setting.

*  Authors:
*     TIMJ: Tim Jenness (Cornell)
*     {enter_new_authors_here}

*  Notes:
*     - Not Yet Implemented in that no tuning parameters have any effect.
*     - HDS Classic tuning parameters are ignored.

*  History:
*     2014-09-10 (TIMJ):
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
hdsTune(const char *param_str, int  value, int  *status) {

  if (*status != SAI__OK) return *status;

  /* HDS supports options:
     - MAP: Mapping mode
     - INAL: Initial file allocation
     - 64BIT: 64-bit mode (ie HDSv4, else HDSv3)
     - MAXW: Working page list
     - NBLOCKS: Size of internal transfer buffer
     - NCOM: Optimum number of structure components
     - SHELL: Shell used for name expansion
     - SYSL: System wide locking flag
     - WAIT: Wait for locked files

     Of these SHELL might be relevant depending on how hdsWild is
     implemented. MAP might be relevant if memory mapping is ever
     enabled in HDF5.

     INAL, MAXW, NBLOCKS, NCOM, SYSL and WAIT are all irrelevant.

     64BIT will have no effect as we are using whatever HDF5 gives us.

     Ignore the ones that are irrelevant. Warn about those that
     might become relevant.

  */

  if (strncmp( param_str, "INAL", 4 ) == 0 ||
      strncmp( param_str, "64BIT", 5 ) == 0 ||
      strncmp( param_str, "MAXW", 4 ) == 0 ||
      strncmp( param_str, "NBLO", 4 ) == 0 ||
      strncmp( param_str, "NCOM", 4 ) == 0 ||
      strncmp( param_str, "SYSL", 4 ) == 0 ||
      strncmp( param_str, "WAIT", 4 ) == 0 ) {
    /* Irrelevant for HDF5 */
  } else if (strncmp( param_str, "MAP", 3) == 0 ||
             strncmp( param_str, "SHEL", 4) == 0) {
    /* Might become relevant. No mechanism for warning
       via EMS. */
  } else {
    *status = DAT__NAMIN;
    emsRepf("hdsTune_1", "hdsTune: Unknown tuning parameter '%s'",
            status, param_str );
  }

  return *status;
}

