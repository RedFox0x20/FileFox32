#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include "FSStructs.h"

#define BYTE_TO_BINARY_FMT "0b%c%c%c%c%c%c%c%c"
#define BYTE_TO_BINARY(byte) \
	(byte & 0x80 ? '1' : '0'), \
	(byte & 0x40 ? '1' : '0'), \
	(byte & 0x20 ? '1' : '0'), \
	(byte & 0x10 ? '1' : '0'), \
	(byte & 0x08 ? '1' : '0'), \
	(byte & 0x04 ? '1' : '0'), \
	(byte & 0x02 ? '1' : '0'), \
	(byte & 0x01 ? '1' : '0')

FSHEADER		Header;
FSROOT			Root;
DIRECTORY_ENTRY	Dir;
FILE 			*DriveFile,
				*ReadFile;
unsigned int	FSRootByteStart;
unsigned char	Verbose;

#define VERBOSE_ONLY if (Verbose)

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

unsigned char LoadHeader(void)
{
	if (fseek(DriveFile, 0, SEEK_SET) != 0)
	{
		printf("Failed to seek to header location!\n");
		return 1;
	}
	int ReadResult = fread((void*)&Header, sizeof(FSHEADER), 1, DriveFile);


	VERBOSE_ONLY
	{
		printf("Header Read result = %i\n", ReadResult);
		printf("Header bytes\n");
		PrintSector(&Header);
	}

	if (!ReadResult)
	{
		printf("Failed to read drive header!\n");
		return 1;
	}

	printf("\n");
	return 0;
}

unsigned char LoadRoot(void)
{
	unsigned int Sector = 
		(Header.FSRootCylinder * SECTORS_PER_CYLINDER)
		+ Header.FSRootSector - 1;

	unsigned int SectorByte = Sector * SECTOR_SIZE;

	if (fseek(DriveFile, SectorByte, SEEK_SET) != 0)
	{
		printf("Failed to seek when loading root!\n");
		return 1;
	}
	int ReadResult = fread((void*)&Root, sizeof(FSROOT), 1, DriveFile);
	
	VERBOSE_ONLY
	{
		printf("Root sector = %i\n", Sector+1);
		printf("Root Read Result = %i\n", ReadResult);

		printf("Sector map bytes:\n");
		PrintSector(&(Root.Map));

		printf("\nRoot bytes:\n");
		PrintSector(&(Root.Root));
	}

	if (!ReadResult)
	{
		printf("Failed to read root!\n");
		return 1;
	}
	return 0;
}

unsigned char IsFreeSector(unsigned short Sector)
{
	unsigned short SectorByte = Sector / 8;
	unsigned char SectorBit = 1 << (7-(Sector % 8));
	return (Root.Map.SectorGroup[SectorByte] & SectorBit) != 0 ? 0 : 1;
}

unsigned short FindFreeSector(unsigned short SearchOffset)
{
	/* Efficiency can be improved by a factor of 8 but not necessary currently
	 */
	for (short i = SearchOffset; i < NUM_SECTORS; i++)
	{
		if (IsFreeSector(i))
		{
			return i;
		}
	}
	return 0;
}

unsigned short FindFreeSectorGroup(
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
			if (!IsFreeSector(j))
			{
				i=j;
				Valid = 0;
			}
		}
	}
	i-=1;
	return (i + Length > NUM_SECTORS) ? 0 : i;
}

unsigned char AddFileEntry(FILE_ENTRY Entry)
{
	int EntryID = -1;
	for (int i = 0; i < FILE_ENTRIES_PER_ROOT; i++)
	{
		if (Root.Root.Entries[i].Flags == 0)
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
			(char*)(Root.Root.Entries) + (EntryID*sizeof(FILE_ENTRY)),
			&Entry,
			sizeof(FILE_ENTRY)
		  );

	return 0;
}

void SetSectorState(unsigned short Sector, unsigned char State)
{
	unsigned int SectorByte = Sector / 8;
	unsigned char SectorBit = 1 << (7 - (Sector % 8));
	if (State)
	{
		Root.Map.SectorGroup[SectorByte] |= SectorBit;
	}
	else
	{
		Root.Map.SectorGroup[SectorByte] &= ~SectorBit;
	}
}

unsigned char WriteSectorMap(void)
{
	unsigned int DestByte = ((Header.FSRootCylinder * SECTORS_PER_CYLINDER)
			+ (Header.FSRootSector - 1)) * SECTOR_SIZE;

	VERBOSE_ONLY
	{
		printf("Writing sector map at %i\n", DestByte);
		PrintSector(&(Root.Map));
	}
	
	if (fseek(DriveFile, DestByte, SEEK_SET) != 0)
	{
		printf("Failed to seek to %i on the drive!\n");
		return 1;
	}
	if (!fwrite(&(Root.Map), sizeof(SECTOR_MAP), 1, DriveFile))
	{
		return 1;
	}
	return 0;
}

unsigned char WriteRoot(void)
{
	unsigned int DestByte = ((Header.FSRootCylinder * SECTORS_PER_CYLINDER)
			+ (Header.FSRootSector)) * SECTOR_SIZE;

	VERBOSE_ONLY
	{
		printf("Writing root sector map at %i\n", DestByte);
		PrintSector(&(Root.Root));
	}

	if (fseek(DriveFile, DestByte, SEEK_SET) != 0)
	{
		printf("Failed to seek to %i on the drive!\n");
		return 1;
	}
	if (!fwrite(&(Root.Root), sizeof(DIRECTORY_ENTRY), 1, DriveFile))
	{
		printf("Failed to write root! Disk data may now incorrect!\n");
		return 1;
	}
	return 0;
}

int main(int argc, char **argv)
{
	ReadFile = NULL;
	DriveFile = NULL;
	Verbose = 0;
	FSRootByteStart = 0;

	int opt;
	
	VERBOSE_ONLY printf(
			"DEBUG INFO:\n"
			"sizeof(FSHEADER) = %i\n"
			"sizeof(FILE_ENTRY) = %i\n"
			"sizeof(SECTOR_MAP) = %i\n"
			"sizeof(DIRECTORY_ENTRY) = %i\n"
			"sizeof(FSROOT) = %i\n\n",
			sizeof(FSHEADER),
			sizeof(FILE_ENTRY),
			sizeof(SECTOR_MAP),
			sizeof(DIRECTORY_ENTRY),
			sizeof(FSROOT)
		  );

	while ((opt = getopt(argc, argv, "vVf:a:d:")) != -1)
	{
		switch (opt)
		{
			case 'f':
				if (DriveFile != NULL)
				{
					printf("Ignoring drive file %s, a drive is already open!\n");
				}
				else
				{
					printf("Opening drive: %s\n\n", optarg);
					DriveFile = fopen(optarg, "rb+");
					if (DriveFile == NULL)
					{
						printf("Failed to open drive %s\n", optarg);
						return 1;
					}
					if (LoadHeader())
					{
						printf("Closing drive file!\n");
						fclose(DriveFile);
						return 1;
					}
					else if (LoadRoot())
					{
						printf("Closing drive file!\n");
						fclose(DriveFile);
						return 1;
					}
					else
					{
						printf("Drive file loaded correctly!\n");
					}
				}
				break;
			case 'a':
				if (DriveFile == NULL)
				{
					printf("Ignoring add file, no drive opened!\n");
				}
				else
				{
					/* Read a file and write it to the DriveFile */
					printf("\nOpening file to add: %s\n", optarg);
					ReadFile = fopen(optarg, "rb");
					if (ReadFile == NULL)
					{
						printf("Failed to open file! Skipping\n");
					}
					else
					{
						unsigned int FileLength;
						if (fseek(ReadFile, 0, SEEK_END) != 0)
						{
							printf("Failed to seek file!");
							break;
						}
						FileLength = ftell(ReadFile);
						if (FileLength == -1)
						{
							printf("Failed to ftell!\n");
							break;
						}
						if (fseek(ReadFile, 0, SEEK_SET) != 0)
						{
							printf("Failed to seek file!");
							break;
						}

						unsigned int SectorsRequired = 
							(FileLength / SECTOR_SIZE) + 
							((FileLength % SECTOR_SIZE) ? 1 : 0);

						unsigned short StartSector = 
							FindFreeSectorGroup(0, SectorsRequired);
						if (StartSector == 0)
						{
							printf("Failed to find a valid sector group!\n");
						}
						else
						{

							unsigned short EndSector = 
								StartSector + SectorsRequired;

							unsigned int SectorByte = StartSector * SECTOR_SIZE;
							if (fseek(DriveFile, SectorByte, SEEK_SET) != 0)
							{
								printf("Failed to seek file!");
								break;
							}

							printf("Loading/Writing file!\n");
							unsigned char c;
							for (int i = 0; i < FileLength; i++)
							{
								c = getc(ReadFile);
								while ((fputc(c, DriveFile)) != c) ;
							}

							FILE_ENTRY Entry;
							Entry.Flags = 0b00001100;
							printf("File name (10 chars)> ");
							scanf("%10s", Entry.Name);
							Entry.StartSector = StartSector;
							Entry.EndSector = EndSector;
							AddFileEntry(Entry);

							VERBOSE_ONLY printf(
									"File info:\n"
									"Name: " NAME_PRINT_FMT "\n"
									"Start: %i\n"
									"End: %i\n"
									"Flags: %i\n",
									NAME_PRINT_VAR(Entry),
									Entry.StartSector,
									Entry.EndSector,
									Entry.Flags);

							for (
									unsigned short i = StartSector;
									i < EndSector;
									i++)
							{
								SetSectorState(i, 1);
							}

							printf("\n");
							if (WriteRoot())
							{
								printf(
										"Failed to write FSRoot! "
										"Disk data may now incorrect!\n");
							}

							printf("\n");
							if (WriteSectorMap())
							{
								printf(
										"Failed to write sector map! "
										"Disk data may now incorrect!\n");
							}

						}
						fclose(ReadFile);
						ReadFile = NULL;
						printf("Finished!\n");
					}
				}
				break;
			case 'd':
				if (DriveFile == NULL)
				{
					printf("Ignoring delete file, no drive opened!\n");
				}
				else
				{
					/* Detect if a file of the same name exists */
					for (
							int EntryID = 0;
							EntryID < FILE_ENTRIES_PER_ROOT;
							EntryID++)
					{
						FILE_ENTRY *Entry = &(Root.Root.Entries[EntryID]);
						if (strncmp(optarg, Entry->Name, 10))
						{
							printf("File %s found, deleting!\n", optarg);
							for (
									unsigned int i = Entry->StartSector;
									i < Entry->EndSector;
									i++	)
							{
								SetSectorState(i, 0);
							}
							memset(Entry, 0, sizeof(FILE_ENTRY));
							printf("\n");
							if (WriteRoot())
							{
								printf(
										"Failed to write FSRoot! "
										"Disk data may now incorrect!\n");
							}

							printf("\n");
							if (WriteSectorMap())
							{
								printf(
										"Failed to write sector map! "
										"Disk data may now incorrect!\n");
							}
							break;
						}
					}
				}
				break;
			case 'V':
			case 'v':
				Verbose = 1;
				break;
			default:
				printf("Ingnoring unexpected argument: %c - %s\n", opt, optarg);
				break;
		}
	}
	if (DriveFile != NULL)
	{
		fclose(DriveFile);
	}
	if (ReadFile != NULL)
	{
		fclose(ReadFile);
	}
	return 0;
}
