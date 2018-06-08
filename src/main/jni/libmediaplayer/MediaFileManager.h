#ifndef FFMPEG_MEDIAFILEMANAGER_H
#define FFMPEG_MEDIAFILEMANAGER_H

#define _LARGEFILE_SOURCE 1
#define _LARGEFILE64_SOURCE
#define _FILE_OFFSET_BITS 64


#include <stdio.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>


class MediaFileManager {
protected:
	FILE					*m_p_fd;
	const char				*m_p_filename;
	int						m_n_fd;
	
public:
	MediaFileManager(const char *filename);
	~MediaFileManager();

	int						fmread(uint8_t *buf, int buf_size);
	int						fmseek(int64_t pos, int origin);
	int64_t					fmsize();
	int64_t					fmcurrentPosition();
	int						fmeof();

};
#endif //FFMPEG_MEDIAFILEMANAGER_H
