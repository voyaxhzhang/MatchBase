#ifdef PQ_DB
#include "StdImage.h"
#include "PrjLog.hpp"

bool	CStdImage::set_workspace_path(const char* lpstrWsPath)
{
	return true;
}

inline void get_special_dir_path(const char* lpstrFilePath, const char* lpstrDirName, char* strPath){
	if (!CStdImage::get_workspace_path()[0]) { strcpy(strPath, lpstrFilePath); *strrpath(strPath) = 0; }
	else{
		sprintf(strPath, "%s/%s", CStdImage::get_workspace_path(), lpstrDirName);
	}
}
inline void get_attach_file(const char* lpstrFilePath, const char* lpstrAttachName, char* strPath){
	char* pS = strPath + strlen(strPath);
	sprintf(pS, "%s", strrpath(lpstrFilePath));
	pS = strrchr(pS, '.');	strcpy(pS, lpstrAttachName);
}

void	CStdImage::get_attach_file_dir(const char* lpstrImagePath, ATTACH_FILE_TYPE type, char* strPath)
{
	static char* strDirTag[] = { "harris", "sift", "sift", "sift", "wallis", "srtm" };
	get_special_dir_path(lpstrImagePath, strDirTag[type], strPath);
}

void	CStdImage::get_attach_file_path(const char* lpstrImagePath, ATTACH_FILE_TYPE type, char* strPath)
{
	static char* strFileTag[] = { ".hrs", ".sift", ".pyrm.sift", ".pyrmgrid.sift", "_wallis.tif", "_srtm.dem" };
	switch (type)
	{
	case AFT_HARRIS:
	case AFT_SIFT:
	case AFT_SIFT_PYRM:
	case AFT_SIFT_PYRMGRID:
	{
							  sprintf(strPath,"t%s", strrpath(lpstrImagePath) + 1);
							  strcpy(strrchr(strPath, '.'), strFileTag[type]);
							  while (*strPath++)
							  {
								  if (*strPath == '-' || *strPath == '.') *strPath = '_';
							  }
	}
		break;
	case AFT_WALLIS:
	case AFT_SRTM:
	case AFT_END:
	default:
		get_attach_file_dir(lpstrImagePath, type, strPath);
		get_attach_file(lpstrImagePath, strFileTag[type], strPath);
		break;
	}
}

void	CStdImage::create_attach_dir()
{
	if (!get_workspace_path()[0]) return;

	char strPath[512];
	for (int i = 0; i < AFT_END; i++){
		get_attach_file_dir(NULL, (ATTACH_FILE_TYPE)i, strPath);
		CreateDir(strPath);
	}
}

void	CStdImage::RemoveAttachFile()
{
	char strPath[512];	int i;
	for ( i = 0; i < AFT_WALLIS; i++){
		get_attach_file_path(GetImagePath(), (ATTACH_FILE_TYPE)i, strPath);
		DBremoveFeaPtTable(strPath);
	}
	for (; i < AFT_END; i++){
		get_attach_file_path(GetImagePath(), (ATTACH_FILE_TYPE)i, strPath);
		remove(strPath);
	}
}

CFeaPtSet::~CFeaPtSet(){
	if(m_knl) free(m_knl);
}

bool CFeaPtSet::create(const char* lpstrTabName, int grid_cols, int grid_rows, int scale_lvl, int win, int interval)
{
	close();
	m_knl = (void*)malloc(strlen(lpstrTabName)+1);
	memcpy(m_knl, lpstrTabName, strlen(lpstrTabName) + 1);

	
	if (!DBcreateFeaPtTable(lpstrTabName, win, interval)){
		return false;
	}
	m_openModel = 1;
	return true;
}

bool CFeaPtSet::load(const char* lpstrTabName)
{
	close();
	m_knl = (void*)malloc(strlen(lpstrTabName)+1);
	memcpy(m_knl, lpstrTabName, strlen(lpstrTabName) + 1);
	m_openModel = 2;
	return DBFeaPtTableIsExist(lpstrTabName) && DBgetFeaPtTableInfo(lpstrTabName, &m_pt_num, &m_extract_win, &m_extract_interval);
}

int CFeaPtSet::write(unsigned int idx, const char* lpstrImageID, int stCol, int stRow, int nCols, int nRows, int gridCol, int gridRow, int scale, float* x, float* y, float* fea, int feaDecriDimension, float* descrip, int decriDimension, int* nBufferSpace, int ptSum)
{
	float fea_tmp[2] = { scale, 0 };
	if (!fea) {
		fea = fea_tmp;
		feaDecriDimension = 0;
	}
	if (!descrip){
		decriDimension = 0;
	}
	
	return DBinsertFeaPts((const char*)m_knl, idx, lpstrImageID, stCol, stRow, scale, x, y, fea, feaDecriDimension, descrip, decriDimension, nBufferSpace, ptSum);
}

int		CFeaPtSet::write(unsigned int idx, const char* lpstrImageID, int stCol, int stRow, int nCols, int nRows, int gridCol, int gridRow, int scale,
	float* x, float* y, float* fea, int feaDecriDimension, BYTE* descrip, int decriDimension, int nBufferSpace[4], int ptSum){
	float fea_tmp[2] = { scale, 0 };
	if (!fea) {
		fea = fea_tmp;
		feaDecriDimension = 0;
	}
	if (!descrip){
		decriDimension = 0;
	}

	return DBinsertFeaPts((const char*)m_knl, idx, lpstrImageID, stCol, stRow, scale, x, y, fea, feaDecriDimension, descrip, decriDimension, nBufferSpace, ptSum);
}

int	CFeaPtSet::read(int stCol, int stRow, int nCols, int nRows, int scale, CArray_PtName* name_array, CArray_PtLoc* location_array, CArray_PtFea* fea_array, CArray_SiftDes* des_array)
{
	return DBselectFeaPts((const char*)m_knl, stCol, stRow, nCols, nRows, scale, name_array, location_array, fea_array, des_array);
}

void CFeaPtSet::close()
{
	if (m_openModel==1){
		DBsetFeaPtTableInfo((const char*)m_knl, m_pt_num, m_extract_win, m_extract_interval);
	}
	if (m_knl) {
		free(m_knl);	m_knl = NULL;
	}
	m_openModel = 0;
}

#endif