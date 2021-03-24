#ifndef RF_STRUCTS
#define RF_STRUCTS

#define FILE_ENTRIES_PER_ROOT 34
#define FILE_NAME_LENGTH 10
#define SECTOR_MAP_NUM_GROUPS 360
#define SECTOR_SIZE 512
#define NUM_SECTORS 2880

#pragma pack(1)

typedef struct FSHEADER
{
	unsigned char jmp[3];
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
	/* MISC */
	unsigned char BootCode[SECTOR_SIZE-25];
	unsigned char BootSignature[2];
} FSHEADER __attribute__((packed));


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
} FSROOT;

#endif
