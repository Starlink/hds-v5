/* Single source file to provide facilities for registering a locator
 * and unregistering a locator. This allows us to keep track of
 * all active locators and, more importantly, allows us to track
 * primary/secondary locator status: once a file no longer has
 * any primary locators associated with it the file will be closed. */

/* Single source file as that simplifies the sharing of the uthash. */

#include <string.h>
#include <pthread.h>
#include <errno.h>
#include <libgen.h>

#include "hdf5.h"
#include "ems.h"
#include "sae_par.h"
#include "hds1.h"
#include "dat1.h"
#include "hds.h"
#include "dat_err.h"

/* Use the uthash macros: https://github.com/troydhanson/uthash */
#include "uthash.h"
#include "utarray.h"

/* Declare the hash table, a set of HdsFile structures keyed by absolute
   file path. Each such structure contains lists of the primary and
   secondary locators associated with each container file. The HdsFile
   structure is defined in dat1.h. */
static HdsFile *hdsFiles = NULL;


/* Mutexes used to serialise access to the above hash table, and the
   structures stored within it. */
static pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
#define LOCK_MUTEX pthread_mutex_lock( &mutex );
#define UNLOCK_MUTEX pthread_mutex_unlock( &mutex );

/* Local functions. */
static int hds2CompareId( const void *a, const void *b );
static char *hds2AbsPath( const char *path, int *status );


/* -----------------------------------------------------------------
   Add a supplied locator into the linked-list of locators associated
   with its container file, keeping primary and secondary locators on
   separate lists. It returns non-zero if there are no primary locators
   on exit and zero otherwise. */

int hds1RegLocator( HDSLoc *locator, int *status ){

/* Local Variables: */
   HDSLoc **head = NULL;
   HDSLoc *old = NULL;
   HdsFile *hdsFile;
   char *abspath;
   char *path;
   int result = 0;

/* Check inherited status */
   if( *status != SAI__OK ) return result;

/* Lock the mutex that serialises access to the hash table */
   LOCK_MUTEX;

/* If the supplied locator already contains a pointer to the HdsFile
   structure describing its container file (e.g. because the locator was
   created from an existing locator), then use it. Otherwise, we need to
   find a suitable HdsFile within the hash table. */
   hdsFile = locator->hdsFile;
   if( !hdsFile ) {

/* Get a dynamically allocated buffer holding the path to the file
   associated with the supplied locator. */
      path = dat1GetFullName( locator->file_id, 1, NULL, status );

/* Convert the above path, which may be relative, into an absolute path.
   The absolute path is returned in a dynamically allocated string. */
      abspath = hds2AbsPath( path, status );

/* Free the momory holding the relative path. */
      if( path ) {
         MEM_FREE( path );
         path = NULL;
      }

/* Search for an existing entry in the hash table for this path. */
      if( *status == SAI__OK ) {
         HASH_FIND_STR( hdsFiles, abspath, hdsFile );

/* If not found, create a new HdsFile structure to describe the file and add
   it into the hash table using its absolute path as the key. The memory
   allocated by realpath ("abspath") is then owned by the HdsFile object
   and should be freed (using free()) when the HdsFile object is freed. */
         if( !hdsFile ){
            hdsFile = MEM_CALLOC( 1, sizeof( HdsFile ) );
            if( hdsFile ) {
               hdsFile->path = abspath;

               HASH_ADD_KEYPTR( hh, hdsFiles, hdsFile->path,
                                strlen(hdsFile->path), hdsFile );

            } else if( *status == SAI__OK ) {
               *status = DAT__FATAL;
               emsRep( " ", "Failed to allocate memory.", status );
            }
         }
      }

/* If OK, store the HdsFile pointer in the locator. */
      if( *status == SAI__OK ) locator->hdsFile = hdsFile;
   }

/* Add the locator onto the head of the appropriate double-linked list of
   primary or secondary locators associated with the file. The "->prev"
   points away from the head, the "->next" link points towards the head. */
   if( hdsFile && *status == SAI__OK ) {
      if( locator->isprimary ) {
         head = &(hdsFile->primhead);
      } else {
         head = &(hdsFile->sechead);
      }

      old = *head;
      *head = locator;
      locator->prev = old;
      locator->next = NULL;
      if( old ) old->next = locator;

      if( !(hdsFile->primhead) ) result = 1;
   }

/* Unlock the mutex that serialises access to the hash table */
   UNLOCK_MUTEX;

/* Context error message */
   if( *status != SAI__OK ) {
      emsRep( " ", "hds1RegLocator: Failed to register locator.", status );
   }

   return result;
}


/* -----------------------------------------------------------------
   Remove a supplied locator from the linked-list of locators associated
   with its container file. Return a non-zero value if this leaves no
   primary locators associated with the file. */
int hds1UnregLocator( HDSLoc *locator, int *status ) {

/* Local Variables: */
   HDSLoc *prev;
   HDSLoc *next;
   HdsFile *hdsFile;
   int result = 0;

/* Check a locator was supplied. */
   if( !locator ) return result;

/* Begin a new error reporting environment */
   emsBegin( status );

/* Connect the locators before and after the supplied locator. */
   next = locator->next;
   prev = locator->prev;
   if( prev ) prev->next = next;
   if( next ) next->prev = prev;

/* If the locator just removed was at the head of a chain, change the
   chain head to the following locator. */
   hdsFile = locator->hdsFile;
   if( !hdsFile ) {  /* Sanity check */
      *status = DAT__FATAL;
      datMsg( "L", locator );
      emsRepf( " ", "Attempt to unregister locator ^L that has no "
               "container file information.", status );

   } else if( hdsFile->sechead == locator && *status == SAI__OK ) {
      hdsFile->sechead = prev;
      if( locator->isprimary ) {  /* Sanity check */
         datMsg( "L", locator );
         *status = DAT__FATAL;
         emsRepf( " ", "Primary locator ^L was found at the head of the "
                  "list of secondary locators for '%s'.", status,
                  hdsFile->path );
      }

   } else if( hdsFile->primhead == locator && *status == SAI__OK ) {
      hdsFile->primhead = prev;
      if( !locator->isprimary ) {  /* Sanity check */
         *status = DAT__FATAL;
         datMsg( "L", locator );
         emsRepf( " ", "Secondary locator ^l was found at the head of the "
                  "list of primary locators for '%s'.", status, hdsFile->path );

/* If the list of primary locators associated with the container file is
   now empty, return a non-zero value to indicate that any remaining
   secondary should be annulled and the file closed. */
      } else if( !prev ){
          result = 1;
      }
   }

/* Nullify the links in the locator. */
   locator->next = NULL;
   locator->prev = NULL;

/* Context error message */
   if( *status != SAI__OK ) {
      emsRep( " ", "hds1UnregLocator: Failed to unregister locator.", status );
   }

/* End the current error reporting environment */
   emsEnd( status );

   return result;
}


/* -----------------------------------------------------------------
   Pop the head of the list of primary locators associated with the
   same container file as the supplied locator. NULL is returned if the
   list is empty. On the initial call, *context should be supplied holding
   a NULL pointer. The returned pointer should not be changed between calls.
   The value of locator is ignored on subsequent calls and may be NULL. */
HDSLoc *hds1PopPrimLocator( HDSLoc *locator, HdsFile **context, int *status ){
   HDSLoc *result = NULL;
   HdsFile *hdsFile = context ? *context : NULL;
   int lstat = *status;

   if( !hdsFile && locator ) {
      hdsFile = locator->hdsFile;
      if( context ) *context = hdsFile;
      if( !hdsFile && *status == SAI__OK ) {  /* Sanity check */
         *status = DAT__FATAL;
         datMsg( "L", locator );
         emsRepf( " ", "A locator (^L) was supplied that has no container "
                  "file information", status );
      }
   }

   if( hdsFile ){
      result = hdsFile->primhead;
      if( result ) {
         hdsFile->primhead = result->prev;
         result->prev = NULL;
         if( result->prev ) result->prev->next = NULL;
      }
   }

/* Context error message */
   if( *status != SAI__OK && lstat == SAI__OK ) {
      emsRep( " ", "hds1PopPrimLocator: Failed to pop the head of a "
              "list of primary locators.", status );
   }

   return result;
}

/* -----------------------------------------------------------------
   Pop the head of the list of secondary locators associated with the
   same container file as the supplied locator. NULL is returned if the
   list is empty. On the initial call, *context should be supplied holding
   a NULL pointer. The returned pointer should not be changed between calls.
   The value of locator is ignored on subsequent calls and may be NULL.
   The locator retains its HdsFile reference, but is removed from the
   chain. */
HDSLoc *hds1PopSecLocator( HDSLoc *locator, HdsFile **context, int *status ){
   HDSLoc *result = NULL;
   HdsFile *hdsFile = context ? *context : NULL;
   int lstat = *status;

   if( !hdsFile && locator ) {
      hdsFile = locator->hdsFile;
      if( context ) *context = hdsFile;
      if( !hdsFile && *status == SAI__OK ) {  /* Sanity check */
         datMsg( "L", locator );
         *status = DAT__FATAL;
         emsRepf( " ", "A locator (^L) was supplied that has no container "
                  "file information", status );

      }
   }

   if( hdsFile ){
      result = hdsFile->sechead;
      if( result ) {
         hdsFile->sechead = result->prev;
         if( result->prev ) result->prev->next = NULL;
         result->prev = NULL;
      }
   }

/* Context error message */
   if( *status != SAI__OK && lstat == SAI__OK ) {
      emsRep( " ", "hds1PopSecLocator: Failed to pop the head of a "
              "list of secondary locators.", status );
   }

   return result;
}


/* -----------------------------------------------------------------
   Free the resources used by an HdsFile structure (including the memory
   holding the structure itself). Remove it from the hash table, and
   return NULL. */
HdsFile *hds1FreeHdsFile( HdsFile *hdsFile, int *status ){

   if( hdsFile ) {
      LOCK_MUTEX;

      HASH_DEL( hdsFiles, hdsFile );

      if( hdsFile->sechead ) { /* Sanity check */
         if( *status == SAI__OK ) {
            *status = DAT__FATAL;
            emsRepf( " ", "hds1FreeHdsFile: Attempt to free an HdsFile "
                     "that still has some associated secondary locators "
                     "(file %s).", status, hdsFile->path );
         }
      } else if( hdsFile->primhead ) { /* Sanity check */
         if( *status == SAI__OK ) {
            *status = DAT__FATAL;
            emsRepf( " ", "hds1FreeHdsFile: Attempt to free an HdsFile "
                     "that still has some associated secondary locators "
                     "(file %s).", status, hdsFile->path );
         }
      }

      if( hdsFile->path ) free( hdsFile->path ); /* Must use free, not MEM_FREE */
      memset( hdsFile, 0, sizeof(*hdsFile) );
      MEM_FREE( hdsFile );
      hdsFile = NULL;

      UNLOCK_MUTEX;
   }

   return NULL;
}


/* -----------------------------------------------------------------
   Count how many primary locators are associated with a particular
   file*/
size_t hds1PrimaryCount( const HDSLoc *locator, int *status ) {
   HDSLoc *loc;
   HdsFile *hdsFile;
   size_t result = 0;

   if( locator ){
      hdsFile = locator->hdsFile;
      if( hdsFile ) {
         loc = hdsFile->primhead;
         while( loc ) {
            result++;
            loc = loc->prev;
         }
      }
   }

   return result;
}



/* -----------------------------------------------------------------
   Return a dynamically allocated array holding a list of any active
   locators associated with a supplied HDF5 file id. Also return a
   dynamically allocated array holding a list of file_ids for the same
   file that have associated locators. "*nloc" is returned holding the
   length of the *loclist array. The end of the *file_ids array is
   indicated by a first zero value. */

void hds1GetLocators( hid_t file_id, int *nloc, HDSLoc ***loclist,
                      hid_t **file_ids, int *status ) {

/* Local Variables; */
   HDSLoc **ploc;
   HDSLoc *loc;
   HdsFile *hdsFile;
   char *abspath;
   char *path;
   hid_t *pr;
   hid_t *pw;
   hid_t *pid;
   int ir;

/* Initialise returned values. */
   *nloc = 0;
   *loclist = NULL;
   *file_ids = NULL;

/* Check inherited status */
   if( *status != SAI__OK ) return;

/* We need the path to the file so that we can use it as a key into the
   hash table. Get a dynamically allocated buffer holding the path to the
   file associated with the supplied file id. */
   path = dat1GetFullName( file_id, 1, NULL, status );

/* Convert the above path, which may be relative, into an absolute path.
   The absolute path is returned in a dynamically allocated string. */
   abspath = hds2AbsPath( path, status );

/* Free the momory holding the relative path. */
   if( path ) {
      MEM_FREE( path );
      path = NULL;
   }

/* Search for an existing entry in the hash table for this path. */
   LOCK_MUTEX;
   if( abspath ) {
      HASH_FIND_STR( hdsFiles, abspath, hdsFile );

/* Free the momory holding the absolute path. Must use plain free, not
   MEM_FREE, since realpath uses plain malloc. */
      free( abspath );
      abspath = NULL;
   }

/* If found... */
   if( hdsFile ){

/* Count the number of primary locators. */
      loc = hdsFile->primhead;
      while( loc ) {
         (*nloc)++;
         loc = loc->prev;
      }

/* Add on the number of secondary locators. */
      loc = hdsFile->sechead;
      while( loc ) {
         (*nloc)++;
         loc = loc->prev;
      }

/* Allocate the returned arrays. */
      *loclist = MEM_CALLOC( *nloc, sizeof(**loclist) );
      *file_ids = MEM_CALLOC( *nloc + 1, sizeof(**file_ids) );
      if( *loclist && *file_ids ) {

/* Copy in the locators and file ids. */
         ploc = *loclist;
         pid = *file_ids;

         loc = hdsFile->primhead;
         while( loc ) {
            *(ploc++) = loc;
            *(pid++) = loc->file_id;
            loc = loc->prev;
         }

         loc = hdsFile->sechead;
         while( loc ) {
            *(ploc++) = loc;
            *(pid++) = loc->file_id;
            loc = loc->prev;
         }

/* Sort the file_ids. */
         qsort( *file_ids, *nloc, sizeof(**file_ids), hds2CompareId );

/* Remove duplicated ids, shuffling later ones down to fill the gaps. */
         pw = pr = (*file_ids) + 1;
         for( ir = 1; ir < *nloc; ir++,pw++ ) {
            if( (*pr != pw[-1]) && ( *pr != 0 )) {
               *(pw++) = *pr;
            }
         }

/* Terminate the returned list of file ids with a zero value. */
         *pw = 0;
      }
   }

   UNLOCK_MUTEX;

/* Context error message */
   if( *status != SAI__OK ) {
      emsRep( " ", "hds1GetLocators: Failed to return a list of the locators "
              "attached to a container file.", status );
   }
}



/* -----------------------------------------------------------------
   Return the number of unique opened files. */

int hds1CountFiles() {
   int num_files;
   LOCK_MUTEX;
   num_files = HASH_COUNT( hdsFiles );
   UNLOCK_MUTEX;
   return num_files;
}


/* -----------------------------------------------------------------
   Returns the number of locators that match a set of supplied filters.
   ncomp = number of components in search filter
   comps = char ** - array of pointers to filter strings
   skip_scratch_root = true, skip HDS_SCRATCH root locators
*/

int hds1CountLocators( size_t ncomp, char **comps, hdsbool_t skip_scratch_root,
                       int * status ) {

/* Local Variables: */
   HDSLoc *loc;
   HdsFile *hdsFile;
   char file_str[1024];
   char path_str[1024];
   const char *root;
   int exclude;
   int match;
   int nlev;
   int prim;
   int result = 0;
   int rootlen;

/* Check inherited status. */
   if( *status != SAI__OK ) return result;

/* Lock the mutex that serialises access to the hash table */
   LOCK_MUTEX;

/* Loop over all HdsFiles in the hash table. */
   hdsFile = hdsFiles;
   while( hdsFile ) {

/* Loop round all primary locators associated with the current hdsFile,
   followed by all secondary locators. */
      prim = 1;
      loc = hdsFile->primhead;
      while( loc ) {

/* If we do not have to filter the list then we just add that to the sum */
         if( ncomp == 0 ){
            result++;

/* Filter provided so we have to check the path of each locator against
   the filter */
         } else {
            hdsTrace( loc, &nlev, path_str, file_str, status, sizeof(path_str),
                      sizeof(file_str) );

/* Now we compare the trace to the filters */
            if( *status == SAI__OK ){
               match = 0;
               exclude = 0;

               if( skip_scratch_root ){
                  root = "HDS_SCRATCH.TEMP_";
                  rootlen = strlen(root);
                  if( strncmp( path_str, root, rootlen ) == 0 ){

/* Exclude if the string only has one "." */
                     if( !strstr( &((path_str)[rootlen-1]), ".") ){
                        exclude = 1;
                     }

                  } else if( strcmp( path_str, "HDS_SCRATCH") == 0 ){

/* HDS seems to hide the underlying root locator of the temp file (the
   global primary locator) and does not even report it with hdsShow --
   we skip it here */
                     exclude = 1;
                  }
               }

               if( !exclude ){
                  size_t j;
                  for( j=0; j<ncomp; j++ ){

/* Matching or anti-matching? */
                     if(  *(comps[j]) == '!'  ){

/* Do not forget to start one character in for the ! */
                        if( strncmp(path_str,( comps[j])+1,
                                    strlen(comps[j])-1) == 0 ){

/* Should be exempt */
                           exclude = 1;
                        }

                     } else {
                        if( strncmp(path_str, comps[j], strlen(comps[j])) == 0 ){

/* Should be included */
                           match = 1;
                        }
                     }
                  }
               }

/* Increment if we either matched something or was not excluded */
               if( match || !exclude ) result++;

/* Plough on regardless if an error has occurred. */
            } else {
               emsAnnul(status);
            }
         }

/* Get the next locator. */
         loc = loc->prev;

/* If we have just got to the end of the linked list of primary locators,
   start to go through the list of secondary locators. */
         if( !loc && prim  ){
            loc = hdsFile->sechead;
            prim = 0;
         }
      }

/* Move on to the next HdsFile structure. */
      hdsFile = hdsFile->hh.next;
   }

/* Unlock the mutex that serialises access to the hash table */
   UNLOCK_MUTEX;

/* Context error message */
   if( *status != SAI__OK ) {
      emsRep( " ", "hds1CountLocators: Failed to count the locators "
              "that match a filter.", status );
   }
   return result;
}




/* -----------------------------------------------------------------
   Returns a pointer to the existing Handle that describes the top level
   data object in the file specified by "file_id", or NULL if the file
   has not previously been opened and so has no top-level Handle as yet.
   Also returns NULL if an error occurs. */

Handle *hds1FindHandle( hid_t file_id, int *status ){

/* Local Variables; */
   HDSLoc *loc;
   Handle *result = NULL;
   char *abspath;
   char *path;
   HdsFile *hdsFile;

/* Check inherited status */
   if( *status != SAI__OK ) return result;

/* We need the path to the file so that we can use it as a key into the
   hash table. Get a dynamically allocated buffer holding the path to the
   file associated with the supplied file id. */
   path = dat1GetFullName( file_id, 1, NULL, status );

/* Convert the above path, which may be relative, into an absolute path.
   The absolute path is returned in a dynamically allocated string. */
   abspath = hds2AbsPath( path, status );

/* Free the momory holding the relative path. */
   if( path ) {
      MEM_FREE( path );
      path = NULL;
   }

/* Search for an existing entry in the hash table for this path. */
   LOCK_MUTEX;
   if( abspath ) {
      HASH_FIND_STR( hdsFiles, abspath, hdsFile );

/* Free the momory holding the absolute path. Must use plain free, not
   MEM_FREE, since realpath uses plain malloc. */
      free( abspath );
      abspath = NULL;
   }

/* If found... */
   if( hdsFile ){

/* Loop round all primary locators associated with this file until we
   find one that has a non_NULL handle. */
      loc = hdsFile->primhead;
      while( loc ) {
         if( loc->handle ) {

/* Get the handle from the locator and navigate from there up the tree of
   handles to the top of the tree. Then break out of the loop. */
            result = dat1TopHandle( loc->handle, status );
            break;
         }

/* Move on to the next primary handle in the linked list. */
         loc = loc->prev;
      }
   }

   UNLOCK_MUTEX;

/* Context error message */
   if( *status != SAI__OK ) {
      emsRep( " ", "hds1FindHandle: Failed to find a handle for a given "
              "file id.", status );
   }
   return result;
}




/* -----------------------------------------------------------------
   Version of hdsShow that uses the internal list of locators rather than
   the HDF5 list of locators. This should duplicate the HDF5 list. */

void hds1ShowFiles( hdsbool_t listfiles, hdsbool_t listlocs, int * status  ){

/* Local Variables: */
   HDSLoc *loc;
   HdsFile *hdsFile;
   char *namestr;
   hid_t objid;
   int nprim;
   int nsec;
   int num_files;

/* Check inherited status */
   if( *status != SAI__OK ) return;

/* Lock the mutex that serialises access to the registry */
   LOCK_MUTEX;

/* Print the number of registered files. */
   num_files = HASH_COUNT( hdsFiles );
   printf("Internal HDS registry: %d file%s\n", num_files,( num_files == 1 ? "" : "s"));

/* Loop over all registered files. */
   hdsFile = hdsFiles;
   while( hdsFile ) {

/* If displaying info about each file... */
      if( listfiles ) {

/* Count the number of primary locators. */
         nprim = 0;
         loc = hdsFile->primhead;
         while( loc ) {
            nprim++;
            loc = loc->prev;
         }

/* Count the number of secondary locators. */
         nsec = 0;
         loc = hdsFile->sechead;
         while( loc ) {
            nsec++;
            loc = loc->prev;
         }

/* Display the info. */
         printf( "File: %s (%d locators of which %d are primary)\n",
                 hdsFile->path, nsec+nprim, nprim );
      }

/* If displaying info about each locator... */
      if( listfiles ) {

         printf("Primary locators:\n");
         loc = hdsFile->primhead;
         while( loc ) {
            objid = dat1RetrieveIdentifier( loc, status );
            if( objid > 0 ) {
               namestr = dat1GetFullName( objid, 0, NULL, status );
            } else {
               namestr = "no groups/datasets";
            }

            printf("   %p [%s] (%s) group=%s\n", loc, namestr,
                   (loc->isprimary ? "primary" : "secondary"),
                   loc->grpname );

            loc = loc->prev;
         }

         printf("Secondary locators:\n");
         loc = hdsFile->sechead;
         while( loc ) {
            objid = dat1RetrieveIdentifier( loc, status );
            if( objid > 0 ) {
               namestr = dat1GetFullName( objid, 0, NULL, status );
            } else {
               namestr = "no groups/datasets";
            }

            printf("   %p [%s] (%s) group=%s\n", loc, namestr,
                   (loc->isprimary ? "primary" : "secondary"),
                   loc->grpname );

            loc = loc->prev;
         }
      }

/* Move on to the next HdsFile structure. */
      hdsFile = hdsFile->hh.next;
   }

/* Unlock the mutex that serialises access to the registry */
   UNLOCK_MUTEX;
}


static int hds2CompareId( const void *a, const void *b ){
   hid_t *pa = (hid_t *) a;
   hid_t *pb = (hid_t *) b;
   if( pa < pb ) {
      return -1;
   } else if( pa > pb ) {
      return 1;
   } else {
      return 0;
   }
}

/* Return the absolute path to a file given the (possibly relative) supplied
   path. We can't just use the systems's realpath function since that
   requires the file to exist, which it may not. */
static char *hds2AbsPath( const char *path, int *status ){

/* Local Variables: */
   char *absdir;
   char *abspath = NULL;
   char *dir;
   char *name;
   char *pathcopy;
   size_t dlen;
   size_t flen;
   size_t plen;

/* Check inherited status and supplied path. */
   if( *status != SAI__OK || !path ) return abspath;

/* The basename and dirname functions may alter the supplied string, so
   pass them a copy rather than the original. */
   plen = strlen(path);
   pathcopy = MEM_CALLOC( plen + 1, sizeof(char) );
   if( pathcopy ) {
      strcpy( pathcopy, path );

/* Get the file name from the path */
      name = basename( pathcopy );

/* Get the directory from the path */
      dir = dirname( pathcopy );

/* Convert the directory into an absolute path. */
      absdir = realpath( dir, NULL );
      if( !absdir ) {
         *status = DAT__FATAL;
         emsSyser( "M", errno );
         emsRepf(" ", "Error getting real path of '%s': ^M", status, path );

/* Create the returned string by joining the absolute directory and the
   file name. */
      } else {
         dlen = strlen( absdir );
         flen = strlen( name );
         abspath = MEM_CALLOC( dlen+flen+2, sizeof(char) );
         if( abspath ) {
            strcpy( abspath, absdir );
            abspath[ dlen ] = '/';
            strcpy( abspath + dlen + 1, name );
         } else {
            *status = DAT__FATAL;
            emsRepf( " ", "Failed to allocate %zu bytes of memory", status,
                     dlen+flen+2 );
         }
      }

/* Free the local copy of the supplied path. */
      MEM_FREE( pathcopy );

   } else {
      *status = DAT__FATAL;
      emsRepf( " ", "Failed to allocate %zu bytes of memory", status,
               plen + 1 );
   }

   return abspath;
}
