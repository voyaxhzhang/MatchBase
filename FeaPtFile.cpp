
#ifndef PQ_DB

#include "SELF.h"

#include "StdImage.h"
#include "GlobalUtil_AtMatch.h"

/************************************************************************/
/*1)reorganize feature file io class                                    */
/*                                                                      */
/************************************************************************/
//////////////////////////////////////////////////////////////////////////////////////////////////////////
//CPtFile
#include <vector>
using namespace std;

class CPtSectionBase :public CSESection
{
public:
	CPtSectionBase(){

	}
	virtual size_t hdr_size()
	{
		return CSESection::hdr_size() + m_nBufferSz;
	}
	virtual void set_hdr_info(void* info)
	{
		memcpy(m_pSectionInfo, info, m_nBufferSz);
	}
	virtual int hdr_in(HANDLE hFile)
	{
		int ret = CSESection::hdr_in(hFile);
		if (ret != ERR_NONE) return ret;
		data_in(hFile, m_pSectionInfo, m_nBufferSz);
		return ERR_NONE;
	}
	virtual int hdr_out(HANDLE hFile)
	{
		int ret = CSESection::hdr_out(hFile);
		if (ret != ERR_NONE) return ret;
		data_out(hFile, m_pSectionInfo, m_nBufferSz);
		return ERR_NONE;
	}
	char*	get_section_info_ptr() { return m_pSectionInfo; }
protected:
	char*	m_pSectionInfo;
	int		m_nBufferSz;
};

#ifndef _PTBLOCKINFO
#define _PTBLOCKINFO
typedef struct tagPtBlockInfo{
	int			nPtNum;
	int			stCol;
	int			stRow;
	int			nCols;
	int			nRows;
	int			gridCol;
	int			gridRow;
	int			scale;
}PtBlockInfo;
#endif

class CPtSection : public CPtSectionBase
{
public:
	CPtSection(){
		m_pSectionInfo = NULL;
		m_nBufferSz = 0;
		init();
	}
	virtual ~CPtSection(){
		if (m_pSectionInfo) free(m_pSectionInfo);
	}
	void init(){
		if (m_pSectionInfo) free(m_pSectionInfo);
		m_pSectionInfo = (char*)malloc(sizeof(PtBlockInfo));
		m_nBufferSz = sizeof(PtBlockInfo);
	}
	static CSESection*		create_object(){
		return new CPtSection;
	}
public:
	int		get_pt_num() { return *((int*)m_pSectionInfo + 0); }
	int		get_stCol()	{ return *((int*)m_pSectionInfo + 1); }
	int		get_stRow()	{ return *((int*)m_pSectionInfo + 2); }
	int		get_nCols()	{ return *((int*)m_pSectionInfo + 3); }
	int		get_nRows()	{ return *((int*)m_pSectionInfo + 4); }	
	int		get_grid_col() { return *((int*)m_pSectionInfo + 5); }
	int		get_grid_row() { return *((int*)m_pSectionInfo + 6); }
	int		get_scale() { return *((int*)m_pSectionInfo + 7); }
};

// class CPtSection : public CSESection
// {
// public:
// 	CPtSection()
// 	{
// 		memset(&m_info, 0, sizeof(m_info));
// 	}
// 	virtual size_t hdr_size()
// 	{
// 		return CSESection::hdr_size() + sizeof(PtBlockInfo);
// 	}
// 	virtual void set_hdr_info(void* info)
// 	{
// 		memcpy(&m_info, info, sizeof(PtBlockInfo));
// 	}
// 	virtual int hdr_in(HANDLE hFile)
// 	{
// 		int ret = CSESection::hdr_in(hFile);
// 		if (ret != ERR_NONE) return ret;
// 		data_in(hFile, &m_info, sizeof(PtBlockInfo));
// 		return ERR_NONE;
// 	}
// 	virtual int hdr_out(HANDLE hFile)
// 	{
// 		int ret = CSESection::hdr_out(hFile);
// 		if (ret != ERR_NONE) return ret;
// 		data_out(hFile, &m_info, sizeof(PtBlockInfo));
// 		return ERR_NONE;
// 	}
// 	static CSESection*		create_object(){
// 		return new CPtSection;
// 	}
// public:
// 	int		get_pt_num() { return m_info.nPtNum; }
// 	int		get_stCol()	{ return m_info.stCol; }
// 	int		get_stRow()	{ return m_info.stRow; }
// 	int		get_nCols()	{ return m_info.nCols; }
// 	int		get_nRows()	{ return m_info.nRows; }
// protected:
// 	PtBlockInfo		m_info;
// };

class CPtFileBase : public CSELF{
public:
	CPtFileBase(){
		m_pHdrInfo = NULL;
		m_nBufferSz = 0;
	}
	virtual ~CPtFileBase(){
		if (m_pHdrInfo) free(m_pHdrInfo);
	}
	int		skip_data(long sz){
		return move_file_pointer(sz);
	}
	virtual void close(){
		if (is_created()) CSELF::hdr_out();
		CSELF::close();
		memset(m_pHdrInfo, 0, m_nBufferSz);
	}
	virtual bool check_file_hdr(const char* lpstrFilePath, void* check_info)
	{
		if (!CSELF::check_file_hdr(lpstrFilePath, NULL)) return false;

		int* pInfo = (int*)m_pHdrInfo;
		int* pInfoC = (int*)check_info;
		if (*pInfo < 3) return false;
		pInfo++;	pInfoC++;
		for (int i = 1; i< int(m_nBufferSz / sizeof(int)); i++){
			if (*pInfoC>0 && *pInfo >0 && *pInfo != *pInfoC) return false;
			pInfo++;	pInfoC++;
		}

		return true;
	}
	int write(unsigned int idx, const char* lpstrImageID, int stCol, int stRow, int nCols, int nRows, int gridCol, int gridRow,int scale,
		float* x, float* y, float* fea, int feaDecriDimension, void* descrip, int decriDimension, int nBufferSpace[4], int ptSum,void (*pFunCvtDescrp)(void* from,BYTE*& to,int len))
	{
		PtBlockInfo sectionInfo;	char name[POINTID_BUFFER_SIZE];		float x_last = -99, y_last = -99;
		
		sectionInfo.stCol = stCol;	sectionInfo.stRow = stRow;	sectionInfo.nCols = nCols;	sectionInfo.nRows = nRows;
		sectionInfo.gridCol = gridCol;	sectionInfo.gridRow = gridRow;	sectionInfo.scale = scale;

		int cnt = 0;
		section_begin_out();	BYTE des[128];	BYTE* pDes = des;	char* pDes_from = (char*)descrip;
		bool bFea = fea&&feaDecriDimension;	bool bDecri = descrip&&decriDimension;
		for (int i = 0; i < ptSum; i++, x += nBufferSpace[0], y += nBufferSpace[1])
		{
			if (!bFea && fabs(*x - x_last) < 0.1f && fabs(*y - y_last) < 0.1f){
				if (bFea)	fea += nBufferSpace[2];
				if (bDecri)	pDes_from += nBufferSpace[3];
				continue;
			}
			x_last = *x;	y_last = *y;
			sprintf(name, "%010u", idx);
			*x = stCol + *x*scale;	*y = stRow + *y*scale;
			section_data_out(name, POINTID_BUFFER_SIZE);
			section_data_out(x, sizeof(float));		
			section_data_out(y, sizeof(float));		
			if (bFea) {
				section_data_out(fea, feaDecriDimension*sizeof(float));	fea += nBufferSpace[2];
			}
			if (bDecri){
//				CvtDescriptorsf2uc(descrip, des, decriDimension);
				pFunCvtDescrp(pDes_from,pDes, decriDimension);
				section_data_out(pDes, decriDimension*sizeof(BYTE));	pDes_from += nBufferSpace[3];
			}
			idx++;	cnt++;
				
		}
		sectionInfo.nPtNum = cnt;
		
		section_end_out(&sectionInfo);
		return cnt;
	}
	int read_section(int section_idx, int stCol, int stRow, int nCols, int nRows, CArray_PtName* name_array, CArray_PtLoc* xy_array, CArray_PtFea* fea_array, CArray_SiftDes* des_array)
	{
		int block_select[4] = { stCol, stRow, stCol + nCols, stRow + nRows };
		section_begin_in(section_idx);
		CPtSection* ptSection = (CPtSection*)get_section_info(section_idx);
		PtLoc pt;	PtName tName;	
		PtFea fea;	 SiftDes des;
		int cnt = 0;
		for (int j = 0; j < ptSection->get_pt_num(); j++){
			section_data_in(&tName, POINTID_BUFFER_SIZE);
			section_data_in(&pt, 2 * sizeof(float));
			if (pt.x < block_select[0] || pt.x >= block_select[2] || pt.y < block_select[1] || pt.y >= block_select[3]) {
				if (fea_array) skip_data(sizeof(float)* 2);
				if (des_array) skip_data(sizeof(BYTE)* 128);
				continue;
			}
			if (fea_array) {
				section_data_in(&fea, sizeof(float)* 2); fea_array->push_back(fea);
			}
			if (des_array){
				section_data_in(&des, sizeof(BYTE)* 128); des_array->push_back(des);
			}
			name_array->push_back(tName);	xy_array->push_back(pt);	
			cnt++;
		}
		section_end_in();
		return cnt;
	}
	
	virtual void set_hdr_info(void* info){
		memcpy(m_pHdrInfo, info, m_nBufferSz);
	}
protected:
	virtual SELF_Off get_section_hdr_start(){
		return CSELF::get_section_hdr_start() + m_nBufferSz;
	}
	virtual int		hdr_in(HANDLE hFile){
		int ret = CSELF::hdr_in(hFile);
		if (ret != ERR_NONE) return ret;
		CSESection::data_in(hFile, m_pHdrInfo, m_nBufferSz);
		return ERR_NONE;
	}
	virtual int		hdr_out(HANDLE hFile){
		int ret = CSELF::hdr_out(hFile);
		if (ret != ERR_NONE) return ret;
		CSESection::data_out(hFile, m_pHdrInfo, m_nBufferSz);
		return ERR_NONE;
	}
	friend int RemoveFeaturePoint(CPtFileBase* srcFile, CPtFileBase* dstFile, int featureVectorSz,
		CStdImage* maskArray, BYTE* Vmin, BYTE* Vmax, int sz, int winSz){
		int section_num = srcFile->get_section_num();
		int half_win = winSz / 2;
		BYTE* pBuf = NULL;	int nBufferSz = 0;
		vector<PtLoc> loc;	vector<BYTE> bValid;
		int total_num = 0;
		LogPrint(0, "there will be %d block to be processed...", section_num);
		//		vector<BYTE> fea;	vector<char> name;
		for (int i = 0; i < section_num; i++){
			LogPrint(0, "Block[%d]", i);
			CPtSection* pSrcSection = (CPtSection*)(srcFile->get_section_info(i));
			if (pSrcSection->get_pt_num() < 1 || pSrcSection->get_section_sz() < POINTID_BUFFER_SIZE) continue;
			loc.resize(pSrcSection->get_pt_num());		//name.resize(POINTID_SIZE*pSrcSection->get_pt_num());	
			bValid.resize(pSrcSection->get_pt_num());
			LogPrint(0, "point num = %d", pSrcSection->get_pt_num());
			int j;
			float* pFea = (float*)&loc[0];	BYTE* pValid = &bValid[0];
			//			char* pN = &name[0];
			srcFile->section_begin_in(i);
			for (j = 0; j < pSrcSection->get_pt_num(); j++){
				srcFile->skip_data(POINTID_BUFFER_SIZE);
				srcFile->section_data_in(pFea, 2 * sizeof(float));	pFea += 2;
				srcFile->skip_data(featureVectorSz);
				*pValid = 1;	pValid++;
			}
			srcFile->section_end_in();

			int nCols = pSrcSection->get_nCols();
			int nRows = pSrcSection->get_nRows();
			if (nCols*nRows>sz){
				sz = nCols*nRows;
				if (pBuf) delete pBuf;
				pBuf = new BYTE[sz];
			}
			for (j = 0; j < sz; j++){
				maskArray[j].ReadBand8(pBuf, 0, pSrcSection->get_stCol(), pSrcSection->get_stRow(), pSrcSection->get_nCols(), pSrcSection->get_nRows());
				pValid = &bValid[0];	pFea = (float*)&loc[0];
				for (int k = 0; k < pSrcSection->get_pt_num(); k++){
					if (!*pValid) goto rfp_lp;
					int stCol = int(pFea[0]) - half_win;	int stRow = int(pFea[1]) - half_win;
					int edCol = int(pFea[0]) + half_win;	int edRow = int(pFea[1]) + half_win;
					if (stCol < 0) stCol = 0;	if (stRow < 0) stRow = 0;
					if (edCol>nCols - 1) edCol = nCols - 1;
					if (edRow>nRows - 1) edRow = nRows - 1;
					for (; stRow <= edRow; stRow++){
						for (int c = stCol; c <= edCol; c++){
							int val = pBuf[stRow*nCols + c];
							if (val >= Vmin[j] && val <= Vmax[j]) {
								*pValid = 0; goto rfp_lp;
							}
						}
					}
				rfp_lp:
					pFea += 2, pValid++;
				}
			}
			srcFile->section_begin_in(i);
			dstFile->section_begin_out();
			int pt_section = 0;	int dataSz = POINTID_BUFFER_SIZE + 2 * sizeof(float)+featureVectorSz;
			BYTE* temp_data = new BYTE[dataSz];
			pValid = &bValid[0];
			for (j = 0; j < pSrcSection->get_pt_num(); j++, pValid++){
				srcFile->section_data_in(temp_data, dataSz);
				if (!*pValid) continue;
				dstFile->section_data_out(temp_data, dataSz);
				pt_section++;
			}
			delete temp_data;
			LogPrint(0, "rest point num = %d", pt_section);
			total_num += pt_section;
			srcFile->section_end_in();
			*((int*)(pSrcSection->get_section_info_ptr())) = pt_section;
			dstFile->section_end_out(pSrcSection->get_section_info_ptr());
		}
		if (pBuf) delete pBuf;
		*((int*)(srcFile->m_pHdrInfo)) = total_num;
		dstFile->set_hdr_info(srcFile->m_pHdrInfo);
		dstFile->close();

		LogPrint(0, "\nTotal Sift Num = %d", total_num);
		return 1;
	}
public:
	int				get_pt_sum()	{ return *((int*)m_pHdrInfo); }

protected:
	char*	m_pHdrInfo;
	int		m_nBufferSz;
};

typedef struct tagPtHdrInfo{
	int			nPtSum;
	int			nWinSz;
	int			interval;
	int			gridCols;
	int			gridRows;
}PtHdrInfo;



class CPtFile : public CPtFileBase
{
public:
	CPtFile(){
		init();
	}
	void init(){
		if (m_pHdrInfo) free(m_pHdrInfo);
		m_pHdrInfo = (char*)malloc(sizeof(PtHdrInfo));
		m_nBufferSz = sizeof(PtHdrInfo);
	}
	int read(int stCol, int stRow, int nCols, int nRows,int scale, CArray_PtName* name_array, CArray_PtLoc* xy_array, CArray_PtFea* fea_array, CArray_SiftDes* des_array){
		int blockNum = get_section_num();
		int block_select[4] = { stCol, stRow, stCol + nCols, stRow + nRows };

		int cnt = 0;
		for (int i = 0; i < blockNum; i++){
			CPtSection* ptSection = (CPtSection*)get_section_info(i);
			if (ptSection->get_pt_num() < 1 || ptSection->get_section_sz() < POINTID_BUFFER_SIZE || (scale>0 && ptSection->get_scale() != scale)) continue;
			int block[4] = { ptSection->get_stCol(), ptSection->get_stRow(), ptSection->get_nCols(), ptSection->get_nRows() };
			block[2] += block[0];	block[3] += block[1];
			if (!IS_RECT_INTERSECT(block, block_select)) continue;
			cnt += read_section(i, stCol, stRow, nCols, nRows, name_array, xy_array, fea_array, des_array);
		}
		return cnt;
	}
	
	bool		SaveSift2Txt(const char* lpstrFilePath, int nRows, size_t skip_space){
		FILE* fp = fopen(lpstrFilePath, "w");	if (!fp) return false;
		int sz = 0;
		fprintf(fp, "%d\n", get_pt_sum());
		char strID[POINTID_BUFFER_SIZE];	float xy[4];
		for (int i = 0; i < get_section_num(); i++){
			CPtSection* ptSection = (CPtSection*)get_section_info(i);
			if (ptSection->get_pt_num() < 1 || ptSection->get_section_sz() < POINTID_BUFFER_SIZE) continue;
			section_begin_in(i);
			for (int j = 0; j < ptSection->get_pt_num(); j++){
				section_data_in(strID, POINTID_BUFFER_SIZE);
				section_data_in(xy, sizeof(float)* 4);
				xy[0] += ptSection->get_stCol();	xy[1] += ptSection->get_stRow();
				fprintf(fp, "%s\t%8.2f\t%8.2f\t%8.2f\t%8f\t%8f\n", strID, xy[0], nRows - 1 - xy[1], xy[1], xy[2], xy[3]);
				move_file_pointer(skip_space);
			}
			section_end_in();
		}

		fclose(fp);

		return true;
	}
	bool		save2text(const char* lpstrFilePath, int nRows, size_t skip_space, bool skip_out){
		FILE* fp = fopen(lpstrFilePath, "w");	if (!fp) return false;
		FILE* fs = NULL;
		float* skip_data = NULL;
		int sz = 0;
		if (skip_out) {
			char strPath[512];	strcpy(strPath, lpstrFilePath);	strcat(strrchr(strPath, '.'), ".fea.txt");
			sz = skip_space / sizeof(float);
			skip_data = new float[sz + 1];
			fs = fopen(strPath, "w");
		}
		fprintf(fp, "%d\n", get_pt_sum());
		char strID[POINTID_BUFFER_SIZE];	float xy[2];
		for (int i = 0; i < get_section_num(); i++){
			CPtSection* ptSection = (CPtSection*)get_section_info(i);
			if (ptSection->get_pt_num() < 1 || ptSection->get_section_sz() < POINTID_BUFFER_SIZE) continue;
			section_begin_in(i);
			for (int j = 0; j < ptSection->get_pt_num(); j++){
				section_data_in(strID, POINTID_BUFFER_SIZE);
				section_data_in(xy, sizeof(float)* 2);
				xy[0] += ptSection->get_stCol();	xy[1] += ptSection->get_stRow();
				fprintf(fp, "%s\t%8.2f\t%8.2f\t%8.2f\n", strID, xy[0], nRows - 1 - xy[1], xy[1]);
				if (fs)
				{
					section_data_in(skip_data, skip_space);
					for (int k = 0; k < sz; k++) fprintf(fs, "%.2f\t", skip_data[k]);
					fputc('\n', fs);
				}
				else move_file_pointer(skip_space);
			}
			section_end_in();
		}

		fclose(fp);
		if (fs){
			delete skip_data;
			fclose(fs);
		}
		return true;
	}
	int				get_win_sz()	{ return *((int*)m_pHdrInfo + 1); }
	int				get_interval()	{ return *((int*)m_pHdrInfo + 2); }
	int				get_grid_cols() { return *((int*)m_pHdrInfo + 3); }
	int				get_grid_rows() { return *((int*)m_pHdrInfo + 4); }
};

// class CPtFile : public CSELF
// {
// public:
// 	int		skip_data(long sz){
// 		return move_file_pointer(sz);
// 	}
// 	virtual void close(){
// 		if (is_created()) CSELF::hdr_out();
// 		CSELF::close();
// 		memset(&m_info, 0, sizeof(PtHdrInfo));
// 	}
// 	virtual bool check_file_hdr(const char* lpstrFilePath, void* check_info)
// 	{
// 		if (!CSELF::check_file_hdr(lpstrFilePath,NULL)) return false;
// 
// 		PtHdrInfo* pInfo = (PtHdrInfo*)check_info;
// 		if (m_info.nPtSum < 3) return false;
// 		if (pInfo->interval > 0 && m_info.interval>0 && pInfo->interval != m_info.interval ) { return false; }
// 		if (pInfo->nWinSz > 0 && m_info.nWinSz>0 && pInfo->nWinSz != m_info.nWinSz) { return false; }
// 		return true;
// 	}
// 	virtual void set_hdr_info(void* info){
// 		memcpy(&m_info, info, sizeof(PtHdrInfo));
// 	}
// 	bool		save2text(const char* lpstrFilePath, int nRows, size_t skip_space,bool skip_out){
// 		FILE* fp = fopen(lpstrFilePath, "w");	if (!fp) return false;		
// 		FILE* fs = NULL;
// 		float* skip_data = NULL;
// 		int sz = 0;
// 		if (skip_out) {
// 			char strPath[512];	strcpy(strPath, lpstrFilePath);	strcat(strrchr(strPath, '.'), ".fea.txt");
// 			sz = skip_space / sizeof(float);
// 			skip_data = new float[sz + 1];
// 			fs = fopen(strPath, "w");
// 		}
// 		fprintf(fp, "%d\n", get_pt_sum());
// 		char strID[POINTID_SIZE];	float xy[2];
// 		for (int i = 0; i < get_section_num(); i++){
// 			CPtSection* ptSection = (CPtSection*)get_section_info(i);
// 			if (ptSection->get_pt_num() < 1 || ptSection->get_section_sz() < POINTID_SIZE) continue;
// 			section_begin_in(i);
// 			for (int j = 0; j < ptSection->get_pt_num(); j++ ){
// 				section_data_in(strID, POINTID_SIZE);
// 				section_data_in(xy, sizeof(float)* 2);
// 				xy[0] += ptSection->get_stCol();	xy[1] += ptSection->get_stRow();				
// 				fprintf(fp, "%s\t%8.2f\t%8.2f\t%8.2f\n", strID, xy[0], nRows - 1 - xy[1], xy[1]);
// 				if (fs)
// 				{
// 					section_data_in(skip_data, skip_space);
// 					for (int k = 0; k < sz; k++) fprintf(fs, "%.2f\t", skip_data[k]);
// 					fputc('\n',fs);
// 				}
// 				else move_file_pointer(skip_space);
// 			}
// 			section_end_in();
// 		}
// 
// 		fclose(fp);
// 		if (fs){
// 			delete skip_data;
// 			fclose(fs);
// 		}
// 		return true;
// 	}
// protected:
// 	virtual SELF_Off get_section_hdr_start(){
// 		return CSELF::get_section_hdr_start() + sizeof(PtHdrInfo);
// 	}
// 	virtual int		hdr_in(HANDLE hFile){
// 		int ret = CSELF::hdr_in(hFile);
// 		if (ret != ERR_NONE) return ret;
// 		CSESection::data_in(hFile, &m_info, sizeof(PtHdrInfo));
// 		return ERR_NONE;
// 	}
// 	virtual int		hdr_out(HANDLE hFile){
// 		int ret = CSELF::hdr_out(hFile);
// 		if (ret != ERR_NONE) return ret;
// 		CSESection::data_out(hFile, &m_info, sizeof(PtHdrInfo));
// 		return ERR_NONE;
// 	}
// public:
// 	int				get_pt_sum()	{ return m_info.nPtSum; }
// 	int				get_win_sz()	{ return m_info.nWinSz; }
// 	int				get_interval()	{ return m_info.interval; }
// protected:
// 	PtHdrInfo		m_info;
// };

CFeaPtSet::~CFeaPtSet(){
	if (m_knl) delete (CPtFile*)m_knl;
}

bool CFeaPtSet::create(const char* lpstrTabName, int grid_cols, int grid_rows,int scale_lvl, int win, int interval)
{
	prepare_write(lpstrTabName);
	CPtFile* pFile = new CPtFile;
	if (!pFile->create(lpstrTabName, CPtSection::create_object, grid_rows*grid_cols*scale_lvl, NULL)){
		LogPrint(ERR_FILE_WRITE, "fail to create file %s.", lpstrTabName);
		return 0;
	}
	m_block_grid_cols = grid_cols;
	m_block_grid_rows = grid_rows;
	m_extract_win = win;
	m_extract_interval = interval;
	if (m_knl) delete (CPtFile*)m_knl;
	m_knl = pFile;
	return true;
}

void CFeaPtSet::close()
{
	PtHdrInfo hdrInfo;
	hdrInfo.nPtSum = m_pt_num;
	hdrInfo.nWinSz = m_extract_win;	
	hdrInfo.interval = m_extract_interval;
	hdrInfo.gridCols = m_block_grid_cols;
	hdrInfo.gridRows = m_block_grid_rows;
	
	CPtFile* pFile = (CPtFile*)m_knl;
	pFile->set_hdr_info(&hdrInfo);
	pFile->close();
}

inline void CvtDescrip_float_to_byte(void* from, BYTE*& to, int len){
	CvtDescriptorsf2uc((float*)from, to, len);
}
inline void CvtDescrip_do_nothing(void* from, BYTE*& to, int len){
	to = (BYTE*)from;
}

int CFeaPtSet::write(unsigned int idx, const char* lpstrImageID, int stCol, int stRow, int nCols, int nRows, int gridCol, int gridRow, int scale, float* x, float* y, float* fea, int feaDecriDimension, float* descrip, int decriDimension, int* nBufferSpace, int ptSum)
{
	CPtFile* pFile = (CPtFile*)m_knl;
	nBufferSpace[3] *= sizeof(float);
	return pFile->write(idx, lpstrImageID, stCol, stRow, nCols, nRows, gridCol, gridRow, scale,
		x, y, fea, feaDecriDimension, descrip, decriDimension, nBufferSpace, ptSum, CvtDescrip_float_to_byte);
}

int CFeaPtSet::write(unsigned int idx, const char* lpstrImageID, int stCol, int stRow, int nCols, int nRows, int gridCol, int gridRow, int scale, float* x, float* y, float* fea, int feaDecriDimension, BYTE* descrip, int decriDimension, int* nBufferSpace, int ptSum)
{
	CPtFile* pFile = (CPtFile*)m_knl;
	return pFile->write(idx, lpstrImageID, stCol, stRow, nCols, nRows, gridCol, gridRow, scale,
		x, y, fea, feaDecriDimension, descrip, decriDimension, nBufferSpace, ptSum, CvtDescrip_do_nothing);
}

bool CFeaPtSet::load(const char* lpstrTabName)
{
	CPtFile* pFile = new CPtFile;
	if (!pFile->load(lpstrTabName, CPtSection::create_object)){
//		LogPrint(ERR_FILE_READ, "fail to read file %s.",  lpstrTabName);
		delete pFile;
		return false;
	}
	m_pt_num = pFile->get_pt_sum();
	m_block_grid_cols = pFile->get_grid_cols();
	m_block_grid_rows = pFile->get_grid_rows();
	m_extract_win = pFile->get_win_sz();
	m_extract_interval = pFile->get_interval();
	if (m_knl) delete (CPtFile*)m_knl;
	m_knl = pFile;
	return true;
}

int CFeaPtSet::read(int stCol, int stRow, int nCols, int nRows, int scale, CArray_PtName* name_array, CArray_PtLoc* location_array, CArray_PtFea* fea_array, CArray_SiftDes* des_array)
{
	CPtFile* pFile = (CPtFile*)m_knl;
	return pFile->read(stCol, stRow, nCols, nRows, scale, name_array, location_array, fea_array, des_array);
}

////////////////////////////////////////////////////////////////////////////////////////////
//CStdImage
bool	CStdImage::set_workspace_path(const char* lpstrWsPath)
{
	char strPath[512];	strcpy(strPath, lpstrWsPath);
	VERF_SLASH(strPath);
	CreateDir(strPath);
	if (!IsExist(strPath)){
		return false;
	}
	if (!IsDir(strPath)){
		*strrpath(strPath) = 0;
	}
	strcpy(m_strWsPath, strPath);
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
	get_attach_file_dir(lpstrImagePath, type, strPath);
	get_attach_file(lpstrImagePath, strFileTag[type], strPath);
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
	char strPath[512];
	for (int i = 0; i < AFT_END; i++){
		get_attach_file_path(GetImagePath(), (ATTACH_FILE_TYPE)i, strPath);
		remove(strPath);
	}
}
#endif