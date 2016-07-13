#include "SELF.h"
#include "PrjLog.hpp"

#ifdef WIN32

#include "windows.h"

#ifndef _CreateFileE
#define _CreateFileE
static HANDLE CreateFileE(LPCSTR lpstrPathName, UINT dwDesiredAccess){
	if (dwDesiredAccess == GENERIC_READ)
		return ::CreateFile(lpstrPathName, GENERIC_READ, FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, OPEN_EXISTING, FILE_FLAG_RANDOM_ACCESS, NULL);
	else return ::CreateFile(lpstrPathName, GENERIC_READ | GENERIC_WRITE, FILE_SHARE_READ, NULL, CREATE_ALWAYS, 0, NULL);
}
#endif
#else
#define INVALID_HANDLE_VALUE			 (HANDLE)-1

#define __USE_LARGEFILE64
#include <sys/types.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <stdio.h>

typedef union _LARGE_INTEGER {
	struct { DWORD LowPart; unsigned int HighPart; };
	struct { DWORD LowPart; unsigned int HighPart; }u;
	LONGLONG QuadPart;
}LARGE_INTEGER;

#define GENERIC_READ                     (0x80000000L)
#define GENERIC_WRITE                    (0x40000000L)
#define FILE_BEGIN						 SEEK_SET	
#define FILE_CURRENT					 SEEK_CUR
#define FILE_END						 SEEK_END
#define HFILE_ERROR						 (-1)

inline char* strlwr(char *str){
	char *orig = str; char d = 'a' - 'A';
	for (; *str != '\0'; str++){ if (*str >= 'A' && *str <= 'Z') *str = *str + d; }
	return orig;
}
inline static void CloseHandle(int hFile){ close(hFile); }
inline static int WriteFile(int hFile, void *pBuf, unsigned int bufSize, void *opSz, int *lpOverlapped){
	return write(hFile, pBuf, bufSize);
}
inline static int ReadFile(int hFile, void *pBuf, unsigned int bufSize, void *opSz, unsigned int *lpOverlapped){
	return read(hFile, pBuf, bufSize);
}
inline static unsigned int SetFilePointer(int hFile, long lDistanceToMove, long *lpDistanceToMoveHigh, int dwMoveMethod){
	LONGLONG ret, dis = 0; if (lpDistanceToMoveHigh) dis = *lpDistanceToMoveHigh;
	dis = (dis << 32) | lDistanceToMove;
	ret = lseek64(hFile, dis, dwMoveMethod);
	return (unsigned int)ret;
}
inline static HANDLE CreateFileE(LPCSTR lpstrPathName, UINT dwDesiredAccess){
	if (dwDesiredAccess == GENERIC_READ) return open64(lpstrPathName, O_RDWR);
	return open64(lpstrPathName, O_CREAT | O_RDWR, S_IRUSR | S_IWUSR);
}

#endif // end for WIN32

///////////////////////////////////////////////////////////////////////////////////////////////
//CSESection
int CSESection::hdr_in(HANDLE hFile)
{
	DWORD rw;
	ReadFile(hFile, &m_section_off, sizeof(SELF_Off), &rw, NULL);
	ReadFile(hFile, &m_section_size, sizeof(SELF_Off), &rw, NULL);
	return ERR_NONE;
}

int CSESection::hdr_out(HANDLE hFile)
{
	DWORD rw;
	WriteFile(hFile, &m_section_off, sizeof(SELF_Off), &rw, NULL);
	WriteFile(hFile, &m_section_size, sizeof(SELF_Off), &rw, NULL);
	return ERR_NONE;
}

int CSESection::section_begin_out(HANDLE hFile)
{
	m_section_off = SetFilePointer(hFile, 0, NULL, FILE_CURRENT);
	return ERR_NONE;
}

int CSESection::section_begin_in(HANDLE hFile)
{
	SetFilePointer(hFile, m_section_off, NULL, FILE_BEGIN);
	return ERR_NONE;
}

int CSESection::section_end_out(HANDLE hFile)
{
	m_section_size = SetFilePointer(hFile, 0, NULL, FILE_CURRENT) - m_section_off;
	return ERR_NONE;
}

int CSESection::section_end_in(HANDLE hFile)
{
	return ERR_NONE;
}

int CSESection::data_in(HANDLE hFile, void *buffer, DWORD size)
{
	DWORD rw;
	return ReadFile(hFile, buffer, size, &rw, NULL);
}

int CSESection::data_out(HANDLE hFile, void *buffer, DWORD size)
{
	DWORD rw;
	return WriteFile(hFile, buffer, size, &rw, NULL);
}


///////////////////////////////////////////////////////////////////////////////////////
//CSELF
CSELF::CSELF(){
	m_hFile = NULL;
	m_section_pBuf = NULL;
	m_section_num = m_section_hdr_size = 0;
//	m_current_buf = NULL;
	m_current_idx = (SELF_Off)-1;

	m_openMode = 0;
}

CSELF::~CSELF()
{
	if(m_openMode)	close();
}

bool CSELF::is_loaded()
{
	if (m_openMode == GENERIC_READ) return true;
	return false;
}

bool CSELF::is_created(){
	if (m_openMode&GENERIC_WRITE) return true;
	return false;
}
bool CSELF::is_opened()
{
	if (!m_hFile || m_hFile == INVALID_HANDLE_VALUE) return false;
	return true;
}

void CSELF::close()
{
	if (m_hFile) {
		CloseHandle(m_hFile); m_hFile = NULL;
		m_openMode = 0;
	}
	if (m_section_pBuf){
		for (int i = 0; i < m_section_num; i++){
			if (m_section_pBuf[i]) delete[] m_section_pBuf[i];
		}
		delete m_section_pBuf;	m_section_pBuf = NULL;
	}
	m_section_num = m_section_hdr_size = 0;
//	m_current_buf = NULL;
	m_current_idx = (SELF_Off)-1;
}

bool CSELF::check_file_hdr(const char* lpstrFilePath, void* info)
{
	HANDLE hFile = CreateFileE(lpstrFilePath, GENERIC_READ);
	if (!hFile || hFile == INVALID_HANDLE_VALUE) return false;
	char str[10] = "";
	DWORD rw;
	ReadFile(hFile, str, strlen(SELF_TAG), &rw, NULL);
	if (strcmp(str, SELF_TAG) != 0){
		LogPrint(ERR_FILE_READ, "%s is not SELF.", lpstrFilePath);
		return false;
	}
	hdr_in(hFile);	if (m_section_num < 1) { LogPrint(ERR_FILE_READ, "no data in %s.", lpstrFilePath); return false; }
	CloseHandle(hFile); 
	return true;
}

bool CSELF::load(const char* lpstrFilePath, CSESection* (*lpfnCreateObject)(), void* check_info /* = NULL */)
{
	close();
	if (!check_info) {
		if (!CSELF::check_file_hdr(lpstrFilePath, NULL)) return false;
	}
	else if (!check_file_hdr(lpstrFilePath, check_info)) return false;

	m_hFile = CreateFileE(lpstrFilePath, GENERIC_READ);
	if (!m_hFile || m_hFile == INVALID_HANDLE_VALUE) return false;
	
	m_section_pBuf = new CSESection*[m_section_num];
	for (int i = 0; i < m_section_num; i++){
		m_section_pBuf[i] = (*lpfnCreateObject)();
	}	

	SetFilePointer(m_hFile, get_section_hdr_off(0), NULL, FILE_BEGIN);
	for (int i = 0; i < m_section_num; i++){
		m_section_pBuf[i]->hdr_in(m_hFile);
	}

	m_openMode = GENERIC_READ;
	return true;
}

bool CSELF::create(const char* lpstrFilePath, CSESection* (*lpfnCreateObject)(),SELF_Off section_num,void* hdr_info)
{
	if (section_num < 1)	return false;
	close();
	m_hFile = CreateFileE(lpstrFilePath, GENERIC_READ | GENERIC_WRITE);
	if (!m_hFile || m_hFile == INVALID_HANDLE_VALUE) return false;

	m_section_num = section_num;
	m_section_pBuf = new CSESection*[m_section_num];
	for (int i = 0; i < m_section_num; i++){
		m_section_pBuf[i] = (*lpfnCreateObject)();
	}

// 	m_current_buf = m_section_buf;
// 	m_section_hdr_size = m_current_buf->hdr_size();
	m_current_idx = 0;
	m_section_hdr_size = m_section_pBuf[0]->hdr_size();
	if(hdr_info)	set_hdr_info(hdr_info);
	hdr_out();
	{
		DWORD rw;
		int sz = m_section_num*m_section_hdr_size;
		char* str = new char[sz];	memset(str, 0, sz);
		WriteFile(m_hFile, str, sz, &rw, NULL);
		delete str;
	}
	
	m_openMode = GENERIC_READ | GENERIC_WRITE;
	return true;
}

int CSELF::hdr_in(HANDLE hFile)
{
	DWORD rw;
	SetFilePointer(hFile, strlen(SELF_TAG), NULL, FILE_BEGIN);
	ReadFile(hFile, &m_section_num, sizeof(SELF_Off), &rw, NULL);
	ReadFile(hFile, &m_section_hdr_size, sizeof(SELF_Off), &rw, NULL);
	return ERR_NONE;
}

int CSELF::hdr_out(HANDLE hFile)
{
	DWORD rw;
	SetFilePointer(hFile, 0, NULL, FILE_BEGIN);
	WriteFile(hFile, SELF_TAG, strlen(SELF_TAG), &rw, NULL);
	WriteFile(hFile, &m_section_num, sizeof(SELF_Off), &rw, NULL);
	WriteFile(hFile, &m_section_hdr_size, sizeof(SELF_Off), &rw, NULL);
	return ERR_NONE;
}

int CSELF::section_hdr_in(SELF_Off idx)
{
	SetFilePointer(m_hFile, get_section_hdr_off(idx), NULL, FILE_BEGIN);
	m_section_pBuf [idx]->hdr_in(m_hFile);
	return ERR_NONE;
}

int CSELF::section_begin_in(SELF_Off idx)
{
	CSESection* pSection = m_section_pBuf[idx];
	pSection->section_begin_in(m_hFile);
	m_current_idx = idx;
	return ERR_NONE;
}

int CSELF::section_begin_out()
{
	SetFilePointer(m_hFile, 0, NULL, FILE_END);
	CSESection* pSection = m_section_pBuf[m_current_idx];
	pSection->section_begin_out(m_hFile);
	return ERR_NONE;
}

int CSELF::section_data_in(void *buffer, DWORD size)
{
	return CSESection::data_in(m_hFile, buffer, size);
}

int CSELF::section_data_out(void *buffer, DWORD size)
{
	return CSESection::data_out(m_hFile, buffer, size);
}

int CSELF::section_end_in()
{
	return ERR_NONE;
}

int CSELF::section_end_out(void* section_info)
{	
	CSESection* pSection = m_section_pBuf[m_current_idx];
	pSection->set_hdr_info(section_info);
	pSection->section_end_out(m_hFile);
	SetFilePointer(m_hFile, get_section_hdr_off(m_current_idx), NULL, FILE_BEGIN);
	pSection->hdr_out(m_hFile);
	m_current_idx++;
	return ERR_NONE;
}

int CSELF::move_file_pointer(long sz)
{
	return SetFilePointer(m_hFile, sz, NULL, FILE_CURRENT);
}