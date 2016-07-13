//StdImage.h
/********************************************************************
	StdImage
	created:	2015/03/10
	author:		LX_whu 
	purpose:	This file is for StdImage function
*********************************************************************/
#if !defined StdImage_h__LX_whu_2015_3_10
#define StdImage_h__LX_whu_2015_3_10

#include <stdio.h>
#include "../imagebase/package_gdal.h"

#define  PYRAMID_PIXELNUM 3
#define  DEN_PYRAMID_PIXELNUM (1.0/PYRAMID_PIXELNUM)

#define GEO_TAG_STD		"TFW"
#define GEO_TAG_FRM		"POS"
#define GEO_TAG_RPC		"RPC"

#define IMAGEID_SIZE	10
#define IMAGEID_BUFFER_SIZE	(IMAGEID_SIZE+1)

//#define POINTID_SIZE	(IMAGEID_SIZE+10)

#if !defined(ATMATCH_CODE) && !defined(STDIMAGE_CODE)
	#ifdef STDIMAGE_EXPORTS
		#define STDIMAGE_API __declspec(dllexport)
	#else
		#define STDIMAGE_API __declspec(dllimport)
		#if defined(_WIN64)  || defined(_X64)
			#pragma comment(lib,"StdImage_x64.lib") 
			#pragma message("Automatically linking with StdImage_x64.lib") 
		#else
			#ifdef _DEBUG_STDIMAGE_API
				#pragma comment(lib,"StdImageD.lib") 
				#pragma message("Automatically linking with StdImageD.lib") 
			#else
				#pragma comment(lib,"StdImage.lib") 
				#pragma message("Automatically linking with StdImage.lib") 
			#endif
		#endif
	#endif
#else 
	#define STDIMAGE_API
#endif

enum ATTACH_FILE_TYPE{
	AFT_HARRIS = 0,
	AFT_SIFT,
	AFT_SIFT_PYRM,
	AFT_SIFT_PYRMGRID,
	AFT_WALLIS,
	AFT_SRTM,
	AFT_END,
};

class STDIMAGE_API CStdImage :
	public CImageBase
{
public:
	CStdImage();
	virtual		~CStdImage();
	virtual bool Open(const char* lpstrPath,bool bLoadImage = true);
	virtual bool Create(const char* lpstrPath, int nCols, int nRows, int nBands,int nBits);
	const char*  GetImageName() const{ return m_strImageName; }
	void operator=(const CStdImage& object) {
		CStdImage::Clear();
		Open(object.GetImagePath());
		CopyGeoInformation(object);
		SetImageID(object.GetImageID());
	}

	void SetImageID(const char* lpstrID) {
		if (lpstrID == NULL) return;
		memset(m_strImageID, 0, sizeof(m_strImageID));
		if (strlen(lpstrID) > IMAGEID_SIZE ) memcpy(m_strImageID, lpstrID, IMAGEID_SIZE);
		else strcpy(m_strImageID, lpstrID);
	}
	void SetImageID(int id) { sprintf(m_strImageID, "%d", id); }
	const char* GetImageID() const { return m_strImageID; }
public:
#ifndef _NO_HARRIS
	int		ExtractHarris2PtFile(const char* lpstrFilePath, int stCol, int stRow, int nCols, int nRows, int nHrsWin , int nHrsInterval );
	int		ExtractHarris2PtFile(const char* lpstrFilePath, int window = 31, int interval = 51){
		return ExtractHarris2PtFile(lpstrFilePath, 0, 0, GetCols(), GetRows(),window, interval);
	}
	int		ReadHarrisFile(const char* lpstrFilePath, int stCol, int stRow, int nCols, int nRows);
#endif
#ifndef _NO_SIFT
	int		ExtractSift2PtFile(const char* lpstrFilePath, int interval = -1){
		return ExtractSift2PtFile(lpstrFilePath, 0, 0, GetCols(), GetRows(), interval);
	}
	int		ExtractSift2PtFile(const char* lpstrFilePath, int stCol, int stRow, int nCols, int nRows, int interval);
	int		ReadSiftFile(const char* lpstrFilePath, int stCol, int stRow, int nCols, int nRows);
	int		ExtractPyrmSift2PtFile(const char* lpstrFilePath);
	int		ReadPyrmSiftFile(const char* lpstrFilePath, int stCol, int stRow, int nCols, int nRows,int pyrmLvl);
	int		ReadPyrmSiftFile(const char* lpstrFilePath, int stCol, int stRow, int nCols, int nRows);

	int		ExtractPyrmGridSift2PtFile(const char* lpstrFilePath, int nGridCols, int nGridRows, int miSz);
	int		ReadPyrmGridSiftFile(const char* lpstrFilePath,int gridCol,int gridRow,int nPyrmLvl);
#endif
	static bool	Wallis(const char* lpstrImagePath, const char* lpstrWallisPath);
	bool	CreatePyrm(const char* lpstrPyrmPath, int stCol, int stRow, int nCols, int nRows, float zoomRate);
	// 	bool		 GeoArea2ImgRect(float* x, float* y, float* z, int nBufferSpace[3], int num, int extendSz,
	// 		float& xmin, float& ymin, float& xmax, float& ymax){
	// 		xmin = ymin = 10e8;	xmax = ymax = -10e8;
	// 		float tx, ty;
	// 		for (int i = 0; i < num; i++, x += nBufferSpace[0], y += nBufferSpace[1], z += nBufferSpace[2]){
	// 			Grd2Pho(*x, *y, *z, &tx, &ty);
	// 			if (xmin>tx) xmin = tx;  if (xmax < tx) xmax = tx;
	// 			if (ymin>ty) ymin = ty;  if (ymax < ty) ymax = ty;
	// 		}
	// 
	// 		xmax += extendSz;	ymax += extendSz;
	// 		xmin -= extendSz;	ymin -= extendSz;
	// 		if (ymax < 0 || xmax < 0 || xmin> GetCols() - 1 || ymin > GetRows() - 1) return false;
	// 		return true;
	// 	}

	//some error if exist org aop6
	virtual bool CompensatePhoAop6(double aop6[6]){
		char strPath[512];	strcpy(strPath, GetImagePath());	strcpy(strrchr(strPath, '.'), "_aop6_txt");
		FILE* fp = fopen(strPath, "w");	if (!fp) return false;
		for (int i = 0; i < 6; i++) fprintf(fp, "%lf\n", aop6[i]);
		fclose(fp);
		return true;
	}
#ifndef _NO_HARRIS
	static int			CheckHarrisFile(const char* lpstrHarrisFile, int window = 0, int interval = 0);
	static int			ExtractHarris2PtFile(CStdImage* img, const char* lpstrHarrisFilePath, int window, int interval);
	int		ExtractHarris2PtFile(int window = 31, int interval = 51){
		char strPath[512];	get_attach_file_path(GetImagePath(), AFT_HARRIS, strPath);
		return CStdImage::ExtractHarris2PtFile(this, strPath, window, interval);
	}
	int		ReadHarrisFile(int stCol, int stRow, int nCols, int nRows){
		char strPath[512];	get_attach_file_path(GetImagePath(), AFT_HARRIS, strPath);
		return ReadHarrisFile(strPath, stCol, stRow, nCols, nRows);
	}
#endif
#ifndef _NO_SIFT
//	static int			CheckPyrmGridSiftFile(const char* lpstrFilePath, int nGridCols, int nGridRows, int miSz);
	static int			CheckSiftFile(const char* lpstrSiftFile,int interval );
	static int			CheckPyrmSiftFile(const char* lpstrSiftFile);	
	static int ExtractSift2PtFile(CStdImage* img, const char* lpstrSiftFilePath, int interval);
	int		ExtractSift2PtFile(int interval){
		char strPath[512];	get_attach_file_path(GetImagePath(), AFT_SIFT, strPath);
		return CStdImage::ExtractSift2PtFile(this, strPath,interval);
	}
	int		ReadSiftFile(int stCol, int stRow, int nCols, int nRows){
		char strPath[512];	get_attach_file_path(GetImagePath(), AFT_SIFT, strPath);
		return ReadSiftFile(strPath, stCol, stRow, nCols, nRows);
	}
	static int ExtractPyrmSift2PtFile(CStdImage* img, const char* lpstrSiftFilePath);
	int		ExtractPyrmSift2PtFile(){
		char strPath[512];	get_attach_file_path(GetImagePath(), AFT_SIFT_PYRM, strPath);
		return CStdImage::ExtractPyrmSift2PtFile(this, strPath);
	}
	int		ReadPyrmSiftFile(int stCol, int stRow, int nCols, int nRows,int pyrmLvl){
		char strPath[512];	get_attach_file_path(GetImagePath(), AFT_SIFT_PYRM, strPath);
		return ReadPyrmSiftFile(strPath, stCol, stRow, nCols, nRows, pyrmLvl);
	}
	int		ReadPyrmSiftFile(int stCol, int stRow, int nCols, int nRows){
		char strPath[512];	get_attach_file_path(GetImagePath(), AFT_SIFT_PYRM, strPath);
		return ReadPyrmSiftFile(strPath, stCol, stRow, nCols, nRows);
	}
	int		GetPyrmPtNum(int pyrmLvl){
		return m_nPyrmPtNum[pyrmLvl];
	}
	int		GetAddupPyrmPtNum(int edPyrmLvl){
		int sz = 0;
		for (int i = 0; i < edPyrmLvl; i++)	sz += m_nPyrmPtNum[i];
		return sz;
	}
#endif
// 	static int	RemoveFeaPoint_PyrmSiftFile(const char* lpstrSrcFile, const char* lpstrDstFile, CStdImage* maskArray, BYTE* Vmin, BYTE* Vmax, int sz, int winSz);
// 	int		RemoveFeaPoint_PyrmSiftFile(CStdImage* maskArray, BYTE* Vmin, BYTE* Vmax, int sz, int winSz);
// 	int		RemoveFeaPoint_PyrmSiftFile(CStdImage* maskArray, int sz){
// 		BYTE* Vmin = new BYTE[sz];	memset(Vmin, 255, sz);
// 		BYTE* Vmax = new BYTE[sz];	memset(Vmax, 255, sz);
// 
// 		sz = RemoveFeaPoint_PyrmSiftFile(maskArray, Vmin, Vmax, sz, 10);
// 
// 		delete Vmin;
// 		delete Vmax;
// 
// 		return sz;
// 	}
// 	FeaPt*		GetPyrmFeaBuf(int pyrmLvl){
// 		return m_FeaPt.GetData() + GetAddupPyrmPtNum(pyrmLvl);
// 	}
// 	BYTE*		GetPyrmSiftDescrptorsBuf(int pyrmLvl){
// 		return m_descriptors.GetData() + GetAddupPyrmPtNum(pyrmLvl) * 128;
// 	}
// 	char*		GetPyrmFeaName(int pyrmLvl){
// 		return *(m_feaName + GetAddupPyrmPtNum(pyrmLvl));
// 	}

// 	static int ExtractPyrmGridSift2PtFile(CStdImage* img, const char* lpstrSiftFilePath, int nGridCols, int nGridRows, int miSz){
// 		int sz = CheckPyrmGridSiftFile(lpstrSiftFilePath, nGridCols, nGridRows, miSz);
// 		if (sz < 1){
// 			return img->ExtractPyrmGridSift2PtFile(lpstrSiftFilePath, nGridCols, nGridRows, miSz);
// 		}
// 		return sz;
// 	}
// 	int		ExtractPyrmGridSift2PtFile(int nGridCols, int nGridRows, int miSz){
// 		char strPath[512];	get_attach_file_path(GetImagePath(), AFT_SIFT_PYRMGRID, strPath);
// 		return CStdImage::ExtractPyrmGridSift2PtFile(this, strPath, nGridCols, nGridRows, miSz);
// 	}
// 	int		ReadPyrmGridSiftFile(int gridCol, int gridRow, int nPyrmLvl){
// 		char strPath[512];	get_attach_file_path(GetImagePath(), AFT_SIFT_PYRMGRID, strPath);
// 		return ReadPyrmGridSiftFile(strPath, gridCol, gridRow, nPyrmLvl);
// 	}
// 	bool	ReadPyrmGridSiftFileHdr(const char* lpstrFilePath, int* nGridCols, int* nGridRows, int* miSz);
// 	bool	ReadPyrmGridSiftFileHdr(int* nGridCols, int* nGridRows, int* miSz){
// 		char strPath[512];	get_attach_file_path(GetImagePath(), AFT_SIFT_PYRMGRID, strPath);
// 		return ReadPyrmGridSiftFileHdr(strPath, nGridCols, nGridRows, miSz);
// 	}

	static bool	CheckWallisImage(const char* lpstrImagePath, const char* lpstrWallisImagePath);
	static bool Wallis(CStdImage* img, const char* lpstrWallisImagePath);
	bool	Wallis(char* strWallisFile){
		char strPath[512];	get_attach_file_path(GetImagePath(), AFT_WALLIS, strPath);
		if (CStdImage::Wallis(this, strPath)){
			if (strWallisFile) strcpy(strWallisFile, strPath);
			return true;
		}
		return false;
	}
public:
#ifndef _NO_HARRIS
	static bool CvtHrs2Txt(const char* lpstrHrsFile, const char* lpstrTxtPath, int nRows);
	bool CvtHrs2Txt(const char* lpstrTxtPath){
		char strPath[512];	get_attach_file_path(GetImagePath(), AFT_HARRIS, strPath);
		return CvtHrs2Txt(strPath, lpstrTxtPath, GetRows());
	}
	bool CvtHrs2Txt(){
		char strPath[512];	get_attach_file_path(GetImagePath(), AFT_HARRIS, strPath);
		char strOut[512];	strcpy(strOut, strPath);	strcat(strOut, ".txt");
		return CvtHrs2Txt(strPath, strOut, GetRows());
	}
#endif
#ifndef _NO_SIFT
	static bool CvtSift2Txt(const char* lpstrSiftFile, const char* lpstrTxtPath, int nRows);
	bool CvtSift2Txt(const char* lpstrTxtPath){
		char strPath[512];	get_attach_file_path(GetImagePath(), AFT_SIFT, strPath);
		return CvtSift2Txt(strPath, lpstrTxtPath,GetRows());
	}
	bool CvtSift2Txt(){
		char strPath[512];	get_attach_file_path(GetImagePath(), AFT_SIFT, strPath);
		char strOut[512];	strcpy(strOut, strPath);	strcat(strOut, ".txt");
		return CvtSift2Txt(strPath, strOut, GetRows());
	}
#endif
// 	static bool CvtPyrmSift2Txt(const char* lpstrSiftFile, const char* lpstrTxtPath, int nRows, int nPyrmLvl);
// 	bool CvtPyrmSift2Txt(const char* lpstrTxtPath, int nPyrmLvl){
// 		char strPath[512];	get_attach_file_path(GetImagePath(), AFT_SIFT_PYRM, strPath);
// 		return CvtPyrmSift2Txt(strPath, lpstrTxtPath, GetRows(), nPyrmLvl);
// 	}
public:

	char*	get_pt_name(int idx);
	float*	GetPtLocation();
	int		GetFeaPtNum();
	BYTE*	GetSiftDescriptors();
	float*	GetSiftFeature();

	void	ClearPtBuffer() { ResizePtBuffer(0); }
	static int GetPointNameSize();
public:
	static bool			set_workspace_path(const char* lpstrWsPath);
	static const char*	get_workspace_path() { return m_strWsPath; }
	static void			get_attach_file_dir(const char* lpstrImagePath, ATTACH_FILE_TYPE type, char* strPath);
	static void			get_attach_file_path(const char* lpstrImagePath, ATTACH_FILE_TYPE type, char* strPath);
	static void			create_attach_dir();
	void				RemoveAttachFile();
protected:
	virtual void Clear();
	void			ResizePtBuffer(int sz);
protected:
	char		m_strImageName[128];

	char		m_strImageID[IMAGEID_BUFFER_SIZE];

	void*		m_ptLocation;
	void*		m_ptName;
	void*		m_descriptors;
	void*		m_ptFea;

	static char	m_strWsPath[512];
	int			m_nPyrmPtNum[3];
};

class COrientation
{
public:
	virtual ~COrientation() {};
	virtual const char*  GetOriTag()const = 0 ;
	virtual const char*  GetOriInfo()const = 0 ;
	virtual bool	 SetOriInfo(const char* lpstrInfo) = 0;
	virtual void PhoZ2Grd(const double* x, const  double* y, const  double* gz, int nPtNum, double *X, double *Y) = 0;
	virtual void Grd2Pho(const double* X, const  double* Y, const double* Z, int nPtNum, double* x, double* y) = 0;
	virtual void PhoZ2Grd(float x, float y,  float gz, float *X, float *Y) = 0;
	virtual void Grd2Pho(float X, float Y,  float Z, float* x, float* y) = 0;
	virtual bool CompensatePhoAop6(double aop6[6]) { return false; };
private:

};

class STDIMAGE_API CGeoImage : public CStdImage
{
public:
	CGeoImage();
	virtual ~CGeoImage();
	void operator= (const CGeoImage& object);

	bool	InitTFWImage(const char* lpstrImagePath);

	bool	InitFrmImage(const char* lpstrImagePath, const char* lpstrXmlPath);

	bool	InitRpcImage(const char* lpstrImagePath, const char* lpstrRpcPath);

	bool	IsGeoInit() const { return m_pOriInfo ? true : false; }
	const char*		GetOriTag() const{ return m_pOriInfo->GetOriTag(); }
	void			GetOriInfo(char* oriInfo, bool bTag) const;
	//	const char*		GetOriInfo() { return m_pOriInfo->GetOriInfo(); }
	bool	InitFromOriInfo(const char* lpstrImagePath, const char* lpstrOriInfo);

	bool			SetGeoSys4WKT(const char* pszWKT){
		return m_geoCvt.Import4WKT(pszWKT);
	}
	bool			SetGeoSys4GCD(const char* pszGCD){
		return m_geoCvt.Import4GCD(pszGCD);
	}
	const char*		GetGeoSysWKT() const { return m_geoCvt.GetWKT(); }

	void PhoZ2Grd(const double* x, const  double* y, const  double* gz, int nPtNum, double *X, double *Y){
		m_pOriInfo->PhoZ2Grd(x, y, gz, nPtNum, X, Y);
	}
	void PhoZ2LB(const double* x, const  double* y, const  double* gz, int nPtNum, double *lon, double *lat){
		PhoZ2Grd(x, y, gz, nPtNum, lon, lat);
		m_geoCvt.Cvt2LBH(nPtNum, lon, lat);
	}
	void Grd2Pho(const double* X, const double* Y, const  double* Z, int nPtNum, double* x, double* y){
		m_pOriInfo->Grd2Pho(X, Y, Z, nPtNum, x, y);
	}
	void LBH2Pho(const double* lon, const double* lat, const  double* h, int nPtNum, double* x, double* y);
	void PhoZ2Grd(float x, float y, float gz, float *X, float *Y){
		m_pOriInfo->PhoZ2Grd(x, y, gz, X, Y);
	}
	void Grd2Pho(float X, float Y, float Z, float* x, float* y) {
		m_pOriInfo->Grd2Pho(X, Y, Z, x, y);
	}

	void GetGeoRgn(float avrH, double x[4], double y[4]);
	void GetGeoRgn_LBH(float avrH, double x[4], double y[4]){
		GetGeoRgn(avrH, x, y);
		m_geoCvt.Cvt2LBH(4, x, y);
	}
	void GetGeoRgn(float avrH, int stCol, int stRow, int edCol, int edRow, double* xmin, double* ymin, double* xmax, double* ymax);
	float	GetGsd(bool bCvt2LBH = true);

	//	friend bool			frame_epi_image(CGeoImage* imgL, CGeoImage* imgR,const char* lpstrLEpiImgPath,const char* lpstrREpiImgPath );
protected:

	bool	InitFrm(const char* lpstrFrameXml);

	bool	InitRpc(const char* lpstrRpcPath);
	void	ResetOriInfo(COrientation* pOriInfo = NULL){
		if (m_pOriInfo) {
			delete[] m_pOriInfo;	 m_pOriInfo = NULL;
		}
		m_pOriInfo = pOriInfo;
	}
	virtual void Clear();
private:
	COrientation *m_pOriInfo;
	//	CGeoTransferBase*	m_pGeoCvt;
	CGeoTransferBase	m_geoCvt;
};

class STDIMAGE_API CDUImage : public CGeoImage
{
public:
	CDUImage();
	virtual ~CDUImage();
	// 	bool	ExtractSRTM(const char* lpstrSTRMPath){
	// 		return CDUImage::ExtractSRTM(this, lpstrSTRMPath);
	// 	}
	bool	ExtractSRTM()
	{
		return SRTM( NULL);
	}
	bool	LoadSRTM(const char* lpstrDemFile);
	bool	LoadSRTM(){
		char strPath[512];	get_attach_file_path(GetImagePath(), AFT_SRTM, strPath);
		return LoadSRTM(strPath);
	}
//	bool	CvtDem2ImageGeoSys(){}
	bool	ixy_to_LBH(double ix, double iy, double* x, double* y, double* z);
public:
	static	bool	ExtractSRTM(CGeoImage* pImg, const char* lpstrSTRMPath);
	static  bool	CheckSRTMFile(const char* lpstrSRTMPath);
	bool			SRTM(char* strSRTMPath){
		char strPath[512];	get_attach_file_path(GetImagePath(), AFT_SRTM, strPath);
		if (CDUImage::SRTM(this, strPath)){
			if (strSRTMPath) strcpy(strSRTMPath, strPath);
			return true;
		}
		return false;
	}
	static  bool	SRTM(CGeoImage* pImg, const char* lpstrSRTMPath){
		if (CheckSRTMFile(lpstrSRTMPath) || ExtractSRTM(pImg, lpstrSRTMPath)){
			return true;
		}
		return false;
	}
private:
	class CStdDem*		m_dem;
	double				m_center_H;
};

#endif // StdImage_h__LX_whu_2015_3_10

