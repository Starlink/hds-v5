
# HDS re-implemented on top of HDF5

The Starlink Hierarchical Data System is a hierarchical data format
initially developed in 1982. It is the data format that underlies the
4 million lines of code in the Starlink software collection. HDS was
never adopted outside of the Starlink community and associated
telescopes and is now a niche data format. HDS has undergone numerous
updates over the years as it was rewritten in C from BLISS and ported
from VMS to Unix. It works well but there is no longer any person
capable of supporting it or willing to learn the details of the
internals. HDS is also an impediment to wider adoption of the
N-dimensional Data Format (NDF) data model.

Despite being developed in the late 1980s, HDF (and currently HDF5)
has been adopted much more widely in the scientific community and is
now a de facto standard for storing scientific data in a
hierarchical manner.

## libhdsh5

This library is an attempt to reimplement the HDS API in terms of the
HDF5 API. The idea is that this library could be dropped in as a
direct replacement to HDS and allow the entire Starlink software
collection to run, using HDF5 files behind the scenes which can then
be read by other general purpose tools such as h5py.

## Migration

I'm not yet worrying about migration of HDS v4 files to HDF5. Should
there be a wrapper HDS library that manages to forward calls to either
the native HDS or HDS/HDF5 based on the file that is being opened? Or
should we just write a conversion tool to copy structures from HDS
classic to HDF5?

## Authors

Tim Jenness
Copyright (C) Cornell University.

BSD 3 clause license.

Currently some high level HDS files are included that use a GPL
license and the original CLRC copyright. These implement functionality
using public HDS API and so work with no modifications. Their
continued use will cause some rethinking on license but that's for a
later time.

## Porting notes

### Error handling

HDF5 maintains its own error stack and HDS (and Starlink) uses EMS. All calls to HDF5
are wrapped by an EMS status check and on error the HDF5 stack is read and stored
in EMS error stack.

### datFind

datFind does not know whether the component to find is a structure or primitive
so it must query the type.

### datName

H5Iget_name returns full path so HDS layer needs to look at string to
extract lowest level.

### TRUE and FALSE is used in the HDF5 APIs

but not defined in HDF5 include files.

### Dimensions

HDS uses hdsdim type which currently is an 32 bit int. HDF5
uses hsize_t which is an unsigned 64bit int. In theory HDS
could switch to that type in the C interface and this will
work so long as people properly use hdsdim.

More critically, HDF5 uses C order for dimensions and
HDS uses Fortran order (even in the C layer). HDF5
transposes the dimensions in the Fortran interface to
HDF5 and we have to do the same in HDS.

### Memory mapping

Not supported by HDF5. Presumably because of things like
compression filters and data type conversion.

datMap must therefore mmap and anonymous memory area to
receive the data and then datUnmap must copy the data back
to the underlying dataset. Must also be careful to ensure
that `datGet`/`datPut` can notice that the locator is memory
mapped and at minimum must complain.

Will require that `datMap` and `datUnmap` lose the `const` in
the API as HDSLoc will need to be updated. Also need `datAnnul`
to automatically do the copy.

Must work out what happens if the program ends before the
datUnmap call has been done and also how `hdsClose` (or
`datAnnul` equivalent on the file root) can close all the mmapped
arrays.

What happens if the primitive is mapped twice? With different
locators? What happens in HDS?

### H5LTfind_dataset

Strange name because it also finds groups.

### _LOGICAL

In HDS (presumably Fortran) a `_LOGICAL` type seems to be
a 4 byte int. This seems remarkably wasteful so in this
library I am using a 1 byte bitfield type.

The `datPutXL` routines are currently using int arrays but I am
considering changing the API to be char arrays. This would require
copying in the Fortran interface. I don't think any C code in Starlink
uses the LOGICAL datPut/datGet routines. Alternatively, we could use 8
byte logicals internally in the file but use the HDF5 data type
conversion facility to read as 4 byte integers.

It might also be worth defining hdsbool_t

### datLen vs datPrec??

How do these routines differ?

SG/4 says Primtive precision vs Storage precision

`datPrec` doesn't seem to be used in any of Starlink.

`datLen` is called but in some cases seemingly as an alias
for `datClen`.

### Incompatibilies

HDF5 does not support an array of structures. Structures must be supported
by adding an explicit collection group. For example:

```
   HISTORY        <HISTORY>       {structure}
      CREATED        <_CHAR*24>      '2009-DEC-14 05:39:05.539'
      CURRENT_RECORD  <_INTEGER>     1
      RECORDS(10)    <HIST_REC>      {array of structures}

      Contents of RECORDS(1)
         DATE           <_CHAR*24>      '2009-DEC-14 05:39:05.579
```
will have to be done as

```
  /HISTORY
    Attr:TYPE=NDF
      /CREATED
      /CURRENT_RECORD
      /RECORDS
        Attr:TYPE=HIST_REC
        Attr:ISARRAY=1
        1
           Attr:TYPE=HIST_REC
           /DATE
        2
           Attr:TYPE=HIST_REC
           /DATE
```

## TO DO

`datRef` and `datMsg` currently do not report the full path to a file,
unlike the HDS library. This is presumably an error in `hdsTrace()`
implementation which may have to determine the directory.  (and
furthermore we may have to store the directory at open time in case a
`chdir` happens).
