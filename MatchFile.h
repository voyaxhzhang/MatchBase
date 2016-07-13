//MatchResult.h
/********************************************************************
	MatchResult
	created:	2015/06/22
	author:		LX_whu 
	purpose:	This file is for MatchResult function
*********************************************************************/
#if !defined MATCHRESULT_H_LX_whu_2015_6_22
#define MATCHRESULT_H_LX_whu_2015_6_22

#ifndef ATMATCH_CODE

#ifdef MATCHFILE_EXPORTS
#define MATCHFILE_API __declspec(dllexport)
#else
#define MATCHFILE_API __declspec(dllimport)

#ifdef _DEBUG_MATCHFILE_API
#pragma comment(lib,"MatchFileD.lib") 
#pragma message("Automatically linking with MatchFileD.lib") 
#else
#pragma comment(lib,"MatchFile.lib") 
#pragma message("Automatically linking with MatchFile.lib") 
#endif

#endif
#else
#define MATCHFILE_API
#endif

class MATCHFILE_API CMatchFile{
public:
	CMatchFile();
	~CMatchFile();

	bool	LoadPrj(const char* lpstrPrjPath);
	int		GetImageNum();
	int		GetGcpNum();
	int		GetTieModelNum();
	int		GetGcpModelNum();
	bool	GetImageInfo(unsigned int idx, char* image_id, char* name, char* path, char* rpc, char* thumb, char* calib, int* nCols, int* nRows, char* gcp);
	bool	ReadTieModelHdr(unsigned int model_idx, unsigned  int* imageL_array_idx, unsigned  int* imageR_array_idx, int* nPtSum);
	bool		ReadTieModel(
		unsigned int model_idx,
		char* ptL_name, float* xl, float* yl,
		char* ptR_name, float* xr, float* yr,
		float* search_area,
		int nBufferSpaceByChar[7]
		);
	bool	ReadGcpFileHdr(unsigned int idx,  int* nPtSum);
	bool	ReadGcpFile(
		unsigned int idx,
		char* pt_name,
		float*	col,
		float*	row,
		float*	image_priori,
		double*	x,
		double*	y,
		double*	z,
		float*	plane_precision,
		float*	elevation_precision,
		char*	type,
		int nBufferSpaceByChar[10]
		);
	/*
	lpstrRpcL: left rpc path
	lpstrRpcR: right rpc path
	ratio_colL: overlap ratio of col on left image
	ratio_rowL: overlap ratio of row on left image
	ratio_colR: overlap ratio of col on right image
	ratio_rowR: overlap ratio of row on right image
	*/
	static bool		Overlap(
		const char* lpstrRpcL, const char* lpstrRpcR,
		float* ratio_colL, float* ratio_rowL,
		float* ratio_colR, float* ratio_rowR
		);
public:
	static bool	ReadModelFileHdr(
		const char* lpstrModelFilePath,
		char* strImageLPath,
		char* strImageRPath,
		int*	nPtSum
		);
	static int		ReadModelFile(
		const char* lpstrModelFilePath,
		const char* lpstrImageLID,
		const char* lpstrImageRID,
		char* ptL_name, float* xl, float* yl,
		char* ptR_name, float* xr, float* yr,
		float* search_area,
		int nBufferSpaceByChar[7]
		);
	static bool		ReadGcpFileHdr(
		const char* lpstrFilePath,
		int*	nPtSum
		);
	static int		ReadGcpFile(
		const char* lpstrFilePath, const char* lpstrImageID,
		char* pt_name,
		float*	col,
		float*	row,
		float*	image_priori,
		double*	x,
		double*	y,
		double*	z,
		float*	plane_precision,
		float*	elevation_precision,
		char*	type,
		int nBufferSpaceByChar[10]
		);
#ifdef ATMATCH_CODE
	bool	SavePrj(const char* lpstrPrjPath);
	static bool	MergePrj(const char* lpstrPrj1, const char* lpstrPrj2, const char* lpstrDstPrj);
	static bool	WriteModelFile(
		const char* lpstrModelFilePath, 
		const char* lpstrImageLID, const char* lpstrImageLPath,
		const char* lpstrImageRID, const char* lpstrImageRPath,
		const char* ptL_name,const float* xl,const float* yl,
		const char* ptR_name,const float* xr,const float* yr,
		const char* attach_info,
		int nPtNum, int nBufferSpaceByChar[7]
		);
	static bool	WriteModelFile(
		const char* lpstrModelFilePath,
		const char* lpstrImageLPath,
		const char* lpstrImageRPath,
		const float* xl,const float* yl,
		const float* xr,const float* yr,
		int nPtNum, int nBufferSpaceByChar[4]
		);
	static bool	WriteGcpFile(
		const char* lpstrFilePath, const char* lpstrImageID,
		const char* pt_name,
		const float*	col,
		const float*	row,
		const double*	x,
		const double*	y,
		const double*	z,
		const float*	plane_precision,
		const float*	elevation_precision,
		int nPtNum, int nBufferSpaceByChar[8]
		);
	int		AppendImage(void* img, int num,int rangX,int rangY);
	int		AppendGcp(void* img, int num,int rangX,int rangY);

public:
	static bool			set_workspace_path(const char* lpstrWsPath);
	static void			get_harris_model_file_path(const char* lpstrImageNameL, const char* lpstrImageNameR, char* strPath);
	static void			get_sift_model_file_path(const char* lpstrImageNameL, const char* lpstrImageNameR, char* strPath);
	static void			get_temp_model_file_path(const char* lpstrImageNameL, const char* lpstrImageNameR, char* strPath);
#endif
private:
	class CStorage*	m_storage;
};

#endif // MATCHRESULT_H_LX_whu_2015_6_22