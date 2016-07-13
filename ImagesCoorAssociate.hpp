//ImagesCoorAssociate.hpp
/********************************************************************
	ImagesCoorAssociate
	created:	2015/06/16
	author:		LX_whu 
	purpose:	This file is for ImagesCoorAssociate function
*********************************************************************/
#if !defined IMAGESCOORASSOCIATE_HPP_LX_whu_2015_6_16
#define IMAGESCOORASSOCIATE_HPP_LX_whu_2015_6_16

#include "StdImage.h"
#include "StdDem.h"
#include <math.h>

#ifdef _DEBUG
#include "GlobalUtil_AtMatch.h"
#endif // _DEBUG
#include "MatchFile.h"

#define EQU_ZERO(a)	(fabs(a) <1e-6)

class CAltitude{
public:
	CAltitude(){};
	virtual ~CAltitude() {};
	virtual float	median() = 0;
	virtual float	minimun() = 0;
	virtual	float	maximum() = 0;
	virtual bool	GetHeightRange(double xmin, double ymin, double xmax, double ymax, float* minH, float* maxH) = 0;
};

class CAvrAltitude : public CAltitude
{
public:
	CAvrAltitude(){
		m_h_min = 0;
		m_h_max = 1000;
	};
	virtual ~CAvrAltitude(){};
	void init(float minH, float maxH){
		m_h_min = minH;
		m_h_max = maxH;
	}
	virtual float median(){
		return (m_h_max + m_h_min) / 2;
	}
	virtual float	minimun(){
		return m_h_min;
	}
	virtual float	maximum(){
		return m_h_max;
	}
	virtual bool GetHeightRange(double xmin, double ymin, double xmax, double ymax, float* minH, float* maxH)
	{
		*minH = m_h_min;
		*maxH = m_h_max;
		return true;
	}
private:
	float	m_h_min;
	float	m_h_max;
};

class CDemAltitude : public CAltitude
{
public:
	CDemAltitude(){
		m_pDem = NULL;
	}
	void init(CStdDem* dem){
		m_pDem = dem;
	}
	virtual float median(){
		return m_pDem->GetMedianZ();
	}
	virtual float minimun(){
		return m_pDem->GetMinimumZ();
	}
	virtual float maximum(){
		return m_pDem->GetMaximumZ();
	}
	virtual bool GetHeightRange(double xmin, double ymin, double xmax, double ymax, float* minH, float* maxH)
	{
		float h;	float hmin = 1e10f, hmax = -1e10f;
		double x[4] = { xmin, xmin, xmax, xmax };
		double y[4] = { ymin, ymax, ymin, ymax };
		for (int i = 0; i<4; i++){
			h = m_pDem->GetDemZValue(x[i], y[i]);
			if (h > NOVALUE_THESHOLD){
				if (hmin > h) hmin = h;
				if (hmax < h) hmax = h;
			}
		}
		if (hmin < NOVALUE_THESHOLD || hmax < NOVALUE_THESHOLD){
			*minH = 0;	*maxH = 1000;
			return false;
		}
		if (EQU_ZERO(hmax) && EQU_ZERO(hmin)) {
			*minH = 0;	*maxH = 1000;
		}
		else{
			*minH = hmin - 240;
			*maxH = hmax + 240;
		}

		return true;
	}
protected:
	CStdDem*	m_pDem;
};

#include "algorithms/Geometry.h"
#include "algorithms/Transformation.h"

#define MAX_VAL(maxval,array,n)  { maxval = *(array);	for (int i = 1; i < n; i++) if (*(array+i)>maxval) maxval = *(array+i); }
#define MIN_VAL(minval,array,n)  { minval = *(array);	for (int i = 1; i < n; i++) if (*(array+i)<minval) minval = *(array+i); }

class CImagesCoorT{
public:
	enum
	{
		row_space = 500,
		col_space = 500,
		grid_rows = 10,
		grid_cols = 10,
	};
	CImagesCoorT(){
		m_pImgL = m_pImgR = NULL;
		m_range = -1;
	}
	virtual ~CImagesCoorT(){

	}
	void	      set_range(int range) { m_range = range; }
	int			  get_range() { return m_range; }
	// return l/r
	virtual float get_gsd_ratio() = 0;
	virtual bool coorL_to_coorR(float colL, float rowL, float* colR, float* rowR) = 0;
	virtual bool coorR_to_coorL(float colR, float rowR, float* colL, float* rowL) = 0;
	virtual CImagesCoorT* CreateInvertT(bool bExchangeImg) = 0;
	virtual bool overlap_on_left(float stColR, float stRowR, float edColR, float edRowR,int rangX,int rangY, float* stColL, float* stRowL, float* edColL, float* edRowL){
		float x[4], y[4];
		coorR_to_coorL(stColR, stRowR, x, y);
		coorR_to_coorL(edColR, edRowR, x + 1, y + 1);
		coorR_to_coorL(stColR, edRowR, x + 2, y + 2);
		coorR_to_coorL(edColR, stRowR, x + 3, y + 3);
		MIN_VAL(*stColL, x, 4);	MAX_VAL(*edColL, x, 4);
		MIN_VAL(*stRowL, y, 4);	MAX_VAL(*edRowL, y, 4);
		*stColL -= rangX;	*edColL += rangX;
		*stRowL -= rangY;	*edRowL += rangY;
		if (*edColL<0 || *edRowL<0 || *stColL>m_pImgL->GetCols() - 1 || *stRowL>m_pImgL->GetRows() - 1) return false;
		if (*stColL < 0) *stColL = 0;	if (*edColL>m_pImgL->GetCols() - 1) *edColL = (float)m_pImgL->GetCols() - 1;
		if (*stRowL < 0) *stRowL = 0;	if (*edRowL>m_pImgL->GetRows() - 1) *edRowL = (float)m_pImgL->GetRows() - 1;
		return true;
	}
	virtual bool overlap_on_right(float stColL, float stRowL, float edColL, float edRowL, int rangX, int rangY, float* stColR, float* stRowR, float* edColR, float* edRowR){
		float x[4], y[4];
		coorL_to_coorR(stColL, stRowL, x, y);
		coorL_to_coorR(edColL, edRowL, x + 1, y + 1);
		coorL_to_coorR(stColL, edRowL, x + 2, y + 2);
		coorL_to_coorR(edColL, stRowL, x + 3, y + 3);
		MIN_VAL(*stColR, x, 4);	MAX_VAL(*edColR, x, 4);
		MIN_VAL(*stRowR, y, 4);	MAX_VAL(*edRowR, y, 4);
		*stColR -= rangX;	*edColR += rangX;
		*stRowR -= rangY;	*edRowR += rangY;
		if (*edColR<0 || *edRowR<0 || *stColR>m_pImgR->GetCols() - 1 || *stRowR>m_pImgR->GetRows() - 1) return false;
		if (*stColR < 0) *stColR = 0;	if (*edColR>m_pImgR->GetCols() - 1) *edColR = (float)m_pImgR->GetCols() - 1;
		if (*stRowR < 0) *stRowR = 0;	if (*edRowR>m_pImgR->GetRows() - 1) *edRowR = (float)m_pImgR->GetRows() - 1;
		return true;
	}
	virtual int update_transform_parameters(
		int transform_model,// 0 represent only offset,1 represent similarity,2 represent affine,3 represent homog
		const float* xl, const float*yl, const  float* xr, const  float* yr, int nBufferSpace[4], bool bBufferByte,
		int(*match_idx)[2], int pairNum, bool* bValid, float err_dist
		) = 0;
	int		 update(
		const float* xl, const float*yl, const  float* xr, const  float* yr, int nBufferSpace[4],bool bBufferByte, int(*match_idx)[2], int pairNum,
		bool* bValid, float err_dist
		){
		int(*match_idx_tmp)[2] = NULL;
		if (!match_idx){
			match_idx_tmp = new int[pairNum][2];
			for (int i = 0; i < pairNum; i++){
				match_idx_tmp[i][0] = match_idx_tmp[i][1] = i;
			}
			match_idx = match_idx_tmp;
		}
		bool* bValidTmp = NULL;
		if (!bValid){
			bValidTmp = new bool[pairNum];
			for (int i = 0; i < pairNum; i++ ){
				bValidTmp[i] = true;
			}
			bValid = bValidTmp;
		}

		int model;
		float correct_ratio = 0.2f;
		if (pairNum > 4 / correct_ratio){
			model = 3;
			correct_ratio = 0.6f;
		}
		else if (pairNum > 3 / correct_ratio){
			model = 2;
			correct_ratio = 0.7f;
		}
		else if (pairNum > 2 / correct_ratio){
			model = 1;
			correct_ratio = 0.8f;
		}
		else{
			model = 0;
			correct_ratio = 0.9f;
		}
		int left_num = 0;
		if(model != 0) left_num = update_transform_parameters(model, xl, yl, xr, yr, nBufferSpace, bBufferByte, match_idx, pairNum, bValid, err_dist);
		
		if (match_idx_tmp) delete[] match_idx_tmp;
		if (bValidTmp) delete bValidTmp;
		return left_num;
	}
	virtual int		get_right_candidate_locations(float colL, float rowL,int rangX,int rangY,float* colR,float* rowR) = 0;
	bool calculate_affine_matrix(int stColL, int stRowL, int nColsL, int nRowsL,int stColR,int stRowR, int nGridCols, int nGridRows, float aop6[6])
	{
		int nPtSum = nGridRows*nGridCols;
		float* xl = new float[nPtSum];
		float* yl = new float[nPtSum];
		float* xr = new float[nPtSum];
		float* yr = new float[nPtSum];

		int winCol = nColsL / (nGridCols - 1);
		int winRow = nRowsL / (nGridRows - 1);
		float* pxl = xl;	float* pyl = yl;
		float* pxr = xr;	float* pyr = yr;

		for (int r = 0; r <= nRowsL; r += winRow){
			for (int c = 0; c <= nColsL; c += winCol){
				*pxl = float(c);	*pyl = float(r);
				coorL_to_coorR(stColL + *pxl, stRowL + *pyl, pxr, pyr);
				*pxr -= stColR;	*pyr -= stRowR;
				pxl++;	pyl++;	pxr++;	pyr++;
			}
		}
		char * argv[] = { "xform", "affine", "method", "norm","message","off" };
		int argc = 6;
		float H[3][3];
		int h_left_num = GeoTransform::EstimateXformMatrix(xl, yl, xr, yr, NULL, nPtSum, NULL, H[0],
			argc, argv);
		memcpy(aop6, H[0], 6 * sizeof(float));
		delete[] yr;		yr = NULL;
		delete[] xr;		xr = NULL;
		delete[] yl;		yl = NULL;
		delete[] xl;		xl = NULL;
		return h_left_num > 0 ? true : false;
	}
	virtual bool calculate_affine_matrix(int stColL, int stRowL, int nColsL, int nRowsL, int stColR, int stRowR, float aop6[6])
	{
		if (nColsL <= grid_cols || nRowsL <= grid_rows) return false;
		return calculate_affine_matrix(stColL, stRowL, nColsL, nRowsL, stColR,stRowR, grid_cols, grid_rows, aop6);
	}

	bool coorL_to_coorR(int colL, int rowL, int* colR, int* rowR){
		float c, r;
		if (!coorL_to_coorR((float)colL, (float)rowL, &c, &r)) return false;
		*colR = (int)c;
		*rowR = (int)r;
		return true;
	}
	bool coorR_to_coorL(int colR, int rowR, int* colL, int* rowL){
		float c, r;
		if (!coorR_to_coorL((float)colR, (float)rowR, &c, &r)) return false;
		*colL = (int)c;
		*rowL = (int)r;
		return true;
	}
	bool overlap_on_left(int stColR, int stRowR, int edColR, int edRowR, int rangX, int rangY, int* stColL, int* stRowL, int* edColL, int* edRowL){
		float sc, sr, ec, er;
		if (!overlap_on_left((float)stColR, (float)stRowR, (float)edColR, (float)edRowR,rangX,rangY, &sc, &sr, &ec, &er)) return false;
		*stColL = (int)sc;	*stRowL = (int)sr;
		*edColL = (int)ec;	*edRowL = (int)er;
		return true;
	}
	virtual bool overlap_on_right(int stColL, int stRowL, int edColL, int edRowL, int rangX, int rangY, int* stColR, int* stRowR, int* edColR, int* edRowR){
		float sc, sr, ec, er;
		if (!overlap_on_right((float)stColL, (float)stRowL, (float)edColL, (float)edRowL,rangX,rangY, &sc, &sr, &ec, &er)) return false;
		*stColR = (int)sc;	*stRowR = (int)sr;
		*edColR = (int)ec;	*edRowR = (int)er;
		return true;
	}
	bool overlap_on_left(int rangX, int rangY,float* stColL, float* stRowL, float* edColL, float* edRowL){
		return overlap_on_left((float)0, (float)0, (float)m_pImgR->GetCols(), (float)m_pImgR->GetRows(),rangX,rangY, stColL, stRowL, edColL, edRowL);
	}
	bool overlap_on_right(int rangX, int rangY, float* stColR, float* stRowR, float* edColR, float* edRowR){
		return overlap_on_right((float)0, (float)0, (float)m_pImgL->GetCols(), (float)m_pImgL->GetRows(),rangX,rangY, stColR, stRowR, edColR, edRowR);
	}
	bool overlap_on_left(int rangX, int rangY, int* stColL, int* stRowL, int* edColL, int* edRowL){
		float sc, sr, ec, er;
		if(!overlap_on_left((float)0, (float)0, (float)m_pImgR->GetCols(), (float)m_pImgR->GetRows(),rangX,rangY, &sc, &sr, &ec, &er)) return false;
		*stColL = (int)sc;	*stRowL = (int)sr;
		*edColL = (int)ec;	*edRowL = (int)er;
		return true;
	}
	bool overlap_on_right(int rangX, int rangY, int* stColR, int* stRowR, int* edColR, int* edRowR){
		float sc, sr, ec, er;
		if (!overlap_on_right((float)0, (float)0, (float)m_pImgL->GetCols(), (float)m_pImgL->GetRows(), rangX, rangY, &sc, &sr, &ec, &er)) return false;
		*stColR = (int)sc;	*stRowR = (int)sr;
		*edColR = (int)ec;	*edRowR = (int)er;
		return true;
	}
	bool overlap_on_left(int* stColL, int* stRowL, int* edColL, int* edRowL)
	{
		if (m_range < 0) {
			*stColL = 0;
			*stRowL = 0;
			*edColL = m_pImgL->GetCols() - 1;
			*edRowL = m_pImgL->GetRows() - 1;
			return true;
		}
		return overlap_on_left(m_range, m_range, stColL, stRowL, edColL, edRowL);
	}	
	bool overlap_on_right(int* stColR, int* stRowR, int* edColR, int* edRowR)
	{
		if (m_range < 0) {
			*stColR = 0;
			*stRowR = 0;
			*edColR = m_pImgR->GetCols() - 1;
			*edRowR = m_pImgR->GetRows() - 1;
			return true;
		}
		return overlap_on_right(m_range, m_range, stColR, stRowR, edColR, edRowR);
	}
	bool overlap_on_left(int stColR, int stRowR, int edColR, int edRowR,int* stColL, int* stRowL, int* edColL, int* edRowL){
		if (m_range < 0) {
			*stColL = 0;
			*stRowL = 0;
			*edColL = m_pImgL->GetCols() - 1;
			*edRowL = m_pImgL->GetRows() - 1;
			return true;
		}
		return overlap_on_left(stColR, stRowR, edColR, edRowR, m_range, m_range, stColL, stRowL, edColL, edRowL);
	}
	bool overlap_on_right(int stColL, int stRowL, int edColL, int edRowL, int* stColR, int* stRowR, int* edColR, int* edRowR){
		if (m_range < 0) {
			*stColR = 0;
			*stRowR = 0;
			*edColR = m_pImgR->GetCols() - 1;
			*edRowR = m_pImgR->GetRows() - 1;
			return true;
		}
		return overlap_on_right(stColL, stRowL, edColL, edRowL,m_range, m_range, stColR, stRowR, edColR, edRowR );
	}
	virtual bool IsUpdated() = 0;
	virtual bool IsInSearchRange(const float* locL, const  float* locR, float range) = 0;
public:
	CStdImage*	m_pImgL;
	CStdImage*	m_pImgR;
	int			m_range;
};

class CStdImagesCoorT : public CImagesCoorT
{
public:
	CStdImagesCoorT(){
		memset(m_homg, 0, sizeof(float)* 8);	m_homg[8] = 1;
		memset(m_homg_inverse, 0, sizeof(float)* 9);
		m_bHomg_Init = false;
	}
	virtual ~CStdImagesCoorT(){

	}
	virtual CImagesCoorT* CreateInvertT(bool bExchangeImg){
		CStdImagesCoorT* p = new CStdImagesCoorT;
		if (bExchangeImg){
			p->m_pImgR = m_pImgL;
			p->m_pImgL = m_pImgR;
		}
		else {
			p->m_pImgL = m_pImgL;
			p->m_pImgR = m_pImgR;
		}
		memcpy(p->m_homg, m_homg_inverse, sizeof(float)* 9);
		memcpy(p->m_homg_inverse, m_homg, sizeof(float)* 9);
		p->m_bHomg_Init = m_bHomg_Init;
		p->m_range = m_range;
		return p;
	}
	bool init(CStdImage* pImgL, CStdImage* pImgR){
		m_pImgL = pImgL;
		m_pImgR = pImgR;
		return true;
	}
	bool init(CStdImage* pImgL, CStdImage* pImgR,float H[3][3]){
		m_pImgL = pImgL;
		m_pImgR = pImgR;
		m_bHomg_Init = true;
		memcpy(m_homg, H[0], sizeof(float)* 9);
		if (!GeoTransform::homog_inverse(H[0], m_homg_inverse)) return false;
		return true;
	}
	virtual bool coorL_to_coorR(float colL, float rowL, float* colR, float* rowR)
	{
		if (!m_bHomg_Init) return false;
		float from[2], to[2];
		from[0] = colL;
		from[1] = rowL;
		GeoTransform::homog_xform_pt(from, to, m_homg);
		*colR = to[0];
		*rowR = to[1];
		return true;
	}
	virtual bool coorR_to_coorL(float colR, float rowR, float* colL, float* rowL){
		if (!m_bHomg_Init) return false;
		float from[2], to[2];
		from[0] = colR;
		from[1] = rowR;
		GeoTransform::homog_xform_pt(from, to, m_homg_inverse);
		*colL = to[0];
		*rowL = to[1];
		return true;
	}
	virtual bool overlap_on_left(float stColR, float stRowR, float edColR, float edRowR, int rangX, int rangY, float* stColL, float* stRowL, float* edColL, float* edRowL)
	{
		if (!m_bHomg_Init){
			*stColL = 0;
			*stRowL = 0;
			*edColL = (float)m_pImgL->GetCols() - 1;
			*edRowL = (float)m_pImgL->GetRows() - 1;
			return true;
		}
		return CImagesCoorT::overlap_on_left(stColR, stRowR, edColR, edRowR,rangX,rangY, stColL, stRowL, edColL, edRowL);
	}
	virtual bool overlap_on_right(float stColL, float stRowL, float edColL, float edRowL, int rangX, int rangY, float* stColR, float* stRowR, float* edColR, float* edRowR){
		if (!m_bHomg_Init){
			*stColR = 0;
			*stRowR = 0;
			*edColR = (float)m_pImgR->GetCols() - 1;
			*edRowR = (float)m_pImgR->GetRows() - 1;
			return true;
		}
		return CImagesCoorT::overlap_on_right(stColL, stRowL, edColL, edRowL,rangX,rangY, stColR, stRowR, edColR, edRowR);
	}
	virtual int update_transform_parameters(
		int transform_model,// 0 represent only offset,1 represent similarity,2 represent affine,3 represent homog
		const float* xl, const float*yl, const  float* xr, const  float* yr, int nBufferSpace[4], bool bBufferByte,
		int(*match_idx)[2], int pairNum, bool* bValid, float err_dist
		)
	{
		if (transform_model < 0) transform_model = 0; else if (transform_model>3) transform_model = 3;
		float H[3][3];	float invH[3][3];	
		int nb[4]; if (bBufferByte) memcpy(nb, nBufferSpace, 4 * sizeof(int)); else { for (int i = 0; i < 4; i++) nb[i] = nBufferSpace[i] * sizeof(float); }
		int left_num = 0;
		if (transform_model == 0){
			const float *pxl = NULL, *pyl = NULL, *pxr = NULL, *pyr = NULL;
			float xoff = 0, yoff = 0;
			for (int i = 0; i < pairNum; i++){
				pxl = (const float *)((const char*)xl + nb[0] * match_idx[i][0]);
				pyl = (const float *)((const char*)yl + nb[1] * match_idx[i][0]);
				pxr = (const float *)((const char*)xr + nb[2] * match_idx[i][1]);
				pyr = (const float *)((const char*)yr + nb[3] * match_idx[i][1]);
				xoff += (*pxr - *pxl);
				yoff += (*pyr - *pyl);
			}
			xoff /= pairNum;
			yoff /= pairNum;
			memset(m_homg, 0, sizeof(float)* 9);
			memset(m_homg_inverse, 0, sizeof(float)* 9);
			m_homg[0] = m_homg_inverse[0] = 1;	m_homg[2] = xoff;	m_homg_inverse[2] = -xoff;
			m_homg[4] = m_homg_inverse[4] = 1;	m_homg[5] = yoff;	m_homg_inverse[5] = -yoff;
			m_homg[8] = m_homg_inverse[8] = 1;
			left_num = pairNum;
		}
		else if (transform_model == 1){
			char * argv[6] = { "xform", "nonreflective similarity", "bufferType", "byte", "DistanceThreshold" };
			int argc = 6;
			char str[100];	sprintf(str, "%f", err_dist);	argv[argc-1] = str;
			left_num = GeoTransform::EstimateXformMatrix(xl, yl, xr, yr, nb, match_idx, pairNum, bValid, H[0],
				argc,argv);
			
		}
		else if (transform_model == 2){
			char * argv[6] = { "xform", "affine", "bufferType", "byte", "DistanceThreshold" };
			int argc = 6;
			char str[100];	sprintf(str, "%f", err_dist);	argv[argc-1] = str;
			left_num = GeoTransform::EstimateXformMatrix(xl, yl, xr, yr, nb, match_idx, pairNum, bValid, H[0],
				argc, argv);
			
		}
		else{
			char * argv[14] = { "method", "ransacA", "bufferType", "byte", "matrixCalc", "inliers", "InlierPercentage", "0.2", "ProbBadSupp", "0.01", "xform", "homography", "DistanceThreshold" };
			int argc = 14;
			char str[100];	sprintf(str, "%f", err_dist);	argv[argc-1] = str;
			left_num = GeoTransform::EstimateXformMatrix(xl, yl, xr, yr, nb, match_idx, pairNum, bValid, H[0],
				argc, argv);
		}
		

// #ifdef _DEBUG
// 		if (left_num>0){
// 			float *xl0 = new float[left_num];
// 			float *yl0 = new float[left_num];
// 			float *xr0 = new float[left_num];
// 			float *yr0 = new float[left_num];
// 			left_num = 0;
// 			for (int i = 0; i < pairNum; i++){
// 				if (!bValid[i]) continue;
// 				xl0[left_num] = xl[nBufferSpace[0] * match_idx[i][0]];
// 				yl0[left_num] = yl[nBufferSpace[1] * match_idx[i][0]];
// 				xr0[left_num] = xr[nBufferSpace[2] * match_idx[i][1]];
// 				yr0[left_num] = yr[nBufferSpace[3] * match_idx[i][1]];
// 				left_num++;
// 			}
// 			CMatchFile result;
// 			char strPath[512];
// 			CMatchFile::get_harris_model_file_path(m_pImgL->GetImageName(), m_pImgR->GetImageName(), strPath);
// 			int nb[4] = { sizeof(float), sizeof(float), sizeof(float), sizeof(float) };
// 			result.WriteModelFile(strPath, m_pImgL->GetImagePath(), m_pImgR->GetImagePath(), xl0, yl0, xr0, yr0, left_num, nb);
// 			delete xl0; delete yl0;
// 			delete xr0;	delete yr0;
// 		}		
// #endif
		
		
		if (left_num > 0) {
			if (!transform_model){
				m_bHomg_Init = true;
			}
			else if (GeoTransform::homog_inverse(H[0], invH[0])){
				memcpy(m_homg, H[0], sizeof(float)* 9);
				memcpy(m_homg_inverse, invH[0], sizeof(float)* 9);
				m_bHomg_Init = true;
			}
		}

		
		return left_num;
	}
	virtual int get_right_candidate_locations(float colL, float rowL, int rangX, int rangY, float* colR, float* rowR){
		
		if (!coorL_to_coorR(colL, rowL, colR, rowR)) {
			*colR = colL;	*rowR = rowL;
//			return 0;
		}
		return 1;
	}
	virtual bool calculate_affine_matrix(int stColL, int stRowL, int nColsL, int nRowsL, int stColR, int stRowR, float aop6[6]){
		if (!m_bHomg_Init){
			memset(aop6, 0, sizeof(float)* 6);
			aop6[2] = 1;	aop6[5] = 1;
//			return false;
			return true;
		}
		return CImagesCoorT::calculate_affine_matrix(stColL, stRowL, nColsL, nRowsL, stColR, stRowR, aop6);
	}
	virtual float get_gsd_ratio(){
		if (m_bHomg_Init){
			return (float)sqrt((m_homg[0] * m_homg[0] + m_homg[1] * m_homg[1] + m_homg[3] * m_homg[3] + m_homg[4] * m_homg[4]) / 2);
		}
		return 1;
	}
	virtual bool IsUpdated(){
		return m_bHomg_Init;
	}
	virtual bool IsInSearchRange(const float* locL, const  float* locR, float range){
		float to[2];
		GeoTransform::homog_xform_pt(locL, to, m_homg);

		float d = calc_euclidean_distance(to, locR, 2);
		if (d < range) return true;
		return false;
	}
private:
	float	m_homg[9];
	float	m_homg_inverse[9];
	bool	m_bHomg_Init;
};

#include "DPGRID/WuGlbHei-v2.0.h"

class CGeoImagesCoorT : public CImagesCoorT
{
public:
	enum 
	{
		row_space = 5000,
		col_space = 5000,
		grid_rows = 10,
		grid_cols = 10,
	};
	CGeoImagesCoorT(){
		
		m_alt = NULL;
		memset(m_compensate, 0, sizeof(m_compensate));
// 		m_hei_min = m_hei_max = 0;
// 		m_hei_init = 0;
		m_alt_min = m_alt_max = m_alt_avr = NULL;
		m_alt_grid_cols = m_alt_grid_rows = 0;
		m_bCompensate = 0;
//		m_alt_avr_area = 0;
	}
	virtual ~CGeoImagesCoorT(){
		reset();
	}
	void reset(){
		if (m_alt_min) delete m_alt_min;
		if (m_alt_max) delete m_alt_max;
		if (m_alt_avr) delete m_alt_avr;
		m_bCompensate = 0;
	}
	virtual CImagesCoorT* CreateInvertT(bool bExchangeImg){
		CGeoImagesCoorT* p = new CGeoImagesCoorT;
		if (bExchangeImg){
			p->m_pImgR = m_pImgL;
			p->m_pImgL = m_pImgR;
		}
		else {
			p->m_pImgL = m_pImgL;
			p->m_pImgR = m_pImgR;
		}
		p->m_alt = m_alt;
		if (m_bCompensate == 1) p->m_bCompensate = 2; else if (m_bCompensate == 2) p->m_bCompensate = 1;
		if (m_bCompensate){
			memcpy(p->m_compensate, m_compensate,sizeof(m_compensate));
		}
		p->init_alt_grid();
		p->m_range = m_range;
		return p;
	}
	bool init(CGeoImage* pImgL, CGeoImage* pImgR, CAltitude* pDem){
		m_pImgL = pImgL;	m_pImgR = pImgR;
		m_alt = pDem;
		reset();
		init_alt_grid(pImgL, pDem, m_alt_min, m_alt_max, m_alt_avr, &m_alt_grid_cols, &m_alt_grid_rows);
		return true;
	}
	bool	coorL_to_coorR(float colL, float rowL, float* colR, float* rowR, float h,int bCompensate){
		double x, y;
		if (bCompensate == 1) ixy_to_pxy(colL, rowL, &colL, &rowL, m_compensate);
		double cl = (double)colL, rl = (double)rowL, alt = (double)h;
		((CGeoImage*)m_pImgL)->PhoZ2LB(&cl, &rl, &alt,1, &x, &y);
		((CGeoImage*)m_pImgR)->LBH2Pho(&x, &y, &alt, 1, &cl, &rl);
		*colR = (float)cl;	*rowR = (float)rl;
		if (bCompensate == 2) pxy_to_ixy(*colR, *rowR, colR, rowR, m_compensate);
		return true;
	}
	bool	coorR_to_coorL(float colR, float rowR, float* colL, float* rowL, float h, int bCompensate){
	
		if (bCompensate == 2) ixy_to_pxy(colR, rowR, &colR, &rowR, m_compensate);

		double x, y;	double c = (double)colR, r = (double)rowR, alt = (double)h;
		((CGeoImage*)m_pImgR)->PhoZ2LB(&c, &r, &alt, 1, &x, &y);
		((CGeoImage*)m_pImgL)->LBH2Pho(&x, &y, &alt, 1, &c, &r);
		*colL = (float)c;	*rowL = (float)r;
		if (bCompensate == 1) pxy_to_ixy((float)*colL, (float)*rowL, colL, rowL, m_compensate);
		return true;
	}
	bool coorL_to_coorR(float colL, float rowL, float* colR, float* rowR, int bCompensate){
		float h;
		get_from_alt_grid((int)colL, (int)rowL, NULL, NULL, &h);
		return coorL_to_coorR(colL, rowL, colR, rowR, h, bCompensate);
	}
	virtual bool coorL_to_coorR(float colL, float rowL, float* colR, float* rowR)
	{
		return coorL_to_coorR(colL, rowL, colR, rowR, m_bCompensate);
	}
	virtual bool coorR_to_coorL(float colR, float rowR, float* colL, float* rowL){
		return coorR_to_coorL(colR, rowR, colL, rowL, 0, m_bCompensate);
	}
	virtual float get_gsd_ratio(){
		return ((CGeoImage*)m_pImgL)->GetGsd() / ((CGeoImage*)m_pImgR)->GetGsd();
	}
	
	virtual bool IsInSearchRange(const float* locL, const  float* locR, float range){
		float r1[2], r2[2];
		float hmax,hmin;
		get_from_alt_grid((int)*locL, (int)*(locL + 1), &hmin, &hmax, NULL);
		coorL_to_coorR(*locL, *(locL + 1), r1, r1 + 1, hmin,m_bCompensate);
		coorL_to_coorR(*locL, *(locL + 1), r2, r2 + 1, hmax,m_bCompensate);
		float d = point_to_segment_euclidean_distance(locR, r1, r2, 2);
		return d < range ? true : false;
	}
// 	virtual bool IsInSearchRange(const float* locL, const  float* locR, float range){
// 		float r1[2];
// 		coorL_to_coorR(*locL, *(locL + 1), r1, r1 + 1, m_alt_avr_area, m_bCompensate);
// 		if (fabs(*locR - r1[0]) < range && fabs(locR[1] - r1[1]) < range) return true;
// 		return false;
// 	}
	int calculate_compensate(
		int compensate_model,// 0 represent only offset,1 represent similarity,2 represent affine
		const float* xl, const float*yl, const  float* xr, const  float* yr, int nBufferSpace[4],bool bBufferByte,
		int(*match_idx)[2], int pairNum, bool* bValid, float err_dist,
		float compensation[6]
		)
	{
		int nb[4]; if (bBufferByte) memcpy(nb, nBufferSpace, 4 * sizeof(int)); else { for (int i = 0; i < 4; i++) nb[i] = nBufferSpace[i] * sizeof(float); }
		float* xrc = new float[pairNum];
		float* yrc = new float[pairNum];
		float* xr0 = new float[pairNum];
		float* yr0 = new float[pairNum];
		float* pxrc = xrc;
		float* pyrc = yrc;
		float* pxr0 = xr0;
		float* pyr0 = yr0;
		const float* pxl = xl;
		const float* pyl = yl;
		const float* pxr = xr;
		const float* pyr = yr;

		for (int i = 0; i < pairNum; i++){
			pxl = (const float*)((const char*)xl + match_idx[i][0] * nb[0]);
			pyl = (const float*)((const char*)yl + match_idx[i][0] * nb[1]);
			pxr = (const float*)((const char*)xr + match_idx[i][1] * nb[2]);
			pyr = (const float*)((const char*)yr + match_idx[i][1] * nb[3]);

//			coorL_to_coorR(*pxl, *pyl, pxrc, pyrc, m_alt_avr_area, 0);
			coorL_to_coorR(*pxl, *pyl, pxrc, pyrc, 0);
			*pxr0++ = *pxr - *pxrc;
			*pyr0++ = *pyr - *pyrc;
			pxrc++;	pyrc++;
		}
		
		int left_num = 0;
		
		if (compensate_model < 0) compensate_model = 0; else if (compensate_model>2) compensate_model = 2;
		if (compensate_model==0){
			pxrc = xrc;	pyrc = yrc;
			pxr0 = xr0;	pyr0 = yr0;
			float xoff = 0, yoff = 0;
			for (int i = 0; i < pairNum; i++,bValid++){
				xoff += *pxr0;
				yoff += *pyr0;
				pxr0++;
				pyr0++;
				*bValid = 0;
			}
			xoff /= pairNum;
			yoff /= pairNum;
			memset(compensation, 0, sizeof(float)* 6);
			compensation[0] = xoff;
			compensation[3] = yoff;
			left_num = pairNum;
		}else if (compensate_model==1){
			float H[3][3];
			char * argv[4] = { "xform", "nonreflective similarity", "DistanceThreshold" };
			int argc = 4;
			char str[100];	sprintf(str, "%f", err_dist);	argv[3] = str;
			left_num = GeoTransform::EstimateXformMatrix(xrc, yrc, xr0, yr0, NULL, pairNum, bValid, H[0],
				argc,argv);
			if (left_num > 0){
				compensation[0] = H[0][2];	memcpy(compensation + 1, H[0], sizeof(float)* 2);
				compensation[3] = H[1][2];	memcpy(compensation + 4, H[1], sizeof(float)* 2);
			}			
		}
		else{
			float H[3][3];
			char * argv[12] = { "method", "ransacA", "matrixCalc", "inliers", "InlierPercentage", "0.4", "ProbBadSupp", "0.01", "xform", "affine", "DistanceThreshold" };
			int argc = 12;
			char str[100];	sprintf(str, "%f", err_dist);	argv[argc-1] = str;
			left_num = GeoTransform::EstimateXformMatrix(xrc, yrc, xr0, yr0, NULL, pairNum, bValid, H[0],
				argc, argv);
			if (left_num > 0){				
				compensation[0] = H[0][2];	memcpy(compensation + 1, H[0], sizeof(float)* 2);
				compensation[3] = H[1][2];	memcpy(compensation + 4, H[1], sizeof(float)* 2);
			}
			
		}

		if (left_num > 1){			
			m_bCompensate = 2;
		}


// 		pxl = xl;
// 		pyl = yl;
// 		pxr = xr;
// 		pyr = yr;
// 		for (int i = 0; i < pairNum; i++){
// 			pxl = (const float*)((const char*)xl + match_idx[i][0] * nb[0]);
// 			pyl = (const float*)((const char*)yl + match_idx[i][0] * nb[1]);
// 			pxr = (const float*)((const char*)xr + match_idx[i][1] * nb[2]);
// 			pyr = (const float*)((const char*)yr + match_idx[i][1] * nb[3]);
// 
// 			coorL_to_coorR(*pxl, *pyl, pxrc, pyrc, m_alt_avr_area, m_bCompensate);
// 			pxrc++;	pyrc++;
// 		}


		delete xrc;
		delete yrc;
		delete xr0;
		delete yr0;
		return left_num;
	}
	virtual  int update_transform_parameters(
		int compensate_model,// 0 represent only offset,1 represent similarity,2 represent affine,3 represent homog
		const float* xl, const float*yl, const  float* xr, const  float* yr, int nBufferSpace[4],bool bBufferByte,
		int(*match_idx)[2], int pairNum, bool* bValid, float err_dist
		)		
	{
		return calculate_compensate(compensate_model, xl, yl, xr, yr, nBufferSpace,bBufferByte, match_idx, pairNum,bValid, err_dist,m_compensate);
	}
	virtual int get_right_candidate_locations(float colL, float rowL, int rangX, int rangY, float* colR, float* rowR){
		float hmin,hmax,haver;
		get_from_alt_grid((int)colL, (int)rowL, &hmin, &hmax, &haver);
		
		coorL_to_coorR((float)colL, (float)rowL, colR, rowR, hmin, m_bCompensate);
		coorL_to_coorR((float)colL, (float)rowL, colR + 1, rowR + 1, haver, m_bCompensate);
		coorL_to_coorR((float)colL, (float)rowL, colR + 2, rowR + 2, hmax, m_bCompensate);

		if (fabs(colR[0] - colR[2]) > rangX || fabs(rowR[0] - rowR[2]) > rangY) return 3;
		return 1;
	}
	virtual bool	IsUpdated()
	{
		return m_bCompensate==0?false:true;
	}
public:
	void init_alt_grid(){
		init_alt_grid((CGeoImage*)m_pImgL, m_alt, m_alt_min, m_alt_max, m_alt_avr, &m_alt_grid_cols, &m_alt_grid_rows);
	}
protected:
	void	init_alt_grid(CGeoImage* img, CAltitude* alt,float*& alt_min, float*& alt_max, float*& alt_avr, int* grid_cols, int* grid_rows){
		int nCols = img->GetCols();
		int nRows = img->GetRows();
		int nGridCols = nCols / col_space;	if (nGridCols*col_space < nCols) nGridCols++;
		int nGridRows = nRows / row_space;	if (nGridRows*row_space < nRows) nGridRows++;
		alt_min = new float[nGridCols*nGridRows];
		alt_max = new float[nGridCols*nGridRows];
		alt_avr = new float[nGridCols*nGridRows];
		int c, r;
		int stCol, stRow, edCol, edRow;
		
		float *pAlt_min = alt_min;
		float *pAlt_max = alt_max;
		float *pAlt_avr = alt_avr;
		stRow = 0;	edRow = stRow + row_space;
		bool flag = true;	//m_alt_avr_area = 0;
		r = 0;
loop:
		for (; r < nGridRows - 1; r++){
			stCol = 0;	edCol = stCol + col_space;
			for (c = 0; c < nGridCols - 1; c++){
				GetAltitudeRange(img, stCol, stRow, edCol, edRow, alt, pAlt_max++, pAlt_min++, pAlt_avr);
				stCol += col_space;	
				edCol += col_space;
				//m_alt_avr_area += *pAlt_avr;
				pAlt_avr++;
			}
			GetAltitudeRange(img, stCol, stRow, nCols, edRow, alt, pAlt_max++, pAlt_min++, pAlt_avr);
			stRow += row_space;
			edRow += row_space;
			//m_alt_avr_area += *pAlt_avr;
			pAlt_avr++;
		}
		if (flag){
			flag = false;
			r = nGridRows - 2;
			edRow = nRows;
			goto loop;
		}
		*grid_cols = nGridCols;
		*grid_rows = nGridRows;
		//m_alt_avr_area = m_alt_avr_area / (nGridCols*nGridRows);
	}
	bool	get_from_alt_grid(int col, int row, float* alt_min, float* alt_max, float* alt_avr){
		col = col / col_space;	if (col > m_alt_grid_cols - 1) col = m_alt_grid_cols - 1;
		row = row / row_space;	if (row > m_alt_grid_rows - 1) row = m_alt_grid_rows - 1;
		int idx = col + row*m_alt_grid_cols;
		if (alt_min) *alt_min = *(m_alt_min + idx);
		if (alt_max) *alt_max = *(m_alt_max + idx);
		if (alt_avr) *alt_avr = *(m_alt_avr + idx);
		return true;
	}
	void GetAltitudeRange(CGeoImage* pImg, int miCol, int miRow, int mxCol, int mxRow, CAltitude* alt,float* max_alt, float* min_alt,float* avr_alt){
		double xmin, ymin, xmax, ymax;
		double x, y;
		double c, r, h = 0;
		c = (double)miCol; r = (double)miRow;
		pImg->PhoZ2LB(&c, &r, &h, 1, &x, &y);	xmin = xmax = x; ymin = ymax = y;
		c = (double)mxCol; r = (double)mxRow;
		pImg->PhoZ2LB(&c, &r, &h, 1, &x, &y);
		if (x < xmin) xmin = x; else xmax = x;
		if (y < ymin) ymin = y; else ymax = y;
//		float hmin, hmax;
		if(alt) alt->GetHeightRange(xmin, ymin, xmax, ymax, min_alt, max_alt);//&hmin, &hmax
		else {
			double t1 = GetZValue4Srtm(xmin, ymin);	if (t1 < -900) t1 = 0;
			double t2 = GetZValue4Srtm(xmax, ymax);	if (t2 < -900) t2 = 0;
			if (t1>t2) { *min_alt = t2 - 100; *max_alt = t1 + 100; }
			else { *min_alt = t1 - 100; *max_alt = t2 + 100; }						
			
// 			CWuGlbHei glbHei;
// 			if (!glbHei.GetHeightRange((xmin+xmax)/2, (ymin+ymax)/2, min_alt, max_alt)){
// 				*min_alt = 0;	*max_alt = 1000;
// 			}
		}
		*avr_alt = (*min_alt + *max_alt) / 2;
// 		m_hei_min = hmin;
// 		m_hei_max = hmax;
// 		m_hei_avr = (hmin + hmax) / 2;
// 		m_hei_area[0] = miCol;
// 		m_hei_area[1] = miRow;
// 		m_hei_area[2] = mxCol;
// 		m_hei_area[3] = mxRow;
	}
	
// 	inline void pxy_to_ixy(float px, float py, float *ix, float *iy, float *ab6){
// 		*ix = px + ab6[3] + ab6[4] * px + ab6[5] * py;
// 		*iy = py + ab6[0] + ab6[1] * px + ab6[2] * py;
// 	}
// 	inline void ixy_to_pxy(float ix, float iy, float *px, float *py, float *ab6){
// 		float t = 1 + ab6[2] + ab6[4] + ab6[4] * ab6[2] - ab6[5] * ab6[1];
// 		*px = (ix - ab6[3] + ab6[2] * ix - ab6[2] * ab6[3] - ab6[5] * iy + ab6[5] * ab6[0]) / t;
// 		*py = (iy - ab6[1] * ix + ab6[1] * ab6[3] - ab6[0] + ab6[4] * iy - ab6[4] * ab6[0]) / t;
// 	}
	inline void pxy_to_ixy(float px, float py, float *ix, float *iy, float *ab6){
		*ix = px + ab6[0] + ab6[1] * px + ab6[2] * py;
		*iy = py + ab6[3] + ab6[4] * px + ab6[5] * py;
	}
	inline void ixy_to_pxy(float ix, float iy, float *px, float *py, float *ab6){
		float t = 1 + ab6[5] + ab6[1] + ab6[1] * ab6[5] - ab6[2] * ab6[4];
		*px = (ix - ab6[0] + ab6[5] * ix - ab6[0] * ab6[5] - ab6[2] * iy + ab6[2] * ab6[3]) / t;
		*py = (iy - ab6[4] * ix + ab6[4] * ab6[0] - ab6[3] + ab6[1] * iy - ab6[1] * ab6[3]) / t;
	}
private:
// 	float		m_hei_min;
// 	float		m_hei_max;
// 	float		m_hei_avr;
// 	float		m_hei_init;
// 	int			m_hei_area[4];

	float*		m_alt_min;
	float*		m_alt_max;
	float*		m_alt_avr;
//	float		m_alt_avr_area;
	int			m_alt_grid_cols;
	int			m_alt_grid_rows;
public:
	CAltitude*	m_alt;
	float		m_compensate[6];
	int			m_bCompensate;
};

inline bool ImageConvert(float xl, float yl, float* xr, float* yr, const void* t){
	CImagesCoorT* pCvt = (CImagesCoorT*)t;
	return pCvt->coorL_to_coorR(xl, yl, xr, yr);
}

inline bool geo_xform_err(const float* locL, const float* locR, const void* M, float dist){
	return ((CImagesCoorT*)M)->IsInSearchRange(locL, locR, dist);
}

#endif // IMAGESCOORASSOCIATE_HPP_LX_whu_2015_6_16