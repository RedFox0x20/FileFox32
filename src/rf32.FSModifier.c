#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include "FS.h"

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


#define VERBOSE_ONLY if (Verbose)
unsigned char Verbose;
#include "FS.h"

struct FileInfo
{
	char *FilePath,
		 *OutputPath;
};

int main(int argc, char **argv)
{
	BOOT_SECTOR		Boot;
	FSROOT			Root;
	DIRECTORY_ENTRY	Dir;
	FILE 			*DriveFile 	= NULL,
					*ReadFile	= NULL;
	unsigned int	FSRootByteStart = 0;
	int opt;
	Verbose = 0;

	struct
	{
		struct FileInfo *Files;
		unsigned int NumFiles;
	} FilesToWrite = { .Files = NULL, .NumFiles = 0 };

	struct
	{
		char **Files;
		unsigned int NumFiles;
	} FilesToRemove = { .Files = NULL, .NumFiles = 0 };




	/* v|V	: Verbose mode
	 * a	: Add a file
	 * o	: The path that the adding file should be put
	 * d	: Create a directory
	 * r	: remove a file/directory
	 */
	while ((opt = getopt(argc, argv, "vVf:a:o:d:r:l")) != -1)
	{
		int Index = -1;
		switch (opt)
		{
			case 'l':
				/* List all files */
				break;

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
						goto Main_Exit;
					}
					if (LoadBootSector(DriveFile, &Boot))
					{
						printf("Closing drive file!\n");
						fclose(DriveFile);
						goto Main_Exit;
					}
					else if (LoadRoot(DriveFile, &Boot.Header, &Root))
					{
						printf("Closing drive file!\n");
						fclose(DriveFile);
						goto Main_Exit;
					}
					else
					{
						printf("Drive file loaded correctly!\n");
					}
				}
				break;

			case 'a':
				FilesToWrite.NumFiles++;
				FilesToWrite.Files = 
					(struct FileInfo *)
					realloc(
							FilesToWrite.Files,
							sizeof(struct FileInfo) * FilesToWrite.NumFiles);

				Index = FilesToWrite.NumFiles-1;
				FilesToWrite.Files[Index].FilePath =
					(char*)malloc(strlen(optarg)+1);
				strcpy(FilesToWrite.Files[Index].FilePath, optarg);
				FilesToWrite.Files[Index].OutputPath = NULL;
				break;


			case 'o':
				if (FilesToWrite.NumFiles == 0)
				{
					printf("-o expects a file to be added with -a before use!\n");
					break;
				}
				if (FilesToWrite.Files[FilesToWrite.NumFiles-1].OutputPath
						!= NULL)
				{
					printf("Output path for an input file can only be given once!\n");
					break;
				}
				FilesToWrite.Files[FilesToWrite.NumFiles-1].OutputPath =
					(char*)malloc(strlen(optarg));
				strcpy(
						FilesToWrite.Files[FilesToWrite.NumFiles-1].OutputPath,
						optarg);
				break;

			case 'd':
				/* Create the directory path given */
				break;

			case 'r':
				FilesToRemove.NumFiles++;
				FilesToRemove.Files = 
					(char**)realloc(
							FilesToRemove.Files,
							sizeof(char*) * FilesToRemove.NumFiles);

				Index = FilesToRemove.NumFiles-1;
				FilesToRemove.Files[Index] = malloc(strlen(optarg)+1);
				strcpy(FilesToRemove.Files[Index], optarg);

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

	VERBOSE_ONLY
	{
		printf(
				"DEBUG INFO:\n"
				"sizeof(BOOT_SECTOR) = %i\n"
				"sizeof(FSHEADER) = %i\n"
				"sizeof(FILE_ENTRY) = %i\n"
				"sizeof(SECTOR_MAP) = %i\n"
				"sizeof(DIRECTORY_ENTRY) = %i\n"
				"sizeof(FSROOT) = %i\n\n",
				sizeof(BOOT_SECTOR),
				sizeof(FSHEADER),
				sizeof(FILE_ENTRY),
				sizeof(SECTOR_MAP),
				sizeof(DIRECTORY_ENTRY),
				sizeof(FSROOT)
			  );
	}
	if (DriveFile == NULL) { goto Main_Exit; }

	/* Read a file and write it to the DriveFile */
	for (int i = 0; i < FilesToWrite.NumFiles; i++)
	{
		struct FileInfo FI = FilesToWrite.Files[i];
		if (FI.OutputPath != NULL)
		{
			printf("\nOpening file to add: %s\n", FI.FilePath);
			ReadFile = fopen(FI.FilePath, "rb");
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
					FindFreeSectorGroup(&Root, 0, SectorsRequired);
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
					AddFileEntry(&Root, Entry);

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
						SetSectorState(&Root, i, 1);
					}

					printf("\n");
					if (WriteRoot(&Root, &Boot.Header, DriveFile))
					{
						printf(
								"Failed to write FSRoot! "
								"Disk data may now incorrect!\n");
					}

					printf("\n");
					if (WriteSectorMap(&Root, &Boot.Header, DriveFile))
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
		else
		{
			printf("No output path provided! Skipping file %s\n", FI.FilePath);
		}


		free(FI.FilePath);
		free(FI.OutputPath);
	}

	/* Files to remove */
	for (unsigned int i = 0; i < FilesToRemove.NumFiles; i++)
	{
		char *RemoveFileName = FilesToRemove.Files[i];
		/* Detect if a file of the same name exists */
		for (
				int EntryID = 0;
				EntryID < FILE_ENTRIES_PER_ROOT;
				EntryID++)
		{
			FILE_ENTRY *Entry = &(Root.Root.Entries[EntryID]);
			if (strncmp(RemoveFileName, Entry->Name, 10))
			{
				printf("File %s found, deleting!\n", RemoveFileName);
				for (
						unsigned int i = Entry->StartSector;
						i < Entry->EndSector;
						i++	)
				{
					SetSectorState(&Root, i, 0);
				}
				memset(Entry, 0, sizeof(FILE_ENTRY));
				printf("\n");
				if (WriteRoot(&Root, &Boot.Header, DriveFile))
				{
					printf(
							"Failed to write FSRoot! "
							"Disk data may now incorrect!\n");
				}

				printf("\n");
				if (WriteSectorMap(&Root, &Boot.Header, DriveFile))
				{
					printf(
							"Failed to write sector map! "
							"Disk data may now incorrect!\n");
				}
				break;
			}
		}
		free(RemoveFileName);
	}
	free(FilesToRemove.Files);

	/* Main exit label used for safety of malloc()ed memory */
Main_Exit:
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
