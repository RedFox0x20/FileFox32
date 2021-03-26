#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include "FS.h"

#define VERBOSE_ONLY if (Verbose)
extern unsigned char Verbose;

void PrintSector(void *Data)
{
	VERBOSE_ONLY
	{
		for (int i = 0; i < SECTOR_SIZE; i++)
		{
			printf("0x%02X ", ((char*)Data)[i] & 0xFF);
		}
		printf("\n");
	}
}

unsigned char LoadBootSector(FILE *DriveFile, BOOT_SECTOR *Boot)
{
	if (fseek(DriveFile, 0, SEEK_SET) != 0)
	{
		printf("Failed to seek to the boot sector!\n");
		return 1;
	}
	int ReadResult = fread((void*)Boot, sizeof(BOOT_SECTOR), 1, DriveFile);


	VERBOSE_ONLY
	{
		printf("Boot sector read result = %i\n", ReadResult);
		printf("Boot sector bytes\n");
		PrintSector(Boot);
	}

	if (!ReadResult)
	{
		printf("Failed to read drive header!\n");
		return 1;
	}

	printf("\n");
	return 0;
}

unsigned char LoadRoot(FILE *DriveFile, FSHEADER *Header, FSROOT *Root)
{
	unsigned int Sector = 
		(Header->FSRootCylinder * SECTORS_PER_CYLINDER)
		+ Header->FSRootSector - 1;

	unsigned int SectorByte = Sector * SECTOR_SIZE;

	if (fseek(DriveFile, SectorByte, SEEK_SET) != 0)
	{
		printf("Failed to seek when loading root!\n");
		return 1;
	}
	int ReadResult = fread((void*)Root, sizeof(FSROOT), 1, DriveFile);
	
	VERBOSE_ONLY
	{
		printf("Root sector = %i\n", Sector+1);
		printf("Root Read Result = %i\n", ReadResult);

		printf("Sector map bytes:\n");
		PrintSector(&(Root->Map));

		printf("\nRoot bytes:\n");
		PrintSector(&(Root->Root));
	}

	if (!ReadResult)
	{
		printf("Failed to read root!\n");
		return 1;
	}
	return 0;
}

unsigned char IsFreeSector(FSROOT *Root, unsigned short Sector)
{
	unsigned short SectorByte = Sector / 8;
	unsigned char SectorBit = 1 << (7-(Sector % 8));
	return (Root->Map.SectorGroup[SectorByte] & SectorBit) != 0 ? 0 : 1;
}

unsigned short FindFreeSector(FSROOT *Root, unsigned short SearchOffset)
{
	/* Efficiency can be improved by a factor of 8 but not necessary currently
	 */
	for (short i = SearchOffset; i < NUM_SECTORS; i++)
	{
		if (IsFreeSector(Root, i))
		{
			return i;
		}
	}
	return 0;
}

unsigned short FindFreeSectorGroup(
		FSROOT *Root,
		unsigned short SearchOffset,
		unsigned short Length)
{
	unsigned short i;
	unsigned char Valid = 0;
	for (i = SearchOffset; i < NUM_SECTORS - Length && !Valid; i++)
	{
		Valid = 1;
		for (unsigned short j = i; j < i+Length; j++)
		{
			if (!IsFreeSector(Root, j))
			{
				i=j;
				Valid = 0;
			}
		}
	}
	i-=1;
	return (i + Length > NUM_SECTORS) ? 0 : i;
}

unsigned char AddFileEntry(FSROOT *Root, FILE_ENTRY Entry)
{
	int EntryID = -1;
	for (int i = 0; i < FILE_ENTRIES_PER_ROOT; i++)
	{
		if (Root->Root.Entries[i].Flags == 0)
		{
			EntryID = i;
			break;
		}
	}

	if (EntryID == -1)
	{
		printf("Failed to add file entry!\n");
		return 1;
	}

	memcpy(
			((char*)(Root->Root.Entries)) + (EntryID*sizeof(FILE_ENTRY)),
			&Entry,
			sizeof(FILE_ENTRY)
		  );

	return 0;
}

void SetSectorState(FSROOT *Root, unsigned short Sector, unsigned char State)
{
	unsigned int SectorByte = Sector / 8;
	unsigned char SectorBit = 1 << (7 - (Sector % 8));
	if (State)
	{
		Root->Map.SectorGroup[SectorByte] |= SectorBit;
	}
	else
	{
		Root->Map.SectorGroup[SectorByte] &= ~SectorBit;
	}
}

unsigned char WriteSectorMap(FSROOT *Root, FSHEADER *Header, FILE *DriveFile)
{
	unsigned int DestByte = ((Header->FSRootCylinder * SECTORS_PER_CYLINDER)
			+ (Header->FSRootSector - 1)) * SECTOR_SIZE;

	VERBOSE_ONLY
	{
		printf("Writing sector map at %i\n", DestByte);
		PrintSector(&(Root->Map));
	}
	
	if (fseek(DriveFile, DestByte, SEEK_SET) != 0)
	{
		printf("Failed to seek to %i on the drive!\n");
		return 1;
	}
	if (!fwrite(&(Root->Map), sizeof(SECTOR_MAP), 1, DriveFile))
	{
		return 1;
	}
	return 0;
}

unsigned char WriteRoot(FSROOT *Root, FSHEADER *Header, FILE *DriveFile)
{
	unsigned int DestByte = ((Header->FSRootCylinder * SECTORS_PER_CYLINDER)
			+ (Header->FSRootSector)) * SECTOR_SIZE;

	VERBOSE_ONLY
	{
		printf("Writing root sector map at %i\n", DestByte);
		PrintSector(&(Root->Root));
	}

	if (fseek(DriveFile, DestByte, SEEK_SET) != 0)
	{
		printf("Failed to seek to %i on the drive!\n");
		return 1;
	}
	if (!fwrite(&(Root->Root), sizeof(DIRECTORY_ENTRY), 1, DriveFile))
	{
		printf("Failed to write root! Disk data may now incorrect!\n");
		return 1;
	}
	return 0;
}
