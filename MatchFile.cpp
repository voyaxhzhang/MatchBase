#include "MatchFile.h"
#include "PrjLog.hpp"
#include "LxString.hpp"


#ifdef ATMATCH_CODE
#include "StdImage.h"
//#include "MatchTask.hpp"
#else
#define IMAGEID_SIZE	10
#define IMAGEID_BUFFER_SIZE	(IMAGEID_SIZE+1)
#endif

#include "../imagebase/rpcbase.h"

#define MAX_VAL(maxval,array,n)  { maxval = *(array);	for (int i = 1; i < n; i++) if (*(array+i)>maxval) maxval = *(array+i); }
#define MIN_VAL(minval,array,n)  { minval = *(array);	for (int i = 1; i < n; i++) if (*(array+i)<minval) minval = *(array+i); }

#define HARRIS_ATTACH_NAME "hrs"
#define SIFT_ATTACH_NAME "sift"
#define TEMP_ATTACH_NAME "sift"

#include  <vector>
#include <map>
using namespace std;

#ifndef _STORINDEX
#define _STORINDEX
typedef struct tagStorIndex{
	char image_id[32];
	unsigned int data_type;
	unsigned int array_index;
}StorIndex;
#endif

#ifndef _MODELINFO
#define _MODELINFO
typedef struct tagModelInfo{
	static char		model_directory[256];
	char		nameL[128];
	char		nameR[128];
	unsigned int data_type_left;
	unsigned int array_index_left;
	unsigned int data_type_right;
	unsigned int array_index_right;
	static void	get_model_name(const char* lpstrNameL, const char* lpstrNameR, const char* lpstrAttachName, char* name){
		sprintf(name, "%s=%s-%s.model", lpstrNameL, lpstrNameR, lpstrAttachName);
	}
	static void split_model_name(const char* model_name,char* nameL,char* nameR){
		while (*model_name != '=') { *nameL++ = *model_name++; }	*nameL = 0;	model_name++;
		while (*model_name) { *nameR++ = *model_name++; } *nameR = 0;
	}
	static void get_model_path(const char* lpstrNameL, const char* lpstrNameR,const char* lpstrAttachName, char* path){
		strcpy(path, model_directory);
		char* pS = path + strlen(path);
		get_model_name(lpstrNameL, lpstrNameR, lpstrAttachName, pS);
	}
	void	get_model_path(char* path){
		get_model_path(nameL, nameR,HARRIS_ATTACH_NAME, path);
	}
}ModelInfo;
char ModelInfo::model_directory[256] = "";
#endif
#ifndef _IMAGEINFO
#define _IMAGEINFO
typedef struct tagImageInfo{
	static char		gcp_directory[256];
	char	name[128];
	char	path[512];
	char	rpc[512];
	char	thumb[512];
	char	calib[512];
	char	index[IMAGEID_BUFFER_SIZE];
	int		nCols;
	int		nRows;
	double	gsd;
	double  x[4];
	double	y[4];
	void	get_gcp_path(char* path){
		sprintf(path, "%s%s.gcp", gcp_directory, name);
	}
}ImageInfo;
char ImageInfo::gcp_directory[256] = "";
#endif

typedef map<string, StorIndex> MAP_STRING_STORINDEX;

inline bool Rpc_Overlap(
	CRpcBase* rpcL, double stColL, double stRowL, double edColsL, double edRowsL,
	CRpcBase* rpcR, double stColR, double stRowR, double edColsR, double edRowsR,
	float* col_overlap, float* row_overlap
	)
{
	double x[4] = { stColL, stColL, edColsL, edColsL }, y[4] = { stRowL, edRowsL, stRowL, edRowsL }, h[4] = { 0 };
	
	rpcL->RPCPhoZ2Grd(x, y, h, 4, y, x, h, true);
	rpcR->RPCGrd2Pho(y, x, h,4, x, y, true);
	double stCol, stRow, edRow, edCol;
	MIN_VAL(stCol, x, 4);	MAX_VAL(edCol, x, 4);
	MIN_VAL(stRow, y, 4);	MAX_VAL(edRow, y, 4);
	
	if (stCol > edColsR - 2 || stRow > edRowsR - 2 || edCol < stColR + 2 || edRow < stRowR + 2) return false;
	if (stCol < 0) stCol = 0;	if (stRow < 0) stRow = 0;
	if (edCol>edColsR - 1) edCol = edColsR - 1;
	if (edRow>edRowsR - 1) edRow = edRowsR - 1;
	int nCols = int(edColsR - stColR) + 1;	if (nCols < 1) nCols = 1;
	int nRows = int(edRowsR - stRowR) + 1;	if (nRows < 1) nRows = 1;
	*col_overlap = (float)((edCol - stCol + 1) / nCols);
	*row_overlap = (float)((edRow - stRow + 1) / nRows);
	return true;
}

inline bool RpcPara_Overlap(RpcPara& rpcL, RpcPara& rpcR){
	double lat_min_l = rpcL.lat_off - rpcL.lat_scale;
	double lat_max_l = rpcL.lat_off + rpcL.lat_scale;
	double lon_min_l = rpcL.long_off - rpcL.long_scale;
	double lon_max_l = rpcL.long_off + rpcL.long_scale;
	double lat_min_r = rpcR.lat_off - rpcR.lat_scale;
	double lat_max_r = rpcR.lat_off + rpcR.lat_scale;
	double lon_min_r = rpcR.long_off - rpcR.long_scale;
	double lon_max_r = rpcR.long_off + rpcR.long_scale;
	if (lat_min_l > lat_max_r || lat_min_r > lat_max_l || lon_min_l > lon_max_r || lon_min_r > lon_max_l) return false;
	return true;
}

bool Rpc_Overlap(CRpcBase* rpcL, CRpcBase* rpcR, float* overlap_colL, float* overlap_RowL, float* overlap_colR, float* overlap_RowR)
{
	*overlap_colL = *overlap_RowL = *overlap_colR = *overlap_RowR = 0;
	RpcPara rpL, rpR;
	rpcL->GetRpcPara(&rpL);	rpcR->GetRpcPara(&rpR);
	if (!RpcPara_Overlap(rpL, rpR)) return false;
	double stColL = rpL.samp_off - rpL.samp_scale;	double stRowL = rpL.line_off - rpL.line_scale;
	double edColL = rpL.samp_off + rpL.samp_scale;	double edRowL = rpL.line_off + rpL.line_scale;
	double stColR = rpR.samp_off - rpR.samp_scale;	double stRowR = rpR.line_off - rpR.line_scale;
	double edColR = rpR.samp_off + rpR.samp_scale;	double edRowR = rpR.line_off + rpR.line_scale;
	if (!Rpc_Overlap(rpcL, stColL, stRowL, edColL, edRowL, rpcR, stColR, stRowR, edColR, edRowR, overlap_colR, overlap_RowR)) return false;
	if (!Rpc_Overlap(rpcR, stColR, stRowR, edColR, edRowR, rpcL, stColL, stRowL, edColL, edRowL, overlap_colL, overlap_RowL)) return false;
	return true;
}

class CStorage{
public:
	enum{
		DT_GCP = 0,
		DT_IMG = 1,
	};
	CStorage(){
		m_image_append_idx = -1;
		m_gcp_append_idx = -1;
		m_image_append_idx = m_gcp_model_append_idx = -1;
	}
	void	Reset(){
		m_image_append_idx = -1;
		m_gcp_append_idx = -1;
		m_image_append_idx = m_gcp_model_append_idx = -1;
		m_data[0].clear();	m_data[1].clear();
		m_model_info[0].clear();	m_model_info[1].clear();
		m_index.clear();
	}
	int		GetImageNum() { return m_data[DT_IMG].size(); }
	int		GetGcpNum() { return m_data[DT_GCP].size(); }
	ImageInfo* GetImageInfo(unsigned int idx){
		return m_data[DT_IMG].data()+idx;
	}
	ImageInfo* GetGcpInfo(unsigned int idx){
		return m_data[DT_GCP].data()+idx;
	}
	int		GetImageModelNum(){
		return m_model_info[DT_IMG].size();
	}
	int		GetGcpModelNum(){
		return m_model_info[DT_GCP].size();
	}
	ModelInfo*	GetImageModelInfo(unsigned int idx)
	{
		return m_model_info[DT_IMG].data() + idx;
	}
	bool	ReadStdImageInfoFile(const char* lpstrInfoFile, const char* lpstrImageIndexFile)
	{
		return ReadImageInfoFile(lpstrInfoFile, lpstrImageIndexFile, DT_IMG, m_data[DT_IMG], m_index);
	}
	bool	ReadGcpImageInfoFile(const char* lpstrInfoFile, const char* lpstrImageIndexFile)
	{
		return ReadImageInfoFile(lpstrInfoFile, lpstrImageIndexFile, DT_GCP, m_data[DT_GCP], m_index);
	}
	bool	ReadStdModelRecordFile(const char* lpstrModelInfo)
	{
		return ReadModelRecordFile(lpstrModelInfo, m_model_info[DT_IMG], m_index);
	}
	bool	ReadGcpModelRecordFile(const char* lpstrModelInfo)
	{
		return ReadModelRecordFile(lpstrModelInfo, m_model_info[DT_GCP], m_index);
	}
	inline static bool ReadImageInfo(FILE* fp, ImageInfo* pInfo){
		char strLine[1024];
		if (!fgets(strLine, 512, fp)) return false;	sscanf(strLine, "%s", pInfo->name);
		if (!fgets(strLine, 512, fp)) return false;	sscanf(strLine, "%s", pInfo->path);
		if (!fgets(strLine, 512, fp)) return false;	sscanf(strLine, "%d%d", &pInfo->nCols, &pInfo->nRows);
		if (!fgets(strLine, 512, fp)) return false;	sscanf(strLine, "%s", pInfo->rpc);
		if (!fgets(strLine, 512, fp)) return false;	sscanf(strLine, "%lf%lf%lf%lf%lf%lf%lf%lf", pInfo->x, pInfo->y, pInfo->x + 1, pInfo->y + 1, pInfo->x + 2, pInfo->y + 2, pInfo->x + 3, pInfo->y + 3);
		if (!fgets(strLine, 512, fp)) return false;	sscanf(strLine, "%lf", pInfo->gsd);
		if (!fgets(strLine, 512, fp)) return false;	sscanf(strLine, "%s", pInfo->thumb);	if (!strcmp(pInfo->thumb, "NULL")) pInfo->thumb[0] = 0;
		if (!fgets(strLine, 512, fp)) return false;	sscanf(strLine, "%s", pInfo->calib);	if (!strcmp(pInfo->calib, "NULL")) pInfo->calib[0] = 0;
		return true;
	}
	static bool ReadImageInfoFile(
		const char* lpstrInfoFile,
		const char* lpstrImageIndexFile,
		unsigned int	data_type,
		vector<ImageInfo>& data,
		map<string, StorIndex>& index
		)
	{
		FILE* fInfo = fopen(lpstrInfoFile, "r");	if (!fInfo) return false;
		FILE* fIndex = fopen(lpstrImageIndexFile, "r");	if (!fIndex) { fclose(fInfo); return false; }
		fgets(ImageInfo::gcp_directory, 256, fInfo);	AddEndSlash(ImageInfo::gcp_directory);

		int num1 = 0; int num2 = 0;	int i;
		char strLine[1024];	StorIndex storIndex;	storIndex.data_type = data_type;
		fgets(strLine, 1024, fIndex);	num1 = atoi(strLine);

		printf("Read image index file %s[num=%d] ...", lpstrImageIndexFile, num1);
		for (i = 0; i < num1; i++){
			if (!fgets(strLine, 1024, fIndex)) break;
			char name[128];
			sscanf(strLine, "%s%s", name, storIndex.image_id);
			MAP_STRING_STORINDEX::iterator it = index.find(name);

			if (it == index.end()){
				index.insert(pair<string, StorIndex>(name, storIndex));
			}
		}
		fclose(fIndex);	printf("Done\n");
		fgets(strLine, 1024, fInfo);	num2 = atoi(strLine);
		ImageInfo info;	int sz = 0;
		data.reserve(data.size() + num2);
		printf("Read image info file %s[num=%d] ...", lpstrInfoFile, num2);
		for (i = 0; i < num2; i++){
			if (!ReadImageInfo(fInfo, &info)) break;
			MAP_STRING_STORINDEX::iterator it = index.find(info.name);

			if (it != index.end()){
				strcpy(info.index, it->second.image_id);
				it->second.array_index = sz;	sz++;
				data.push_back(info);
			}
		}
		fclose(fInfo);	printf("Done\n");
		return sz > 0 ? true : false;
	}
	static bool ReadModelRecordFile(const char* lpstrModelRecordFile, vector<ModelInfo>& model, MAP_STRING_STORINDEX& index)
	{
		if (index.size() < 1) return false;
		FILE* fp = fopen(lpstrModelRecordFile, "r");	if (!fp) return false;
		char strLine[1024];
		fgets(ModelInfo::model_directory, 256, fp);	AddEndSlash(ModelInfo::model_directory);
		fgets(strLine, 1024, fp);	int num = atoi(strLine);

		model.reserve(model.size() + num);
		printf("Read model list file %s[num=%d]", lpstrModelRecordFile, num);
		for (int i = 0; i < num; i++){
			if (!fgets(strLine, 1024, fp)) break;
			ModelInfo name;	sscanf(strLine, "%s%s", name.nameL, name.nameR);
			MAP_STRING_STORINDEX::iterator itL = index.find(name.nameL);
			MAP_STRING_STORINDEX::iterator itR = index.find(name.nameR);
			if (itL == index.end() || itR == index.end()) continue;
			name.data_type_left = itL->second.data_type;
			name.array_index_left = itL->second.array_index;
			name.data_type_right = itR->second.data_type;
			name.array_index_right = itR->second.array_index;
			model.push_back(name);
		}

		fclose(fp);	printf("Done\n");
		return true;
	}
#ifdef ATMATCH_CODE
	bool	WriteStdImageInfoFile(const char* lpstrInfoFile,const char* lpstrImageIndexFile){
		return WriteImageInfoFile(lpstrInfoFile, lpstrImageIndexFile, &m_data[DT_IMG][0], m_data[DT_IMG].size());
	}
	bool	WriteGcpImageInfoFile(const char* lpstrInfoFile, const char* lpstrImageIndexFile)
	{
		return WriteImageInfoFile(lpstrInfoFile, lpstrImageIndexFile, &m_data[DT_GCP][0], m_data[DT_GCP].size());
	}
	bool	WriteStdModelRecordFile(const char* lpstrModelRecordFile)
	{
		return WriteModelRecordFile(lpstrModelRecordFile, &m_model_info[DT_IMG][0], m_model_info[DT_IMG].size());
	}
	bool	WriteGcpModelRecordFile(const char* lpstrModelRecordFile)
	{
		return WriteModelRecordFile(lpstrModelRecordFile, &m_model_info[DT_GCP][0], m_model_info[DT_GCP].size());
	}
	inline static bool WriteImageInfo(FILE* fp, ImageInfo* pInfo){
		fprintf(fp, "%s\n", pInfo->name);
		fprintf(fp, "%s\n", pInfo->path);
		fprintf(fp, "%d\t%d\n", pInfo->nCols, pInfo->nRows);
		fprintf(fp, "%s\n", pInfo->rpc);
		for (int i = 0; i < 4; i++) fprintf(fp, "%lf\t%lf\t", pInfo->x[i], pInfo->y[i]);
		fprintf(fp, "%lf\n", pInfo->gsd);
		fprintf(fp, "%s\n", pInfo->thumb[0] ? pInfo->thumb : "NULL");
		fprintf(fp, "%s\n", pInfo->calib[0] ? pInfo->calib : "NULL");
		return true;
	}
	static bool	WriteImageInfoFile(
		const char* lpstrInfoFile,
		const char* lpstrImageIndexFile,
		ImageInfo* pInfo, int num
		){
		FILE* fInfo = fopen(lpstrInfoFile, "w");	if (!fInfo) return false;
		FILE* fIndex = fopen(lpstrImageIndexFile, "w");	if (!fIndex) { fclose(fInfo); return false; }

		fprintf(fInfo, "%s\n", ImageInfo::gcp_directory);
		fprintf(fInfo, "%d\n", num);
		fprintf(fIndex, "%d\n", num);
		for (int i = 0; i < num; i++, pInfo++){
			WriteImageInfo(fInfo, pInfo);

			fprintf(fIndex, "%s\t%s\n", pInfo->name, pInfo->index);
		}
		fclose(fInfo);
		fclose(fIndex);

		return true;
	}
	static bool	WriteModelRecordFile(const char* lpstrModelRecordFile, ModelInfo* path, int num)
	{
		FILE* fp = fopen(lpstrModelRecordFile, "w");	if (!fp) return false;

		fprintf(fp, "%s\n", ModelInfo::model_directory);
		fprintf(fp, "%d\n", num);

		for (int i = 0; i < num; i++, path++){
			fprintf(fp, "%s\t%s\n", path->nameL, path->nameR);
		}

		fclose(fp);
		return true;
	}
	static inline void GeoImage_to_ImageInfo(CGeoImage* pImg, ImageInfo* info){
		strcpy(info->name, pImg->GetImageName());
		strcpy(info->path, pImg->GetImagePath());
		pImg->GetOriInfo(info->rpc, false);
		info->thumb[0] = 0;
		info->calib[0] = 0;
		strcpy(info->index, pImg->GetImageID());
		info->nCols = pImg->GetCols();	info->nRows = pImg->GetRows();
		pImg->GetGeoRgn_LBH(0, info->x, info->y);
		info->gsd = pImg->GetGsd();
	}
	static inline bool IsOverlap(const ImageInfo* info, CGeoImage* pImg, int rangX, int rangY){
		double x[4], y[4];
		double h[4] = { 0 };
		pImg->LBH2Pho(info->x, info->y, h, 4, x, y);
		double stCol, stRow, edRow, edCol;
		MIN_VAL(stCol, x, 4);	MAX_VAL(edCol, x, 4);
		MIN_VAL(stRow, y, 4);	MAX_VAL(edRow, y, 4);
		stCol -= rangX;	stRow -= rangY;
		edCol += rangX;	edRow += rangY;
		if (stCol > pImg->GetCols() - 2 || stRow > pImg->GetRows() - 2 || edCol < 2 || edRow < 2) return false;
		return true;
	}
	int		AppendImage(CGeoImage* img, int num,int rangX,int rangY){
		int i,j;	
		if (m_image_append_idx < 0) m_image_append_idx = m_data[DT_IMG].size();
		if (m_image_model_append_idx < 0) m_image_model_append_idx = m_model_info[DT_IMG].size();
		if (m_gcp_model_append_idx < 0) m_gcp_model_append_idx = m_model_info[DT_GCP].size();
		printf("Append image ");
		ModelInfo model_info;	model_info.data_type_right = DT_IMG;
		int step = 1;
		int step_len = num / 5;	if (step_len <= 0) step_len = 1;
		int sz = m_data[DT_IMG].size();
		m_data[DT_IMG].reserve(sz + num);
		for (i = 0; i < num; i++,img++){
			if (!((i + 1) % step_len)) {
				printf("...%d", step * 20);	step++;
			}
			ImageInfo info;	GeoImage_to_ImageInfo(img, &info);
			
			ImageInfo* pInfo = &m_data[DT_IMG][0];			
			
			model_info.data_type_left = DT_IMG;		
			model_info.array_index_right = sz;
			strcpy(model_info.nameR, img->GetImageName());
			for (j = 0; j < sz; j++,pInfo++){
				if (IsOverlap(pInfo,img,rangX,rangY)){
					model_info.array_index_left = j;
					strcpy(model_info.nameL, pInfo->name);
					m_model_info[DT_IMG].push_back(model_info);
				}
			}
			m_data[DT_IMG].push_back(info);
			pInfo = &m_data[DT_GCP][0];
			sz = m_data[DT_GCP].size();
			model_info.data_type_left = DT_GCP;
			for (j = 0; j < sz;j++){
				if (IsOverlap(pInfo, img, rangX, rangY)){
					model_info.array_index_left = j;
					strcpy(model_info.nameL, pInfo->name);
					m_model_info[DT_GCP].push_back(model_info);
				}
			}
			sz = m_data[DT_IMG].size();
		}
		if (step < 6) printf("...100");
		printf("...done.\n");
		return num;
	}
	int AppendGcpImage(CGeoImage* img, int num, int rangX, int rangY){
		int i, j;
		if (m_gcp_append_idx < 0) m_gcp_append_idx = m_data[DT_GCP].size();
		if (m_image_model_append_idx < 0) m_image_model_append_idx = m_model_info[DT_IMG].size();
		
		printf("Append Gcp image ");
		ModelInfo model_info;	
		model_info.data_type_right = DT_IMG;
		model_info.data_type_left = DT_GCP;

		int step = 1;
		int step_len = num / 5;	if (step_len <= 0) step_len = 1;
		int sz = m_data[DT_IMG].size();
		m_data[DT_GCP].reserve(m_data[DT_GCP].size() + num);
		for (i = 0; i < num; i++, img++){
			if (!((i + 1) % step_len)) {
				printf("...%d", step * 20);	step++;
			}
			ImageInfo info;	GeoImage_to_ImageInfo(img, &info);

			ImageInfo* pInfo = &m_data[DT_IMG][0];

			model_info.array_index_left = m_data[DT_GCP].size();
			strcpy(model_info.nameL, img->GetImageName());
			for (j = 0; j < sz; j++, pInfo++){
				if (IsOverlap(pInfo, img, rangX, rangY)){
					model_info.array_index_right = j;
					strcpy(model_info.nameR, pInfo->name);
					m_model_info[DT_GCP].push_back(model_info);
				}
			}
			m_data[DT_GCP].push_back(info);
		}
		if (step < 6) printf("...100");
		printf("...done.\n");
		return num;
	}
#endif
protected:
	vector<ImageInfo>	m_data[2];
	vector<ModelInfo>	m_model_info[2];
	MAP_STRING_STORINDEX	m_index;
	unsigned int		m_image_append_idx;
	unsigned int		m_gcp_append_idx;
	unsigned int		m_image_model_append_idx;
	unsigned int		m_gcp_model_append_idx;
};

///////////////////////////////////////////////////////////////////////////////////////
//CMatchFile

CMatchFile::CMatchFile(){
	m_storage = new CStorage;
}
CMatchFile::~CMatchFile(){
	delete m_storage;
}

#ifdef PQ_DB
bool	CMatchFile::set_workspace_path(const char* lpstrWsPath)
{
	return true;
}
#else

#ifdef ATMATCH_CODE
bool	CMatchFile::set_workspace_path(const char* lpstrWsPath)
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
	char* pS = AddEndSlash(strPath);	pS++;
	strcpy(pS, "model/");	CreateDir(strPath); strcpy(ModelInfo::model_directory, strPath);
	strcpy(pS, "gcp/");	CreateDir(strPath);	strcpy(ImageInfo::gcp_directory, strPath);
	return true;
}

void CMatchFile::get_harris_model_file_path(const char* lpstrImageNameL, const char* lpstrImageNameR, char* strPath)
{
	ModelInfo::get_model_path(lpstrImageNameL, lpstrImageNameR, HARRIS_ATTACH_NAME, strPath);
}
void CMatchFile::get_sift_model_file_path(const char* lpstrImageNameL, const char* lpstrImageNameR, char* strPath)
{
	ModelInfo::get_model_path(lpstrImageNameL, lpstrImageNameR, SIFT_ATTACH_NAME, strPath);
}
void CMatchFile::get_temp_model_file_path(const char* lpstrImageNameL, const char* lpstrImageNameR, char* strPath)
{
	ModelInfo::get_model_path(lpstrImageNameL, lpstrImageNameR, TEMP_ATTACH_NAME, strPath);
}

bool CMatchFile::WriteModelFile(
	const char* lpstrModelFilePath,
	const char* lpstrImageLID, const char* lpstrImageLPath,
	const char* lpstrImageRID, const char* lpstrImageRPath,
	const char* ptL_name, const float* xl, const float* yl,
	const char* ptR_name, const float* xr, const float* yr,
	const char* attach_info,
	int nPtNum, int nBufferSpaceByChar[7]
	)
{
	FILE* fp = fopen(lpstrModelFilePath, "w");	if (!fp) return false;
	fprintf(fp, "leftimage= %s\n", lpstrImageLPath);
	fprintf(fp, "rightimage= %s\n", lpstrImageRPath);
	int len = ftell(fp);
	fprintf(fp, "0       \n");	
	int sz = 0;
	char ct = 0;
	char at[] = { "0" };
	if (!lpstrImageLID) lpstrImageLID = (const char*)&ct;
	if (!lpstrImageRID) lpstrImageRID = (const char*)&ct;
	if (!attach_info) {
		attach_info = at;
		nBufferSpaceByChar[6] = 0;
	}

	float xl_last = -99, yl_last = -99, xr_last = -99, yr_last = -99;
	for (int i = 0; i < nPtNum; i++){
		if (fabs(xl_last - *xl) < 1e-5 && fabs(yl_last - *yl) < 1e-5 && fabs(xr_last - *xr) < 1e-5 && fabs(yr_last - *yr) < 1e-5) {
			printf("Repeat match info : %8.2f %8.2f %8.2f %8.2f\n", *xl, *yl, *xr, *yr);
		}
		else{
			xl_last = *xl;	yl_last = *yl;	xr_last = *xr;	yr_last = *yr;
			fprintf(fp, "%s%s\t%8.2f\t%8.2f\t%8.2f\t%8.2f\t%s%s\t%s\n",
				lpstrImageLID, ptL_name, *xl, *yl,
				*xr, *yr, lpstrImageRID, ptR_name,
				attach_info);
			sz++;
		}
		
		ptL_name += nBufferSpaceByChar[0];
		xl = (const float*)((const char*)xl + nBufferSpaceByChar[1]);
		yl = (const float*)((const char*)yl + nBufferSpaceByChar[2]);
		ptR_name += nBufferSpaceByChar[3];
		xr = (const float*)((const char*)xr + nBufferSpaceByChar[4]);
		yr = (const float*)((const char*)yr + nBufferSpaceByChar[5]);
		attach_info += nBufferSpaceByChar[6];
	}
	fseek(fp, len, SEEK_SET);
	fprintf(fp, "%d", sz);
	fclose(fp);
	return true;
}

bool CMatchFile::WriteModelFile(
	const char* lpstrModelFilePath,
	const char* lpstrImageLPath,
	const char* lpstrImageRPath,
	const float* xl, const float* yl,
	const float* xr, const float* yr,
	int nPtNum, int nBufferSpaceByChar[4]
	)
{
	FILE* fp = fopen(lpstrModelFilePath, "w");	if (!fp) return false;
	fprintf(fp, "leftimage= %s\n", lpstrImageLPath);
	fprintf(fp, "rightimage= %s\n", lpstrImageRPath);

	fprintf(fp, "%d       \n",nPtNum);

	for (int i = 0; i < nPtNum; i++){
		fprintf(fp, "%05d\t%f\t%f\t%f\t%f\n", i, *xl, *yl, *xr, *yr);
		xl = (const float*)((const char*)xl + nBufferSpaceByChar[0]);
		yl = (const float*)((const char*)yl + nBufferSpaceByChar[1]);
		xr = (const float*)((const char*)xr + nBufferSpaceByChar[2]);
		yr = (const float*)((const char*)yr + nBufferSpaceByChar[3]);
	}
	fclose(fp);
	return true;
}

bool	CMatchFile::WriteGcpFile(
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
	)
{
	FILE* fp = fopen(lpstrFilePath, "w");	if (!fp) return false;
	fprintf(fp, "%d\n", nPtNum);
	char ct = 0;
	if (!lpstrImageID) lpstrImageID = (const char*)&ct;
	for (int i = 0; i < nPtNum; i++){
		fprintf(fp, "%s%s\t%8.2f\t%8.2f\t1\t%lf\t%lf\t%lf\t%8.2f\t%8.2f\tm\n", lpstrImageID, pt_name, *col, *row, *x, *y, *z, *plane_precision, *elevation_precision);
		pt_name += nBufferSpaceByChar[0];
		col = (const float*)((const char*)col + nBufferSpaceByChar[1]);
		row = (const float*)((const char*)row + nBufferSpaceByChar[2]);
		x = (const double*)((const char*)x + nBufferSpaceByChar[3]);
		y = (const double*)((const char*)y + nBufferSpaceByChar[4]);
		z = (const double*)((const char*)z + nBufferSpaceByChar[5]);
		plane_precision = (const float*)((const char*)plane_precision + nBufferSpaceByChar[6]);
		elevation_precision = (const float*)((const char*)elevation_precision + nBufferSpaceByChar[7]);
	}
	fclose(fp);
	return true;
}

int CMatchFile::AppendImage(void* img, int num,int rangX,int rangY)
{
	return m_storage->AppendImage((CGeoImage*)img, num,rangX,rangY);
}

int CMatchFile::AppendGcp(void* img, int num,int rangX,int rangY)
{
	return m_storage->AppendGcpImage((CGeoImage*)img, num, rangX, rangY);
}

bool CMatchFile::SavePrj(const char* lpstrPrjPath)
{
	LogPrint(0, ">>>>Save Project File>>>>");
	FILE* fp = fopen(lpstrPrjPath, "w");	if (!fp) return false;
	char str1[512], str2[512];
	strcpy(str1, lpstrPrjPath);	char* pS = strrpath(str1)+1;
	strcpy(pS, "image_index.txt");	strcpy(str2, str1);
	strcpy(pS, "image_info.txt");	
	LogPrint(0, "save image info file %s ...", str1);
	if (!m_storage->WriteStdImageInfoFile(str1, str2)) {
		fclose(fp);
		return false;
	}
	fprintf(fp, "image_info=\t%s\n", str1);
	fprintf(fp, "image_index=\t%s\n", str2);

	strcpy(pS, "gcp_image_index.txt");	strcpy(str2, str1);
	strcpy(pS, "gcp_image_info.txt");
	LogPrint(0, "save gcp image info file %s ...", str1);
	if (!m_storage->WriteGcpImageInfoFile(str1, str2)) {
		fclose(fp);
		return false;
	}
	fprintf(fp, "gcp_image_info=\t%s\n", str1);
	fprintf(fp, "gcp_image_index=\t%s\n", str2);

	strcpy(pS, "image_model_list.txt");
	LogPrint(0, "save image model list file %s ...", str1);
	if (!m_storage->WriteStdModelRecordFile(str1)) {
		fclose(fp);
		return false;
	}
	fprintf(fp, "image_model_list=\t%s\n", str1);

	strcpy(pS, "gcp_image_model_list.txt");
	LogPrint(0, "save gcp image model list file %s ...", str1);
	if (!m_storage->WriteGcpModelRecordFile(str1)) {
		fclose(fp);
		return false;
	}
	fprintf(fp, "gcp_image_model_list=\t%s\n", str1);
	fclose(fp);
	LogPrint(0, "<<<<Save Project File<<<<");
	return true;
}
#endif

bool CMatchFile::LoadPrj(const char* lpstrPrjPath)
{
	FILE* fp = fopen(lpstrPrjPath, "r");	if (!fp) return false;
	LogPrint(0, ">>>>Load Project File>>>>");
	char image_info[512] = "", image_index[512] = "", gcp_image_info[512] = "", gcp_image_index[512] = "", image_model_list[512] = "", gcp_image_model_list[512] = "";

	while (!feof(fp))
	{
		char strLine[1024];
		if (!fgets(strLine, 1024, fp)) break;
		char* pS = strchr(strLine, '=');	if (!pS) continue;
		if (strstr(strLine, "image_info")){
			sscanf(pS + 1, "%s", image_info);
		}
		else if (strstr(strLine, "image_index")){
			sscanf(pS + 1, "%s", image_index);
		}
		else if (strstr(strLine, "gcp_image_info")){
			sscanf(pS + 1, "%s", gcp_image_info);
		}
		else if (strstr(strLine, "gcp_image_index")){
			sscanf(pS + 1, "%s", gcp_image_index);
		}
		else if (strstr(strLine, "image_model_list")){
			sscanf(pS + 1, "%s", image_model_list);
		}
		else if (strstr(strLine, "gcp_image_model_list")){
			sscanf(pS + 1, "%s", gcp_image_model_list);
		}		
	}
	if (!image_info[0] || !image_index[0] || !gcp_image_info[0] || !gcp_image_index[0] || !image_model_list[0] || !gcp_image_model_list[0]){
		fclose(fp);
		LogPrint(0, "Some information is lost in %s.",lpstrPrjPath);
		return false;
	}
	m_storage->Reset();

	LogPrint(0, "load image info file %s...", image_info);
	if (!m_storage->ReadStdImageInfoFile(image_info,image_index)){
		fclose(fp);
		return false;
	}
	LogPrint(0, "load gcp image info file %s...", gcp_image_info);
	if (!m_storage->ReadGcpImageInfoFile(gcp_image_info, gcp_image_index)){
		fclose(fp);
		return false;
	}
	LogPrint(0, "load image model list file %s...", image_model_list);
	if (!m_storage->ReadStdModelRecordFile(image_model_list)){
		fclose(fp);
		return false;
	}
	LogPrint(0, "load gcp image model list file %s...", gcp_image_model_list);
	if (!m_storage->ReadGcpModelRecordFile(gcp_image_model_list)){
		fclose(fp);
		return false;
	}
	fclose(fp);
	LogPrint(0, "<<<<Load Project File<<<<");
	return true;
}

int		CMatchFile::GetImageNum()
{
	return m_storage->GetImageNum();
}

int		CMatchFile::GetGcpNum()
{
	return m_storage->GetGcpNum();
}

int		CMatchFile::GetTieModelNum()
{
	return m_storage->GetImageModelNum();
}

int		CMatchFile::GetGcpModelNum()
{
	return m_storage->GetGcpModelNum();
}

bool	CMatchFile::GetImageInfo(unsigned int idx, char* image_id, char* name, char* path, char* rpc, char* thumb, char* calib, int* nCols, int* nRows, char* gcp)
{
	ImageInfo* pInfo = m_storage->GetImageInfo(idx);
	if (image_id) strcpy(image_id, pInfo->index);
	if (name) strcpy(name, pInfo->name);
	if (path) strcpy(path, pInfo->path);
	if (rpc) strcpy(rpc, pInfo->rpc);
	if (thumb) strcpy(thumb, pInfo->thumb);
	if (calib) strcpy(calib, pInfo->calib);
	if (nCols) *nCols = pInfo->nCols;
	if (nRows) *nRows = pInfo->nRows;
	if (gcp) pInfo->get_gcp_path(gcp);
	return true;
}

bool CMatchFile::ReadTieModelHdr(unsigned int model_idx, unsigned int* imageL_array_idx, unsigned int* imageR_array_idx, int* nPtSum)
{
	ModelInfo* pInfo = m_storage->GetImageModelInfo(model_idx);
	char str[512];
	pInfo->get_model_path(str);
	*imageL_array_idx = pInfo->array_index_left;
	*imageR_array_idx = pInfo->array_index_right;

	return ReadGcpFileHdr(str, nPtSum);

}

bool CMatchFile::ReadTieModel(unsigned int model_idx, char* ptL_name, float* xl, float* yl, char* ptR_name, float* xr, float* yr, float* search_area, int nBufferSpaceByChar[7])
{
	ModelInfo* pInfo = m_storage->GetImageModelInfo(model_idx);
	char str[512];
	pInfo->get_model_path(str);	
	unsigned int idxL = pInfo->array_index_left;
	ImageInfo* pImg = m_storage->GetImageInfo(idxL);

	return ReadModelFile(str, pImg->index, "", ptL_name, xl, yl, ptR_name, xr, yr, search_area, nBufferSpaceByChar) > 0 ? true : false;
}

bool		CMatchFile::ReadGcpFileHdr(
	const char* lpstrFilePath,
	int*	nPtSum
	)
{
	FILE* fp = fopen(lpstrFilePath, "r");	if (!fp) return false;
	char strLine[1024];
	fgets(strLine, 1024, fp);
	*nPtSum = atoi(strLine);
	fclose(fp);
	return true;
}
int		CMatchFile::ReadGcpFile(
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
	)
{
	FILE* fp = fopen(lpstrFilePath, "r");	if (!fp) return 0;

	char strLine[1024];
	int sz = 0;
	char nameL[128];
	strcpy(nameL, lpstrImageID);	char* pL = nameL + strlen(nameL);

	fgets(strLine, 1024, fp);
	int nPtNum = atoi(strLine);

	for (int i = 0; i < nPtNum; i++){
		if (!fgets(strLine, 1024, fp)) break;
		sscanf(strLine, "%s%f%f%f%lf%lf%lf%f%f%s", pt_name, col, row, image_priori, x, y, z, plane_precision, elevation_precision, type);

		sz++;
		pt_name += nBufferSpaceByChar[0];
		col = (float*)((char*)col + nBufferSpaceByChar[1]);
		row = (float*)((char*)row + nBufferSpaceByChar[2]);
		image_priori = (float*)((char*)image_priori + nBufferSpaceByChar[3]);
		x = (double*)((char*)x + nBufferSpaceByChar[4]);
		y = (double*)((char*)y + nBufferSpaceByChar[5]);
		z = (double*)((char*)z + nBufferSpaceByChar[6]);
		plane_precision = (float*)((char*)plane_precision + nBufferSpaceByChar[7]);
		elevation_precision = (float*)((char*)elevation_precision + nBufferSpaceByChar[8]);
		type += nBufferSpaceByChar[9];
	}

	fclose(fp);
	return sz;
}

bool CMatchFile::ReadModelFileHdr(const char* lpstrModelFilePath, char* strImageLPath, char* strImageRPath, int* nPtSum)
{
	FILE* fp = fopen(lpstrModelFilePath, "r");	if (!fp) return false;

	bool ret = true;

	char strLine[2048];	char* pS = NULL;
	fgets(strLine, 2048, fp);	if (strImageLPath){ pS = strchr(strLine, '=');  if (!pS) { ret = false; goto loop; } sscanf(pS + 1, "%s", strImageLPath); }
	fgets(strLine, 2048, fp);	if (strImageRPath){ pS = strchr(strLine, '=');  if (!pS) { ret = false; goto loop; } sscanf(pS + 1, "%s", strImageRPath); }

	fgets(strLine, 2048, fp);	if (nPtSum) *nPtSum = atoi(strLine);

loop:
	fclose(fp);
	return ret;
}

int CMatchFile::ReadModelFile(
	const char* lpstrModelFilePath,
	const char* lpstrImageNameL,
	const char* lpstrImageNameR,
	char* ptL_name, float* xl, float* yl,
	char* ptR_name, float* xr, float* yr,
	float* search_area,
	int nBufferSpaceByChar[7]
	)
{
	FILE* fp = fopen(lpstrModelFilePath, "r");	if (!fp) return 0;

	char strLine[1024];

	fgets(strLine, 1024, fp);
	fgets(strLine, 1024, fp);

	fgets(strLine, 1024, fp);
	int nPtNum = atoi(strLine);

	int sz = 0;
	char nameL[128], nameR[128];
	strcpy(nameL, lpstrImageNameL);	char* pL = nameL + strlen(nameL);
	strcpy(nameR, lpstrImageNameR);	char* pR = nameR + strlen(nameR);
	for (int i = 0; i < nPtNum; i++){
		if (!fgets(strLine, 1024, fp)) break;
		sscanf(strLine, "%s%f%f%f%f%s%f", pL, xl, yl, xr, yr, pR, search_area);
		strcpy(ptL_name, nameL);
		strcpy(ptR_name, nameR);
		sz++;
		ptL_name += nBufferSpaceByChar[0];
		xl = (float*)((char*)xl + nBufferSpaceByChar[1]);
		yl = (float*)((char*)yl + nBufferSpaceByChar[2]);
		ptR_name += nBufferSpaceByChar[3];
		xr = (float*)((char*)xr + nBufferSpaceByChar[4]);
		yr = (float*)((char*)yr + nBufferSpaceByChar[5]);
		search_area = (float*)((char*)search_area + nBufferSpaceByChar[6]);
	}

	fclose(fp);
	return sz;
}

bool CMatchFile::Overlap(const char* lpstrRpcL, const char* lpstrRpcR, float* ratio_colL, float* ratio_rowL, float* ratio_colR, float* ratio_rowR)
{
	CRpcBase rpcL, rpcR;
	if (!rpcL.Load4File(lpstrRpcL) || !rpcR.Load4File(lpstrRpcR)) return false;
	return ::Rpc_Overlap(&rpcL, &rpcR, ratio_colL, ratio_rowL, ratio_colR, ratio_rowR);
}
#endif