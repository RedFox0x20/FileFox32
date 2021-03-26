#ifndef RF_FS_H 
#define RF_FS_H 

#define FILE_ENTRIES_PER_ROOT 34
#define FILE_NAME_LENGTH 10
#define SECTOR_MAP_NUM_GROUPS 360
#define SECTORS_PER_CYLINDER 18
#define SECTOR_SIZE 512
#define NUM_SECTORS 2880

#define NAME_PRINT_FMT "%c%c%c%c%c%c%c%c%c%c"
#define NAME_PRINT_VAR(Entry) \
	Entry.Name[0], Entry.Name[1], \
	Entry.Name[2], Entry.Name[3], \
	Entry.Name[4], Entry.Name[5], \
	Entry.Name[6], Entry.Name[7], \
	Entry.Name[8], Entry.Name[9] \

/* Force alignment to 1 byte */
#pragma pack(push, 1)
/* Ignore warnings for unused attributes */
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wattributes"

typedef struct FSHEADER
{
	unsigned char Magic[2];
	unsigned char BootFlag;
	unsigned char DiskName[8];
	unsigned char DriveNumber;
	unsigned char NumCylinders;
	unsigned char NumHeads;
	unsigned char NumSectorsPerCylinder;
	/* ROOT */
	unsigned char NumReservedCylinders;
	unsigned char FSRootCylinder;
	unsigned char FSRootSector;
	unsigned char FSRootHead;
	unsigned char FSRootTableMapLength; /* remove */
} FSHEADER __attribute__((packed));

typedef struct BOOT_SECTOR
{
	unsigned char jmp[3];
	FSHEADER Header;
	unsigned char BootCode[SECTOR_SIZE-sizeof(FSHEADER)-3];
	unsigned char BootSignature[2];
} BOOT_SECTOR __attribute__((packed));

typedef struct FILE_ENTRY
{
	unsigned char Flags;
	unsigned char Name[FILE_NAME_LENGTH];
	unsigned short StartSector;
	unsigned short EndSector;
} FILE_ENTRY __attribute__((packed));

typedef struct SECTOR_MAP
{
	unsigned short UsedSectors;
	unsigned short TotalSectors;
	unsigned char SectorGroup[SECTOR_MAP_NUM_GROUPS];
	unsigned char Unused[148];
} SECTOR_MAP __attribute__((packed));

typedef struct DIRECTORY_ENTRY 
{
	FILE_ENTRY Entries[FILE_ENTRIES_PER_ROOT];
	unsigned short Next;
} DIRECTORY_ENTRY __attribute__((packed));

typedef struct FSROOT
{
	SECTOR_MAP Map;
	DIRECTORY_ENTRY Root;
} FSROOT __attribute__((packed));

void PrintSector(void *Data);
unsigned char LoadBootSector(FILE *DriveFile, BOOT_SECTOR *Boot);
unsigned char LoadRoot(FILE *DriveFile, FSHEADER *Header, FSROOT *Root);
unsigned char IsFreeSector(FSROOT *Root, unsigned short Sector);
unsigned short FindFreeSector(FSROOT *Root, unsigned short SearchOffset);
unsigned short FindFreeSectorGroup(
		FSROOT *Root,
		unsigned short SearchOffset,
		unsigned short Length);
unsigned char AddFileEntry(FSROOT *Root, FILE_ENTRY Entry);
void SetSectorState(FSROOT *Root, unsigned short Sector, unsigned char State);
unsigned char WriteSectorMap(FSROOT *Root, FSHEADER *Header, FILE *DriveFile);
unsigned char WriteRoot(FSROOT *Root, FSHEADER *Header, FILE *DriveFile);

#pragma GCC diagnostic pop
#pragma pack(pop)
#endif
