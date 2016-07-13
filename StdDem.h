#pragma once

#define		NOVALUE_THESHOLD				-9000
#define		NOVALUE				-99999

class CStdDem
{
public:
	CStdDem();
	virtual ~CStdDem();
	bool			Load4File(const char* lpstrDemFile);
	bool			Save2File(const char* lpstrDemFile);
	bool			SetGeoSys4WKT(const char* pszWKT);
	bool			SetGeoSys4GCD(const char* pszGCD);

	bool			CvtGeoSys(const char* lpstrGcdPath, const char* lpstrDstDemPath){
		return CvtGeoSys(lpstrGcdPath, lpstrDstDemPath, 0, 0);
	}

	int				GetCols();
	int				GetRows();

	bool			GetDemRgn(double x[4], double y[4]);

	inline void		GetDemZValue(int column, int row, float* DemZValue){ *DemZValue = GetDemZVal(column, row); };
	float			GetDemZVal(int column, int row);
	
	inline void		GetDemZValue(double x, double y, double *z){ *z = GetDemZValue(x, y); };
	inline void		GetDemZValue(double x, double y, float  *z){ *z = GetDemZValue(x, y); };
	float			GetDemZValue(double x, double y);
	float			GetDemZValue4LB(double lon, double lat);
	void			GetGlobalXYZValue(int column, int row, double * DemXValue, double * DemYValue, float * DemZValue);
	void			GetGlobalXYZValue(int column, int row, double * DemXValue, double * DemYValue, double * DemZValue);

	float			GetCenterH(){
		return GetDemZVal(GetCols() / 2, GetRows() / 2);
	}
	void			SetDemZValue(int column, int row, double z);
	bool			CvtFormat(const char* lpstrPath, double xgsd, double ygsd);
	float	GetMedianZ();
	float	GetMinimumZ();
	float	GetMaximumZ();
protected:
	void			Clear();
	bool			CvtGeoSys(const char* lpstrGcdPath, const char* lpstrDstDemPath,double xgsd,double ygsd);
private:
	class CDemBase* m_pDem;
};

