
#ifndef FILEIO_H_
#define FILEIO_H_

/* Function prototypes */
uint8_t *loadFromZipByName(char *archive, const char *filename, uint32_t *filesize);
int32_t check_zip(const char *filename);
//int gzsize(gzFile *gd);

#endif /* FILEIO_H_ */
