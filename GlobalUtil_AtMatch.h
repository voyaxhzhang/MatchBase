//GlobalUtil_AtMatch.h
/********************************************************************
	GlobalUtil_AtMatch
	created:	2015/04/22
	author:		LX_whu 
	purpose:	This file is for GlobalUtil_AtMatch function
*********************************************************************/
#if !defined GlobalUtil_AtMatch_h__LX_whu_2015_4_22
#define GlobalUtil_AtMatch_h__LX_whu_2015_4_22

#include "PrjLog.hpp"
#include "LxBasicDefine.h"
#include <vector>

#if  defined(WIN32) && defined(_DEBUG)

#define DEBUG_CLIENTBLOCK new( _CLIENT_BLOCK, __FILE__, __LINE__)
#define _CRTDBG_MAP_ALLOC
#include <stdlib.h>
#include <crtdbg.h>
#define new DEBUG_CLIENTBLOCK

#endif  // _DEBUG

#define POINTID_SIZE	10
#define POINTID_BUFFER_SIZE	(POINTID_SIZE+1)

typedef struct tagPtName{
	char name[POINTID_BUFFER_SIZE];
}PtName;
typedef std::vector<PtName> CArray_PtName;
//ARRAY_CLASS(PtName, 512);
typedef struct tagSIFTDESCRIPTOR{
	BYTE	feature[128];
}SiftDes;
//ARRAY_CLASS(SiftDes, 512);
typedef std::vector<SiftDes> CArray_SiftDes;
typedef struct tagPTLOC
{
	float x, y;
}PtLoc;
//ARRAY_CLASS(PtLoc, 256);
typedef std::vector<PtLoc> CArray_PtLoc;
typedef struct tagPTFEA{
	float scale;
	float orietation;
}PtFea;
//ARRAY_CLASS(PtFea, 512);
typedef std::vector<PtFea> CArray_PtFea;


class GlobalParam{
public:
	static int		g_extract_buffer_size ;
	static int		g_match_buffer_size;
	static float	g_least_feature_ratio ;
	static int		g_match_pyrm_ratio;

	static bool		g_sift_extract_gpu;
	static bool		g_sift_match_gpu;
	static bool		g_siftgpu_initial;

#ifdef PQ_DB
	static char		g_db_name[50] ;
	static char		g_db_user[50];
	static char		g_db_password[50];
	static char		g_db_hostaddr[50];
	static char		g_db_port[50];
#endif
};

#ifdef PQ_DB
typedef enum
{
	DBRES_EMPTY_QUERY = 0,		/* empty query string was executed */
	DBRES_COMMAND_OK,			/* a query command that doesn't return
								* anything was executed properly by the
								* backend */
	DBRES_TUPLES_OK,			/* a query command that returns tuples was
								* executed properly by the backend, DBRESult
								* contains the result tuples */
	DBRES_COPY_OUT,				/* Copy Out data transfer in progress */
	DBRES_COPY_IN,				/* Copy In data transfer in progress */
	DBRES_BAD_RESPONSE,			/* an unexpected response was recv'd from the
								* backend */
	DBRES_NONFATAL_ERROR,		/* notice or warning message */
	DBRES_FATAL_ERROR			/* query failed */
} DBExecStatusType;

const char*	DBmanagerErrorMessage();
void*		DBmanagerExec(const char *query);

const char*	DBfeaPtErrorMessage();
void*		DBfeaPtExec(const char *query);

const char*	DBmodelErrorMessage();
void*		DBmodelExec(const char *query);


DBExecStatusType DBresultStatus(const void *res);
int			DBntuples(const void *res);
char*		DBgetvalue(const void *res, int tup_num, int field_num);
void		DBclear(void *res);

bool		DBcreateProject(const char* lpstrWsPath);
//int			DBcheckFeaPtInfo(const char* lpstrFeaTabName, int extract_win, int extract_interval);
bool		DBgetFeaPtTableInfo(const char* lpstrFeaTabName, int* ptSum, int* extract_win, int* extract_interval);
bool		DBcreateFeaPtTable(const char* lpstrFeaTabName, int extract_win, int extract_interval);
bool		DBremoveFeaPtTable(const char* lpstrFeaTabName);
bool		DBFeaPtTableIsExist(const char* lpstrFeaTabName);
bool		DBsetFeaPtTableInfo(const char* lpstrFeaTabName, int ptNum, int extract_win, int extract_interval);
int			DBinsertFeaPts(const char* lpstrFeaTabName, unsigned int idx, const char* lpstrImageID, int stCol, int stRow, int scale,
	float* x, float* y, float* fea, int feaDecriDimension, float* descrip, int decriDimension, int nBufferSpace[4], int ptSum);
int			DBinsertFeaPts(const char* lpstrFeaTabName, unsigned int idx, const char* lpstrImageID, int stCol, int stRow, int scale,
	float* x, float* y, float* fea, int feaDecriDimension, BYTE* descrip, int decriDimension, int nBufferSpace[4], int ptSum);
int			DBselectFeaPts(const char* lpstrFeaTabName, int stCol, int stRow, int nCols, int nRows, int scale, CArray_PtName* name_array, CArray_PtLoc* location_array, CArray_PtFea* fea_array, CArray_SiftDes* des_array);
#endif


#define MAX_VAL(maxval,array,n)  { maxval = *(array);	for (int i = 1; i < n; i++) if (*(array+i)>maxval) maxval = *(array+i); }
#define MIN_VAL(minval,array,n)  { minval = *(array);	for (int i = 1; i < n; i++) if (*(array+i)<minval) minval = *(array+i); }

//minx miny maxx maxy
#define GET_INTERSECT_RECT(rect1,rect2,rectIns)	\
{	\
	rectIns[0] = max(rect1[0], rect2[0]);	\
	rectIns[1] = max(rect1[1], rect2[1]);	\
	rectIns[2] = min(rect1[2], rect2[2]);	\
	rectIns[3] = min(rect1[3], rect2[3]);	\
}	\

#define IS_RECT_INTERSECT(rect1,rect2)			\
	(max(rect1[0], rect2[0]) < min(rect1[2], rect2[2]) && max(rect1[1], rect2[1]) < min(rect1[3], rect2[3]))	\



inline void CvtDescriptorsf2uc(const float* des1, BYTE* des2, int num ){
	for (int i = 0; i < num; ++i)
	{
		*des2 = int(512 * *des1 + 0.5);
		des1++;	des2++;
	}
}

inline void prepare_write(const char* lpstrPathName){
	char strPath[512];	strcpy(strPath, lpstrPathName);
	try
	{
		*strrpath(strPath) = 0;
	}
	catch (...)
	{
		LogPrint(ERR_FILE_WRITE, "Warning : %s maybe not exist.", strPath);
		return;
	}

	CreateDir(strPath);
}

class CFeaPtSet{
public:
	CFeaPtSet(){
		m_block_grid_cols = 0;
		m_block_grid_rows = 0;
		m_pt_num = 0;
		m_extract_win = 0;
		m_extract_interval = 0;
		m_knl = NULL;
		m_openModel = 0;
	}
	virtual ~CFeaPtSet();

	bool	create(const char* lpstrTabName, int grid_cols, int grid_rows,int scale_lvl, int win, int interval);
	bool	load(const char* lpstrTabName);
	int		write(unsigned int idx, const char* lpstrImageID, int stCol, int stRow, int nCols, int nRows, int gridCol, int gridRow, int scale,
		float* x, float* y, float* fea, int feaDecriDimension, float* descrip, int decriDimension, int nBufferSpace[4], int ptSum);
	int		write(unsigned int idx, const char* lpstrImageID, int stCol, int stRow, int nCols, int nRows, int gridCol, int gridRow, int scale,
		float* x, float* y, float* fea, int feaDecriDimension, BYTE* descrip, int decriDimension, int nBufferSpace[4], int ptSum);

	int		read(int stCol, int stRow, int nCols, int nRows, int scale,
		CArray_PtName* name_array, CArray_PtLoc* location_array, CArray_PtFea* fea_array, CArray_SiftDes* des_array);

	void	close();

	static int	check_hdr(const char* lpstrTabName, int least_pt_num, int extract_win, int extract_interval){
		CFeaPtSet feaPtSet;
		if (!feaPtSet.load(lpstrTabName)){
			return false;
		}
		if (feaPtSet.get_point_sum()<1 || (least_pt_num>0 && feaPtSet.get_point_sum() < least_pt_num)) return 0;
		if (extract_win>0 && feaPtSet.get_extract_win() != extract_win) return 0;
		if (extract_interval > 0 && feaPtSet.get_extract_interval() != extract_interval) return 0;
		return feaPtSet.get_point_sum();
	}
public:
	void	set_point_sum(int sz){ m_pt_num = sz; }
	int		get_point_sum() { return m_pt_num; }
	int		get_extract_win(){ return m_extract_win; }
	int		get_extract_interval() { return m_extract_interval; }
protected:
	int		m_block_grid_cols;
	int		m_block_grid_rows;
	int		m_pt_num;
	int		m_extract_win;
	int		m_extract_interval;

	void*	m_knl;
	int		m_openModel;
};

#endif // GlobalUtil_AtMatch_h__LX_whu_2015_4_22