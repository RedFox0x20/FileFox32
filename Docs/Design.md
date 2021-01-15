# FileFox32 Design
This document will outline all design considerations within the FileFox32 FS.

## Key notes
* This FS is suitable for real mode (16 bit) and protected mode (32 bit)
operation.
* There is no understanding of users implemented, a modified implementation
would allow for users but it is not going to be implemented for my use until
a later date.
* The only media that should use this are floppy drives.
* It is a FAR from perfect idea, most people should probably implement a well
known, well tested FS such as FAT 12/16/32.
* This document is bound to undergo many changes as I discover new problems from
time to time or be updated with more information while being developed.
* FileFox32 is NOT going to be space efficient, it is a practice of some theory,
* it is not designed to use your disk in the most efficient format!

## Disk types
FileFox32 will be designed and created solely for 32 bit machines using low
capacity storage devices, specifcally floppy drives. There will be reserved
sectors for storing data about the drive, this will be mentioned later in this
document.

## Boot and non-boot disks
Disks containing the BIOS boot signature have the key bytes `0xAA55` at the end
of the first sector of the first cylinder so thankfully we do not need to worry
about this, however this does affect where we store data in regards to the
FileSystem header information.

Taking from the FAT filesystem, a common method is to have the following
structure:

```
[3 bytes]
[Header information]
[Bootloader code]
[Potentially some partition data]
[Boot signature]
```

For FileFox32 a similar structure will be used, however for a simple FS such as
this, partitions will not be included, this is for ease of development and
programming. In addition the FS is designed for small capacity storage devices
such as floppy drives. In doing this some storage may be wasted as where there
would be boot code, there is now nothing, however this will only be a single
sector therefore in the majority of cases, not a lot of data in comparison to
the rest of the disk.

### Bootable drive specifics
Bootable drives will hold the Bootloader, the bootloader will be designed to use
entire cylinders, rather than giving a set number of sectors, this is simply for
ease of programming. This is an inefficient method of loading/storing the
bootloader, however it makes data handling slightly easier.

## Filesystem header
The file system header will have the following structure:

```
[2 bytes magic]
[8 bytes disk name]
[1 byte  drive number, used by the bootloader]
[1 byte  number of cylinders]
[1 byte  number of heads]
[1 byte  number of sectors per cylinder]
[1 byte  number of reserved cylinders for the bootloader]
[1 byte  cylinder containing the FS root]
[1 byte  FS root sector offset]
[1 byte  FS root number of sectors used for the sector table map]

Total: 18 bytes
```

## FS root
The FS root will be located at a sector whose position is calculated using bytes
16 and 17 of the FS header, it will have a sector length of `header byte 18 + 1
sector for the root table`.

The root will consistor of a sector map, where each byte represents 8 sectors on
the disk, enough sectors will be allocated to this table to cover the full disk
as defined by header byte 18. The sector following this will be the root
directory table.

## Directory tables
Directory tables consist of an array of entries containing information about
files and directories on the disk. There will also be an additional reserved 2
bytes at the end of the table which will identify whether the table extends more
than 1 sector and which sector contains the rest of the table.

### Table entries
Table entries should have the following structure:

```
[1  byte  Entru flags]
[10 bytes File name]
[2  bytes Start sector]
[2  bytes End sector]
```
This is enough basic information to produce an efficient, easy to use file
system in combination with the `Sector map`.

### Entry flags
Table entries will have a flags byte, this will contain the properties of an
entry, this can be used to hold information such as access rights, visibility,
type of entry and more.

```
0b########
  ||||||||
  |||||||\_ 0: File     1: Directory
  ||||||\__ 0:          1: Executable
  |||||\___ 0:          1: Writeable
  ||||\____ 0:          1: Readable
  |||\_____ 0: Visible  1: Hidden
  ||\______ 0: User     1: System
  |\_______ 0:          1:
  \________ 0:          1:
```

### Data on the disk
Positions of data will be logged and a file will always consume 1-\>n
sectors. This can lead to wasted space however this is done to simplify the FS.
