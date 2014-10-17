/*
*+
*  Name:
*     hdsInfoI

*  Purpose:
*     Retrieve internal state from HDS at integer

*  Language:
*     Starlink ANSI C

*  Type of Module:
*     Library routine

*  Invocation:
*     hdsInfoI(const HDSLoc* loc, const char * topic, const char * extra,
*              int *result, int * status);

*  Arguments:
*     loc = const HDSLoc* (Given)
*        HDS locator, if required by the particular topic. Will be
*        ignored for FILES and LOCATORS topics and can be NULL pointer.
*     topic = const char * (Given)
*        Topic on which information is to be obtained. Allowed values are:
*        - LOCATORS : Return the number of active locators.
*                     Internal root scratch locators are ignored.
*        - ALOCATORS: Returns the number of all active locators, including
*                     scratch space.
*        - FILES : Return the number of open files
*     extra = const char * (Given)
*        Extra options to control behaviour. The content depends on
*        the particular TOPIC. See NOTES for more information.
*     result = int* (Returned)
*        Answer to the question.
*     status = int* (Given & Returned)
*        Variable holding the status value. If this variable is not
*        SAI__OK on input, the routine will return without action.
*        If the routine fails to complete, this variable will be
*        set to an appropriate error number.

*  Description:
*     Retrieves integer information associated with the current state
*     of the HDS internals.

*  Authors:
*     TIMJ: Tim Jenness (Cornell)
*     {enter_new_authors_here}

*  Notes:
*     - Not Yet Implemented

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

#include "hdf5.h"
#include "hdf5_hl.h"

#include "ems.h"
#include "sae_par.h"

#include "hds1.h"
#include "dat1.h"
#include "hds.h"

#include "dat_err.h"

int
hdsInfoI(const HDSLoc* loc, const char *topic_str, const char *extra,
	 int *result, int  *status) {

  if (*status != SAI__OK) return *status;

  *status = DAT__FATAL;
  emsRep("hdsInfoI", "hdsInfoI: Not yet implemented for HDF5",
         status);

  return *status;
}
