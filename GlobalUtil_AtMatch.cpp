#include "GlobalUtil_AtMatch.h"

#ifdef PQ_DB
#include "libpq/libpq-fe.h"
#pragma comment(lib,"libpq.lib")
#endif

#define EXTRACT_BUFFER_SIZE	(16*1024*1024)
#define MATCH_BUFFER_SIZE	(128*1024*1024)
#define MIN_BUFFER_SIZE	(8*1024*1024)

#define LEAST_FEATURE_RATIO	0.05f

#define TABLE_NAME_ATMATCH_PROJ		"AtMatch_project"
#define TABLE_NAME_FEATURE			"AtMatch_feature"
#define TABLE_NAME_MATCH_MODEL		"AtMatch_model"

#define	DATABASE_NAME_FEATURE		"MPI_MATCH_FEATURE"	//"mpi_match_feature"
#define DATABASE_NAME_MODEL			"MPI_MATCH_MODEL"	//"mpi_match_model"

int		GlobalParam::g_extract_buffer_size = EXTRACT_BUFFER_SIZE;
int		GlobalParam::g_match_buffer_size = MATCH_BUFFER_SIZE;
float	GlobalParam::g_least_feature_ratio = LEAST_FEATURE_RATIO;
int		GlobalParam::g_match_pyrm_ratio = 3;

bool	GlobalParam::g_siftgpu_initial = false;
#ifdef ATMATCH_CODE
bool	GlobalParam::g_sift_extract_gpu = false;
bool	GlobalParam::g_sift_match_gpu = false;
#else
bool	GlobalParam::g_sift_extract_gpu = true;
bool	GlobalParam::g_sift_match_gpu = true;
#endif

#ifdef PQ_DB
char	GlobalParam::g_db_name[50] = "";
char	GlobalParam::g_db_user[50] = "";
char	GlobalParam::g_db_password[50] = "";
char	GlobalParam::g_db_hostaddr[50] = "";
char	GlobalParam::g_db_port[50] = "";

static PGconn*	g_manager_conn = NULL;
static PGconn*	g_fea_conn = NULL;
static PGconn*	g_model_conn = NULL;

const char* DBmanagerErrorMessage()
{
	return PQerrorMessage(g_manager_conn);
}

void*	DBmanagerExec(const char *query)
{
	return PQexec(g_manager_conn, query);
}
const char* DBfeaPtErrorMessage()
{
	return PQerrorMessage(g_fea_conn);
}

void*	DBfeaPtExec(const char *query)
{
	return PQexec(g_fea_conn, query);
}
const char* DBmodelErrorMessage()
{
	return PQerrorMessage(g_model_conn);
}

void*	DBmodelExec(const char *query)
{
	return PQexec(g_model_conn, query);
}

DBExecStatusType DBresultStatus(const void *res){
	return (DBExecStatusType)PQresultStatus((const PGresult*)res);
}

inline void DB_Close(PGconn* conn){
	if (conn) {
		PQfinish(conn); conn = NULL;
	}
}

inline bool DBCreate(const char* dbname){
	char sql[512];
//	sprintf(sql, "DROP DATABASE %s",dbname);	
	sprintf(sql, "	CREATE DATABASE \"%s\" WITH OWNER = \"%s\" ENCODING = \'UTF8\' TABLESPACE = pg_default 	LC_COLLATE = \'Chinese (Simplified)_China.936\' LC_CTYPE = \'Chinese (Simplified)_China.936\' CONNECTION LIMIT = -1;", dbname, GlobalParam::g_db_user);
	void* query_result = DBmanagerExec(sql);
	if (!query_result || DBresultStatus(query_result) != DBRES_COMMAND_OK) {
		DBclear(query_result);
		return false;
	}
	DBclear(query_result);
	return true;
}

inline PGconn* DB_Connect(const char* hostaddr, const char* port, const char* db_name, const char* db_user, const char* password){
// 	DB_Close();
	char sql[512];
	sprintf(sql, "dbname=%s user=%s password=%s host=%s port=%s", db_name, db_user, password, hostaddr, port);
	PGconn* conn = PQconnectdb(sql);
	if (PQstatus(conn) != CONNECTION_OK)
	{
		fprintf(stderr, "Connection to database failed: %s",
			PQerrorMessage(conn));
		return NULL;
	}
	return conn;
}

DBExecStatusType DBresultStatusz(const void *res){
	return (DBExecStatusType)PQresultStatus((const PGresult*)res);
}
int			DBntuples(const void *res){
	return PQntuples((const PGresult*)res);
}
char*		DBgetvalue(const void *res, int tup_num, int field_num){
	return PQgetvalue((const PGresult*)res, tup_num, field_num);
}
void		DBclear(void *res){
	PQclear((PGresult*)res);
}

bool		DBtableIsExist(PGconn* fea_conn, const char* fea_record_table_name)
{
	char sql[512];
	sprintf(sql, "select count(*) from pg_class where relname = \'%s\';", fea_record_table_name);
	PGresult* query_result = PQexec(fea_conn, sql);
	if (!query_result || DBresultStatus(query_result) != DBRES_TUPLES_OK || DBntuples(query_result) < 1){
		PQclear(query_result);
		return false;
	}
	bool ret = false;
	if (atoi(DBgetvalue(query_result, 0, 0))>0)  ret = true;
	PQclear(query_result);
	return ret;
}

inline bool		DB_create_prj_table(){
	if ( DBtableIsExist(g_manager_conn, TABLE_NAME_ATMATCH_PROJ) ){
		return true;
	}
	char sql[512];
	sprintf(sql, "CREATE TABLE \"%s\" ( "
				 "prj_name  VARCHAR(128) NOT NULL PRIMARY KEY,"
				 "prj_path  VARCHAR(512) NOT NULL,"
				 "create_time timestamp NOT NULL,"
				 "change_time timestamp NOT NULL);", TABLE_NAME_ATMATCH_PROJ);
	void* query_result = DBmanagerExec(sql);
	if (!query_result || DBresultStatus(query_result) != DBRES_COMMAND_OK) {
		DBclear(query_result);
		return false;
	}
	DBclear(query_result);
	return true;
}

inline bool		DBcreateProject(const char* lpstrWsPath)
{
	char name[512];	
	const char* pW = lpstrWsPath;
	char* pN = name;
	while (!*pW++)
	{
		if (*pW == '\\' || *pW == '/' || *pW == ':') *pN++ = *pW;
	}
	char sql[1024];
	sprintf(sql, "select * from \"%s\" where prj_name=\'%s\';", TABLE_NAME_ATMATCH_PROJ, name);
	void* query_result = DBmanagerExec(sql);
	if (!query_result || DBresultStatus(query_result) != DBRES_TUPLES_OK) {
		DBclear(query_result);
		sprintf(sql, "INSERT INTO \"%s\"\nVALUES(\'%s\',\'%s\',NOW(),NOW());", TABLE_NAME_ATMATCH_PROJ, name, lpstrWsPath);
		query_result = DBmanagerExec(sql);
		if (!query_result || DBresultStatus(query_result) != DBRES_COMMAND_OK) {
			DBclear(query_result);
			return false;
		}
		DBclear(query_result);
		return true;
	}
	DBclear(query_result);
	sprintf(sql, "update \"%s\" set change_time = NOW() where prj_name=\'%s\';", TABLE_NAME_ATMATCH_PROJ, name);
	query_result = DBmanagerExec(sql);
	if (!query_result || DBresultStatus(query_result) != DBRES_COMMAND_OK) {
		DBclear(query_result);
		return false;
	}
	DBclear(query_result);
	return true;
}

inline bool		DB_create_feature_table(const char* feature_table_name){
	if (DBtableIsExist(g_manager_conn, feature_table_name)){
		return true;
	}
	char sql[512];
	sprintf(sql, "CREATE TABLE \"%s\"("
				"image_name  VARCHAR(128) NOT NULL PRIMARY KEY,"
// 				"cols	integer,"
// 				"rows	integer,"
				"point_num	integer,"
				"extract_window integer,"
				"extract_interval integer);", feature_table_name);
	void* query_result = DBmanagerExec(sql);
	if (!query_result || DBresultStatus(query_result) != DBRES_COMMAND_OK) {
		DBclear(query_result);
		return false;
	}
	DBclear(query_result);
	return true;
}

inline bool			DBgetFeaInfo(const char* fea_record_table_name, const char* lpstrFeaTabName, int* ptSum, int* extract_win, int* extract_interval){
	char sql[512];
	sprintf(sql, "select * from \"%s\" where image_name=\'%s\';", fea_record_table_name, lpstrFeaTabName);
	void* query_result = DBmanagerExec(sql);
	if (!query_result || DBresultStatus(query_result) != DBRES_TUPLES_OK || DBntuples(query_result)<1){
		DBclear(query_result);
		return false;
	}
	*ptSum = atoi(DBgetvalue(query_result, 0, 1));
	*extract_win = atoi(DBgetvalue(query_result, 0, 2));
	*extract_interval = atoi(DBgetvalue(query_result, 0, 3));
	DBclear(query_result);
	return true;
}

inline int			DBcheckFea(const char* fea_record_table_name, const char* lpstrFeaTabName, int extract_win, int extract_interval)
{
	int db_ex_win = -1;
	int db_ex_interval = -1;
	int db_pt_sum = -1;
	if (!DBgetFeaInfo(fea_record_table_name, lpstrFeaTabName, &db_pt_sum, &db_ex_win, &db_ex_interval)) return 0;
	
	if (extract_win > 0 && extract_win != db_ex_win) return 0;
	if (extract_interval > 0 && extract_interval != db_ex_interval) return 0;
	return db_pt_sum;
}

inline bool		DBsetFeaTableInfo(const char* fea_record_table_name, const char* lpstrFeaTabName, int ptNum, int extract_win, int extract_interval){
	char sql[512];
	sprintf(sql, "update \"%s\" set point_num = %d,extract_window=%d,extract_interval=%d where image_name=\'%s\';", fea_record_table_name,ptNum, extract_win, extract_interval, lpstrFeaTabName);
	void* query_result = DBmanagerExec(sql);
	if (!query_result || DBresultStatus(query_result) != DBRES_COMMAND_OK) {
		DBclear(query_result);
		return false;
	}
	DBclear(query_result);
	return true;
}

inline bool		DBremoveFeaSet(PGconn* fea_conn, const char* fea_record_table_name, const char* lpstrFeaTabName){
	char sql[512];
	sprintf(sql, "DROP TABLE IF EXISTS \"%s\";", lpstrFeaTabName);
	void* query_result = PQexec(fea_conn, sql);	DBclear(query_result);
	sprintf(sql, "delete from \"%s\" where image_name=\'%s\';", fea_record_table_name, lpstrFeaTabName);
	query_result = DBmanagerExec(sql);
	if (!query_result || DBresultStatus(query_result) != DBRES_COMMAND_OK) {
		DBclear(query_result);
		return false;
	}
	DBclear(query_result);
	return true;
}

inline bool		DBcreateFeaSet(PGconn* fea_conn, const char* lpstrFeaTabName){
	char sql[512];
	sprintf(sql, "CREATE TABLE \"%s\" ("
				"point_id	VARCHAR(20) NOT NULL PRIMARY KEY,"
				"col			float NOT NULL,"
				"row			float NOT NULL,"
				"pyrmScale	integer NOT NULL,"
				"scale		float ,"
				"orietation	float, "
				"descriptors BYTEA);", lpstrFeaTabName);
	PGresult* query_result = PQexec(fea_conn, sql);
	if (!query_result || PQresultStatus(query_result) != PGRES_COMMAND_OK) {
		PQclear(query_result);
		return false;
	}
	PQclear(query_result);
	sprintf(sql, "create index  on \"%s\"(col);", lpstrFeaTabName);
	query_result = PQexec(fea_conn, sql);	PQclear(query_result);
	sprintf(sql, "create index  on \"%s\"(row);", lpstrFeaTabName);
	query_result = PQexec(fea_conn, sql);	PQclear(query_result);
	sprintf(sql, "create index  on \"%s\"(pyrmScale);", lpstrFeaTabName);
	query_result = PQexec(fea_conn, sql);	PQclear(query_result);
	return true;
}

inline bool		DBcreateFea(PGconn* fea_conn, const char* fea_record_table_name, const char* lpstrFeaTabName, int extract_win, int extract_interval)
{
	char sql[512];
	DBremoveFeaSet(fea_conn, fea_record_table_name, lpstrFeaTabName);
	
	if (!DBcreateFeaSet(fea_conn, lpstrFeaTabName)){
		return false;
	}

	sprintf(sql, "INSERT INTO \"%s\"\nVALUES(\'%s\',0,%d,%d);", fea_record_table_name,lpstrFeaTabName, extract_win, extract_interval);
	void* query_result = DBmanagerExec(sql);
	if (!query_result || DBresultStatus(query_result) != DBRES_COMMAND_OK) {
		DBclear(query_result);
		return false;
	}
	DBclear(query_result);
	return true;
}

bool	DBFeaPtTableIsExist(const char* lpstrFeaTabName)
{
	return DBtableIsExist(g_fea_conn, lpstrFeaTabName);
}

inline void hex_to_byte(char hex, BYTE* c){
	if (hex >= '0'&& hex <= '9') *c = hex - '0';
	else *c = hex - 'A' + 10;
}

inline void	DBcvtHexFormat2BYTE(const char* hex, BYTE* str, int sz){
	for (int i = 0; i < sz; i++){
		BYTE l, h;
		hex_to_byte(*hex, &h);	hex++;
		hex_to_byte(*hex, &l);	hex++;
		*str = (h << 4) | l;
		str++;
	}
}

inline void	DBcvtLiteral2BYTE(const char* literal, BYTE* str, int sz){
//	LogPrint(ERR_UNKNOW, "literal bytea= %s", literal);
	for (int i = 0; i < sz; i++){
		if (*literal == '\\') {
			literal++;
			*str = (*literal - '0') << 6;	literal++;
			*str = *str | ((*literal - '0') << 3);	literal++;
			*str = *str | (*literal - '0');	
		}
		else{
			*str = *literal;
		}
		literal++; str++;
	}
	return;
}

inline void byte_to_hex(BYTE c, char* hex){
	if (c < 10) *hex = '0' + c;
	else *hex = 'A' + c - 10;
}

inline void	DBcvtBYTE2HexFormat(const BYTE* str, int sz, char* hex){
	for (int i = 0; i < sz; i++){	
		byte_to_hex(*str >> 4, hex);	hex++;
		byte_to_hex(*str & 0x0F, hex);	hex++;
		str++;
	}
}

int		DBinsertFeaData(PGconn* fea_conn, const char* lpstrFeaTabName, 
	unsigned int idx, const char* lpstrImageID, int stCol, int stRow, int scale,
	float* x, float* y, float* fea, int feaDecriDimension, void* descrip, int decriDimension, int nBufferSpace[4], int ptSum, void(*pFunCvtDescrp)(void* from, BYTE*& to, int len)){
	BYTE des[128];	memset(des, 0, sizeof(BYTE)* 128);	BYTE* pDes = des;	char* pDes_from = (char*)descrip;
	char des16[128 * 2 + 8];	memset(des16, 0, 128 * 2 + 8);
	char sql[256 * 256];	
	sprintf(sql, "INSERT INTO \"%s\"\nVALUES ", lpstrFeaTabName);
	int len_str = 256 * 256 - 512 - strlen(sql);
	char* pSt = sql + strlen(sql);
	char* pCur = pSt;

	int len_cnt = 0;
	int cnt = 0;
	while (ptSum-->0)
	{
		pFunCvtDescrp(pDes_from, pDes, decriDimension);
		DBcvtBYTE2HexFormat(pDes, decriDimension, des16);
// 		BYTE* des16;	size_t length;
// 		des16 = ::PQescapeByteaConn(fea_conn,des, 128, &length);

//		sprintf(pCur, "(\'%010u\',%.2f,%.2f,%d,%f,%f,E\'%s\')", idx++, *x*scale + stCol, *y*scale + stRow, scale, *fea, *(fea + 1), des16);	PQfreemem(des16);
		sprintf(pCur, "(\'%010u\',%.2f,%.2f,%d,%f,%f,E\'\\\\x%s\')", idx++, *x*scale + stCol, *y*scale + stRow,scale, *fea, *(fea + 1),des16);
		x += nBufferSpace[0];	y += nBufferSpace[1];	fea += nBufferSpace[2];	pDes_from += nBufferSpace[3];
		cnt++;
		len_cnt += strlen(pCur) + 1;
		if (len_cnt >= len_str){
			strcat(pCur, ";");
			PGresult* query_result = PQexec(fea_conn, sql);
			PQclear(query_result);
			len_cnt = 0;
			pCur = pSt;
		}
		else{
			strcat(pCur, ",");
			pCur += strlen(pCur);
		}
	}
	if (len_cnt > 0){
		pCur[strlen(pCur) - 1] = ';';
		PGresult* query_result = PQexec(fea_conn, sql);
		PQclear(query_result);
	}
	return cnt;
}

int		DBselectFeaData(PGconn* fea_conn, const char* lpstrFeaTabName,
	int stCol, int stRow, int nCols, int nRows, int scale, CArray_PtName* name_array, CArray_PtLoc* location_array, CArray_PtFea* fea_array, CArray_SiftDes* des_array)
{
	char sql[512];	size_t length;
	sprintf(sql, "select * from \"%s\" where col between %d and %d and row between %d and %d", lpstrFeaTabName, stCol, stCol + nCols, stRow, stRow + nRows);
	char* pS = sql + strlen(sql);
	if (scale > 0) sprintf(pS, " and pyrmScale = %d;", scale); else strcpy(pS, ";");
	PGresult * query_result = PQexec(fea_conn, sql);
	if (!query_result || PQresultStatus(query_result) != DBRES_TUPLES_OK){
		PQclear(query_result);
		return 0;
	}
	int sz = PQntuples(query_result);
	PtName* name = NULL;	PtLoc* loc = NULL;	PtFea* fea = NULL;	SiftDes* des = NULL;
	{
		int old_sz = name_array->size();
		name_array->resize(old_sz + sz);	name = &name_array->at(old_sz);
		location_array->resize(old_sz + sz);	loc = &location_array->at(old_sz);
		if (fea_array){
			fea_array->resize(old_sz + sz);	fea = &fea_array->at(old_sz);
		}
		if (des_array){
			des_array->resize(old_sz + sz);	des = &des_array->at(old_sz);
		}
	}
	for (int i = 0; i < sz; i++){
		strcpy((char*)name, PQgetvalue(query_result, i, 0));	name++;
 		loc->x = atof(PQgetvalue(query_result, i, 1));
		loc->y = atof(PQgetvalue(query_result, i, 2));	loc++;
		if (fea_array){
			fea->scale = atof(PQgetvalue(query_result, i, 4));
			fea->orietation = atof(PQgetvalue(query_result, i, 5)); fea++;
//			fea_array->Append(fea);
		}
		if (des_array){
// 			BYTE* desRet = PQunescapeBytea((const BYTE*)PQgetvalue(query_result, i, 6), &length);				
// 			memcpy(des, desRet, length);	des++;
// 			PQfreemem(desRet);
			char * str = PQgetvalue(query_result, i, 6);
			if (str[1] == 'x' || str[1] == 'X') { DBcvtHexFormat2BYTE(str + 2, (BYTE*)des, 128); }
			else {
				DBcvtLiteral2BYTE(str, (BYTE*)des, 128);
//				sz = 0;  goto SFD_lp;
			}
			des++;
//			des_array->Append(des);
		}
//		name_array->Append(name);	location_array->Append(loc);
			
	}
SFD_lp:
	PQclear(query_result);
	return sz;
}

bool DBgetFeaPtTableInfo(const char* lpstrFeaTabName, int* ptSum, int* extract_win, int* extract_interval)
{
	return DBgetFeaInfo(TABLE_NAME_FEATURE, lpstrFeaTabName, ptSum, extract_win, extract_interval);
}

bool DBcreateFeaPtTable(const char* lpstrFeaTabName, int extract_win, int extract_interval)
{
	return DBcreateFea(g_fea_conn, TABLE_NAME_FEATURE, lpstrFeaTabName, extract_win, extract_interval);
}

inline void CvtDescrip_float_to_byte(void* from, BYTE*& to, int len){
	CvtDescriptorsf2uc((float*)from, to, len);
}
inline void CvtDescrip_do_nothing(void* from, BYTE*& to, int len){
	to = (BYTE*)from;
}
int DBinsertFeaPts(const char* lpstrFeaTabName, unsigned int idx, const char* lpstrImageID, int stCol, int stRow, int scale, float* x, float* y, float* fea, int feaDecriDimension, float* descrip, int decriDimension, int nBufferSpace[4], int ptSum)
{
	nBufferSpace[3] *= sizeof(float);
	return DBinsertFeaData(g_fea_conn, lpstrFeaTabName, idx, lpstrImageID, stCol, stRow, scale, x, y, fea, feaDecriDimension, descrip, decriDimension, nBufferSpace, ptSum, CvtDescrip_float_to_byte);
}
int DBinsertFeaPts(const char* lpstrFeaTabName, unsigned int idx, const char* lpstrImageID, int stCol, int stRow, int scale,
	float* x, float* y, float* fea, int feaDecriDimension, BYTE* descrip, int decriDimension, int nBufferSpace[4], int ptSum){
	return DBinsertFeaData(g_fea_conn, lpstrFeaTabName, idx, lpstrImageID, stCol, stRow, scale, x, y, fea, feaDecriDimension, descrip, decriDimension, nBufferSpace, ptSum, CvtDescrip_do_nothing);
}

bool		DBremoveFeaPtTable(const char* lpstrFeaTabName){
	return DBremoveFeaSet(g_fea_conn, TABLE_NAME_FEATURE, lpstrFeaTabName);
}

bool		DBsetFeaPtTableInfo(const char* lpstrFeaTabName, int ptNum, int extract_win, int extract_interval)
{
	return DBsetFeaTableInfo(TABLE_NAME_FEATURE, lpstrFeaTabName, ptNum, extract_win, extract_interval);
}

int			DBselectFeaPts(const char* lpstrFeaTabName, int stCol, int stRow, int nCols, int nRows, int scale, CArray_PtName* name_array, CArray_PtLoc* location_array, CArray_PtFea* fea_array, CArray_SiftDes* des_array){
	return DBselectFeaData(g_fea_conn, lpstrFeaTabName, stCol, stRow, nCols, nRows, scale, name_array, location_array, fea_array, des_array);
}

bool	DB_create_model_table(){
	if (DBtableIsExist(g_manager_conn, TABLE_NAME_MATCH_MODEL)){
		return true;
	}
	char sql[512];
	sprintf(sql, "CREATE TABLE \"%s\" ("
				"model_name VARCHAR(128) NOT NULL PRIMARY KEY,"
				"left_image_path	VARCHAR(512) NOT NULL,"
				"left_image_geoinfo	 VARCHAR(512) NOT NULL,"
				"left_image_id	 VARCHAR(30) NOT NULL,"
				"right_image_path	 VARCHAR(512) NOT NULL,"
				"right_image_geoinfo	 VARCHAR(512) NOT NULL,"
				"right_image_id	 VARCHAR(30) NOT NULL,"
				"point_num	integer);", TABLE_NAME_MATCH_MODEL);
	void* query_result = DBmanagerExec(sql);
	if (!query_result || DBresultStatus(query_result) != DBRES_COMMAND_OK) {
		DBclear(query_result);
		return false;
	}
	DBclear(query_result);
	return true;
}
#endif

#define strlwr _strlwr

class std_global_register
{
public:
	std_global_register()
	{
		char config[256] = "";  readlink("/proc/self/exe", config, 256);
		char* pS = strrchr(config, '.');	if (!pS) pS = config + strlen(config);
		strcpy(pS, ".ini");
		char strVal[256];
		::GetPrivateProfileString("Config", "path", "", strVal, 512, config);
		
		if (!IsExist(strVal)){
			char tmp[256];	strcpy(tmp, config);
			char* pS = strrpath(tmp);
			if (pS) {
				strcpy(pS + 1, strVal);	strcpy(strVal, tmp);
			}
		}
		if (IsExist(strVal) && !IsDir(strVal))	strcpy(config, strVal);
		

		::GetPrivateProfileString("Match", "extract_buffer_size(MB)", "", strVal, 20, config);
		if (strlen(strVal) > 0)	GlobalParam::g_extract_buffer_size = int(1024 * 1024 * atof(strVal));
		::GetPrivateProfileString("Match", "match_buffer_size(MB)", "", strVal, 20, config);
		if (strlen(strVal) > 0)	GlobalParam::g_match_buffer_size = int(1024 * 1024 * atof(strVal));
		::GetPrivateProfileString("Match", "least_feature_ratio", "", strVal, 20, config);
		if (strlen(strVal) > 0)	GlobalParam::g_least_feature_ratio = (float)atof(strVal);
		::GetPrivateProfileString("Sift", "gpu_extract", "", strVal, 20, config);
		if (strlen(strVal) > 0){
			strlwr(strVal);
			if (strVal[0] == 'n' || strVal[0] == '0' || strVal[0] == 'f') GlobalParam::g_sift_extract_gpu = false;
		}
#ifdef PQ_DB
		::GetPrivateProfileString("database", "name", "lx", GlobalParam::g_db_name, 50, config);
		::GetPrivateProfileString("database", "user", "LX_whu", GlobalParam::g_db_user, 50, config);
		::GetPrivateProfileString("database", "password", "LX_whu", GlobalParam::g_db_password, 50, config);
		::GetPrivateProfileString("database", "host", "localhost", GlobalParam::g_db_hostaddr, 50, config);
		::GetPrivateProfileString("database", "port", "5432", GlobalParam::g_db_port, 50, config);
		g_manager_conn = DB_Connect(GlobalParam::g_db_hostaddr, GlobalParam::g_db_port, GlobalParam::g_db_name, GlobalParam::g_db_user, GlobalParam::g_db_password);
		if (!g_manager_conn){
			exit(-1);
		}
		
		g_fea_conn = DB_Connect(GlobalParam::g_db_hostaddr, GlobalParam::g_db_port, DATABASE_NAME_FEATURE, GlobalParam::g_db_user, GlobalParam::g_db_password);
		if (!g_fea_conn){
			DBCreate(DATABASE_NAME_FEATURE);
			g_fea_conn = DB_Connect(GlobalParam::g_db_hostaddr, GlobalParam::g_db_port, DATABASE_NAME_FEATURE, GlobalParam::g_db_user, GlobalParam::g_db_password);
			if (!g_fea_conn)
				exit(-1);
		}
		
		g_model_conn = DB_Connect(GlobalParam::g_db_hostaddr, GlobalParam::g_db_port, DATABASE_NAME_MODEL, GlobalParam::g_db_user, GlobalParam::g_db_password);
		if (!g_model_conn){
			DBCreate(DATABASE_NAME_MODEL);
			g_model_conn = DB_Connect(GlobalParam::g_db_hostaddr, GlobalParam::g_db_port, DATABASE_NAME_MODEL, GlobalParam::g_db_user, GlobalParam::g_db_password);
			if (!g_model_conn)
				exit(-1);
		}
		DB_create_prj_table();
		DB_create_feature_table(TABLE_NAME_FEATURE);
		DB_create_model_table();
		
#endif
	}
	~std_global_register(){
#ifdef PQ_DB
		DB_Close(g_manager_conn);
		DB_Close(g_fea_conn);
		DB_Close(g_model_conn);
#endif
	}
} g_global_register;

