#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <sys/types.h>

FILE* openfile(char const *filename[]);
size_t filesize( FILE *fp);
size_t find_eocd_zip_signature(FILE  *workfile, size_t const* f_size);
struct EOCD geteocd(FILE *workfile, size_t *startreadpos);
void printfilename(FILE *workfile, struct EOCD eocd, size_t *startreadpos);

#pragma pack(1)
struct CDFH
{
    // Обязательная сигнатура, равна 0x02014b50 
    uint32_t signature;
    // Версия для создания
    uint16_t versionMadeBy;
    // Минимальная версия для распаковки
    uint16_t versionToExtract;
    // Битовый флаг
    uint16_t generalPurposeBitFlag;
    // Метод сжатия (0 - без сжатия, 8 - deflate)
    uint16_t compressionMethod;
    // Время модификации файла
    uint16_t modificationTime;
    // Дата модификации файла
    uint16_t modificationDate;
    // Контрольная сумма
    uint32_t crc32;
    // Сжатый размер
    uint32_t compressedSize;
    // Несжатый размер
    uint32_t uncompressedSize;
    // Длина название файла
    uint16_t filenameLength;
    // Длина поля с дополнительными данными
    uint16_t extraFieldLength;
    // Длина комментариев к файлу
    uint16_t fileCommentLength;
    // Номер диска
    uint16_t diskNumber;
    // Внутренние аттрибуты файла
    uint16_t internalFileAttributes;
    // Внешние аттрибуты файла
    uint32_t externalFileAttributes;
    // Смещение до структуры LocalFileHeader
    uint32_t localFileHeaderOffset;
    // Имя файла (длиной filenameLength)
    //uint8_t *filename;
    // Дополнительные данные (длиной extraFieldLength)
 	//uint8_t *extraField;
    // Комментарий к файла (длиной fileCommentLength)
    //uint8_t *fileComment;*/
};
#pragma pack(0)

#pragma pack(1)
struct EOCD
{
    // Обязательная сигнатура, равна 0x06054b50
    uint32_t signature;
    // Номер диска
    uint16_t diskNumber;
    // Номер диска, где находится начало Central Directory
    uint16_t startDiskNumber;
    // Количество записей в Central Directory в текущем диске
    uint16_t numberCentralDirectoryRecord;
    // Всего записей в Central Directory
    uint16_t totalCentralDirectoryRecord;
    // Размер Central Directory
    uint32_t sizeOfCentralDirectory;
    // Смещение Central Directory
    uint32_t centralDirectoryOffset;
    // Длина комментария
    uint16_t commentLength;
    // Комментарий (длиной commentLength)
    //uint8_t *comment;
};
#pragma pack(0)

const size_t EOF_SIZE = 4;
const size_t EOCD_SIZE = 23;
const size_t COMMEMT_SIZE = 65535; 
const size_t CDFH_FILENAME_OFFSET = 46;

const uint32_t  EOCD_SIGNATURE = 0x06054b50;
const uint32_t  CDFH_SIGNATURE = 0x02014b50;

int main(int argc, char const *argv[])
{
	printf("Numbers arguments %d; \n" "File %s;\n", argc, argv[argc - 1]);
	if (argc == 1 || argc >= 3 ) {
		printf("Error, please select one file!");
		getchar();
		exit(0);
	}
	FILE *workfile = openfile(&argv[argc - 1]);
	size_t  f_size = filesize(workfile);
	size_t locsign = find_eocd_zip_signature(workfile, &f_size);
	struct EOCD fneocd = geteocd(workfile, &locsign);
	printfilename(workfile, fneocd, &locsign);
	if(workfile != NULL) {
		fclose(workfile);
	}
	return 0;
}


FILE* openfile(char const *filename[])
{
	FILE *ofile = fopen(*filename, "rb");
	if (ofile == NULL) {
		printf("Not open file: %s;\n", *filename);
		getchar();
		exit(-1);
	}
	return ofile;
}


size_t filesize(FILE *fp)
{
	struct stat buf; 
	fstat(fileno (fp), &buf);
	printf("File size: %u\n", (unsigned int)buf.st_size);
	return (size_t)buf.st_size;
}

size_t find_eocd_zip_signature(FILE *workfile, size_t const *f_size)
{
	uint32_t *signature = 0;
	const size_t bufsizesearch = EOF_SIZE + COMMEMT_SIZE + EOCD_SIZE;
	fseek(workfile, *f_size - bufsizesearch,  SEEK_SET);
	uint8_t buf[bufsizesearch];
	fread(&buf,sizeof(uint8_t), bufsizesearch, workfile);
	for (size_t i = 0; i < sizeof(buf) - EOF_SIZE; i++) {
		signature = (uint32_t*)(buf + i);
		if(*signature == EOCD_SIGNATURE) {
			return *f_size - bufsizesearch + i;
		}
	}
	printf("Not a zip file!\n");
	exit(0);
	return 0;
}


struct EOCD geteocd(FILE *workfile, size_t *startreadpos) {
	fseek(workfile, *startreadpos,  SEEK_SET);
	struct EOCD buf;
	fread(&buf, sizeof(struct EOCD), 1, workfile);
	return buf;
}


void printfilename(FILE *workfile, struct EOCD eocd, size_t *starteocdpos) {
	uint32_t *signature = 0;
	size_t start_cdfh = *starteocdpos - eocd.sizeOfCentralDirectory;
	fseek(workfile, start_cdfh,  SEEK_SET);
	struct CDFH bufcdfh;
	printf("Files which are inside Zip file:\n");
	for(uint16_t i = 0; i < eocd.numberCentralDirectoryRecord; i++)
	{
		fread(&bufcdfh, sizeof(struct CDFH), 1, workfile);
		unsigned char filename[bufcdfh.filenameLength+1];
		fread(&filename, bufcdfh.filenameLength, 1, workfile);
		filename[bufcdfh.filenameLength] = 0;
		printf("%i:%s\n",i + 1 ,filename);
		fseek(workfile, bufcdfh.extraFieldLength + bufcdfh.fileCommentLength,  SEEK_CUR);
	}
	return;
}