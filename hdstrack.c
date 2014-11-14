/* Single source file to provide facilities for registering a locator
 * and unregistering a locator. This allows us to keep track of
 * all active locators and, more importantly, allows us to track
 * primary/secondary locator status: once a file id no longer has
 * any primary locators associated with it the file will be closed. */

/* Single source file as that simplifies the sharing of the uthash.
   Implementation similar to hdsgroups but here we use the HDF5 file_id
   as the key. */

#include <string.h>

#include "hdf5.h"
#include "ems.h"
#include "sae_par.h"
#include "star/one.h"
#include "hds1.h"
#include "dat1.h"
#include "hds.h"

#include "dat_err.h"

/* Have a simple hash table keyed by file_id with
 * values being an array of locators.
 */

/* Use the uthash macros: https://github.com/troydhanson/uthash */
#include "uthash.h"
#include "utarray.h"

/* Because we are using a non-standard data type as hash
   key we have to define new macros */
#define HASH_FIND_FILE_ID(head,findfile,out)    \
  HASH_FIND(hh,head,findfile,sizeof(hid_t),out)
#define HASH_ADD_FILE_ID(head,filefield,add)    \
  HASH_ADD(hh,head,filefield,sizeof(hid_t),add)

/* We do not want to clone so we just copy the pointer */
/* UTarray takes a copy so we create a mini struct to store
   the actual pointer. */
typedef struct {
  HDSLoc * locator;    /* Actual HDS locator */
} HDSelement;
UT_icd all_locators_icd = { sizeof(HDSelement *), NULL, NULL, NULL };

typedef struct {
  hid_t file_id;              /* File id: the key */
  UT_array * locators;        /* Pointer to hash of element structs */
  UT_hash_handle hh;          /* Mandatory for UTHASH */
} HDSregistry;

/* Declare the hash */
HDSregistry *all_locators = NULL;

/*
*+
*  Name:
*     hds1RegLocator

*  Purpose:
*     Register locator

*  Language:
*     Starlink ANSI C

*  Type of Module:
*     Library routine

*  Invocation:
*     hds1Reg(const HDSLoc *locator, int *status);

*  Arguments:
*     locator = const HDSLoc * (Given)
*        Object locator
*     status = int* (Given and Returned)
*        Pointer to global status.

*  Description:
*     Register a new locator.

*  Authors:
*     TIMJ: Tim Jenness (Cornell)
*     {enter_new_authors_here}

*  Notes:
*     - See also hds1UnregLocator.

*  History:
*     2014-11-10 (TIMJ):
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


int
hds1RegLocator(HDSLoc *locator, int *status) {
  HDSregistry *entry = NULL;
  HDSelement elt;
  hid_t file_id = 0;
  memset(&elt, 0, sizeof(elt));

  if (*status != SAI__OK) return *status;

  /* Sanity check */
  if (locator->file_id <= 0) {
    *status = DAT__FATAL;
    emsRep("hds1RegLocator_1", "Can not register a locator that is not"
           " associated with a file", status );
    return *status;
  }

  /* See if this entry already exists in the hash */
  file_id = locator->file_id;
  HASH_FIND_FILE_ID( all_locators, &file_id, entry );
  if (!entry) {
    entry = calloc( 1, sizeof(HDSregistry) );
    entry->file_id = locator->file_id;
    utarray_new( entry->locators, &all_locators_icd);
    HASH_ADD_FILE_ID( all_locators, file_id, entry );
  }

  /* Now we have the entry, we need to store the locator inside.
     We do not clone the locator, the locator is now owned by the group. */
  elt.locator = locator;
  utarray_push_back( entry->locators, &elt );
  return *status;
}

/*
*+
*  Name:
*     hds1FlushFile

*  Purpose:
*     Annul all locators associated with a specific file_id

*  Language:
*     Starlink ANSI C

*  Type of Module:
*     Library routine

*  Invocation:
*     hdsFlush( hid_t file_id, int *status);

*  Arguments:
*     file_id = hid_t (Given)
*        Group name
*     status = int* (Given and Returned)
*        Pointer to global status.

*  Description:
*     Annuls all locators currently assigned to a specified file.

*  Authors:
*     TIMJ: Tim Jenness (Cornell)
*     {enter_new_authors_here}

*  Notes:
*     - See also hds1RegLocator
*     - Will annul all locators and will not attempt
*       to check that there is more than one primary locator.

*  History:
*     2014-11-11 (TIMJ):
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

int
hds1FlushFile( hid_t file_id, int *status) {
  HDSregistry * entry = NULL;
  HDSelement * elt = NULL;

  if (*status != SAI__OK) return *status;

  /* See if this entry already exists in the hash */
  HASH_FIND_FILE_ID( all_locators, &file_id, entry );
  if (!entry) {
    *status = DAT__GRPIN;
    emsRepf("hdsFlush_1", "Can not flush a file %lld that does not exist",
            status, (long long)file_id );
    return *status;
  }

  /* Read all the elements from the entry and annul them */
  for ( elt = (HDSelement *)utarray_front(entry->locators);
        elt != NULL;
        elt = (HDSelement *)utarray_next(entry->locators, elt )) {
    HDSLoc * loc = elt->locator;
    /* clear the file information in the locator as we do not
       want to confuse datAnnul when it then attempts to unregister
       this file from the active list. */
    loc->file_id = 0;

    /* Auto-annulling the locator will free the memory. This will
       cause problems if a program attempts to access a secondary
       locator after they have annulled the primary locator. In HDSv4
       the locator struct was distinct from all the machinery so
       using an annulled locator would trigger an error, but not
       a SEGV. May have to have a special datAnnul that does not free
       the struct memory itself, storing them all in a group that
       can be freed at exit. */
    datAnnul( &loc, status );
  }

  /* Now we close the file itself */
  H5Fclose(file_id);

  /* Free the array and delete the hash entry */
  utarray_free( entry->locators );
  HASH_DEL( all_locators, entry );

  return *status;
}

/* Remove a locator from the master list.
   Helper routine for datAnnul which can remove the locator
   from the master list. If this was the last locator associated
   with the file or if it is the last primary locator associated
   with the file, the file will be closed and all other locators
   associated with the file will be annulled.

   Returns true if the locator was removed.

*/

hdsbool_t
hds1UnregLocator( HDSLoc * locator, int *status ) {
  HDSregistry * entry = NULL;
  HDSelement * elt = NULL;
  hid_t file_id = -1;
  int pos = -1;
  unsigned int len = 0;
  unsigned int i = 0;
  int removed = 0;
  int nprimary = 0;

  if (*status != SAI__OK) return removed;

  /* Not associated with a file, should not happen */
  if (locator->file_id <= 0) {
    *status = DAT__FATAL;
    emsRep("hds1RegLocator_1", "Can not register a locator that is not"
           " associated with a file", status );
    return *status;
  }

  /* Look for the entry associated with this name */
  file_id = locator->file_id;
  HASH_FIND_FILE_ID( all_locators, &file_id, entry );
  if ( !entry ) {
    /* if we did not find it for now trigger an error because
       this code is meant to have a store of all locators that
       were allocated */
    if (*status == SAI__OK) {
      *status = DAT__FATAL;
      emsRepf("hds1UnregLocator_2", "Internal error with locator tracking"
              " (Possible programming error)", status );
    }
    return removed;
  }

  len = utarray_len( entry->locators );
  /* Read all the elements from the entry, looking for the relevant one
     but also counting how many primary locators we have. */
  for ( i = 0; i < len; i++) {
    HDSLoc * thisloc;
    elt = (HDSelement *)utarray_eltptr( entry->locators, i );
    thisloc = elt->locator;
    if (thisloc == locator) {
      pos = i;
    } else {
      /* Do not count the found locator as it is no
         longer relevant. We need to have more than zero
         remaining primaries */
      if (thisloc->isprimary) nprimary++;
    }
  }

  if (pos > -1) {
    unsigned int upos = pos;
    utarray_erase( entry->locators, upos, 1 );

    /* This locator is being annulled so it's okay to remove
       it from the list. We remove the file_id element so as not
       to confuse datAnnul into thinking it should close the file. */
    locator->file_id = 0;

    /* if we have no more primary locators we have to free
       resources -- we call flush even if we know this is the
       only locator so that we do not duplicate the hash delete code */
    if (nprimary == 0) {
      /* Close all locators */
      hds1FlushFile( file_id, status );
    }

    removed = 1;
  } else {
    /* Somehow we did not find the locator. This should not happen. */
    if (*status == SAI__OK) {
      *status = DAT__WEIRD;
      emsRep("hds1UnregLocator", "Could not find locator associated with file"
             " (possible programming error)", status);
    }
  }

  return removed;
}

/* Count how many primary locators are associated with a particular
   file_id */

size_t
hds1PrimaryCount( hid_t file_id, int * status ) {
  HDSregistry * entry = NULL;
  HDSelement * elt = NULL;
  unsigned int len = 0;
  unsigned int i = 0;
  size_t nprimary = 0;

  if (*status != SAI__OK) return nprimary;

  /* Look for the entry associated with this name */
  HASH_FIND_FILE_ID( all_locators, &file_id, entry );

  /* Possibly should be an error */
  if ( !entry ) return nprimary;

  len = utarray_len( entry->locators );
  /* Read all the elements from the entry, looking for the relevant one
     but also counting how many primary locators we have. */
  for ( i = 0; i < len; i++) {
    HDSLoc * thisloc;
    elt = (HDSelement *)utarray_eltptr( entry->locators, i );
    thisloc = elt->locator;
    if (thisloc->isprimary) nprimary++;
  }

  return nprimary;
}


/* Version of hdsShow that uses the internal list of locators
   rather than the HDF5 list of locators. This should duplicate
   the HDF5 list.
*/

void
hds1ShowFiles( hdsbool_t listfiles, hdsbool_t listlocs, int * status ) {
  HDSregistry *entry;
  unsigned int num_files;
  if (*status != SAI__OK) return;

  num_files = HASH_COUNT(all_locators);
  printf("Internal HDS registry: %u file%s\n", num_files, (num_files == 1 ? "" : "s"));
  for (entry = all_locators; entry != NULL; entry = entry->hh.next) {
    unsigned intent = 0;
    hid_t file_id = 0;
    unsigned int len = 0;
    char * name_str = NULL;
    const char * intent_str = NULL;
    size_t nprim = 0;
    file_id = entry->file_id;
    H5Fget_intent( file_id, &intent );
    if (intent == H5F_ACC_RDONLY) {
      intent_str = "R";
    } else if (intent == H5F_ACC_RDWR) {
      intent_str = "U";
    } else {
      intent_str = "Err";
    }
    len = utarray_len( entry->locators );
    nprim = hds1PrimaryCount( file_id, status );
    name_str = dat1GetFullName( file_id, 1, NULL, status );
    if (listfiles) printf("File: %s [%s] (%d) (%u locator%s) (refcnt=%zu)\n", name_str, intent_str, file_id,
                          len, (len == 1 ? "" : "s"), nprim);
    if (listlocs) hds1ShowLocators( file_id, status );
    if (name_str) MEM_FREE(name_str);
  }
}

void
hds1ShowLocators( hid_t file_id, int * status ) {
  HDSregistry * entry = NULL;
  HDSelement * elt = NULL;
  unsigned int len = 0;
  unsigned int i = 0;
  size_t nprimary = 0;

  if (*status != SAI__OK) return;

  /* Look for the entry associated with this name */
  HASH_FIND_FILE_ID( all_locators, &file_id, entry );

  /* Possibly should be an error */
  if ( !entry ) return;

  len = utarray_len( entry->locators );
  /* Read all the elements from the entry, looking for the relevant one
     but also counting how many primary locators we have. */
  printf("File %d has %u locator%s:\n", file_id, len, (len == 1 ? "" : "s"));
  for ( i = 0; i < len; i++) {
    HDSLoc * thisloc;
    char * namestr = NULL;
    hid_t objid = 0;
    elt = (HDSelement *)utarray_eltptr( entry->locators, i );
    thisloc = elt->locator;
    objid = dat1RetrieveIdentifier( thisloc, status );
    if (objid > 0) namestr = dat1GetFullName( objid, 0, NULL, status );
    printf("Locator %p [%s] (%s) group=%s\n", thisloc, (namestr ? namestr : "no groups/datasets"),
           (thisloc->isprimary ? "primary" : "secondary"),thisloc->grpname);
    if (thisloc->isprimary) nprimary++;
    MEM_FREE(namestr);
  }
}
