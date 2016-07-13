//SELF.h
/********************************************************************
	SELF
	created:	2015/03/10
	author:		LX_whu 
	purpose:	This file is for SELF function
*********************************************************************/
#if !defined SELF_h__LX_whu_2015_3_10
#define SELF_h__LX_whu_2015_3_10

#include <stdio.h>
#include "typedef.h"
typedef unsigned int SELF_Off;

#define SELF_TAG	"SELF"
// class CSEHdr
// {
// public:
// 	CSEHdr(){
// 		m_section_off = 0;
// 	}
// 	virtual ~CSEHdr(){};
// 	virtual size_t hdr_size(){
// 		return sizeof(SELF_Off);
// 	}
// 	virtual void serialize_in(FILE* fp){
// 		fread(&m_section_off, sizeof(SELF_Off), 1, fp);
// 	}
// 	virtual void serialize_out(FILE* fp){
// 		fwrite(&m_section_off, sizeof(SELF_Off), 1, fp);
// 	}
// protected:
// 	SELF_Off		m_section_off;
// };

class CSESection
{
public:
	CSESection(){
		m_section_off = 0;
		m_section_size = 0;
	}
	virtual ~CSESection(){}
	virtual size_t hdr_size(){
		return 2 * sizeof(SELF_Off);
	}
	virtual void set_hdr_info(void* info) {};
	virtual int	 hdr_in(HANDLE hFile);
	virtual int	 hdr_out(HANDLE hFile);
	virtual int	 section_begin_out(HANDLE hFile);
	virtual int	 section_begin_in(HANDLE hFile);
	virtual int	 section_end_out(HANDLE hFile);
	virtual int	 section_end_in(HANDLE hFile);
	static int	 data_in(HANDLE hFile, void *buffer, DWORD size);
	static int	 data_out(HANDLE hFile, void *buffer, DWORD size);
public:
	SELF_Off		get_section_off()	{ return m_section_off; }
	SELF_Off		get_section_sz()	{ return m_section_size; }
	static CSESection*		create_object(){
		return new CSESection;
	}
protected:
	//	CSEHdr*			m_hdr;
	SELF_Off		m_section_off;
	SELF_Off		m_section_size;
};

class CSELF
{
public:
	CSELF();
	virtual ~CSELF();
public:
	bool	is_opened();
	bool	is_loaded();
	bool	is_created();
	virtual	void	close();
	virtual bool	check_file_hdr(const char* lpstrFilePath, void* check_info);
	bool			load(const char* lpstrFilePath, CSESection* (*lpfnCreateObject)(), void* check_info = NULL);
	bool			create(const char* lpstrFilePath, CSESection* (*lpfnCreateObject)(), SELF_Off section_num, void* hdr_info);
	virtual void	set_hdr_info(void* info){};
	virtual	int		section_begin_in(SELF_Off idx);
	//	virtual int		section_begin_out(SELF_Off idx);
	virtual int		section_begin_out();
	int				section_data_in(void *buffer, DWORD size);
	int				section_data_out(void *buffer, DWORD size);
	virtual int		section_end_in();
	virtual int		section_end_out(void* section_info);

	CSESection*		get_section_info(SELF_Off idx) { return m_section_pBuf[idx]; }
	int				get_section_num()	{ return m_section_num; }
protected:
	virtual SELF_Off	get_section_hdr_start(){
		return strlen(SELF_TAG) + sizeof(SELF_Off)* 2;
	}
	SELF_Off	get_section_hdr_off(SELF_Off idx){
		return get_section_hdr_start() + m_section_hdr_size *idx;
	}
	int				hdr_in() { return hdr_in(m_hFile); }
	virtual int		hdr_in(HANDLE hFile);
	int				hdr_out() { return hdr_out(m_hFile); }
	virtual int		hdr_out(HANDLE hFile);
	int				section_hdr_in(SELF_Off idx);

	int				move_file_pointer(long sz);
protected:
	CSESection**	m_section_pBuf;
	SELF_Off		m_section_num;
	SELF_Off		m_section_hdr_size;

	//	CSESection*		m_current_buf;
	SELF_Off		m_current_idx;
	//	SELF_Off		m_current_file_pos;

	HANDLE			m_hFile;
	unsigned int	m_openMode;
	//	char			m_strFilePath[512];
};



#endif // SELF_h__LX_whu_2015_3_10

