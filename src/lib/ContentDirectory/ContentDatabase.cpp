/***************************************************************************
 *            ContentDatabase.cpp
 *
 *  FUPPES - Free UPnP Entertainment Service
 *
 *  Copyright (C) 2005-2009 Ulrich Völkel <u-voelkel@users.sourceforge.net>
 ****************************************************************************/

/*
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License
 *  as published by the Free Software Foundation; either version 2
 *  of the License, or (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License along
 *  with this program; if not, write to the Free Software Foundation, Inc.,
 *  51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 */
 
#ifdef HAVE_CONFIG_H
#include "../../config.h"
#endif

#include "ContentDatabase.h"
#include "../SharedConfig.h"
#include "../SharedLog.h"
#include "FileDetails.h"
#include "../Common/RegEx.h"
#include "../Common/Common.h"
#include "iTunesImporter.h"
#include "PlaylistParser.h"

#include <sstream>
#include <string>
#include <stdio.h>
#include <iostream>
#include <fstream>
#include <cstdio>
#ifndef WIN32
#include <dirent.h>
#endif
#include <sys/types.h>
#include <sys/stat.h> 
 
using namespace std;

static bool g_bIsRebuilding;
static bool g_bFullRebuild;
static bool g_bAddNew;
static bool g_bRemoveMissing;

unsigned int g_nObjId = 0;

std::string CSelectResult::GetValue(std::string p_sFieldName)
{
  return m_FieldValues[p_sFieldName];
}

bool CSelectResult::IsNull(std::string p_sFieldName)
{
  std::string sValue = GetValue(p_sFieldName);
  if((sValue.length() == 0) || (sValue.compare("NULL") == 0))
    return true;
  else
    return false;
}

unsigned int CSelectResult::GetValueAsUInt(std::string p_sFieldName)
{
  unsigned int nResult = 0;
  if(!IsNull(p_sFieldName)) {
    nResult = strtoul(GetValue(p_sFieldName).c_str(), NULL, 0);
  }
  return nResult;
}

int CSelectResult::asInt(std::string fieldName)
{
	int result = 0;
  if(!IsNull(fieldName)) {
    result = atoi(GetValue(fieldName).c_str());
  }
  return result;
}


CContentDatabase* CContentDatabase::m_Instance = 0;

CContentDatabase* CContentDatabase::Shared()
{
	if(m_Instance == 0) {
		m_Instance = new CContentDatabase(true);
  }
	return m_Instance;
}

CContentDatabase::CContentDatabase(bool p_bShared)
{ 
	m_pDbHandle 			= NULL;
	m_nRowsReturned 	= 0;  
  m_RebuildThread 	= (fuppesThread)NULL;	
  m_bShared       	= p_bShared;	
  m_bInTransaction 	= false;
  m_nLockCount 			= -1;
  
	if(m_bShared) {            
		m_sDbFileName 	  = CSharedConfig::Shared()->GetDbFileName();	
		g_bIsRebuilding   = false;
    g_bFullRebuild    = true;
    g_bAddNew         = false;
    g_bRemoveMissing  = false;    
    m_nLockCount 		  = 0;
    
    m_pFileAlterationMonitor = CFileAlterationMgr::Shared()->CreateMonitor(this);
    fuppesThreadInitMutex(&m_Mutex);
  }
    
  if(!m_bShared) {
    if(!Open())
      cout << "FAILED OPENING DB FILE" << endl;
  }  
}
 
CContentDatabase::~CContentDatabase()
{                            
	if(m_bShared) {                  
    fuppesThreadDestroyMutex(&m_Mutex);
    delete m_pFileAlterationMonitor;    
  }
	
	if(m_bShared && (m_RebuildThread != (fuppesThread)NULL)) {
	  fuppesThreadClose(m_RebuildThread);
		m_RebuildThread = (fuppesThread)NULL;
	}

  ClearResult();  
  Close();
}

std::string CContentDatabase::GetLibVersion()
{
  return sqlite3_libversion();
}

bool CContentDatabase::Init(bool* p_bIsNewDB)
{   
  if(!m_bShared) {
		return false;
	}
  
  bool bIsNewDb = !FileExists(m_sDbFileName);
  *p_bIsNewDB = bIsNewDb;  
  
  Open();
  
  if(bIsNewDb) {
	
    if(!Execute("CREATE TABLE OBJECTS ( "
				"  ID INTEGER PRIMARY KEY AUTOINCREMENT, "
				"  OBJECT_ID INTEGER NOT NULL, "
        "  DETAIL_ID INTEGER DEFAULT NULL, "
				"  TYPE INTEGER NOT NULL, "
        "  DEVICE TEXT DEFAULT NULL, "  
				"  PATH TEXT NOT NULL, "
				"  FILE_NAME TEXT DEFAULT NULL, "
				"  TITLE TEXT DEFAULT NULL, "
				"  MD5 TEXT DEFAULT NULL, "
				"  MIME_TYPE TEXT DEFAULT NULL, "
				"  REF_ID INTEGER DEFAULT NULL "
				//"  UPDATE_ID INTEGER DEFAULT 0"
				");"))   
			return false;
    
    if(!Execute("CREATE TABLE OBJECT_DETAILS ( "
        "  ID INTEGER PRIMARY KEY AUTOINCREMENT, "
        "  AV_BITRATE INTEGER, "
				"  AV_DURATION TEXT, "
				"  A_ALBUM TEXT, "
				"  A_ARTIST TEXT, "
				"  A_CHANNELS INTEGER, "
				"  A_DESCRIPTION TEXT, "
				"  A_GENRE TEXT, "
				"  A_SAMPLERATE INTEGER, "
				"  A_TRACK_NO INTEGER, "
				"  DATE TEXT, "
				"  IV_HEIGHT INTEGER, "
				"  IV_WIDTH INTEGER, "
        "  A_CODEC TEXT, "
        "  V_CODEC TEXT, "
				"  ALBUM_ART_ID INTEGER, "
				"  ALBUM_ART_MIME_TYPE TEXT, "
        "  SIZE INTEGER DEFAULT 0, "
				"  DLNA_PROFILE TEXT DEFAULT NULL,"
				"  DLNA_MIME_TYPE TEXT DEFAULT NULL"
				");"))   
			return false;
    
    if(!Execute("CREATE TABLE MAP_OBJECTS ( "
        "  ID INTEGER PRIMARY KEY AUTOINCREMENT, "
        "  OBJECT_ID INTEGER NOT NULL, "
        "  PARENT_ID INTEGER NOT NULL, "
        "  DEVICE TEXT "
        ");"))
      return false;               
    

    if(!Execute("CREATE INDEX IDX_OBJECTS_OBJECT_ID ON OBJECTS(OBJECT_ID);"))
      return false;
    
    if(!Execute("CREATE INDEX IDX_MAP_OBJECTS_OBJECT_ID ON MAP_OBJECTS(OBJECT_ID);"))
      return false;
    
    if(!Execute("CREATE INDEX IDX_MAP_OBJECTS_PARENT_ID ON MAP_OBJECTS(PARENT_ID);"))
      return false;	
        
    if(!Execute("CREATE INDEX IDX_OBJECTS_DETAIL_ID ON OBJECTS(DETAIL_ID);"))
      return false;

    if(!Execute("CREATE INDEX IDX_OBJECT_DETAILS_ID ON OBJECT_DETAILS(ID);"))
      return false;
  }
	
  Execute("pragma temp_store = MEMORY");
  Execute("pragma synchronous = OFF;");  
  
	
	// setup file alteration monitor
	if(m_pFileAlterationMonitor->isActive()) {
		
		Select("select PATH from OBJECTS where TYPE = 1 and DEVICE is NULL");
		while(!Eof()) {
			m_pFileAlterationMonitor->addWatch(GetResult()->GetValue("PATH"));
			Next();
		}
	}
	
  return true;
}

void CContentDatabase::Lock()
{
  if(m_bShared) {
    fuppesThreadLockMutex(&m_Mutex);
    //m_nLockCount++;
    //cout << "CDB LOCK: " << m_nLockCount << endl; fflush(stdout);    
  }
  else
    CContentDatabase::Shared()->Lock();
}

void CContentDatabase::Unlock()
{
  if(m_bShared) {
    //cout << "CDB UNLOCK: " << m_nLockCount << endl; fflush(stdout);
    //m_nLockCount--;    
    fuppesThreadUnlockMutex(&m_Mutex);    
  }
  else
    CContentDatabase::Shared()->Unlock();
}

void CContentDatabase::ClearResult()
{
  // clear old results
  for(m_ResultListIterator = m_ResultList.begin(); m_ResultListIterator != m_ResultList.end();)
  {
    if(m_ResultList.empty())
      break;
    
    CSelectResult* pResult = *m_ResultListIterator;
    std::list<CSelectResult*>::iterator tmpIt = m_ResultListIterator;          
    ++tmpIt;
    m_ResultList.erase(m_ResultListIterator);
    m_ResultListIterator = tmpIt;
    delete pResult;
  } 
  
  m_ResultList.clear();
  m_nRowsReturned = 0;
}

bool CContentDatabase::Open()
{  
  if(!m_bShared) {	
		m_pDbHandle = CContentDatabase::Shared()->m_pDbHandle;		
		return (m_pDbHandle != NULL);
	}	

  if(m_pDbHandle != NULL) {
    return true;
  }
  
  if(sqlite3_open(m_sDbFileName.c_str(), &m_pDbHandle) != SQLITE_OK) {
    fprintf(stderr, "Can't create/open database: %s\n", sqlite3_errmsg(m_pDbHandle));
    sqlite3_close(m_pDbHandle);
    return false;
  }
  //JM: Tell sqlite3 to retry queries for up to 1 second if the database is locked.
  sqlite3_busy_timeout(m_pDbHandle, 1000);
	
  Select("select max(OBJECT_ID) as VALUE from OBJECTS where DEVICE is NULL");
  if(!Eof()) {  
    g_nObjId = GetResult()->GetValueAsUInt("VALUE");
  }
  
	return true;
}

void CContentDatabase::Close()
{
  if(m_bShared) {
    sqlite3_close(m_pDbHandle);
    m_pDbHandle = NULL;
	}
}

void CContentDatabase::BeginTransaction()
{
  if(m_bInTransaction) {
    return;
  }
  
  Lock();  
  char* szErr = 0;
  
  if(sqlite3_exec(m_pDbHandle, "BEGIN TRANSACTION;", NULL, NULL, &szErr) != SQLITE_OK) {
    fprintf(stderr, "CContentDatabase::BeginTransaction() :: SQL error: %s\n", szErr);
  }
  else {
    m_bInTransaction = true;
  }

  Unlock();
}

void CContentDatabase::Commit()
{
  if(!m_bInTransaction) {
    return;
  }
  
  Lock();
  char* szErr = 0;  
  
  if(sqlite3_exec(m_pDbHandle, "COMMIT;", NULL, NULL, &szErr) != SQLITE_OK) {
    fprintf(stderr, "CContentDatabase::Commit() :: SQL error: %s\n", szErr);
  }
  else {
    m_bInTransaction = false;
  }
  
  Unlock();  
}

unsigned int CContentDatabase::Insert(std::string p_sStatement)
{  
  bool bTransaction = false;
  
  if(!m_bInTransaction) {
    bTransaction = true;
    BeginTransaction();
  }

  Lock();
  
  char* szErr  = NULL;
  bool  bRetry = true;
  int   nResult;
    
  while(bRetry) {  
    
    nResult = sqlite3_exec(m_pDbHandle, p_sStatement.c_str(), NULL, NULL, &szErr);  
    switch(nResult) {
      case SQLITE_BUSY:
        bRetry = true;
        fuppesSleep(50);
        break;
      
      case SQLITE_OK:
        bRetry  = false;
        nResult = sqlite3_last_insert_rowid(m_pDbHandle);
        break;
      
      default:
        bRetry = false;
        CSharedLog::Log(L_NORM, __FILE__, __LINE__, "CContentDatabase::Insert - insert :: SQL error: %s\nStatement: %s", szErr, p_sStatement.c_str());
        sqlite3_free(szErr);
        nResult = 0;
        break;
    }
    
  }
   
  Unlock();
  
  if(bTransaction) {
    Commit();
  }
  
  return nResult;  
}

bool CContentDatabase::Execute(std::string p_sStatement)
{
  Lock();
  //Open();
	bool bResult = false;
  char* szErr  = NULL;
	
  int nStat = sqlite3_exec(m_pDbHandle, p_sStatement.c_str(), NULL, NULL, &szErr);  
  if(nStat != SQLITE_OK) {
    fprintf(stderr, "CContentDatabase::Execute :: SQL error: %s, statement: \n", szErr, p_sStatement.c_str());  
    sqlite3_free(szErr);  
    bResult = false;
  }
  else {
    bResult = true;
  }
	
	//Close();
	Unlock();
	return bResult;
}

bool CContentDatabase::Select(std::string p_sStatement)
{  
  Lock();
  //Open();  
  ClearResult();    
  bool bResult = true;
  
  char* szErr = NULL;
  char** szResult;
  int nRows = 0;
  int nCols = 0;
  
  int nResult = SQLITE_OK;  
  int nTry = 0;
  
  CSharedLog::Log(L_DBG, __FILE__, __LINE__, "SELECT %s", p_sStatement.c_str());
  
  do {
    nResult = sqlite3_get_table(m_pDbHandle, p_sStatement.c_str(), &szResult, &nRows, &nCols, &szErr);   
    if(nTry > 0) {      
      //CSharedLog::Shared()->Log(L_EXTENDED_WARN, "SQLITE_BUSY", __FILE__, __LINE__);
      fuppesSleep(100);
    }
    nTry++;
  }while(nResult == SQLITE_BUSY);
    
  if(nResult != SQLITE_OK) {    
    CSharedLog::Log(L_DBG, __FILE__, __LINE__, "SQL error: %s, Statement: %s\n", szErr, p_sStatement.c_str());
    sqlite3_free(szErr);
    bResult = false;
  }
  else {

   CSelectResult* pResult;
       
    for(int i = 1; i < nRows + 1; i++) {
      pResult = new CSelectResult();
            
      for(int j = 0; j < nCols; j++) {        
        pResult->m_FieldValues[szResult[j]] =  szResult[(i * nCols) + j] ? szResult[(i * nCols) + j] : "NULL";
      }
      
      m_ResultList.push_back(pResult);
      m_nRowsReturned++;
    }
    m_ResultListIterator = m_ResultList.begin();       
       
    sqlite3_free_table(szResult);
  }
	Unlock();
	
  return bResult;
}

bool CContentDatabase::Eof()
{
  return (m_ResultListIterator == m_ResultList.end());
}
    
CSelectResult* CContentDatabase::GetResult()
{
  return *m_ResultListIterator;
}

void CContentDatabase::Next()
{
  if(m_ResultListIterator != m_ResultList.end()) {
    m_ResultListIterator++;
  } 
}



fuppesThreadCallback BuildLoop(void *arg);
void DbScanDir(std::string p_sDirectory, long long int p_nParentId);
void BuildPlaylists();
void ParsePlaylist(CSelectResult* pResult);
void ParseM3UPlaylist(CSelectResult* pResult);
void ParsePLSPlaylist(CSelectResult* pResult);


unsigned int InsertFile(CContentDatabase* pDb, unsigned int p_nParentId, std::string p_sFileName);

unsigned int GetObjectIDFromFileName(CContentDatabase* pDb, std::string p_sFileName);

void CContentDatabase::BuildDB()
{     
  if(CContentDatabase::Shared()->IsRebuilding())
	  return;
	
	if(!m_bShared)
	  return;
		
	if(m_RebuildThread != (fuppesThread)NULL) {
	  fuppesThreadClose(m_RebuildThread);
		m_RebuildThread = (fuppesThread)NULL;
	}
		
	fuppesThreadStart(m_RebuildThread, BuildLoop);
  g_bIsRebuilding = true;  
}

void CContentDatabase::RebuildDB()
{     
  if(CContentDatabase::Shared()->IsRebuilding())
	  return;
	
	if(!m_bShared)
	  return;
	
  g_bFullRebuild = true;
  g_bAddNew = false;
  g_bRemoveMissing = false;
  
  BuildDB();
}

void CContentDatabase::UpdateDB()
{
  if(CContentDatabase::Shared()->IsRebuilding())
    return;
  
  if(!m_bShared)
    return;
  
  g_bFullRebuild = false;
  g_bAddNew = true;
  g_bRemoveMissing = true;  
    
  Select("select max(OBJECT_ID) as VALUE from OBJECTS where DEVICE is NULL");
  if(!Eof()) {  
    g_nObjId = GetResult()->GetValueAsUInt("VALUE");
  }
  ClearResult();
  
  BuildDB();
}

void CContentDatabase::AddNew()
{
  if(CContentDatabase::Shared()->IsRebuilding())
    return;
  
  if(!m_bShared)
    return;
  
  g_bFullRebuild = false;
  g_bAddNew = true;
  g_bRemoveMissing = false;  
  
  Select("select max(OBJECT_ID) as VALUE from OBJECTS where DEVICE is NULL");
  if(!Eof()) {  
    g_nObjId = GetResult()->GetValueAsUInt("VALUE");
  } 
  ClearResult();
  
  BuildDB();
}

void CContentDatabase::RemoveMissing()
{
  if(CContentDatabase::Shared()->IsRebuilding())
    return;
  
  if(!m_bShared)
    return;
  
  g_bFullRebuild = false;
  g_bAddNew = false;
  g_bRemoveMissing = true;  
  
  BuildDB();
}


bool CContentDatabase::IsRebuilding()
{
  return g_bIsRebuilding;
}

unsigned int CContentDatabase::GetObjId()
{
  return ++g_nObjId;
}

void DbScanDir(CContentDatabase* pDb, std::string p_sDirectory, long long int p_nParentId)
{
  // append trailing slash if neccessary
  if(p_sDirectory.substr(p_sDirectory.length()-1).compare(upnpPathDelim) != 0) {
    p_sDirectory += upnpPathDelim;
  }    
     
  #ifdef WIN32
  char szTemp[MAX_PATH];
  strcpy(szTemp, p_sDirectory.c_str());
  
  // Add search criteria
  strcat(szTemp, "*");

  /* Find first file */
  WIN32_FIND_DATA data;
  HANDLE hFile = FindFirstFile(szTemp, &data);
  if(NULL == hFile)
    return;

  /* Loop trough all subdirectories and files */
  while(TRUE == FindNextFile(hFile, &data))
  {
    if(((string(".").compare(data.cFileName) != 0) && 
      (string("..").compare(data.cFileName) != 0)))
    {        
      
      /* Save current filename */
      strcpy(szTemp, p_sDirectory.c_str());
      //strcat(szTemp, upnpPathDelim);
      strcat(szTemp, data.cFileName);
      
      string sTmp;
      sTmp = szTemp;
      
      string sTmpFileName = data.cFileName;
  #else
      
  DIR*    pDir;
  dirent* pDirEnt;
  string  sTmp;
  
  if((pDir = opendir(p_sDirectory.c_str())) != NULL)
  {    
    CSharedLog::Log(L_EXT, __FILE__, __LINE__, "read directory: %s",  p_sDirectory.c_str());
    while((pDirEnt = readdir(pDir)) != NULL)
    {
      if(((string(".").compare(pDirEnt->d_name) != 0) && 
         (string("..").compare(pDirEnt->d_name) != 0)))
      {        
        sTmp = p_sDirectory + pDirEnt->d_name;        
        string sTmpFileName = pDirEnt->d_name;        
  #endif  
                
        string sExt = ExtractFileExt(sTmp);
        unsigned int nObjId = 0;
        
        /* directory */
        if(IsDirectory(sTmp))
        {				
          sTmpFileName = ToUTF8(sTmpFileName);          
          sTmpFileName = SQLEscape(sTmpFileName);        
          
          stringstream sSql;
          
          appendTrailingSlash(&sTmp);
          
          if(g_bAddNew) {
            nObjId = GetObjectIDFromFileName(pDb, sTmp);
          }
          
          if(nObjId == 0) {
            nObjId = pDb->GetObjId();          
            pDb->BeginTransaction();          
                
            sSql << "insert into objects ( " <<
              "  OBJECT_ID, TYPE, " <<
              "  PATH, FILE_NAME, TITLE) " <<
              "values ( " << 
                 nObjId << ", " <<  
                 CONTAINER_STORAGE_FOLDER << 
                 ", '" << SQLEscape(sTmp) << "', '" <<
                 sTmpFileName << "', '" <<
                 sTmpFileName <<
              "');";

            pDb->Insert(sSql.str());          
            
            sSql.str("");
            sSql << "insert into MAP_OBJECTS (OBJECT_ID, PARENT_ID) " <<
                    "values (" << nObjId << ", " << p_nParentId << ")";          
            pDb->Insert(sSql.str());
            
            pDb->Commit();
            
            pDb->fileAlterationMonitor()->addWatch(sTmp);
          }
            
          // recursively scan subdirectories
          DbScanDir(pDb, sTmp, nObjId);          
        }
        else if(IsFile(sTmp) && CFileDetails::Shared()->IsSupportedFileExtension(sExt))
        {
          InsertFile(pDb, p_nParentId, sTmp);
        }         
       
      }
    }  /* while */  
  #ifndef WIN32
    closedir(pDir);
  } /* if opendir */
  #endif         
}

unsigned int InsertAudioFile(CContentDatabase* pDb, unsigned int objectId, std::string p_sFileName, std::string* p_sTitle)
{
	metadata_t metadata;
	init_metadata(&metadata);
	
	if(!CFileDetails::Shared()->getMusicTrackDetails(p_sFileName, &metadata)) {
		free_metadata(&metadata);
	  return 0;
	}

  string sDlna; // = CFileDetails::Shared()->GuessDLNAProfileId(p_sFileName);
  fuppes_off_t fileSize = getFileSize(p_sFileName);
	
	unsigned int imgId = 0;
	string imgMimeType;
	if(metadata.has_image == 1) {
		imgId = objectId;
		imgMimeType = metadata.image_mime_type;
	}
	
	stringstream sSql;
	sSql << 
	  "insert into OBJECT_DETAILS " <<
		"(A_ARTIST, A_ALBUM, A_TRACK_NO, A_GENRE, AV_DURATION, DATE, " <<
    "A_CHANNELS, AV_BITRATE, A_SAMPLERATE, " <<
		"ALBUM_ART_ID, ALBUM_ART_MIME_TYPE, SIZE, DLNA_PROFILE) " <<
		"values (" <<
		"'" << SQLEscape(metadata.artist) << "', " <<
		"'" << SQLEscape(metadata.album) << "', " <<
		metadata.track_no << ", " <<
		"'" << SQLEscape(metadata.genre) << "', " <<
		"'" << metadata.duration << "', " <<
		"'" << "" << "', " <<
		metadata.channels << ", " <<
		metadata.bitrate << ", " <<
		metadata.samplerate << ", " <<
		imgId << ", " <<
		"'" << imgMimeType << "', " <<
    fileSize << ", " <<
    "'" << sDlna << "')";
		
  *p_sTitle = metadata.title;
	
	free_metadata(&metadata);	
  return pDb->Insert(sSql.str());
}

unsigned int InsertImageFile(CContentDatabase* pDb, std::string fileName)
{
  SImageItem ImageItem;
	if(!CFileDetails::Shared()->GetImageDetails(fileName, &ImageItem))
	  return 0;
	
  string dlna;
	string mimeType;
	string ext = ExtractFileExt(fileName);
	if(CPluginMgr::dlnaPlugin()) {
		CPluginMgr::dlnaPlugin()->getImageProfile(ext, ImageItem.nWidth, ImageItem.nHeight, &dlna, &mimeType);
	}
	
	stringstream sSql;
	sSql << 
	  "insert into OBJECT_DETAILS " <<
		"(SIZE, IV_WIDTH, IV_HEIGHT, DATE, " <<
		"DLNA_PROFILE, DLNA_MIME_TYPE) " <<
		"values (" <<
		getFileSize(fileName) << ", " <<
		ImageItem.nWidth << ", " <<
		ImageItem.nHeight << ", " <<
   (ImageItem.sDate.empty() ? "NULL" : "'" + ImageItem.sDate + "'") << ", " <<
    "'" << dlna << "', " <<
		"'" << mimeType << "')";
	
  return pDb->Insert(sSql.str());  
} 

unsigned int InsertVideoFile(CContentDatabase* pDb, std::string p_sFileName)
{
  SVideoItem VideoItem;
	if(!CFileDetails::Shared()->GetVideoDetails(p_sFileName, &VideoItem))
	  return 0;  
  
  string sDlna; // = CFileDetails::Shared()->GuessDLNAProfileId(p_sFileName);
	VideoItem.nSize = getFileSize(p_sFileName);
	 
	stringstream sSql;
	sSql << 
	  "insert into OBJECT_DETAILS " <<
		"(IV_WIDTH, IV_HEIGHT, AV_DURATION, SIZE, " <<
    "AV_BITRATE, A_CODEC, V_CODEC, DLNA_PROFILE) " <<
		"values (" <<
		VideoItem.nWidth << ", " <<
		VideoItem.nHeight << "," <<
		"'" << VideoItem.sDuration << "', " <<
		VideoItem.nSize << ", " <<
		VideoItem.nBitrate << ", " <<
    "'" << VideoItem.sACodec << "', " <<
    "'" << VideoItem.sVCodec << "', " <<
    "'" << sDlna << "');";
  
  return pDb->Insert(sSql.str());
} 

unsigned int InsertFile(CContentDatabase* pDb, unsigned int p_nParentId, std::string p_sFileName)
{
  unsigned int nObjId = 0;

  if(g_bAddNew) {
    nObjId = GetObjectIDFromFileName(pDb, p_sFileName);
    if(nObjId > 0) {
      return nObjId;
    }
  } 
  
  OBJECT_TYPE nObjectType = CFileDetails::Shared()->GetObjectType(p_sFileName);  
  if(nObjectType == OBJECT_TYPE_UNKNOWN)
    return false;
   
	nObjId = pDb->GetObjId();
	
  // we insert file details first to get the detail ID
  unsigned int nDetailId = 0;
  string sTitle;
  switch(nObjectType)
  {
    case ITEM_AUDIO_ITEM:
    case ITEM_AUDIO_ITEM_MUSIC_TRACK:     
      nDetailId = InsertAudioFile(pDb, nObjId, p_sFileName, &sTitle); 
      break;
    case ITEM_IMAGE_ITEM:
    case ITEM_IMAGE_ITEM_PHOTO:
      nDetailId = InsertImageFile(pDb, p_sFileName);
      break;
    case ITEM_VIDEO_ITEM:
    case ITEM_VIDEO_ITEM_MOVIE:
      nDetailId = InsertVideoFile(pDb, p_sFileName);
      break;

    default:
      break;
  }  

  // insert object  
  string sTmpFileName =  p_sFileName;
  
  // format file name
  int nPathLen = ExtractFilePath(sTmpFileName).length();
  sTmpFileName = sTmpFileName.substr(nPathLen, sTmpFileName.length() - nPathLen);  
  sTmpFileName = ToUTF8(sTmpFileName);
  sTmpFileName = SQLEscape(sTmpFileName);  
  
  if(!sTitle.empty()) {
    sTitle = SQLEscape(sTitle);
  }
  else {
    sTitle = sTmpFileName;
  }
  
  stringstream sSql;
  sSql << "insert into objects (" <<
    "  OBJECT_ID, DETAIL_ID, TYPE, " <<
    "  PATH, FILE_NAME, TITLE) values (" <<
       nObjId << ", " <<
       nDetailId << ", " <<   
       nObjectType << ", " <<
       "'" << SQLEscape(p_sFileName) << "', " << 
       "'" << sTmpFileName << "', " << 
       "'" << sTitle << "')";
               
  pDb->Insert(sSql.str());  
  sSql.str("");
  
  sSql << "insert into MAP_OBJECTS (OBJECT_ID, PARENT_ID) " <<
            "values (" << nObjId << ", " << p_nParentId << ")";  
  pDb->Insert(sSql.str());

	return nObjId;         
}

unsigned int InsertURL(std::string p_sURL,
											 std::string p_sTitle = "",
											 std::string p_sMimeType = "")
{
	OBJECT_TYPE nObjectType = OBJECT_TYPE_UNKNOWN;
	#warning FIXME: object type
	nObjectType = ITEM_AUDIO_ITEM_AUDIO_BROADCAST;
	
	CContentDatabase* pDb = new CContentDatabase();          								 
	unsigned int nObjId = pDb->GetObjId();

  stringstream sSql;
  sSql << 
	"insert into objects (TYPE, OBJECT_ID, PATH, FILE_NAME, TITLE, MIME_TYPE) values " <<
  "(" << nObjectType << ", " <<
  nObjId << ", " <<
  "'" << SQLEscape(p_sURL) << "', " <<
  "'" << SQLEscape(p_sURL) << "', " <<
  "'" << SQLEscape(p_sTitle) << "', " <<  
  "'" << SQLEscape(p_sMimeType) << "');";

  pDb->Insert(sSql.str());
  delete pDb;
  return nObjId;
}

void BuildPlaylists()
{
  CContentDatabase* pDb = new CContentDatabase();
	
	stringstream sGetPlaylists;
    sGetPlaylists << 
    "select     " << 
    "  *        " <<
    "from       " <<
    "  OBJECTS  " <<
    "where      " <<
    "  TYPE = " << CONTAINER_PLAYLIST_CONTAINER; 
  
  if(!pDb->Select(sGetPlaylists.str())) {
	  delete pDb;
    return;
  }
  
  CSelectResult* pResult = NULL;
  while(!pDb->Eof()) {
    pResult = pDb->GetResult();    
    ParsePlaylist(pResult);    
    pDb->Next();
  }

  delete pDb;
}
    
unsigned int GetObjectIDFromFileName(CContentDatabase* pDb, std::string p_sFileName)
{
  unsigned int nResult = 0;
  stringstream sSQL;
  sSQL << "select OBJECT_ID from OBJECTS where PATH = '" << SQLEscape(p_sFileName) << "' and DEVICE is NULL";
  
  pDb->Select(sSQL.str());
  if(!pDb->Eof())
    nResult = pDb->GetResult()->GetValueAsUInt("OBJECT_ID");
  
  return nResult;
}

bool MapPlaylistItem(unsigned int p_nPlaylistID, unsigned int p_nItemID)
{
  CContentDatabase* pDB = new CContentDatabase();
  
  stringstream sSql;  
  sSql <<
    "insert into MAP_OBJECTS (OBJECT_ID, PARENT_ID) values " <<
    "( " << p_nItemID <<
    ", " << p_nPlaylistID << ")";
  
  pDB->Insert(sSql.str());
  
  delete pDB;
	return true;
}
    

void ParsePlaylist(CSelectResult* pResult)
{
  CPlaylistParser Parser;
  if(!Parser.LoadPlaylist(pResult->GetValue("PATH"))) {
    return;
  }

  unsigned int nPlaylistID = pResult->GetValueAsUInt("OBJECT_ID");
  unsigned int nObjectID   = 0;  

	cout << "playlist id: " << nPlaylistID << endl;
		
  CContentDatabase* pDb = new CContentDatabase();
  
  while(!Parser.Eof()) {
    if(Parser.Entry()->bIsLocalFile && FileExists(Parser.Entry()->sFileName)) {       
            
      nObjectID = GetObjectIDFromFileName(pDb, Parser.Entry()->sFileName);
      
      if(nObjectID == 0) {        
        nObjectID = InsertFile(pDb, nPlaylistID, Parser.Entry()->sFileName);
      }            
      else {
        MapPlaylistItem(nPlaylistID, nObjectID);
      }
    }
    else if(!Parser.Entry()->bIsLocalFile) {
      nObjectID = InsertURL(Parser.Entry()->sFileName, Parser.Entry()->sTitle, Parser.Entry()->sMimeType);
			MapPlaylistItem(nPlaylistID, nObjectID);
    }    
    
    Parser.Next();
  }
  
  delete pDb;
}

std::string ReadFile(std::string p_sFileName)
{
  fstream fsPlaylist;
  char*   szBuf;
  long    nSize;  
  string  sResult;
  
  fsPlaylist.open(p_sFileName.c_str(), ios::in);
  if(fsPlaylist.fail())
    return ""; 
   
  fsPlaylist.seekg(0, ios::end); 
  nSize = streamoff(fsPlaylist.tellg()); 
  fsPlaylist.seekg(0, ios::beg);
  szBuf = new char[nSize + 1];  
  fsPlaylist.read(szBuf, nSize); 
  szBuf[nSize] = '\0';
  fsPlaylist.close();
    
  sResult = szBuf;
  delete[] szBuf;  
   
  return sResult;
}

fuppesThreadCallback BuildLoop(void* arg)
{  
	#ifndef WIN32
  time_t now;
  char nowtime[26];
  time(&now);  
  ctime_r(&now, nowtime);
	nowtime[24] = '\0';
	string sNowtime = nowtime;
	#else		
  char timeStr[9];    
  _strtime(timeStr);	
	string sNowtime = timeStr;	
	#endif  

  CSharedLog::Print("[ContentDatabase] create database at %s", sNowtime.c_str());
  CContentDatabase* pDb = new CContentDatabase();
  stringstream sSql;
		
  if(g_bFullRebuild) {
    pDb->Execute("delete from OBJECTS");
    pDb->Execute("delete from OBJECT_DETAILS");
    pDb->Execute("delete from MAP_OBJECTS");
  }

	/*pDb->Execute("drop index IDX_OBJECTS_OBJECT_ID");
	pDb->Execute("drop index IDX_MAP_OBJECTS_OBJECT_ID");
	pDb->Execute("drop index IDX_MAP_OBJECTS_PARENT_ID");
	pDb->Execute("drop index IDX_OBJECTS_DETAIL_ID");
	pDb->Execute("drop index IDX_OBJECT_DETAILS_ID");*/

	if(!g_bFullRebuild && g_bRemoveMissing) {
	
		CSharedLog::Print("remove missing");
		
		pDb->Select("select * from OBJECTS");
		CContentDatabase* pDel = new CContentDatabase();
			
		while(!pDb->Eof()) {
			
			if(pDb->GetResult()->GetValueAsUInt("TYPE") < ITEM) {
				if(DirectoryExists(pDb->GetResult()->GetValue("PATH"))) {
					pDb->Next();
					continue;
				}
			}
			else {
				if(FileExists(pDb->GetResult()->GetValue("PATH"))) {
					pDb->Next();
					continue;
				}
			}
				
			sSql << "delete from OBJECT_DETAILS where ID = " << pDb->GetResult()->GetValue("OBJECT_ID");
			pDel->Execute(sSql.str());
			sSql.str("");
			
			sSql << "delete from MAP_OBJECTS where OBJECT_ID = " << pDb->GetResult()->GetValue("OBJECT_ID");
			pDel->Execute(sSql.str());
			sSql.str("");
				
			sSql << "delete from OBJECTS where OBJECT_ID = " << pDb->GetResult()->GetValue("OBJECT_ID");
			pDel->Execute(sSql.str());
			sSql.str("");
			
			pDb->Next();
		}

		delete pDel;
		CSharedLog::Print("[DONE] remove missing");		
	}
		
  pDb->Execute("vacuum");  
  
  int i;
  unsigned int nObjId;
  string sFileName;
  bool bInsert = true;
  
  CSharedLog::Print("read shared directories");

  for(i = 0; i < CSharedConfig::Shared()->SharedDirCount(); i++)
  {
    if(DirectoryExists(CSharedConfig::Shared()->GetSharedDir(i)))
    { 	
			pDb->fileAlterationMonitor()->addWatch(CSharedConfig::Shared()->GetSharedDir(i));
      
      ExtractFolderFromPath(CSharedConfig::Shared()->GetSharedDir(i), &sFileName);
      bInsert = true;
      if(g_bAddNew) {
        if((nObjId = GetObjectIDFromFileName(pDb, CSharedConfig::Shared()->GetSharedDir(i))) > 0) {
          bInsert = false;
        }
      }

      sSql.str("");
      if(bInsert) {      
        nObjId = pDb->GetObjId();      
      
        sSql << 
          "insert into OBJECTS (OBJECT_ID, TYPE, PATH, FILE_NAME, TITLE) values " <<
          "(" << nObjId << 
          ", " << CONTAINER_STORAGE_FOLDER << 
          ", '" << SQLEscape(CSharedConfig::Shared()->GetSharedDir(i)) << "'" <<
          ", '" << SQLEscape(sFileName) << "'" <<
          ", '" << SQLEscape(sFileName) << "');";
        
        pDb->Insert(sSql.str());
    
        sSql.str("");
        sSql << "insert into MAP_OBJECTS (OBJECT_ID, PARENT_ID) " <<
          "values (" << nObjId << ", 0)";
      
        pDb->Insert(sSql.str());      
      }
      
      DbScanDir(pDb, CSharedConfig::Shared()->GetSharedDir(i), nObjId);
    }
    else {      
      CSharedLog::Log(L_EXT, __FILE__, __LINE__,
        "shared directory: \" %s \" not found", CSharedConfig::Shared()->GetSharedDir(i).c_str());
    }
  } // for
  CSharedLog::Print("[DONE] read shared directories");
 
	/*if( !pDb->Execute("CREATE INDEX IDX_OBJECTS_OBJECT_ID ON OBJECTS(OBJECT_ID);") )
		CSharedLog::Shared()->Log(L_NORMAL, "Create index failed", __FILE__, __LINE__, false);
	if( !pDb->Execute("CREATE INDEX IDX_MAP_OBJECTS_OBJECT_ID ON MAP_OBJECTS(OBJECT_ID);") )
		CSharedLog::Shared()->Log(L_NORMAL, "Create index failed", __FILE__, __LINE__, false);
	if( !pDb->Execute("CREATE INDEX IDX_MAP_OBJECTS_PARENT_ID ON MAP_OBJECTS(PARENT_ID);") )
		CSharedLog::Shared()->Log(L_NORMAL, "Create index failed", __FILE__, __LINE__, false);
	if( !pDb->Execute("CREATE INDEX IDX_OBJECTS_DETAIL_ID ON OBJECTS(DETAIL_ID);") )
		CSharedLog::Shared()->Log(L_NORMAL, "Create index failed", __FILE__, __LINE__, false);
	if( !pDb->Execute("CREATE INDEX IDX_OBJECT_DETAILS_ID ON OBJECT_DETAILS(ID);") )
		CSharedLog::Shared()->Log(L_NORMAL, "Create index failed", __FILE__, __LINE__, false);*/
	
  
  CSharedLog::Print("parse playlists");
  BuildPlaylists();
  CSharedLog::Print("[DONE] parse playlists");
    
  delete pDb;
  
  
  // import iTunes db
  CSharedLog::Print("parse iTunes databases");
  CiTunesImporter* pITunes = new CiTunesImporter();
  for(i = 0; i < CSharedConfig::Shared()->SharedITunesCount(); i++) {
    pITunes->Import(CSharedConfig::Shared()->GetSharedITunes(i));
  }
  delete pITunes;
  CSharedLog::Print("[DONE] parse iTunes databases");
  	
	#ifndef WIN32
  time(&now);
  ctime_r(&now, nowtime);
	nowtime[24] = '\0';
	sNowtime = nowtime;
	#else	  
  _strtime(timeStr);	
	sNowtime = timeStr;	
	#endif
  CSharedLog::Print("[ContentDatabase] database created at %s", sNowtime.c_str());

  g_bIsRebuilding = false;
  fuppesThreadExit();
}


void CContentDatabase::FamEvent(CFileAlterationEvent* event)
{
  if(g_bIsRebuilding)
    return;
  
  cout << "[ContentDatabase] fam event: ";
  
  switch(event->type()) {
    case FAM_CREATE:
      cout << "CREATE";
      break;
    case FAM_DELETE:
      cout << "DELETE";
      break;
    case FAM_MOVE:
      cout << "MOVE";
      break;
    case FAM_MODIFY:
      cout << "MODIFY";
      break;
    default:
      cout << "UNKNOWN";
      break;
  }
  
  cout << (event->isDir() ? " DIR " : " FILE ") << endl;
  cout << event->fullPath() << endl << endl;
  
  stringstream sSql;
	unsigned int objId;
	unsigned int parentId;  
  
  // newly created or moved in directory
	if((event->type() == FAM_CREATE || event->type() == (FAM_CREATE | FAM_MOVE)) && event->isDir()) {

		objId = GetObjId();
		parentId = GetObjectIDFromFileName(this, event->path());
		
    sSql << 
          "insert into OBJECTS (OBJECT_ID, TYPE, PATH, FILE_NAME, TITLE) values " <<
          "(" << objId << 
          ", " << CONTAINER_STORAGE_FOLDER << 
          ", '" << SQLEscape(appendTrailingSlash(event->fullPath())) << "'" <<
          ", '" << SQLEscape(event->file()) << "'" <<
          ", '" << SQLEscape(event->file()) << "');";        
    Insert(sSql.str());
    
    sSql.str("");
    sSql << "insert into MAP_OBJECTS (OBJECT_ID, PARENT_ID) " <<
          "values (" << objId << ", " << parentId << ")";      
    Insert(sSql.str());
    
    if(event->type() == (FAM_CREATE | FAM_MOVE)) {
      cout << "scan moved in" << endl;
      DbScanDir(this, appendTrailingSlash(event->fullPath()), objId);
    }
	} // new directory
  
	// directory deleted
  else if(event->type() == FAM_DELETE && event->isDir()) {    
    deleteContainer(event->fullPath());    
  } // directory deleted  
  
	// directory moved/renamed
  else if(event->type() == FAM_MOVE && event->isDir()) {        
     
    // update moved folder
    objId = GetObjectIDFromFileName(this, appendTrailingSlash(event->oldFullPath()));
    sSql << 
      "update OBJECTS set " <<
      " PATH = '" << SQLEscape(appendTrailingSlash(event->fullPath())) << "', "
      " FILE_NAME = '" << SQLEscape(event->file()) << "', " <<
      " TITLE = '" << SQLEscape(event->file()) << "' " <<
      "where ID = " << objId;
      
    //cout << sSql.str() << endl;
    Execute(sSql.str());
    sSql.str("");
        
    // update mapping
    parentId = GetObjectIDFromFileName(this, event->path());
    
    sSql << 
      "update MAP_OBJECTS set " <<
      " PARENT_ID = " << parentId << " "
      "where OBJECT_ID = " << objId << " and DEVICE is NULL";
      
    //cout << sSql.str() << endl;
    Execute(sSql.str());
    sSql.str("");
    
    
    // update child object's path
    sSql << "select ID, PATH from OBJECTS where PATH like '" << SQLEscape(appendTrailingSlash(event->oldFullPath())) << "%' and DEVICE is NULL";
		Select(sSql.str());    
		sSql.str("");

    string newPath;  
    CContentDatabase db; 
		while(!Eof()) {

      newPath = StringReplace(GetResult()->GetValue("PATH"), appendTrailingSlash(event->oldFullPath()), appendTrailingSlash(event->fullPath()));

      #warning sql prepare
      sSql << 
        "update OBJECTS set " <<
        " PATH = '" << SQLEscape(newPath) << "' " <<
        "where ID = " << GetResult()->GetValue("ID");
      
      //cout << sSql.str() << endl;
      db.Execute(sSql.str());
      sSql.str("");
      
      Next();
		}   
    
  } // directory moved/renamed  
 
  
  
  // file created
  else if(event->type() == FAM_CREATE && !event->isDir()) { 
    //cout << "FAM_FILE_NEW: " << path << " name: " << name << endl;
        
    parentId = GetObjectIDFromFileName(this, event->path());
    InsertFile(this, parentId, event->fullPath());    
  } // file created

	// file deleted
  else if(event->type() == FAM_DELETE && !event->isDir()) { 
    //cout << "FAM_FILE_DEL: " << path << " name: " << name << endl;
    
    objId = GetObjectIDFromFileName(this, event->fullPath());
    deleteObject(objId);
    
  } // file deleted 
  
	// file moved
  else if(event->type() == FAM_MOVE && !event->isDir()) { 
    //cout << "FAM_FILE_MOVE: " << path << " name: " << name << " old: " << oldPath << " - " << oldName << endl;
        
    objId = GetObjectIDFromFileName(this, event->oldFullPath());
    parentId = GetObjectIDFromFileName(this, event->path());
    
    // update mapping
    sSql << 
        "update MAP_OBJECTS set " <<
        "  PARENT_ID = " << parentId << " " <<        
        "where OBJECT_ID = " << objId << " and DEVICE is NULL";
    Execute(sSql.str());
    sSql.str("");
    
    // update object
    sSql << "update OBJECTS set " <<
        "PATH = '" << SQLEscape(event->fullPath()) << "', " <<
        "FILE_NAME = '" << SQLEscape(event->file()) << "', " <<
        "TITLE = '" << SQLEscape(event->file()) << "' " <<
        "where ID = " << objId;
    //cout << sSql.str() << endl;
    Execute(sSql.str());
    sSql.str("");

  } // file moved
  
  // file modified
  else if(event->type() == FAM_MOVE && !event->isDir()) {
    cout << "file modified" << endl;
  }
  
}


/*void CContentDatabase::FamEvent(FAM_EVENT_TYPE eventType,
																std::string path,
																std::string name,
                                std::string oldPath,
                                std::string oldName)
{
  stringstream sSql;
	unsigned int objId;
	unsigned int parentId;
	
  // new directory
	if(eventType == FAM_DIR_NEW) {

		objId = GetObjId();
		parentId = GetObjectIDFromFileName(this, path);
		
    sSql << 
          "insert into OBJECTS (OBJECT_ID, TYPE, PATH, FILE_NAME, TITLE) values " <<
          "(" << objId << 
          ", " << CONTAINER_STORAGE_FOLDER << 
          ", '" << SQLEscape(appendTrailingSlash(path + name)) << "'" <<
          ", '" << SQLEscape(name) << "'" <<
          ", '" << SQLEscape(name) << "');";
        
    Insert(sSql.str());
    
    sSql.str("");
    sSql << "insert into MAP_OBJECTS (OBJECT_ID, PARENT_ID) " <<
          "values (" << objId << ", " << parentId << ")";
      
    Insert(sSql.str());
		
		//DbScanDir(this, appendTrailingSlash(path + name), nObjId);		
	} // new directory
  
  
	// directory deleted
  else if(eventType == FAM_DIR_DEL) {
    
    deleteContainer(path + name);
    
  } // directory deleted
  
  
	// directory moved/renamed
  else if(eventType == FAM_DIR_MOVE) {
        
    //cout << "FAM_DIR_MOVE: " << path << " name: " << name << " old: " << oldPath << " - " << oldName << endl;    
    
    // update moved folder
    objId = GetObjectIDFromFileName(this, appendTrailingSlash(oldPath + oldName));
    sSql << 
      "update OBJECTS set " <<
      " PATH = '" << SQLEscape(appendTrailingSlash(path + name)) << "', "
      " FILE_NAME = '" << SQLEscape(name) << "', " <<
      " TITLE = '" << SQLEscape(name) << "' " <<
      "where ID = " << objId;
      
    //cout << sSql.str() << endl;
    Execute(sSql.str());
    sSql.str("");
        
    parentId = GetObjectIDFromFileName(this, path);
    
    sSql << 
      "update MAP_OBJECTS set " <<
      " PARENT_ID = " << parentId << " "
      "where OBJECT_ID = " << objId << " and DEVICE is NULL";
      
    //cout << sSql.str() << endl;
    Execute(sSql.str());
    sSql.str("");
    
    
    sSql << "select ID, PATH from OBJECTS where PATH like '" << SQLEscape(appendTrailingSlash(oldPath + oldName)) << "%' and DEVICE is NULL";
		Select(sSql.str());    
		sSql.str("");

    string newPath;  
    CContentDatabase db; 
		while(!Eof()) {

      newPath = StringReplace(GetResult()->GetValue("PATH"), appendTrailingSlash(oldPath + oldName), appendTrailingSlash(path + name));

      #warning sql prepare
      sSql << 
        "update OBJECTS set " <<
        " PATH = '" << SQLEscape(newPath) << "' " <<
        "where ID = " << GetResult()->GetValue("ID");
      
      //cout << sSql.str() << endl;
      db.Execute(sSql.str());
      sSql.str("");
      
      Next();
		}
    
    
  } // directory moved/renamed
  
  
  
	// new file
  else if(eventType == FAM_FILE_NEW) {
    //cout << "FAM_FILE_NEW: " << path << " name: " << name << endl;
        
    parentId = GetObjectIDFromFileName(this, path);
    InsertFile(this, parentId, path + name);    
  } // new file

	// file deleted
  else if(eventType == FAM_FILE_DEL) {
    //cout << "FAM_FILE_DEL: " << path << " name: " << name << endl;
    
    objId = GetObjectIDFromFileName(this, path + name);
    deleteObject(objId);
    
  } // file deleted  
  
	// file moved
  else if(eventType == FAM_FILE_MOVE) {
    //cout << "FAM_FILE_MOVE: " << path << " name: " << name << " old: " << oldPath << " - " << oldName << endl;
        
    objId = GetObjectIDFromFileName(this, oldPath + oldName);
    parentId = GetObjectIDFromFileName(this, path);
    
    // update mapping
    sSql << 
        "update MAP_OBJECTS set " <<
        "  PARENT_ID = " << parentId << " " <<        
        "where OBJECT_ID = " << objId << " and DEVICE is NULL";
    Execute(sSql.str());
    sSql.str("");
    
    
    // update object
    sSql << "update OBJECTS set " <<
        "PATH = '" << SQLEscape(path + name) << "', " <<
        "FILE_NAME = '" << SQLEscape(name) << "', " <<
        "TITLE = '" << SQLEscape(name) << "' " <<
        "where ID = " << objId;
    //cout << sSql.str() << endl;
    Execute(sSql.str());
    sSql.str("");

  } // file moved  
    
	// file modified
  else if(eventType == FAM_FILE_MOD) {
    cout << "FAM_FILE_MOD: " << path << " name: " << name << " todo" << endl;
  } // file modified  
  
	//Unlock();
	g_bIsRebuilding = false;
}*/

void CContentDatabase::deleteObject(unsigned int objectId)
{
  stringstream sql;
	CContentDatabase db;
	
  // get type
  sql << "select TYPE, PATH from OBJECTS where OBJECT_ID = " << objectId << " and DEVICE is NULL";		
  db.Select(sql.str());
	sql.str("");
    
  string objects;
	if(db.Eof()) {
	  return;
	}
  
  OBJECT_TYPE type = (OBJECT_TYPE)db.GetResult()->asUInt("TYPE");
  if(type < ITEM) { // is a container
		fileAlterationMonitor()->removeWatch(db.GetResult()->GetValue("PATH"));
    deleteContainer(db.GetResult()->GetValue("PATH"));    
  }
  else {  
     cout << "contentdb: delete object : " << db.GetResult()->GetValue("PATH") << endl;
  
    // delete details    
    sql << "select DETAIL_ID from OBJECTS where OBJECT_ID = " << objectId;
		db.Select(sql.str());
		sql.str("");
		if(!db.Eof() && !db.GetResult()->IsNull("DETAIL_ID")) {      
      sql << "delete from OBJECT_DETAILS where ID = " << db.GetResult()->GetValue("DETAIL_ID");
      db.Execute(sql.str());
      sql.str("");
		}
    
    // delete mapping
    sql << "delete from MAP_OBJECTS where OBJECT_ID = " << objectId;
    db.Execute(sql.str());
    sql.str("");
    
    // delete object
    sql << "delete from OBJECTS where OBJECT_ID = " << objectId;
    db.Execute(sql.str());
    sql.str("");
  }
  
}

void CContentDatabase::deleteContainer(std::string path)
{
  stringstream sSql;

	CContentDatabase db;
	
  // delete object details
  sSql << 
    "select DETAIL_ID from OBJECTS where PATH like '" <<
    SQLEscape(appendTrailingSlash(path)) << "%' and DEVICE is NULL";
	db.Select(sSql.str());
	sSql.str("");
  
  string details;
	while(!db.Eof()) {
    if(!db.GetResult()->IsNull("DETAIL_ID")) {
      details += db.GetResult()->GetValue("DETAIL_ID") + ", ";
    }
    db.Next();
	}
  
  if(details.length() > 0) {
    details = details.substr(0, details.length() -2);
    
    sSql << "delete from OBJECT_DETAILS where ID in (" << details << ")";
    //cout << sSql.str() << endl;
    db.Execute(sSql.str());
    sSql.str("");
  }
  
  // delete mappings
  sSql << "select OBJECT_ID from OBJECTS where PATH like '" << SQLEscape(appendTrailingSlash(path)) << "%' and DEVICE is NULL";		
	db.Select(sSql.str());
	sSql.str("");
  
  string objects;
	while(!db.Eof()) {
    objects += db.GetResult()->GetValue("OBJECT_ID") + ", ";
    db.Next();
	}
  
  if(objects.length() > 0) {
    objects = objects.substr(0, objects.length() -2);
    
    sSql << "delete from MAP_OBJECTS where OBJECT_ID in (" << objects << ")";
    //cout << sSql.str() << endl;      
    db.Execute(sSql.str());
    sSql.str("");
  }
  
#warning todo: remove fam watches
  
  // delete objects
  sSql << "delete from OBJECTS where PATH like '" << SQLEscape(appendTrailingSlash(path)) << "%'";
  //cout << sSql.str() << endl;    
  db.Execute(sSql.str());
  sSql.str("");
}

