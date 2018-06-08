#include <android/log.h>

#include "MediaFileManager.h"
#include "Constants.h"



#define _F_DESCRIPTION


MediaFileManager::MediaFileManager(const char *fname)
{
	m_p_filename = fname;
#ifdef _F_DESCRIPTION
	m_p_fd = fopen(fname, "rb");
#else
	m_n_fd = open(fname, O_RDONLY);
#endif

}

MediaFileManager::~MediaFileManager()
{
	
#ifdef _F_DESCRIPTION
	fclose(m_p_fd);
#else
	close(m_n_fd);
#endif
}

int MediaFileManager::fmread(uint8_t *buf, int buf_size)
{
#ifdef _F_DESCRIPTION
	int len = fread(buf, 1, buf_size, m_p_fd);
#else
	int len = read(m_n_fd, buf, buf_size);
#endif
	return len;
}

int MediaFileManager::fmseek(off64_t pos, int origin)
{
#ifdef _F_DESCRIPTION
	int ret = fseek(m_p_fd, pos, origin);
#else
	int ret = lseek64(m_n_fd, pos, origin);
#endif

	if(ret < 0) {
		LOGE("[Seek]ERRNO %d ", errno);
	}
	return ret;
}

int64_t MediaFileManager::fmsize()
{
	int64_t ret = -1;

	struct stat stat_buf;
	int rc = stat(m_p_filename, &stat_buf);
	if(rc == 0)
		ret = stat_buf.st_size;

	return ret;
}

int64_t MediaFileManager::fmcurrentPosition()
{
	int64_t ret = ftell(m_p_fd);
	return ret;
}

int MediaFileManager::fmeof()
{
	int ret = feof(m_p_fd);
	return ret;
}

