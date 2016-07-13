#include "stdafx.h"
#include <math.h>

#include "typedef.h"
#include "StdImage.h"
#include "StdDem.h"

#include "WhuUAV/Include/UAVXml.h"
#include "WhuUAV/Include/LxCamera.hpp"

#define _SRTM30

#ifdef _SRTM30
#include "DPGRID/WuGlbHei-v2.0.h"
#else
#include "DPGRID/WuGlbHei-v1.0.h"
#endif

#include "algorithms/Pretreatment.h"

#include "GlobalUtil_AtMatch.h"

#ifndef _NO_SIFT
#include "../SiftMatch/SiftMatch.h"
#endif

#ifndef _NO_HARRIS
#include "../HarrisMatch/HarrisMatch.h"
#endif
//////////////////////////////////////////////////////////////////////////////////////////////////////////
//
inline void getfilename(const char* strPath, char* name)
{
	char* pS = strrpath(strPath);	if (!pS) { strcpy(name, strPath); return; }
	strcpy(name, pS + 1);
	pS = strrchr(name, '.');	if (!pS) return;
	*pS = 0;
}
char CStdImage::m_strWsPath[512] = "";

#define PTLOC_ARRAY_PTR(p)		((CArray_PtLoc*)p)
#define PTNAME_ARRAY_PTR(p)		((CArray_PtName*)p)
#define SIFTDES_ARRAY_PTR(p)	((CArray_SiftDes*)p)
#define PTFEA_ARRAY_PTR(p)		((CArray_PtFea*)p)

CStdImage::CStdImage()
{
	m_ptLocation = NULL; 
	m_ptName = NULL;
	m_descriptors = NULL;
	m_ptFea = NULL;
	memset(m_nPyrmPtNum, 0, sizeof(int)* 3);
}
CStdImage::~CStdImage()
{
	Clear();
}

void	CStdImage::Clear()
{
	memset(m_strImageName, 0, sizeof(m_strImageName));
	memset(m_strImageID, 0, sizeof(m_strImageID));
	if (m_ptLocation) {
		delete PTLOC_ARRAY_PTR(m_ptLocation); m_ptLocation = NULL;
	}
	if (m_ptName) {
		delete PTNAME_ARRAY_PTR(m_ptName); m_ptName = NULL;
	}
	if (m_descriptors) {
		delete SIFTDES_ARRAY_PTR(m_descriptors); m_descriptors = NULL;
	}
	if (m_ptFea) {
		delete PTFEA_ARRAY_PTR(m_ptFea); m_ptFea = NULL;
	}
}

void	CStdImage::ResizePtBuffer(int sz)
{
// 	m_ptLocation.Reset(NULL, sz);
// 	m_ptName.Reset(NULL, sz);
// 	m_descriptors.Reset(NULL, sz);
	if (!m_ptLocation) m_ptLocation = new CArray_PtLoc;
	if (!m_ptName) m_ptName = new CArray_PtName;
	if (!m_descriptors) m_descriptors = new CArray_SiftDes;
	if (!m_ptFea) m_ptFea = new CArray_PtFea;

	PTLOC_ARRAY_PTR(m_ptLocation)->clear();
	PTLOC_ARRAY_PTR(m_ptLocation)->reserve(sz);
	PTNAME_ARRAY_PTR(m_ptName)->clear();
	PTNAME_ARRAY_PTR(m_ptName)->reserve(sz);
	SIFTDES_ARRAY_PTR(m_descriptors)->clear();
	SIFTDES_ARRAY_PTR(m_descriptors)->reserve(sz);
	PTFEA_ARRAY_PTR(m_ptFea)->clear();
	PTFEA_ARRAY_PTR(m_ptFea)->reserve(sz);
}

int CStdImage::GetPointNameSize(){
	return POINTID_BUFFER_SIZE;
}

inline void auto_image_id(const char* lpstrPath, char* name){
	const char* pN = strrpath(lpstrPath);	if (!pN) pN = lpstrPath; else pN = pN + 1;
	CompressStr(pN, name, 5);
	for (int i = 0; i < 5; i++){
		*name = '0' + (BYTE)*name % 10;
		name++;
	}
	return;
	static char strVal[100] = "";
	if (!strVal[0]){
		char config[256] = "";  readlink("/proc/self/exe", config, 256);
		char* pS = strrchr(config, '.');	if (!pS) pS = config + strlen(config);
		strcpy(pS, ".ini");
		::GetPrivateProfileString("Match", "auto_image_id", "10 11 12 14 17 18 19 20", strVal, 100, config);
	}
	
	char t[10] = "";	int sz = strlen(pN);
	char* pName = name;
	char* pT = t;	char* pV = strVal;
	for (; 1; pV++)
	{
		if (!*pV || *pV == '\r' || *pV == '\n') {
			if (strlen(t) >= 1) {
				int id = atoi(t);
				if (id >= 0 && id <= sz - 1) {
					*pName++ = *(pN + id);
				}
			}
			*pName = 0; break;
		}
		if (*pV == ' ' || *pV == '\t'){
			if (strlen(t)<1) continue;
			int id = atoi(t);	
			if (id >= 0 && id <= sz - 1) {
				*pName++ = *(pN + id);
			}			
			memset(t, 0, 10);
			pT = t;			
		}
		*pT++ = *pV;
	}
}

bool CStdImage::Open(const char* lpstrPath,bool bLoadImage /* = true */)
{
	Clear();
	if (bLoadImage && !CImageBase::Open(lpstrPath)) return false;
	getfilename(lpstrPath,m_strImageName);
	auto_image_id(lpstrPath, m_strImageID);
	return true;
}
bool CStdImage::Create(const char* lpstrPath, int nCols, int nRows, int nBands,int nBits)
{
	Clear();
	if (!CImageBase::Create(lpstrPath, nCols, nRows, nBands,nBits)) return false;
	getfilename(lpstrPath, m_strImageName);
	return true;
}

char*	CStdImage::get_pt_name(int idx) { return (char*)(PTNAME_ARRAY_PTR(m_ptName)->data() + idx); }
float*	CStdImage::GetPtLocation() { return (float*)(PTLOC_ARRAY_PTR(m_ptLocation)->data()); }
int		CStdImage::GetFeaPtNum()		{ return PTNAME_ARRAY_PTR(m_ptName)->size(); }
BYTE*	CStdImage::GetSiftDescriptors() { return (BYTE*)(SIFTDES_ARRAY_PTR(m_descriptors)->data()); }
float*	CStdImage::GetSiftFeature() { return (float*)(PTFEA_ARRAY_PTR(m_ptFea)->data()); }

#ifndef _NO_HARRIS
int		CStdImage::ExtractHarris2PtFile(const char* lpstrFilePath, int stCol, int stRow, int nCols, int nRows, int nHrsWin, int nHrsInterval)
{
	char strPath[512];	strcpy(strPath, lpstrFilePath);
	LogPrint(0, "[%s]\nHarrris Extracting...", GetImagePath());
	int	nPtID = 0;
	BYTE* pBuf = new BYTE[GlobalParam::g_extract_buffer_size];	if (!pBuf) { LogPrint(0, "Error:fail to malloc buffer!"); return 0; }	
	int nColNum, nRowNum;
	int* rc_split = Pretreatment::split_image(nColNum, nRowNum, stCol, stRow, nCols, nRows, 8, (int)(GlobalParam::g_extract_buffer_size), Pretreatment::RECT_FIRST);
	
	CHarris harris;	harris.SetParameters(nHrsWin, nHrsInterval);
	CArray_PtLoc	loc;
	CFeaPtSet	hrsSet;
	if (!hrsSet.create(strPath, nColNum, nRowNum, 1, nHrsWin, nHrsInterval)){
		goto  et_hrs_lp1;
	}

	int nBufferSpace[4] = { 2, 2,0,0 };
	int nRestRows;
	
	int *pSplitRc = rc_split;
	for (int i = 0; i < nColNum*nRowNum; i++, pSplitRc += 4){
		ReadGray8(pBuf, pSplitRc[0], pSplitRc[1], pSplitRc[2], pSplitRc[3]);
		printf("[(%d,%d) Cols = %d Rows = %d] harris extracting...", pSplitRc[0], pSplitRc[1], pSplitRc[2], pSplitRc[3]);
		if (harris.RunHarris(pBuf, pSplitRc[2], pSplitRc[3])) {
			int num = harris.GetFeatureNum();
			if (num < 1) continue;
			loc.resize(num);
			harris.GetFeatureVector((float*)loc.data());

			nPtID += hrsSet.write(nPtID, m_strImageID, pSplitRc[0], pSplitRc[1], pSplitRc[2], pSplitRc[3], -1, -1, 1,
				(float*)&loc[0], (float*)&loc[0] + 1, NULL, 0, (BYTE*)NULL, 0, nBufferSpace, num);
		}
	}

	hrsSet.set_point_sum(nPtID);
	hrsSet.close();

	LogPrint(0, ">>>Harris point total num = %d", nPtID);
#ifdef _DEBUG
	CvtHrs2Txt();
#endif
et_hrs_lp1:
	delete rc_split;
	delete pBuf;
	return nPtID;
}

int		CStdImage::ReadHarrisFile(const char* lpstrFilePath, int stCol, int stRow, int nCols, int nRows)
{
	char strPath[512];	strcpy(strPath, lpstrFilePath);

	CFeaPtSet	hrsSet;
	if (!hrsSet.load(strPath)){
		return 0;
	}

	int ptSum = hrsSet.get_point_sum();
	int ptNum_threshold = hrsSet.get_extract_interval();
	if (ptNum_threshold <= 0) ptNum_threshold = 100;	
	
	
	LogPrint(0, "[%s]\nread harris file [ptsum=%d]...", lpstrFilePath, ptSum);	
	if (ptSum < 3){
		LogPrint(ERR_ATMCH_FLAG, "too less feature point.");
		return 0;
	}	

	ResizePtBuffer(8196);

	int sz = hrsSet.read(stCol, stRow, nCols, nRows, -1, PTNAME_ARRAY_PTR(m_ptName), PTLOC_ARRAY_PTR(m_ptLocation), NULL, NULL);
	
	hrsSet.close();
	LogPrint(0, "ptnum in block is %d", sz);

	ptNum_threshold = int(GlobalParam::g_least_feature_ratio*nCols*nRows / (ptNum_threshold*ptNum_threshold));
	if (sz < ptNum_threshold){
		LogPrint(ERR_ATMCH_FLAG, "too less feature point (<%d).",ptNum_threshold);
		return 0;
	}

	return sz;
}
#endif

#ifndef _NO_SIFT
//ARRAY_CLASS(float, 128);
typedef std::vector<BYTE> CArray_VecType;

int		CStdImage::ExtractSift2PtFile(const char* lpstrFilePath,int stCol,int stRow,int nCols,int nRows,int interval )
{
	char strPath[512];	strcpy(strPath, lpstrFilePath);
	LogPrint(0, "[%s]\nSift Feature Extracting...", GetImagePath());
	int  ptSum = 0;
	BYTE* pBuf = new BYTE[GlobalParam::g_extract_buffer_size];	if (!pBuf) { LogPrint(0, "Error:fail to malloc buffer!"); return 0; }
	
	int nColNum, nRowNum;
	CFeaPtSet siftSet;
	CArray_VecType	descriptors;
	CArray_PtLoc	loc;
	CArray_PtFea	fea;

	CSift sift;
	if (!sift.InitEnvi(GlobalParam::g_sift_extract_gpu)) {
		goto et_st_lp1;
	}

	int* rc_split = Pretreatment::split_image(nColNum, nRowNum, stCol, stRow, nCols, nRows, 8, (int)(GlobalParam::g_extract_buffer_size), Pretreatment::RECT_FIRST);
	
	int blk_pt_num = -1;
	if (interval>0){
		blk_pt_num = int(1.0*nCols*nRows / (interval*interval) / (nColNum*nRowNum));
		if (blk_pt_num < 100) blk_pt_num = 100;
	}

	char c_i[10];	sprintf(c_i, "%d", blk_pt_num);
	LogPrint(0, "sift block max num = %s", c_i);
	char * argv[] = { "-v", "1","-tc", c_i };//"-loweo",
	int argc = sizeof(argv) / sizeof(char*);
	sift.ParseParam(argc, argv);

	if (!siftSet.create(strPath, nColNum, nRowNum,1, -1, interval)){
		goto et_st_lp2;
	}

	int *pSplitRc = rc_split ;
	int nBufferSpace[4] = { 2, 2, 2, 128 };

	for (int i = 0; i < nColNum*nRowNum; i++, pSplitRc += 4){//
		NORMALISE_IMAGE_SIDE(pSplitRc[2]);		NORMALISE_IMAGE_SIDE(pSplitRc[3]);
		printf( "[(%d,%d) Cols = %d Rows = %d] sift extracting...", pSplitRc[0], pSplitRc[1], pSplitRc[2], pSplitRc[3]);
		ReadGray8(pBuf, pSplitRc[0], pSplitRc[1], pSplitRc[2], pSplitRc[3]);
 //		SaveImageFile("F:/Data/shanxi/ws/sift/mux.tif", pBuf, pSplitRc[2], pSplitRc[3], 1);
		if (sift.RunSIFT(pBuf, pSplitRc[2], pSplitRc[3])) {
			int num = sift.GetFeatureNum();
			if (num < 1) continue;
			loc.resize(num);  fea.resize(num);  descriptors.resize(128 * num);
			sift.GetFeatureVector((float*)&loc[0], (float*)&fea[0], &descriptors[0]);
			
			ptSum += siftSet.write(ptSum, m_strImageID, pSplitRc[0], pSplitRc[1], pSplitRc[2], pSplitRc[3], -1, -1, 1,
				(float*)&loc[0], (float*)&loc[0] + 1, (float*)&fea[0], 2, &descriptors[0], 128, nBufferSpace, num);
		}
	}

	siftSet.set_point_sum(ptSum);
	siftSet.close();

#ifdef _DEBUG
 	CvtSift2Txt();
#endif

	LogPrint(0, ">>>Sift point total num = %d", ptSum);
et_st_lp2:
	delete rc_split;
et_st_lp1:
	delete pBuf;
	return ptSum;
}
int		CStdImage::ExtractPyrmSift2PtFile(const char* lpstrFilePath)
{
	CFeaPtSet		siftSet;
	CArray_VecType	descriptors;
	CArray_PtLoc	loc;
	CArray_PtFea	fea;
	CSift sift;

	char strPath[512];	strcpy(strPath, lpstrFilePath);
	LogPrint(0, "[%s]\nSift Feature Extracting to %s...", GetImagePath(), lpstrFilePath);

	int  ptSum = 0;
	BYTE* pBuf0 = new BYTE[GlobalParam::g_extract_buffer_size];	if (!pBuf0) { LogPrint(0, "Error:fail to malloc buffer!"); goto et_ps_lp1; }
	BYTE* pBuf1 = new BYTE[GlobalParam::g_extract_buffer_size / (PYRAMID_PIXELNUM*PYRAMID_PIXELNUM) + 8];	if (!pBuf1) { LogPrint(0, "Error:fail to malloc buffer!"); goto et_ps_lp2; }
	int nCols = GetCols();
	int nRows = GetRows();
	int nColNum, nRowNum;

	int* rc_split = Pretreatment::split_image(nColNum, nRowNum, 0, 0, nCols, nRows, 8, (int)(GlobalParam::g_extract_buffer_size), Pretreatment::RECT_FIRST);
	if (!rc_split){
		LogPrint(ERR_ATMCH_FLAG, "Error: fail to split image !");
		goto et_ps_lp3;
	}

	if (!sift.InitEnvi(GlobalParam::g_sift_extract_gpu)){
		goto et_ps_lp4;
	}

	if (!siftSet.create(strPath, nColNum, nRowNum, 3, -1, -1)){
		goto et_ps_lp5;
	}
	
	
	int *pSplitRc = rc_split;
	int nBufferSpace[4] = { 2, 2, 2, 128 };
	int scale_pyrm[3] = { 1, 3, 9 };
	for (int i = 0; i < nColNum*nRowNum; i++, pSplitRc += 4){
		LogPrint(0, "[(%d,%d) Cols = %d Rows = %d] sift extracting...", pSplitRc[0], pSplitRc[1], pSplitRc[2], pSplitRc[3]);
		BYTE* pBuf = pBuf0;	BYTE* pBufT = NULL;
		ReadGray8(pBuf, pSplitRc[0], pSplitRc[1], pSplitRc[2], pSplitRc[3]);
		int l = 0;	int nCols0 = pSplitRc[2], nRows0 = pSplitRc[3];	int nCols1 = nCols0 / PYRAMID_PIXELNUM, nRows1 = nRows0 / PYRAMID_PIXELNUM;
		while (1){
			if (sift.RunSIFT(pBuf, nCols0, nRows0)) {
				int num = sift.GetFeatureNum();			
				if (num<4) break;

				loc.resize(num);  fea.resize(num);  descriptors.resize(128 * num);
				sift.GetFeatureVector((float*)&loc[0], (float*)&fea[0], &descriptors[0]);
				
				ptSum += siftSet.write(ptSum, m_strImageID, pSplitRc[0], pSplitRc[1], pSplitRc[2], pSplitRc[3], -1, -1, scale_pyrm[l],
					(float*)&loc[0], (float*)&loc[0] + 1, (float*)&fea[0], 2, &descriptors[0], 128, nBufferSpace, num);
			}
			l++;
			if (l >= 3) break;
			float aop[6] = { PYRAMID_PIXELNUM, 0, 0, 0, PYRAMID_PIXELNUM, 0 };
			if (l == 1){ pBuf = pBuf1; pBufT = pBuf0; }
			else { pBuf = pBuf0; pBufT = pBuf1; }
			Pretreatment::AffineResample(pBufT, nCols0, nRows0,
				pBuf, nCols1,nRows1, aop);
			nCols0 = nCols1;	nRows0 = nRows1;
			nCols1 = nCols0 / PYRAMID_PIXELNUM, nRows1 = nRows0 / PYRAMID_PIXELNUM;
		}
	}

	siftSet.set_point_sum(ptSum);
	siftSet.close();

	LogPrint(0, ">>>PyrmSift point total num = %d", ptSum);
	
et_ps_lp5:
et_ps_lp4:
	delete rc_split;
et_ps_lp3:
	delete pBuf1;
et_ps_lp2:
	delete pBuf0;
et_ps_lp1:
	return ptSum;
}
/*
int CStdImage::ExtractPyrmGridSift2PtFile(const char* lpstrFilePath, int nGridCols, int nGridRows, int miSz)
{
	char strPath[512];	strcpy(strPath, lpstrFilePath);
	LogPrint(0, "[%s]\nPyrmGirdSift[%d*%d] Feature Extracting...", GetImagePath(), nGridCols, nGridRows);
	prepare_write(strPath);

	CPyrmGridPlan gridPlan;
	if (!gridPlan.plan(GetCols(), GetRows(), nGridCols, nGridRows, miSz)){
		LogPrint(ERR_ATMCH_FLAG, "too little area to extract.");
		return 0;
	}
	
	SiftGPU  *sift = new SiftGPU;
	if (!g_siftgpu_initial) {
		if (sift->CreateContextGL() != SiftGPU::SIFTGPU_FULL_SUPPORTED){
			delete sift;
			LogPrint(0, "Fail to initialize SIFTGPU!");
			return 0;
		}
		g_siftgpu_initial = true;
	}
	else
		sift->VerifyContextGL();

	int nPyrmSum = gridPlan.get_pyrm_sum();
	CPtPyrmGridFile siftFile;	PtPyrmGridHdrInfo hdrInfo;	memset(&hdrInfo, 0, sizeof(PtPyrmGridHdrInfo));
	if (!siftFile.create(strPath, CPtPyrmGridSection::create_object, nGridCols*nGridRows*nPyrmSum, NULL)){
		delete sift;
		LogPrint(ERR_FILE_WRITE, "Error:fail to write sift point file %s!", strPath); return 0;
	}
	BYTE* pBuf0 = new BYTE[gridPlan.get_pyrm_cols(0) * gridPlan.get_pyrm_rows(0)];	BYTE* pBuf1 = NULL;
	if (!pBuf0) { LogPrint(ERR_BUFFER_MALLOC, "Error:fail to malloc buffer!"); return 0; }
	if (nPyrmSum > 1) {
		pBuf1 = new BYTE[gridPlan.get_pyrm_cols(1) * gridPlan.get_pyrm_rows(1)];
		if (!pBuf1) { LogPrint(ERR_BUFFER_MALLOC, "Error:fail to malloc buffer!"); return 0; }
	}

	CArray_float	descriptors;
	CArray_SiftPoint keys;
	int c, r, l;
	int stCol, stRow;
	int  ptSum = 0;
	stRow = gridPlan.get_grid_stRow(0);
	for (r = 0; r < nGridRows; r++, stRow += gridPlan.get_blk_rows()){
		stCol = gridPlan.get_grid_stCol(0);
		for (c = 0; c < nGridCols; c++, stCol += gridPlan.get_blk_cols()){
			BYTE* pBuf = pBuf0;	BYTE* pBufT = NULL;
			ReadGray8(pBuf, stCol, stRow, gridPlan.get_pyrm_cols(0), gridPlan.get_pyrm_rows(0));
			l = 0;
			while (1){
				if (sift->RunSIFT(gridPlan.get_pyrm_cols(l), gridPlan.get_pyrm_rows(l), pBuf, GL_LUMINANCE, GL_UNSIGNED_BYTE)) {
					int num = sift->GetFeatureNum();
					LogPrint(0, "get %d sift feature from gpu...", num);
					if (num >= 1) {
						keys.Init(num);    descriptors.Init(128 * num);
						sift->GetFeatureVector((SiftGPU::SiftKeypoint*)&keys[0], &descriptors[0]);
					}					
					num = siftFile.WriteSift2File(ptSum, m_strImageID, 
						stCol, stRow, gridPlan.get_pyrm_cols(0), gridPlan.get_pyrm_rows(0), 
						c,r, l,//gridPlan.get_pyrm_cols(l), gridPlan.get_pyrm_rows(l)
						num, (float*)&keys[0], &descriptors[0]);
					ptSum += num;
				}
				l++;
				if (l>=nPyrmSum) break;
				double aop[6] = { 0, gridPlan.get_pyrm_cols(l - 1)*1.0 / gridPlan.get_pyrm_cols(l), 0, 0, 0, gridPlan.get_pyrm_rows(l - 1) * 1.0 / gridPlan.get_pyrm_rows(l) };
				if (l == 1){ pBuf = pBuf1; pBufT = pBuf0; }
				else { pBuf = pBuf0; pBufT = pBuf1; }
				CPretreatment::AffineResample(pBufT, gridPlan.get_pyrm_cols(l - 1), gridPlan.get_pyrm_rows(l - 1), 
					pBuf, gridPlan.get_pyrm_cols(l), gridPlan.get_pyrm_rows(l), aop);
			}
		}
	}
	hdrInfo.nPtSum = ptSum;
	hdrInfo.gridCols = nGridCols;
	hdrInfo.gridRows = nGridRows;
	hdrInfo.pyrmSum = nPyrmSum;
	hdrInfo.miSz = gridPlan.get_miSz();
	hdrInfo.pyrmRatio = gridPlan.get_pyrm_ratio();
	gridPlan.get_pyrm_cols(hdrInfo.pyrmCols);
	gridPlan.get_pyrm_rows(hdrInfo.pyrmRows);
	siftFile.set_hdr_info(&hdrInfo);
	siftFile.close();

	LogPrint(0, "\nTotal Sift Num = %d", ptSum);
	sift->DestroyContextGL();
	delete sift;
	delete pBuf0;
	if(pBuf1) delete pBuf1;
	return ptSum;
}
int CStdImage::ReadPyrmGridSiftFile(const char* lpstrFilePath,int gridCol,int gridRow,int nPyrmLvl)
{
char strPath[512];	strcpy(strPath, lpstrFilePath);

CPtPyrmGridFile siftFile;
if (!siftFile.load(strPath, CPtPyrmGridSection::create_object)){
LogPrint(ERR_FILE_READ, "fail to read sift file %s.", strPath);
return 0;
}
int ptSum = siftFile.get_pt_sum();

LogPrint(0, "[%s]\nread sift file [ptsum=%d]...", GetImagePath(), ptSum);
if (ptSum < 3){
LogPrint(ERR_ATMCH_FLAG, "too less feature point.");
return 0;
}

int idx = siftFile.get_section_idx(gridCol, gridRow, nPyrmLvl);

CPtPyrmGridSection* ptSection = NULL;
if (idx >= 0){
ptSection = (CPtPyrmGridSection*)siftFile.get_section_info(idx);
ptSum = ptSection->get_pt_num();
}
else ptSum = 0;
LogPrint(0, "grid[%d,%d,%d] point num = %d", gridCol, gridRow, nPyrmLvl, ptSum);
if (ptSum < 3) return 0;
ResizePtBuffer(8196);

float ratio_col = siftFile.get_pyrm_cols(0)*1.0f / siftFile.get_pyrm_cols(nPyrmLvl);
float ratio_row = siftFile.get_pyrm_rows(0)*1.0f / siftFile.get_pyrm_rows(nPyrmLvl);
int stCol = ptSection->get_stCol();
int stRow = ptSection->get_stRow();
siftFile.section_begin_in(idx);
PtName tName;	PtLoc pt;	SiftDes feature;
for (int j = 0; j < ptSection->get_pt_num(); j++){
siftFile.section_data_in(&tName, POINTID_BUFFER_SIZE);
siftFile.section_data_in(&pt, 2 * sizeof(float));
pt.x = pt.x*ratio_col + stCol;	pt.y = pt.y*ratio_row + stRow;
siftFile.section_data_in(&feature, sizeof(BYTE)* 128);
m_ptName.Append(tName);	m_ptLocation.Append(pt);	m_descriptors.Append(feature);
}
siftFile.section_end_in();

siftFile.close();
//	LogPrint(0, "ptnum in block is %d ", m_FeaPt.size());
return m_ptLocation.size();

}
bool CStdImage::ReadPyrmGridSiftFileHdr(const char* lpstrFilePath, int* nGridCols, int* nGridRows, int* miSz)
{
CPtPyrmGridFile siftFile;
if (!siftFile.load(lpstrFilePath, CPtPyrmGridSection::create_object)){
return false;
}
*nGridCols = siftFile.get_grid_cols();	*nGridRows = siftFile.get_grid_rows();
*miSz = siftFile.get_miSz();
return true;
}
*/
int CStdImage::ReadSiftFile(const char* lpstrFilePath, int stCol, int stRow, int nCols, int nRows)
{
	char strPath[512];	strcpy(strPath, lpstrFilePath);
	
	CFeaPtSet siftSet;
	if (!siftSet.load(strPath)){
		return 0;
	}
	int ptSum = siftSet.get_point_sum();
	int ptNum_threshold = siftSet.get_extract_interval();
	if (ptNum_threshold <= 0) ptNum_threshold = 100;

	LogPrint(0, "[%s]\nRead sift point [ptsum=%d]...", GetImagePath(), ptSum);	
	if (ptSum < 3){
		LogPrint(ERR_ATMCH_FLAG, "too less feature point.");
		return 0;
	}	
	ResizePtBuffer(8196);
	int sz = siftSet.read(stCol, stRow, nCols, nRows, 1, PTNAME_ARRAY_PTR(m_ptName), PTLOC_ARRAY_PTR(m_ptLocation), PTFEA_ARRAY_PTR(m_ptFea), SIFTDES_ARRAY_PTR(m_descriptors));
	siftSet.close();
	
	ptNum_threshold = int(GlobalParam::g_least_feature_ratio*nCols*nRows / (ptNum_threshold*ptNum_threshold));
	LogPrint(0, "ptnum in block is %d ", sz);
	if (sz < ptNum_threshold){
		LogPrint(ERR_ATMCH_FLAG, "too less feature point (<%d).", ptNum_threshold);
		return 0;
	}
	return sz;

}

/*
int CStdImage::ReadSiftFile(const char* lpstrFilePath, int stCol, int stRow, int nCols, int nRows, bool bSimpleTo64){
	int nPtNum = ReadSiftFile(lpstrFilePath, stCol, stRow, nCols, nRows);
	if (nPtNum < 1) return nPtNum;

	BYTE* pDes = (BYTE*)SIFTDES_ARRAY_PTR(m_descriptors)->data();
	int nDesNum = SIFTDES_ARRAY_PTR(m_descriptors)->size();
	for (int i = 0; i < nPtNum; i++){
		int v = 0;
		BYTE* pd = pDes;
		for (v = 0; v < 128; v += 8){
			pd[0] += pd[4]; pd[4] = 0;	pd++;
			pd[0] += pd[4]; pd[4] = 0;	pd++;
			pd[0] += pd[4]; pd[4] = 0;	pd++;
			pd[0] += pd[4]; pd[4] = 0;	pd += 5;
		}
		pd = pDes; float sq = 0;
		for (v = 0; v < 128; v++, pd++)	sq += (*pd)*(*pd);
		sq = 512.0f / sqrtf(sq);

		for (v = 0; v < 128; v++, pDes++)	*pDes = BYTE(*pDes*sq + 0.5);
	}

	return nPtNum;
}
*/
int CStdImage::ReadPyrmSiftFile(const char* lpstrFilePath, int stCol, int stRow, int nCols, int nRows, int pyrmLvl)
{
	char strPath[512];	strcpy(strPath, lpstrFilePath);

	CFeaPtSet siftSet;
	if (!siftSet.load(strPath)){
		return 0;
	}
	int ptSum = siftSet.get_point_sum();
	
	LogPrint(0, "[%s]\nRead sift point [ptsum=%d]...", GetImagePath(), ptSum);
	if (ptSum < 3){
		LogPrint(ERR_ATMCH_FLAG, "too less feature point.");
		return 0;
	}
	ResizePtBuffer(8196);
	int scale_pyrm[3] = { 1, 3, 9 };
	int sz = siftSet.read(stCol, stRow, nCols, nRows, scale_pyrm[pyrmLvl], PTNAME_ARRAY_PTR(m_ptName), PTLOC_ARRAY_PTR(m_ptLocation), PTFEA_ARRAY_PTR(m_ptFea), SIFTDES_ARRAY_PTR(m_descriptors));
	siftSet.close();

	LogPrint(0, "ptnum in block is %d ", sz);
	if (sz < 10){
		LogPrint(ERR_ATMCH_FLAG, "too less feature point (<%d).", 10);
		return 0;
	}
	return sz;

}

int CStdImage::ReadPyrmSiftFile(const char* lpstrFilePath, int stCol, int stRow, int nCols, int nRows)
{
	char strPath[512];	strcpy(strPath, lpstrFilePath);
	memset(m_nPyrmPtNum, 0, sizeof(int)* 3);

	CFeaPtSet siftSet;
	if (!siftSet.load(strPath)){
		return 0;
	}
	int ptSum = siftSet.get_point_sum();

	LogPrint(0, "[%s]\nRead PyrmSift point [ptsum=%d]...", GetImagePath(), ptSum);
	if (ptSum < 3){
		LogPrint(ERR_ATMCH_FLAG, "too less feature point.");
		return 0;
	}
	ResizePtBuffer(8196);
	int scale_pyrm[3] = { 1, 3, 9 };
	for (int i = 0; i < 3; i++){
		m_nPyrmPtNum[i] = siftSet.read(stCol, stRow, nCols, nRows, scale_pyrm[i], PTNAME_ARRAY_PTR(m_ptName), PTLOC_ARRAY_PTR(m_ptLocation), PTFEA_ARRAY_PTR(m_ptFea), SIFTDES_ARRAY_PTR(m_descriptors));
	}
	
	siftSet.close();

	int sz = GetFeaPtNum();
	LogPrint(0, "ptnum in block is %d ", sz);
	if (sz < 10){
		LogPrint(ERR_ATMCH_FLAG, "too less feature point (<%d).", 10);
		return 0;
	}
	return sz;
}

// int CStdImage::RemoveFeaPoint_PyrmSiftFile(const char* lpstrSrcFile, const char* lpstrDstFile, CStdImage* maskArray, BYTE* Vmin, BYTE* Vmax, int sz, int winSz)
// {
// 	CFeaPtSet srcFile;
// 	if (!srcFile.load(lpstrSrcFile)){
// 		LogPrint(ERR_FILE_READ, "fail to read sift file %s.", lpstrSrcFile);
// 		return 0;
// 	}
// 	CFeaPtSet dstFile;
// 	
// 	if (!dstFile.create(lpstrDstFile, CPtPyrmSection::create_object, srcFile.get_section_num(), NULL)){
// 		
// 		LogPrint(ERR_FILE_WRITE, "Error:fail to write sift point file %s!", lpstrDstFile); return 0;
// 	}
// 	
// 	return RemoveFeaturePoint(&srcFile, &dstFile, 128, maskArray, Vmin, Vmax, sz, winSz);
// }
// 
// int CStdImage::RemoveFeaPoint_PyrmSiftFile(CStdImage* maskArray, BYTE* Vmin, BYTE* Vmax, int sz, int winSz)
// {
// 	char strPath[512];	get_attach_file_path(GetImagePath(), AFT_SIFT_PYRM, strPath);
// 	char strTmp[512];	strcpy(strTmp, strPath);	strcpy(strrchr(strTmp, '.'), ".tmp");
// 	sz = RemoveFeaPoint_PyrmSiftFile(strPath, strTmp, maskArray, Vmin, Vmax, sz, winSz);
// 	remove(strPath);	rename(strTmp, strPath);
// 	return sz;
// }
#endif

#ifndef _NO_HARRIS
int	 CStdImage::CheckHarrisFile(const char* lpstrHarrisFile, int window /* = 0 */, int interval /* = 0 */)
{
	return CFeaPtSet::check_hdr(lpstrHarrisFile,-1,window,interval);
}
int CStdImage::ExtractHarris2PtFile(CStdImage* img, const char* lpstrHarrisFilePath, int window, int interval){
	int sz = CheckHarrisFile(lpstrHarrisFilePath, window, interval);
	if (sz<1){
		int sz = img->GetRows() > img->GetCols() ? img->GetCols() : img->GetRows();
		sz = sz / 10;	if (sz < 51) sz = 51;
		if (interval>0 && window > 0) return img->ExtractHarris2PtFile(lpstrHarrisFilePath, window, interval);
		return img->ExtractHarris2PtFile(lpstrHarrisFilePath, sz / 2 + 5, sz);
	}
	LogPrint(0, "----use exist harris points data----");
	return sz;
}
#endif
#ifndef _NO_SIFT
int CStdImage::CheckSiftFile(const char* lpstrSiftFile,int interval /* = 0 */)
{
	return CFeaPtSet::check_hdr(lpstrSiftFile, -1, -1, interval);
}
int CStdImage::CheckPyrmSiftFile(const char* lpstrSiftFile)
{
	return CFeaPtSet::check_hdr(lpstrSiftFile, -1, -1, -1);
}
// int CStdImage::CheckPyrmGridSiftFile(const char* lpstrSiftFile, int nGridCols, int nGridRows, int miSz)
// {
// 	CPtPyrmGridFile siftFile;
// 	PtPyrmGridHdrInfo info;	memset(&info, 0, sizeof(PtPyrmGridHdrInfo));
// 	info.gridCols = nGridCols;
// 	info.gridRows = nGridRows;
// 	info.miSz = miSz;
// 	if (!siftFile.check_file_hdr(lpstrSiftFile, &info)){
// 		return 0;
// 	}
// 	return siftFile.get_pt_sum();
// }

int CStdImage::ExtractSift2PtFile(CStdImage* img, const char* lpstrSiftFilePath, int interval){
	int sz = CheckSiftFile(lpstrSiftFilePath, interval);
	if (sz < 1){
		return img->ExtractSift2PtFile(lpstrSiftFilePath, interval);
	}
	LogPrint(0, "----use exist sift points data = %s----", lpstrSiftFilePath);
	return sz;
}
int CStdImage::ExtractPyrmSift2PtFile(CStdImage* img, const char* lpstrSiftFilePath){
	int sz = CheckPyrmSiftFile(lpstrSiftFilePath);
	if (sz < 1){
		return img->ExtractPyrmSift2PtFile(lpstrSiftFilePath);
	}
	LogPrint(0, "----use exist pyrm_sift points data----");
	return sz;
}
#endif
bool CStdImage::Wallis(const char* lpstrImagePath,const char* lpstrWallisPath)
{
	prepare_write(lpstrWallisPath);
	return Pretreatment::Wallisfilter(lpstrImagePath, lpstrWallisPath);
}

bool CStdImage::CheckWallisImage(const char* lpstrImagePath,const char* lpstrWallisImagePath)
{
	CImageBase img;
	if (img.Open(lpstrWallisImagePath) && Pretreatment::CheckWallisParConfig(lpstrImagePath)){
		return true;
	}
	return false;
}

bool CStdImage::Wallis(CStdImage* img, const char* lpstrWallisImagePath){
	if (CheckWallisImage(img->GetImagePath(), lpstrWallisImagePath) || Wallis(img->GetImagePath(), lpstrWallisImagePath)){
		return true;
	}
	LogPrint(0, "----use exist wallis image data : %$s----", lpstrWallisImagePath);
	return false;
}

bool	CStdImage::CreatePyrm(const char* lpstrPyrmPath, int stCol, int stRow, int nCols, int nRows, float zoomRate)
{
// 	char strPath[512];	strcpy(strPath, lpstrPyrmPath);
// 	LogPrint(0, "[%s]\nCreate pyramid image[(%d,%d)+(%d,%d),zoomRate = %.3f]...", GetImagePath(), stCol,  stRow,  nCols,  nRows, zoomRate);
// 	int nPyrmCols = int(nCols*zoomRate);	int nPyrmRows = int(nRows*zoomRate);
// 	LogPrint(0, "Pyramid image = %dx%d", nPyrmCols, nPyrmRows);
// 	if (nPyrmCols < 32 || nPyrmRows < 32 ) {
// 		LogPrint(0, "Pyramid image is too small.");
// 		return false;
// 	}
// 
// 	BYTE* pBuf = new BYTE[GlobalParam::g_extract_buffer_size];	if (!pBuf) { LogPrint(0, "Error:fail to malloc buffer!"); return false; }
// 
// 	CImageBase imgW;
// 	if (!imgW.Create(strPath, nPyrmCols, nPyrmRows, 1))	return false;
// 
// 	int nColNum, nRowNum;
// 	int* rc_split = Pretreatment::split_image(nColNum, nRowNum, 0, 0, nPyrmCols, nPyrmRows, 8, (int)(GlobalParam::g_extract_buffer_size), Pretreatment::RECT_FIRST);
// 
// 	int *pSplitRc = rc_split;
// 	int num = nColNum*nRowNum;
// 	int step = 1;
// 	int step_len = num / 5;	if (step_len <= 0) step_len = 1;
// 	printf("processing");
// 	for (int i = 0; i < num; i++, pSplitRc += 4){//		
// 		if (!((i + 1) % step_len)) {
// 			printf("...%d", step * 20);	step++;
// 		}
// 		ReadGray8(pBuf, int(pSplitRc[0]/zoomRate)+stCol, int(pSplitRc[1]/zoomRate)+stRow, pSplitRc[2], pSplitRc[3],zoomRate);
// 		
// 	}
// 	if (step < 6) printf("...100");	printf("...done.\n");
// 	LogPrint(0, ">>>Sift point total num = %d", ptSum);
// et_st_lp2:
// 	delete rc_split;
// et_st_lp1:
// 	delete pBuf;
 	return true;
}
#ifndef _NO_HARRIS
bool	CStdImage::CvtHrs2Txt(const char* lpstrHrsFile, const char* lpstrTxtPath, int nRows)
{
	CFeaPtSet file;
	if (!file.load(lpstrHrsFile)) return false;
	int num = file.get_point_sum();
	CArray_PtLoc		ptLocation;
	CArray_PtName		ptName;
	
	ptLocation.reserve(num);	ptName.reserve(num);	
	LogPrint(0, ">>>>Convert %s to %s", lpstrHrsFile, lpstrTxtPath);
	printf("read feature points[ptSum = %d]...\n", num);
	num = file.read(0, 0, 1000000000, 1000000000, 1, &ptName, &ptLocation, NULL, NULL);

	printf("save feature to text...");
	FILE* fp = fopen(lpstrTxtPath, "w");	if (!fp) return false;

	fprintf(fp, "%d\n", num);

	float* pLoc = (float*)&ptLocation[0];
	char*  pN = (char*)&ptName[0];
	for (int i = 0; i < num; i++){
		fprintf(fp, "%s\t%8.2f\t%8.2f\t%8.2f\n", pN, *pLoc, nRows - 1 - *(pLoc + 1), *(pLoc + 1));
		pN += POINTID_BUFFER_SIZE;
		pLoc += 2;
	}

	fclose(fp);
	printf("ok\n");
	return true;
}
#endif
#ifndef _NO_SIFT
bool	CStdImage::CvtSift2Txt(const char* lpstrSiftFile, const char* lpstrTxtPath, int nRows)
{
	CFeaPtSet file;
	if (!file.load(lpstrSiftFile)) return false;
	int num = file.get_point_sum();
	CArray_PtLoc		ptLocation;
	CArray_PtName		ptName;
	CArray_SiftDes		descriptors;
	CArray_PtFea		ptFea;
	ptLocation.reserve(num);	ptName.reserve(num);	descriptors.reserve(num);	ptFea.reserve(num);
	LogPrint(0, ">>>>Convert %s to %s", lpstrSiftFile, lpstrTxtPath);
	printf( "read feature points[ptSum = %d]...\n", num);
	num = file.read(0, 0, 1000000000, 1000000000, 1, &ptName, &ptLocation, &ptFea, &descriptors);

	printf("save feature to text...");
	FILE* fp = fopen(lpstrTxtPath, "w");	if (!fp) return false;

	fprintf(fp, "%d\n", num);

	float* pLoc = (float*)&ptLocation[0];
	float* pFea = (float*)&ptFea[0];
	char*  pN = (char*)&ptName[0];
	for (int i = 0; i < num; i++){
		fprintf(fp, "%s\t%8.2f\t%8.2f\t%5.2f\t%5.2f\n", pN, *pLoc, nRows - 1 - *(pLoc + 1), *pFea, *(pFea + 1));
		pN += POINTID_BUFFER_SIZE;
		pFea += 2;
		pLoc += 2;
	}

	fclose(fp);
	printf("ok\n");
	return true;
}
#endif
// 
// bool CStdImage::CvtPyrmSift2Txt(const char* lpstrSiftFile, const char* lpstrTxtPath, int nRows, int nPyrmLvl)
// {
// 	CPtPyrmFile siftFile;
// 	if (!siftFile.load(lpstrSiftFile, CPtPyrmSection::create_object)){
// 		LogPrint(ERR_FILE_READ, "fail to read sift file %s.", lpstrSiftFile);
// 		return 0;
// 	}
// 	return siftFile.save2text(lpstrTxtPath, nRows,nPyrmLvl, 128 * sizeof(BYTE));
// }

/////////////////////////////////////////////////////////////////////////////////////
//CTfw

class CTFW : public COrientation
{
public:
	CTFW(){

	}
	virtual ~CTFW(){

	}
	virtual const char* GetOriTag()const{
		return GEO_TAG_STD;
	}
	virtual const char* GetOriInfo()const{
		static char strVal[100];	
		sprintf(strVal, "%.10lf\t%.10lf\t%.10lf\t%.10lf\t%.10lf\t%.10lf", m_aop6[0], m_aop6[1], m_aop6[2], m_aop6[3], m_aop6[4], m_aop6[5]);
		return strVal;
	}
	virtual bool	 SetOriInfo(const char* lpstrInfo){
		double aop6[6];
		if (sscanf(lpstrInfo, "%lf%lf%lf%lf%lf%lf", aop6, aop6 + 1, aop6 + 2, aop6 + 3, aop6 + 4, aop6+5 ) < 6){
			return false;
		}
		UpdateThisGeoTransform(aop6);
		return true;
	}
	virtual void PhoZ2Grd(const double* x, const  double* y, const  double* gz, int nPtNum, double *X, double *Y)
	{
		while (nPtNum-->0)
		{
			ApplyGeoTransform(m_aop6, *x, *y, X, Y);
			x++;	y++;	
			X++;	Y++;	
		}
	}
	virtual void Grd2Pho(const double* X, const  double* Y, const double* Z, int nPtNum, double* x, double* y)
	{
		while (nPtNum-- > 0)
		{
			ApplyGeoTransform(m_aop6Reverse, *X, *Y, x, y);
			x++;	y++;
			X++;	Y++;	
		}
	}
	virtual void PhoZ2Grd(float x, float y, float gz, float *X, float *Y){
		double tx, ty;
		ApplyGeoTransform(m_aop6, x, y, &tx, &ty);
		*X = (float)tx;
		*Y = (float)ty;
	}
	virtual void Grd2Pho(float X, float Y, float Z, float* x, float* y){
		double tx, ty;
		ApplyGeoTransform(m_aop6Reverse, X, Y, &tx, &ty);
		*x = (float)tx;
		*y = (float)ty;
	}
	
	bool ReadTfw(const char* lpstrPathName)
	{
		FILE* fp = fopen(lpstrPathName, "rt");	if (!fp) return false;
		double adfGeoTransform[6];
		fscanf(fp, "%lf%lf%lf%lf%lf%lf",
			adfGeoTransform + 1, adfGeoTransform + 2,
			adfGeoTransform + 4, adfGeoTransform + 5,
			adfGeoTransform + 0, adfGeoTransform + 3);
		fclose(fp);
		UpdateThisGeoTransform(adfGeoTransform);
		return true;
	}
	void UpdateThisGeoTransform(double* padfTransform)
	{
		memcpy(m_aop6, padfTransform, sizeof(m_aop6));
		InvGeoTransform(m_aop6, m_aop6Reverse);
	}
private:
	double		m_aop6[6];
	double		m_aop6Reverse[6];
};

#ifdef ATMATCH_CODE
/////////////////////////////////////////////////////////////////////////////////////
//CFrameOrietation

class CFrameOrietation : public COrientation
{
public:
	CFrameOrietation(){

	}
	virtual ~CFrameOrietation(){

	}

	bool LoadOriXml(const char* lpstrPath,const char* lpstrImageName){
		if (!lpstrImageName) return false;
		static CUAVXml xml;
		if (!xml.IsLoaded() && xml.Load4Prj(lpstrPath) != ERR_NONE) return false;
		return xml.GetPosCam(lpstrImageName, &m_POSCam);
	}
	virtual const char*  GetOriTag()const{
		return GEO_TAG_FRM;
	}
	virtual const char*  GetOriInfo()const{
//		return m_POSCam.info_out();
		return NULL;
	}
	virtual bool	 SetOriInfo(const char* lpstrInfo){
		const char* pS = 0;
		return m_POSCam.info_in(lpstrInfo, pS);
	}

	virtual void PhoZ2Grd(const double* x, const  double* y, const  double* gz, int nPtNum, double *X, double *Y){
		for (int i = 0; i < nPtNum; i++){
			m_POSCam.ixyH_to_XY(*x, *y, *gz, X, Y);
			x++; y++;
			X++; Y++; gz++;
		}
	}
	virtual void Grd2Pho(const double* X, const  double* Y, const  double* Z, int nPtNum, double* x, double* y){

		for (int i = 0; i < nPtNum; i++){
			m_POSCam.XYH_to_ixy(*X, *Y, *Z, x, y);
			x++; y++;
			X++; Y++; Z++;
		}
	}
	virtual void PhoZ2Grd(float x, float y, float gz, float *X, float *Y)
	{
		double x0, y0;
		x0 = x;	y0 = y;
		m_POSCam.ixyH_to_XY(x0, y0, gz, &x0, &y0);
		*X = (float)x0;	*Y = (float)y0;
	}
	virtual void Grd2Pho(float X, float Y, float Z, float* x, float* y)
	{
		double x0, y0;
		x0 = X;	y0 = Y;
		m_POSCam.XYH_to_ixy(x0, y0, Z, &x0, &y0);
		*x = (float)x0;	*y = (float)y0;
	}
public:
	CPOSCam m_POSCam;
};

// bool CFrameImage::PhoZ2Grd(double px, double py, double gz, double *X, double *Y, double *Z)
// {
// 	px -= m_x0;	py -= m_y0;
// 	double R[9];	GetR(R);
// 	double f = m_f / m_pixSz;
// 	double x_ = R[0] * px + R[1] * py + R[2] * (-f);
// 	double y_ = R[3] * px + R[4] * py + R[5] * (-f);
// 	double f_ = R[6] * px + R[7] * py + R[8] * (-f);
// 
// 	if (fabs(f_) < 10e-6) return false;
// 
// 	*Z = gz;
// 	gz = gz - m_Zs;
// 
// 	double Xw = gz*x_ / f_;
// 	double Yw = gz*y_ / f_;
// 
// 	*X = Xw + m_Xs;
// 	*Y = Yw + m_Ys;
// 
// 	return true;
// }
// 
// bool CFrameImage::Grd2Pho(double X, double Y, double Z, double* px, double* py)
// {
// 	double R[9];	GetR(R);
// 	X = X - m_Xs;
// 	Y = Y - m_Ys;
// 	Z = Z - m_Zs;
// 	double X_ = R[0] * X + R[3] * Y + R[6] * Z;
// 	double Y_ = R[1] * X + R[4] * Y + R[7] * Z;
// 	double Z_ = R[2] * X + R[5] * Y + R[8] * Z;
// 	double f = m_f / m_pixSz;
// 
// 	if (fabs(Z_) < 10e-6) return false;
// 
// 	*px = -f*X_ / Z_ + m_x0;
// 	*py = -f*Y_ / Z_ + m_y0;
// 
// 	return true;
// }
#endif

//////////////////////////////////////////////////////////////////////////////////////////////////
//CRpcOrietation
class CRpcOrietation : public COrientation
{
public:
	CRpcOrietation(){

	}
	virtual ~CRpcOrietation(){

	}
	bool	LoadRpcFile(const char* lpstrPathName)
	{
		if (!m_rpc.Load4File(lpstrPathName)) return false;
		strcpy(m_strRpcPath, lpstrPathName);
		return true;
	}
	virtual const char*  GetOriTag()const{
		return GEO_TAG_RPC;
	}
	virtual const char*  GetOriInfo()const{

		return m_strRpcPath;
	}
	virtual bool	 SetOriInfo(const char* lpstrInfo){
		char strRpc[512];
		if (sscanf(lpstrInfo, "%s", strRpc) < 1) return false;
		if (!LoadRpcFile(strRpc)) return false;
		return true;
	}

	virtual void PhoZ2Grd(const double* x, const  double* y, const  double* gz, int nPtNum, double *X, double *Y){
		double Z;
		for (int i = 0; i < nPtNum; i++){
			m_rpc.RPCPhoZ2Grd(*x, *y, *gz, Y, X, &Z, true);
			x++; y++; gz++; X++; Y++; 
		}
	}
	virtual void Grd2Pho(const double* X, const  double* Y, const  double* Z, int nPtNum, double* x, double* y){
		for (int i = 0; i < nPtNum; i++){
			m_rpc.RPCGrd2Pho(*Y, *X, *Z, x, y, true);
			x++; y++; X++; Y++; Z++;
		}
	}

	virtual void PhoZ2Grd(float x, float y, float gz, float *X, float *Y)
	{
		double x0, y0, z0;
		x0 = x;	y0 = y;
		m_rpc.RPCPhoZ2Grd(x0, y0, gz, &y0, &x0, &z0, true);
		*X = (float)x0;	*Y = (float)y0;
	}
	virtual void Grd2Pho(float X, float Y, float Z, float* x, float* y)
	{
		double x0, y0;
		x0 = X;	y0 = Y;
		m_rpc.RPCGrd2Pho(y0, x0, Z, &x0, &y0, true);
		*x = (float)x0;	*y = (float)y0;
	}
	virtual bool CompensatePhoAop6(double aop6[6]){
		m_rpc.SetAop6(aop6);
		return m_rpc.Save2File(m_strRpcPath);
	}
private:
	CRpcBase m_rpc;
	char	 m_strRpcPath[512];
};

CGeoImage::CGeoImage()
{
	m_pOriInfo = NULL;
//	m_pGeoCvt = NULL;
}
CGeoImage::~CGeoImage()
{
	Clear();
//	if (m_pGeoCvt) { delete m_pGeoCvt; m_pGeoCvt = NULL; }
}

void CGeoImage::Clear()
{
	CStdImage::Clear();
	ResetOriInfo();
}

#ifdef ATMATCH_CODE
bool	CGeoImage::InitFrm(const char* lpstrFrameXml)
{
	if (!GetImageName()[0]) return false;
	CFrameOrietation* pOriInfo = new CFrameOrietation;
	if (!pOriInfo->LoadOriXml(lpstrFrameXml, GetImageName())) {
		delete pOriInfo;
		LogPrint(ERR_FILE_TYPE, "Error: fail to load frame info xml %s!", lpstrFrameXml);
		return false;
	}
	ResetOriInfo(pOriInfo);
	
	return true;
}
bool	CGeoImage::InitFrmImage(const char* lpstrImagePath, const char* lpstrXmlPath)
{
	if (!Open(lpstrImagePath) || !InitFrm(lpstrXmlPath)) return false;
	return true;
}

#endif

bool	CGeoImage::InitRpc(const char* lpstrRpcPath)
{
	CRpcOrietation* pOriInfo = new CRpcOrietation;
	if (!pOriInfo->LoadRpcFile(lpstrRpcPath)) {
		delete pOriInfo;
		LogPrint(ERR_FILE_TYPE, "Error: fail to load RPC file %s!", lpstrRpcPath);
		return false;
	}
	ResetOriInfo(pOriInfo);
	return true;
}

bool	CGeoImage::InitTFWImage(const char* lpstrImagePath)
{
	if (!Open(lpstrImagePath)) return false;
	CTFW* pOriInfo = new CTFW;
	double aop6[6];	GetGeoTransform(aop6);
	pOriInfo->UpdateThisGeoTransform(aop6);
	ResetOriInfo(pOriInfo);
	m_geoCvt.Import4WKT(GetProjectionRef());
	return true;
}

bool	CGeoImage::InitRpcImage(const char* lpstrImagePath, const char* lpstrRpcPath)
{
	if (!Open(lpstrImagePath) || !InitRpc(lpstrRpcPath)) return false;
	return true;
}

void	CGeoImage::GetOriInfo(char* oriInfo, bool bTag) const
{
	if (bTag)	sprintf(oriInfo, "%s\t%s", m_pOriInfo->GetOriTag(), m_pOriInfo->GetOriInfo());
	else sprintf(oriInfo, "%s", m_pOriInfo->GetOriInfo());
}

bool	CGeoImage::InitFromOriInfo(const char* lpstrImagePath, const char* lpstrOriInfo)
{
	if (!Open(lpstrImagePath)) return false;
	
	if (strstr(lpstrOriInfo, GEO_TAG_STD)){
		m_pOriInfo = new CTFW;
		lpstrOriInfo += strlen(GEO_TAG_STD) + 1;
		m_geoCvt.Import4WKT(GetProjectionRef());
	}
#ifdef ATMATCH_CODE
	else if (strstr(lpstrOriInfo, GEO_TAG_FRM)){
		m_pOriInfo = new CFrameOrietation;
		lpstrOriInfo += strlen(GEO_TAG_FRM) + 1;
	}
#endif
	else if (strstr(lpstrOriInfo, GEO_TAG_RPC)){
		m_pOriInfo = new CRpcOrietation;
		lpstrOriInfo += strlen(GEO_TAG_RPC) + 1;
	}
	
//	lpstrOriInfo = strchr(lpstrOriInfo, '\t') + 1;
	
	if (!m_pOriInfo->SetOriInfo(lpstrOriInfo)){
		return false;
	}

	return true;

}

void CGeoImage::operator =(const CGeoImage& object){
	CGeoImage::Clear();

	Open(object.GetImagePath());
	CopyGeoInformation(object);
	SetImageID(object.GetImageID());
	if (object.IsGeoInit()){
		char strOriInfo[512];	object.GetOriInfo(strOriInfo, true);
		const char* lpstrOriInfo = strOriInfo;

		if (strstr(lpstrOriInfo, GEO_TAG_STD)){
			m_pOriInfo = new CTFW;
			lpstrOriInfo += strlen(GEO_TAG_STD) + 1;
		}
#ifdef ATMATCH_CODE
		else if (strstr(lpstrOriInfo, GEO_TAG_FRM)){
			m_pOriInfo = new CFrameOrietation;
			lpstrOriInfo += strlen(GEO_TAG_FRM) + 1;
		}
#endif
		else if (strstr(lpstrOriInfo, GEO_TAG_RPC)){
			m_pOriInfo = new CRpcOrietation;
			lpstrOriInfo += strlen(GEO_TAG_RPC) + 1;
		}
		m_pOriInfo->SetOriInfo(lpstrOriInfo);
		m_geoCvt.Import4WKT(object.GetGeoSysWKT());
	}	
}

void CGeoImage::GetGeoRgn(float avrH, double x[4], double y[4])
{
	double ix[4], iy[4],z[4];
	memset(ix, 0, sizeof(double)* 4);
	memset(iy, 0, sizeof(double)* 4);
	ix[1] = GetCols()-1;	ix[3] = GetCols()-1;
	iy[2] = GetRows()-1;	iy[3] = GetRows()-1;
	for (int i = 0; i < 4; i++) z[i] = avrH;
	PhoZ2Grd(ix, iy, z, 4, x, y);
}

void CGeoImage::GetGeoRgn(float avrH, int stCol, int stRow, int edCol, int edRow, double* xmin, double* ymin, double* xmax, double* ymax){
	double c[4] = { stCol, stCol, edCol, edCol }, r[4] = { stRow, edRow, stRow, edRow };
	double h[4] = { avrH, avrH, avrH, avrH };
	double x[4], y[4];
	PhoZ2Grd(c, r, h, 4, x, y);
	MIN_VAL(*xmin, x, 4);	MIN_VAL(*ymin, y, 4);
	MAX_VAL(*xmax, x, 4);	MAX_VAL(*ymax, y, 4);
}

float	CGeoImage::GetGsd(bool bCvt2LBH )
{
	double col[2], row[2], X[2], Y[2], Z[2] = { 0, 0 };
	col[0] = 0;
	row[0] = 0;
	col[1] = GetCols();	row[1] = GetRows();
	PhoZ2Grd(col, row, Z, 2, X, Y);
	if (bCvt2LBH) m_geoCvt.Cvt2LBH(2, X, Y);
	float s = (float)sqrt(((X[0] - X[1])*(X[0] - X[1]) + (Y[0] - Y[1])*(Y[0] - Y[1])) / (col[1] * col[1] + row[1] * row[1]));
	return s;
}

void CGeoImage::LBH2Pho(const double* lon, const double* lat, const  double* h, int nPtNum, double* x, double* y){
	if (m_geoCvt.IsGeocentric())  return;
	if (m_geoCvt.IsProjected()){
		memcpy(x, lon, sizeof(double)*nPtNum);
		memcpy(y, lat, sizeof(double)*nPtNum);
		m_geoCvt.LBH2Prj(nPtNum, x, y);
		Grd2Pho(x, y, h, nPtNum, x, y);
	}
	else	Grd2Pho(lon, lat, h, nPtNum, x, y);
}
// #include "DPGRID/EplImg_BX.h"
// bool frame_epi_image(CGeoImage* imgL, CGeoImage* imgR, const char* lpstrLEpiImgPath, const char* lpstrREpiImgPath)
// {
// 	LogPrint(0, "----build epi image----");
// 	CEplImg_BX epi;
// 	CFrameOrietation* pFrameL = (CFrameOrietation*)imgL->m_pOriInfo;	CPOSCam* pCamL = &(pFrameL->m_POSCam);
// 	CFrameOrietation* pFrameR = (CFrameOrietation*)imgR->m_pOriInfo;	CPOSCam* pCamR = &(pFrameR->m_POSCam);
// 
// 	double phiL, omegaL, kappaL, phiR, omegaR, kappaR;
// 	double xyzL[3], xyzR[3];
// 	pCamL->GetCam_angle(&phiL, &omegaL, &kappaL);	pCamL->GetCam_T(xyzL);
// 	pCamR->GetCam_angle(&phiR, &omegaR, &kappaR);	pCamR->GetCam_T(xyzR);
// 	LogPrint(0, "set epi parameters...");
// 	if (!epi.SetPars(imgL->GetRows(),imgL->GetCols(),1,xyzL[0],xyzL[1],xyzL[2],phiL,omegaL,kappaL,
// 		pCamL->get_focus(),pCamL->get_x0(),pCamL->get_y0(),pCamL->get_pixelSize(),
// 		pCamL->get_k1(),pCamL->get_k2(),pCamL->get_p1(),pCamL->get_p2(),
// 		imgR->GetRows(), imgR->GetCols(), 1, xyzR[0], xyzR[1], xyzR[2], phiR, omegaR, kappaR,
// 		pCamR->get_focus(), pCamR->get_x0(), pCamR->get_y0(), pCamR->get_pixelSize(),
// 		pCamR->get_k1(), pCamR->get_k2(), pCamR->get_p1(), pCamR->get_p2()	)	){
// 		LogPrint(ERR_FLOW_FLAG, "some error in setting epi parameters!");
// 		return false;
// 	}
// 	
// 	int stColL, nColL, stRowL, nRowL, stColR, nColR, stRowR, nRowR;
// 	stColL = stRowL = 0;
// 	nColL = imgL->GetCols();	nRowL = imgL->GetRows();
// 	BYTE* pBufL = new BYTE[nColL*nRowL];	
// 	LogPrint(0, "read left image...");
// 	imgL->ReadBand8(pBufL, 0, stColL, stRowL, nColL, nRowL);
// 	stColR = stRowR = 0;
// 	nColR = imgR->GetCols();	nRowR = imgR->GetRows();
// 	BYTE* pBufR = new BYTE[nColR*nRowR];
// 	LogPrint(0, "read right image...");
// 	imgR->ReadBand8(pBufR, 0, stColR, stRowR, nColR, nRowR);
// 
// 	LogPrint(0, "calculate epi image data...");
// 	if (!epi.GetElpStereoImgs(pBufL,stRowL,stColL,nRowL,nColL,1,
// 		pBufR,stRowR,stColR,nRowR,nColR,1,false)){
// 		LogPrint(ERR_FLOW_FLAG, "fail to calculate!");
// 		return false;
// 	}
// 
// 	LogPrint(0, "save left epi image ...");
// 	if (!SaveImageFile(lpstrLEpiImgPath, epi.m_pLEplImgBits, epi.m_nLEplImgWidth, epi.m_nLEntireEplImgHeight, 1)){
// 		LogPrint(ERR_FILE_WRITE, "fail to save left image to %s", lpstrLEpiImgPath);
// 		return false;
// 	}
// 	LogPrint(0, "save right epi image ...");
// 	if (!SaveImageFile(lpstrREpiImgPath, epi.m_pREplImgBits, epi.m_nREplImgWidth, epi.m_nREntireEplImgHeight, 1)){
// 		LogPrint(ERR_FILE_WRITE, "fail to save left image to %s", lpstrREpiImgPath);
// 		return false;
// 	}
// 	epi.ReleaseData();
// 	delete[] pBufL;
// 	delete[] pBufR;
// 	LogPrint(0, "----epi end----");
// 	return true;
// }

/////////////////////////////////////////////////////////////////////////////////////////////////
//CDUImage
CDUImage::CDUImage()
{
	m_center_H = 0;
	m_dem = new CStdDem;
}

CDUImage::~CDUImage(){
	delete m_dem;
}

#ifdef _SRTM30
inline BOOL srtm_to_dem(double minLon, double minLat, double maxLon, double maxLat, int margin_pixel_num, LPCSTR lpstrDem)
{
	CWuGlbHei glbSrtm; WUHRDEMHDR hdr;
	int cols, rows; float *pZ = NULL;
	glbSrtm.GetGlbHeiHdr(&hdr);
	minLon -= hdr.dlon*margin_pixel_num;
	minLat -= hdr.dlat*margin_pixel_num;
	maxLon += hdr.dlon*margin_pixel_num;
	maxLat += hdr.dlat*margin_pixel_num;

	minLon = int(minLon / hdr.dlon)*hdr.dlon;
	minLat = int(minLat / hdr.dlat)*hdr.dlat;
	cols = int((maxLon - minLon) / hdr.dlon + 1);
	rows = int((maxLat - minLat) / hdr.dlat + 1);
	pZ = new float[cols*rows];
	memset(pZ, 0, sizeof(float)*cols*rows);
	glbSrtm.GetSRTM(minLon, minLat, cols, rows, pZ, NULL);
	struct SPDEMHDR
	{
		double  startX;
		double  startY;
		double  kapa;
		double  intervalX;
		double  intervalY;
		int       column;
		int       row;
	}demHdr; char str[512]; DWORD rw;
	demHdr.startX = minLon; demHdr.startY = minLat; demHdr.kapa = 0;
	demHdr.intervalX = hdr.dlon; demHdr.intervalY = hdr.dlat;
	demHdr.column = cols; demHdr.row = rows;
	sprintf(str, "DEMBIN VER:1.0 Supresoft Inc. Format: Tag(512Bytes),minX(double),minY(double),Kap(double),Dx(double),Dy(double),Cols(int),Rows(int),Z(float)... %.4lf %.4lf %.4lf %.12f %.12f %d %d",
		demHdr.startX, demHdr.startY, demHdr.kapa, demHdr.intervalX, demHdr.intervalY, demHdr.column, demHdr.row);
	HANDLE hFile = ::CreateFile(lpstrDem, GENERIC_READ | GENERIC_WRITE, FILE_SHARE_READ, NULL, CREATE_ALWAYS, 0, NULL);
	if (!hFile || hFile == INVALID_HANDLE_VALUE){ delete[] pZ; return FALSE; }
	::WriteFile(hFile, str, 512, &rw, NULL);
	::WriteFile(hFile, &demHdr, sizeof(demHdr), &rw, NULL);
	::WriteFile(hFile, pZ, cols*rows*sizeof(float), &rw, NULL);
	::CloseHandle(hFile);
	delete[] pZ;

	char strCmd[2048] = "";	readlink("/proc/self/exe", strCmd, 256);
	char* pS = strrchr(strCmd, '.');	if (!pS) pS = strCmd + strlen(strCmd); strcpy(pS, ".ini");
	pS = strrpath(strCmd);	
	::GetPrivateProfileString("Exec", "EgmCvt", "EgmCvt.exe", pS ? pS + 1 : strCmd, 128, strCmd);
	sprintf(strCmd + strlen(strCmd), " -dem %s -Norm2Geo -outpath %s", lpstrDem, lpstrDem);

	return TRUE;
}
#else
inline BOOL srtm_to_dem(double minLon, double minLat, double maxLon, double maxLat, int margin_pixel_num, LPCSTR lpstrDem)
{
	return GetSRTM(minLon- margin_pixel_num * SRTM_GSD,minLat- margin_pixel_num * SRTM_GSD,maxLon+ margin_pixel_num * SRTM_GSD,maxLat+ margin_pixel_num * SRTM_GSD,lpstrDem);
}
#endif

bool CDUImage::ExtractSRTM(CGeoImage* pImg,const char* lpstrSTRMPath)
{
	prepare_write(lpstrSTRMPath);
	double x[4], y[4];	double xmin, xmax, ymin, ymax;
	pImg->GetGeoRgn_LBH(0, x, y);
	MIN_VAL(xmin, x, 4);	MIN_VAL(ymin, y, 4);
	MAX_VAL(xmax, x, 4);	MAX_VAL(ymax, y, 4);
	LogPrint(0, "GeoRange[%.3lf,%.3lf]=>[%.3lf,%.3lf]", xmin, ymin, xmax, ymax);
	LogPrint(0, "Cut SRTM file to %s", lpstrSTRMPath);

	return srtm_to_dem(xmin, ymin, xmax, ymax, 10, lpstrSTRMPath) ? true : false;
}

bool CDUImage::CheckSRTMFile(const char* lpstrSRTMPath)
{
	if (IsExist(lpstrSRTMPath)) return true;
	return false;
}
bool	CDUImage::LoadSRTM(const char* lpstrDemFile){
	if (!m_dem->Load4File(lpstrDemFile)) return false;
	m_center_H = m_dem->GetCenterH();
	return true;
}
bool CDUImage::ixy_to_LBH(double ix, double iy, double* x, double* y, double* z)
{
#define HEIGHT_RMS 1
	double h;
	*z = m_center_H;
	int it = 15;
	while (it > 0)
	{
		PhoZ2LB(&ix, &iy, z, 1, x, y);
		h = m_dem->GetDemZValue4LB(*x, *y);

		if (h == NOVALUE) {
			*z = m_center_H; break;
		}
		if (fabs(h - *z) < HEIGHT_RMS) { break; }
		*z = h;
		it--;
	}
	return it>0 ? true : false;
}
