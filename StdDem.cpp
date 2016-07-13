#include "StdDem.h"
#include "PrjLog.hpp"
#include "../imagebase/package_gdal.h"
#include "DPGRID/SpDem.hpp"

#define strlwr _strlwr

#define MAX_VAL(maxval,array,n)  { maxval = *(array);	for (int i = 1; i < n; i++) if (*(array+i)>maxval) maxval = *(array+i); }
#define MIN_VAL(minval,array,n)  { minval = *(array);	for (int i = 1; i < n; i++) if (*(array+i)<minval) minval = *(array+i); }

class CDemBase
{
public:
	CDemBase(){		
	}
	virtual ~CDemBase(){

	}
	static bool IsGDAL(const char* lpstrDemFile){
		const char* strGDALDemTag[] = { "img", "tif", "tiff" };
		const char* strSpDemTag[] = { "bil", "dem", "bem", "dsm", "gem" };
		if (strlen(lpstrDemFile) < 3) return false;
		char suffix[10];	const char* pS = strrchr(lpstrDemFile, '.');
		if (!pS) { LogPrint(ERR_FILE_TYPE, "%s is unknown dem file type!", lpstrDemFile);  return false; }
		strcpy(suffix, pS + 1);
		strlwr(suffix);
		bool bGDAL = false;
		for (int i = 0; i < sizeof(strGDALDemTag) / sizeof(const char*); i++){
			if (!strcmp(strGDALDemTag[i], suffix)) {
				bGDAL = true;
			}
		}
		return bGDAL;
	}
	bool	Save2File(const char* lpstrPathName);
	virtual	int			GetCols() = 0;
	virtual	int			GetRows() = 0;
	virtual int			GetBits() = 0;
	virtual	float	GetMedianZ() = 0;
	virtual float	GetMinimumZ() = 0;
	virtual	float	GetMaximumZ() = 0;
	virtual void		GetGeoXY(double col, double row, double* x, double* y) = 0;
	virtual	bool		CreateDem(const char* lpstrPathName, int nCols, int nRows, const char* lpstrWKT, double* paGeoTransform) = 0;
	virtual bool		WriteDemRow(float *pBuf, int row) = 0;
	virtual void	SetDemZVal(int column, int row, double z) = 0;
	virtual void*	GetBuf() = 0;

	inline	void		GetDemZValue(int column, int row, float* DemZValue){ *DemZValue = GetDemZVal(column, row); };
	virtual	float		GetDemZVal(int column, int row) = 0;
//	float						GetDemZValue(double x, double y, bool bLi = true);
	inline void					GetDemZValue(double x, double y, double *z){ *z = GetDemZValue(x, y); };
	inline void					GetDemZValue(double x, double y, float  *z){ *z = GetDemZValue(x, y); };
	virtual float		GetDemZValue(double x, double y) = 0;
	float GetDemZValue4LB(double lon, double lat){
		if (m_geoSys.IsProjected()){
			m_geoSys.LBH2Prj(1, &lon, &lat);
		}
		return GetDemZValue(lon, lat);
	}
	virtual	void		GetGlobalXYZValue(int column, int row, double * DemXValue, double * DemYValue, float * DemZValue) = 0;
	virtual	void		GetGlobalXYZValue(int column, int row, double * DemXValue, double * DemYValue, double * DemZValue) = 0;

	bool			SetGeoSys4WKT(const char* pszWKT){
		return m_geoSys.Import4WKT(pszWKT);
	}
	bool			SetGeoSys4GCD(const char* pszGCD){
		return m_geoSys.Import4GCD(pszGCD);
	}
	const char*			GetWKT(){
		return m_geoSys.GetWKT();
	}
	CGeoTransferBase*  GetGeoTransferPtr(){
		return &m_geoSys;
	}
	virtual void		GetGeoTransform(double* paTransform) = 0;
	void GetGeoRgn(int stCol, int stRow, int edCol, int edRow, double x[4],double y[4]){
		double c[4] = { stCol, stCol, edCol, edCol }, r[4] = { stRow, edRow, stRow, edRow };
		for (int i = 0; i < 4; i++){
			GetGeoXY(c[i], r[i], x + i, y + i);
		}
	}
	void GetGeoRgn(int stCol, int stRow, int edCol, int edRow, double* xmin, double* ymin, double* xmax, double* ymax){
		double x[4], y[4];
		GetGeoRgn(stCol, stRow, edCol, edRow, x, y);
		MIN_VAL(*xmin, x, 4);	MIN_VAL(*ymin, y, 4);
		MAX_VAL(*xmax, x, 4);	MAX_VAL(*ymax, y, 4);
	}
	bool        IsGeographic() const{
		return m_geoSys.IsGeographic();
	}
	bool        IsProjected() const{
		return m_geoSys.IsProjected();
	}
private:
	CGeoTransferBase	m_geoSys;
};

class CImgDem	: public CDemBase,public CImageBase
{
public:
	CImgDem(){

	}
	virtual ~CImgDem(){

	}
	virtual bool Open(const char* lpstrPath){
		if (!CImageBase::Open(lpstrPath)) return false;
		SetGeoSys4WKT(GetProjectionRef());
		return true;
	}
	bool Load4File(const char* lpstrPathName){
		if (!CImageBase::Open(lpstrPathName)) return false;
		SetGeoSys4WKT(GetProjectionRef());
//		malloc_data_buf(GetCols(), GetRows(), GetBands(), GetByte());
//		GetBandVal(0, 0, 0);
		return true;
	}
	static bool Save2File(const char* lpstrPathName, void* pBuf,int nCols, int nRows, int nBands, int nBits,const char* lpstrWkt,double* aop ){
		CImageBase imgW;
		if (!imgW.Create(lpstrPathName, nCols, nRows, nBands, nBits)) return false;
		imgW.SetGeoTransform(aop);	imgW.SetProjectionRef(lpstrWkt);
		imgW.Write(pBuf, 0, 0, nCols,nRows, CImageBase::SS_BSQ);
		imgW.Close();
		return true;
	}
	virtual void* GetBuf() { return GetBandBuf(0, 0, 0); }
	virtual int GetBits() { return CImageBase::GetBits(); }
	virtual void SetDemZVal(int column, int row, double z){
		SetBandBufVal(column, row,0, z);
	}
	virtual bool CreateDem(const char* lpstrPathName, int nCols, int nRows,const char* lpstrWKT, double* paGeoTransform)
	{
		if (!Create(lpstrPathName, nCols, nRows, 1, 32)) return false;
		SetProjectionRef(lpstrWKT);
		SetGeoTransform(paGeoTransform);
		return true;
	}
	virtual int		GetCols() { return CImageBase::GetCols(); }
	virtual int		GetRows() { return CImageBase::GetRows(); }
	virtual void	GetGeoTransform(double* paTransform)
	{
		CImageBase::GetGeoTransform(paTransform);
	}
	virtual	float	GetDemZVal(int column, int row){
		return (float)CImageBase::GetBandVal(column, row, 0);
	}
	virtual float	GetDemZValue(double x, double y){
		return (float)CImageBase::GetBandVal4Geo(x, y, 0);
	}
	virtual void	GetGlobalXYZValue(int column, int row, double * DemXValue, double * DemYValue, float * DemZValue)
	{
		*DemXValue = column;	*DemYValue = row;
		CImageBase::GetGeoXY(DemXValue,DemYValue);
		*DemZValue = GetDemZVal(column, row);
	}
	virtual	void		GetGlobalXYZValue(int column, int row, double * DemXValue, double * DemYValue, double * DemZValue)
	{
		*DemXValue = column;	*DemYValue = row;
		CImageBase::GetGeoXY(DemXValue, DemYValue);
		*DemZValue = CImageBase::GetBandVal(column, row, 0);
	}
	virtual void GetGeoXY(double col, double row, double* x, double* y){
		*x = col;	*y = row;
		CImageBase::GetGeoXY(x, y);
	}
	virtual bool WriteDemRow(float *pBuf, int row){
		return CImageBase::Write(pBuf, 0, row, GetCols(), 1);
	}
	virtual	float	GetMedianZ(){
		double dfMin, dfMax, dfMean, adfStdDev;
		if( !GetStatistics(0, &dfMin, &dfMax, &dfMean, &adfStdDev)) return 0;
		return (float)dfMean;
	}
	virtual float	GetMinimumZ(){
		double dfMin, dfMax, dfMean, adfStdDev;
		if (!GetStatistics(0, &dfMin, &dfMax, &dfMean, &adfStdDev)) return 0;
		return (float)dfMin;
	}
	virtual	float	GetMaximumZ(){
		double dfMin, dfMax, dfMean, adfStdDev;
		if (!GetStatistics(0, &dfMin, &dfMax, &dfMean, &adfStdDev)) return 0;
		return (float)dfMax;
	}
private:

};

class CSpDemTmp : public CDemBase,public CSpDem
{
public:
	CSpDemTmp(){

	}
	virtual ~CSpDemTmp(){

	}
	virtual void* GetBuf() { return m_pGrid; }
	virtual int	  GetBits() { return 32; }
	static bool Save2File(const char* lpstrPathName, void* pBuf, int nCols, int nRows, int nBands, int nBits, const char* lpstrWkt, double* aop){
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
		char data_type[10];	
		if (nBits == 8) strcpy(data_type, "char"); else if (nBits == 16) strcpy(data_type, "short"); 
		else if (nBits == 64) strcpy(data_type, "double"); else strcpy(data_type, "float");

		GeoTransform2xyk(aop, false, nCols, nRows, &demHdr.startX, &demHdr.startY, &demHdr.kapa, &demHdr.intervalX, &demHdr.intervalY);
		sprintf(str, "DEMBIN VER:1.0 Supresoft Inc. Format: Tag(512Bytes),minX(double),minY(double),Kap(double),Dx(double),Dy(double),Cols(int),Rows(int),Z(%s)... %.4lf %.4lf %.4lf %.12f %.12f %d %d",
			data_type,demHdr.startX, demHdr.startY, demHdr.kapa, demHdr.intervalX, demHdr.intervalY, demHdr.column, demHdr.row);
		HANDLE hFile = ::CreateFile(lpstrPathName, GENERIC_READ | GENERIC_WRITE, FILE_SHARE_READ, NULL, CREATE_ALWAYS, 0, NULL);
		if (!hFile || hFile == INVALID_HANDLE_VALUE){ return FALSE; }
		::WriteFile(hFile, str, 512, &rw, NULL);
		::WriteFile(hFile, &demHdr, sizeof(demHdr), &rw, NULL);
		::WriteFile(hFile, pBuf, nCols*nRows*nBits/8, &rw, NULL);
		::CloseHandle(hFile);

		strcpy(str, lpstrPathName);	strcpy(strrchr(str, '.'), ".prj");
		FILE* fp = fopen(str, "w");	if (fp) { fprintf(fp, "%s", lpstrWkt); fclose(fp); }
		return true;
	}
	static void  GeoTransform2xyk(double* paGeoTransform, double* x0, double* y0, double* kap, double* gsdX, double* gsdY, bool bCorner, int nCols, int nRows){
		if (bCorner){ 
			paGeoTransform[0] += paGeoTransform[1] / 2; 
			paGeoTransform[3] += paGeoTransform[5] / 2;
		}// pixel corner to center. (the start pos in vz is center of pixel, so must be sub half of dx and dy) 
		
		*kap = atan2(paGeoTransform[2], paGeoTransform[1]);
		double cosKap = cos(*kap);	double sinKap = sin(*kap);
		if (cosKap == 0){
			*gsdX = paGeoTransform[4] / sinKap;
			if(gsdY) *gsdY = paGeoTransform[2] / sinKap;
		}
		else{
			*gsdX = paGeoTransform[1] / cosKap;
			if (gsdY) *gsdY = -paGeoTransform[5] / cosKap;
		}
		*x0 = (paGeoTransform[5] >= 0) ? (paGeoTransform[0]) : (paGeoTransform[0] + sinKap* (gsdY ? *gsdY : *gsdX)*(nRows - 1));
		*y0 = (paGeoTransform[5] >= 0) ? (paGeoTransform[3]) : (paGeoTransform[3] - cosKap* (gsdY ? *gsdY : *gsdX)*(nRows - 1));
	}
	static void xyk2GeoTransform(double x0, double y0, double kap, double gsdX, double gsdY, bool bCorner,int nCols,int nRows,double* paGeoTransform){
		double cosKap, sinKap;
		cosKap = cos(kap); sinKap = sin(kap);
		paGeoTransform[1] = cosKap*gsdX; //  cosA*dx
		paGeoTransform[2] = sinKap*gsdY; //  sinA*dy
		paGeoTransform[4] = sinKap*gsdX; //  sinA*dx
		paGeoTransform[5] = -cosKap*gsdY; // -cosA*dy
		paGeoTransform[0] = (paGeoTransform[5] >= 0) ? (x0) : (x0 - sinKap*gsdY*(nRows - 1));
		paGeoTransform[3] = (paGeoTransform[5] >= 0) ? (y0) : (y0 + cosKap*gsdY*(nRows - 1));
		if (bCorner){ paGeoTransform[0] -= paGeoTransform[1] / 2; paGeoTransform[3] -= paGeoTransform[5] / 2; }// pixel center to corner.
	}
	static void GeoTransform2xyk(double paGeoTransform[6], bool bCorner, int nCols, int nRows, double* x0, double* y0, double* kap, double* gsdX, double* gsdY){
		double Dx, Dy, Rx, Ry, Ex, Ny, cosKap, sinKap;
		Dx = paGeoTransform[1];	Rx = paGeoTransform[2];
		Ry = paGeoTransform[4];	Dy = paGeoTransform[5];
		Ex = paGeoTransform[0];	Ny = paGeoTransform[3];
		if (bCorner){ Ex += Dx / 2; Ny += Dy / 2; }
		*kap = atan2(Rx, Dx); cosKap = cos(*kap); sinKap = sin(*kap);
		if (cosKap == 0){
			*gsdX = Ry / sinKap;
			if (gsdY) *gsdY = Rx / sinKap;
		}
		else{
			*gsdX = Dx / cosKap;
			if (gsdY) *gsdY = -Dy / cosKap;
		}

		*x0 = (Dy >= 0) ? (Ex) : (Ex + sinKap* (gsdY ? *gsdY : *gsdX)*(nRows - 1));
		*y0 = (Dy >= 0) ? (Ny) : (Ny - cosKap* (gsdY ? *gsdY : *gsdX)*(nRows - 1));
	}
	virtual int		GetCols() { return CSpDem::GetCols(); }
	virtual int		GetRows() { return CSpDem::GetRows(); }
	virtual bool CreateDem(const char* lpstrPathName, int nCols, int nRows, const char* lpstrWKT, double* paGeoTransform)
	{
		SPDEMHDR hdr;
		hdr.column = nCols;	hdr.row = nRows;
		GeoTransform2xyk(paGeoTransform, &hdr.startX, &hdr.startY, &hdr.kapa, &hdr.intervalX, &hdr.intervalY, false, nCols, nRows);
		if (!Open(lpstrPathName, modeCreate, &hdr)) return false;
		char strPath[512];	strcpy(strPath, lpstrPathName);	
		char* pS = (char*)strrchr(strPath, '.');	if (!pS) pS = strPath + strlen(strPath);
		strcpy(pS, ".prj");
		FILE* fp = fopen(strPath, "w");	
		if (fp){
			fputs(lpstrWKT, fp);
			fclose(fp);
		}
		return true;
	}
	virtual void	GetGeoTransform(double* paTransform)
	{
		xyk2GeoTransform(m_demHdr.startX, m_demHdr.startY, m_demHdr.kapa, m_demHdr.intervalX, m_demHdr.intervalY, false, m_demHdr.column, m_demHdr.row, paTransform);
	}
	virtual	float	GetDemZVal(int column, int row){
		return CSpDem::GetDemZVal(column, m_demHdr.row - 1 - row);
	}
	virtual float	GetDemZValue(double x, double y){
		return CSpDem::GetDemZValue(x, y);
	}
	virtual void	GetGlobalXYZValue(int column, int row, double * DemXValue, double * DemYValue, float * DemZValue)
	{
		CSpDem::GetGlobalXYZValue(column, m_demHdr.row - 1 - row, DemXValue, DemYValue, DemZValue);
	}
	virtual void	GetGlobalXYZValue(int column, int row, double * DemXValue, double * DemYValue, double * DemZValue){
		CSpDem::GetGlobalXYZValue(column, m_demHdr.row - 1 - row, DemXValue, DemYValue, DemZValue);
	}
	virtual void GetGeoXY(double col, double row, double* x, double* y)
	{
		double tempX = col*m_demHdr.intervalX; 
		double tempY = (m_demHdr.row - 1 - row)*m_demHdr.intervalY;;
		LocalToGlobal(&tempX, &tempY, m_demHdr.kapa);
		*x = tempX + m_demHdr.startX;  *y = tempY + m_demHdr.startY;	
	}
	
	virtual bool		WriteDemRow(float *pBuf, int row){
		return WriteRow(pBuf, m_demHdr.row - 1 - row)?true:false;
	}
	virtual	float	GetMedianZ(){
		return GetMidZ();
	}
	virtual float	GetMinimumZ(){
		return GetMinZ();
	}
	virtual	float	GetMaximumZ(){
		return GetMaxZ();
	}
	virtual void SetDemZVal(int column, int row, double z){
		SetDemZValue(column, row, (float)z);
	}
private:

};

CStdDem::CStdDem()
{
	m_pDem = NULL;
}


CStdDem::~CStdDem()
{
	Clear();
}

void CStdDem::Clear()
{
	if (m_pDem) {
		delete m_pDem; m_pDem = NULL;
	}
}

bool CStdDem::SetGeoSys4WKT(const char* pszWKT)
{
	return m_pDem->SetGeoSys4WKT(pszWKT);
}
bool CStdDem::SetGeoSys4GCD(const char* pszGCD)
{
	return m_pDem->SetGeoSys4GCD(pszGCD);
}

bool CStdDem::Load4File(const char* lpstrDemFile)
{
	Clear();
	
	bool bGDAL = CDemBase::IsGDAL(lpstrDemFile);

	if (bGDAL){
		CImgDem* pDem = new CImgDem;
		if (!pDem->Load4File(lpstrDemFile)) {
			delete pDem; return false;
		}
		m_pDem = pDem;
		return true;
	}
	CSpDemTmp* pDem = new CSpDemTmp;
	if ( !pDem->Load4File(lpstrDemFile) ){
		delete pDem;
		return false;
	}
	m_pDem = pDem;

	char strPrjFile[1024];	strcpy(strPrjFile, lpstrDemFile);
	strcpy(strrchr(strPrjFile, '.'), ".prj");
	if (IsExist(strPrjFile)){
		FILE* fp = fopen(strPrjFile, "r");
		fseek(fp, 0, SEEK_END);	int len = ftell(fp);
		rewind(fp);
		fread(strPrjFile, len, 1, fp);
		fclose(fp);
		CGeoTransferBase geoCvt;
		if (geoCvt.Import4WKT(strPrjFile)){
			m_pDem->SetGeoSys4WKT(strPrjFile);
		}
		
	}
	return true;
}

bool CStdDem::Save2File(const char* lpstrDemFile){
	return m_pDem->Save2File(lpstrDemFile);
}

int CStdDem::GetCols()
{
	return m_pDem->GetCols();
}
int CStdDem::GetRows()
{
	return m_pDem->GetRows();
}
float CStdDem::GetDemZVal(int column, int row)
{
	return m_pDem->GetDemZVal(column, row);
}

bool CStdDem::GetDemRgn(double x[4], double y[4])
{
	m_pDem->GetGeoRgn(0, 0, GetCols() - 1, GetRows() - 1, x, y);
	return true;
}

float CStdDem::GetDemZValue(double x, double y)
{
	return m_pDem->GetDemZValue(x, y);
}
float CStdDem::GetDemZValue4LB(double lon, double lat)
{
	return m_pDem->GetDemZValue4LB(lon, lat);
}
void CStdDem::GetGlobalXYZValue(int column, int row, double * DemXValue, double * DemYValue, float * DemZValue)
{
	m_pDem->GetGlobalXYZValue(column, row, DemXValue, DemYValue, DemZValue);
}

void CStdDem::GetGlobalXYZValue(int column, int row, double * DemXValue, double * DemYValue, double * DemZValue)
{
	m_pDem->GetGlobalXYZValue(column, row, DemXValue, DemYValue, DemZValue);
}

void CStdDem::SetDemZValue(int column, int row, double z){
	m_pDem->SetDemZVal(column, row, z);
}

float CStdDem::GetMaximumZ()
{
	return m_pDem->GetMaximumZ();
}

float CStdDem::GetMedianZ()
{
	return m_pDem->GetMedianZ();
}

float CStdDem::GetMinimumZ()
{
	return m_pDem->GetMinimumZ();
}

//combine all condition
bool CStdDem::CvtFormat(const char* lpstrPath, double xgsd, double ygsd){
	double xmin, ymin, xmax, ymax;
	m_pDem->GetGeoRgn(0, 0, m_pDem->GetCols() - 1, m_pDem->GetRows() - 1, &xmin, &ymin, &xmax, &ymax);
	int nCols_new = int((xmax - xmin) / xgsd);
	int nRows_new = int( (ymax - ymin) / ygsd);

	CDemBase* pDstDem = NULL;
	if (CDemBase::IsGDAL(lpstrPath)){
		pDstDem = new CImgDem;
	}
	else{
		pDstDem = new CSpDemTmp;
	}
	double paTransform[6];	m_pDem->GetGeoTransform(paTransform);
	if (!pDstDem->CreateDem(lpstrPath, nCols_new, nRows_new, m_pDem->GetWKT(), paTransform)) return false;

	float* pRows = new float[nCols_new + 8];
	float ro, co;
	float br, bc;
	br = m_pDem->GetCols()*1.0f / nCols_new;
	bc = m_pDem->GetRows()*1.0f / nRows_new;
	ro = 0; 
	for (int r = 0; r < nRows_new; r++, ro += br){
		float* pR = pRows;	co = 0;
		for (int c = 0; c < nCols_new; c++, co += bc){
			*pR = GetDemZVal(int(co), int(ro));	pR++;
		}
		pDstDem->WriteDemRow(pRows, r);
	}
	delete pRows;
	delete pDstDem;
	return true;
}
#define WGS84B		6356752.31424517929	/* Semi-minor axis of ellipsoid            WGS84 */
#define PI          3.1415926535
//precious
bool CStdDem::CvtGeoSys(const char* lpstrGcdPath, const char* lpstrDstDemPath,double xgsd,double ygsd){
	printf("----cvt to %s---\n", lpstrDstDemPath);
	double paTransform[6];	m_pDem->GetGeoTransform(paTransform);
	CGeoTransferBase  dstSys;
	if (xgsd <= 0) {
		double x0, y0, kap;
		CSpDemTmp::GeoTransform2xyk(paTransform, &x0, &y0,&kap, &xgsd, &ygsd, false, m_pDem->GetCols(), m_pDem->GetRows());
	}
	if (!dstSys.Import4GCD(lpstrGcdPath)) return false;
	if (dstSys.IsSameSpatialReference(m_pDem->GetGeoTransferPtr()))
		return CvtFormat(lpstrDstDemPath, xgsd, ygsd);

	CCoordinationTransferBase coordCvt;
	if (!coordCvt.init(*(m_pDem->GetGeoTransferPtr()), dstSys) ) return false;
	if (dstSys.IsProjected()){
		if (xgsd < 0.1){
			xgsd = xgsd*(PI*WGS84B) / 180;
			ygsd = xgsd;
		}
	}else if (dstSys.IsGeographic()){
		if (xgsd>0.1) {
			xgsd = xgsd *(180 / (PI*WGS84B));
			ygsd = xgsd;
		}
	}
	
	double xmin, ymin, xmax, ymax;
	double x[4], y[4];
	m_pDem->GetGeoRgn(0, 0, m_pDem->GetCols() - 1, m_pDem->GetRows() - 1, x, y);
	coordCvt.Src2Dst(4, x, y);
	MIN_VAL(xmin, x, 4);	MIN_VAL(ymin, y, 4);
	MAX_VAL(xmax, x, 4);	MAX_VAL(ymax, y, 4);

	printf("area=[%lf,%lf]->[%lf,%lf]\n", xmin, ymin, xmax, ymax);
	printf("xgsd= %lf\tygxd= %lf\n", xgsd, ygsd);

	int nCols_new = int((xmax - xmin) / xgsd);
	int nRows_new = int((ymax - ymin) / ygsd);
	printf("col=\t%d\trow=\t%d\n", nCols_new, nRows_new);

	CDemBase* pDstDem = NULL;
	if (CDemBase::IsGDAL(lpstrDstDemPath)){
		pDstDem = new CImgDem;
	}
	else{
		pDstDem = new CSpDemTmp;
	}
		
	paTransform[0] = xmin;	paTransform[1] = xgsd;	paTransform[2] = 0;
	paTransform[3] = ymax;	paTransform[4] = 0;		paTransform[5] = -ygsd;
	if (!pDstDem->CreateDem(lpstrDstDemPath, nCols_new, nRows_new, dstSys.GetWKT(), paTransform)) return false;

	float* pRows = new float[nCols_new + 8];
	double ro, co;
	
	printf("process...%d/      ", nRows_new);
	for (int r = 0; r < nRows_new; r++){
		float* pR = pRows;
		for (int c = 0; c < nCols_new; c++){
			pDstDem->GetGeoXY(c, r, &co, &ro);
			coordCvt.Dst2Src(1, &co, &ro);
			*pR = GetDemZValue(co, ro);			
			pR++;
		}
		pDstDem->WriteDemRow(pRows, r);
		printf("\b\b\b\b\b\b%6d", r + 1);
	}
	delete pRows;
	delete pDstDem;
	printf("\n----cvt end----\n");
	return true;
}

bool CDemBase::Save2File(const char* lpstrPathName){
	if (strlen(lpstrPathName) < 3) return false;
	bool bGDAL = IsGDAL(lpstrPathName);
	double aop[6];	GetGeoTransform(aop);
	bool ret;
	if (bGDAL){
		ret = CImgDem::Save2File(lpstrPathName, GetBuf(), GetCols(), GetRows(), 1, GetBits(), GetWKT(), aop);
	}
	else 
		ret = CSpDemTmp::Save2File(lpstrPathName, GetBuf(), GetCols(), GetRows(), 1, GetBits(), GetWKT(), aop);
	return ret;
}