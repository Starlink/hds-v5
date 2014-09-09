/*
*+
*  Name:
*     datPrmry

*  Purpose:
*     Set or enquire primary/secondary locator status

*  Language:
*     Starlink ANSI C

*  Type of Module:
*     Library routine

*  Invocation:
*     datPrmry(int set, HDSLoc **locator, int *prmry, int *status);

*  Arguments:
*     set = int (Given)
*        If a true value is given for this argument, then the routine will perform a "set"
*        operation to set the primary/secondary status of a locator. Otherwise it will perform
*        an "enquire" operation to return the value of this status without changing it.
*     locator = HDSLoc ** (Given and Returned)
*        The locator whose primary/secondary status is to be set or enquired.
*     prmry = int * (Given and Returned)
*        If "set" is true, then this is an input argument and specifies the new value to
*        be set (true for a primary locator, false for a secondary locator). If "set" is
*        false, then this is an output argument and will return a value indicating whether
*        or not a primary locator was supplied.
*     status = int* (Given and Returned)
*        Pointer to global status.

*  Description:
*     Currently a null operation as HDF5 does not have a primary/secondary
*     distinction.

*  Authors:
*     TIMJ: Tim Jenness (Cornell)
*     {enter_new_authors_here}

*  Notes:
*     - Always a primary.

*  History:
*     2014-09-09 (TIMJ):
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

#include "hdf5.h"
#include "hdf5_hl.h"

#include "ems.h"
#include "sae_par.h"

#include "hds1.h"
#include "dat1.h"
#include "hds.h"

int
datPrmry(int set, HDSLoc **locator, int *prmry, int *status) {

  if (*status != SAI__OK) return *status;

  if (set) {
    /* trigger an error? */

  } else {
    *prmry = 1;
  }
  return *status;
}