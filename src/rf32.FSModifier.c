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

void PrintSector(void *Data)
{
	for (int i = 0; i < 512; i++)
	{
		printf("0x%02X ", ((char*)Data)[i] & 0xFF);
	}
	printf("\n");
}

unsigned char LoadHeader(void)
{
	fseek(DriveFile, 0, SEEK_SET);
	int ReadResult = fread((void*)&Header, sizeof(FSHEADER), 1, DriveFile);
	printf("Header Read result = %i\n", ReadResult);
	printf("Header bytes\n");
	PrintSector(&Header);

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
		(Header.FSRootCylinder * 18)
		+ Header.FSRootSector - 1;

	printf("Root sector = %i\n", Sector+1);
	unsigned int SectorByte = Sector * 512;

	fseek(DriveFile, SectorByte, SEEK_SET);
	int ReadResult = fread((void*)&Root, sizeof(FSROOT), 1, DriveFile);
	printf("Root Read Result = %i\n", ReadResult);

	printf("Sector map bytes:\n");
	PrintSector(&(Root.Map));

	printf("\nRoot bytes:\n");
	PrintSector(&(Root.Root));

	if (!ReadResult)
	{
		printf("Failed to read root!\n");
		return 1;
	}
	return 0;
}

int main(int argc, char **argv)
{
	ReadFile = NULL;
	DriveFile = NULL;
	int opt;
	printf(
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

	while ((opt = getopt(argc, argv, "f:a:d:")) != -1)
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
				}
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
