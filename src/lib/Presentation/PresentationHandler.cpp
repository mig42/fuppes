/* -*- Mode: C++; indent-tabs-mode: nil; c-basic-offset: 2; tab-width: 2 -*- */
/***************************************************************************
 *            PresentationHandler.cpp
 *
 *  FUPPES - Free UPnP Entertainment Service
 *
 *  Copyright (C) 2005-2010 Ulrich Völkel <u-voelkel@users.sourceforge.net>
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
 
#include "PresentationHandler.h"
//#include "Stylesheet.h"
#include "../SharedConfig.h"
#include "../SharedLog.h"
#include "../Common/Common.h"
#include "../ContentDirectory/ContentDatabase.h"
#include "../ContentDirectory/DatabaseConnection.h"
#ifdef HAVE_VFOLDER
#include "../ContentDirectory/VirtualContainerMgr.h"
#endif
#include "../ContentDirectory/FileDetails.h"
#include "../Transcoding/TranscodingMgr.h"
#include "../HTTP/HTTPParser.h"
#include "../DeviceSettings/DeviceIdentificationMgr.h"
#include "../Plugins/Plugin.h"

#include <sstream>
#include <fstream>
#include <iostream>

const std::string LOGNAME = "PresentationHandler"; 


CPresentationHandler::CPresentationHandler(std::string httpServerUrl)
{
  m_httpServerUrl = httpServerUrl;
}

CPresentationHandler::~CPresentationHandler()
{
}


void CPresentationHandler::OnReceivePresentationRequest(CHTTPMessage* pMessage, CHTTPMessage* pResult)
{
  PRESENTATION_PAGE nPresentationPage = PRESENTATION_PAGE_UNKNOWN;
  string sContent;
  std::string sPageName = "undefined";
  
  if((pMessage->GetRequest().compare("/") == 0) || (ToLower(pMessage->GetRequest()).compare("/index.html") == 0)) {    
    //CSharedLog::Shared()->ExtendedLog(LOGNAME, "send index.html");
    nPresentationPage = PRESENTATION_PAGE_INDEX;
    sContent = this->GetIndexHTML();
    sPageName = "Start";
  }
	
	else if(ToLower(pMessage->GetRequest()).compare("/presentation/style.css") == 0) {
    nPresentationPage = PRESENTATION_STYLESHEET;
		pResult->LoadContentFromFile(CSharedConfig::Shared()->dataDir() + "style.css");
  }
	
  else if(ToLower(pMessage->GetRequest()).compare("/presentation/about.html") == 0)
  {
    //CSharedLog::Shared()->ExtendedLog(LOGNAME, "send about.html");
    nPresentationPage = PRESENTATION_PAGE_ABOUT;
    sContent = this->GetAboutHTML();
    sPageName = "About";
  }
  
  else if(ToLower(pMessage->GetRequest()).compare("/presentation/options.html") == 0) {
    nPresentationPage = PRESENTATION_PAGE_OPTIONS;
    sContent = this->GetOptionsHTML();
    sPageName = "Options";
  }
  else if(ToLower(pMessage->GetRequest()).compare("/presentation/options.html?db=rebuild") == 0) {
    CSharedConfig::Shared()->Refresh();
    if(!CContentDatabase::Shared()->IsRebuilding())
      CContentDatabase::Shared()->RebuildDB();

    nPresentationPage = PRESENTATION_PAGE_OPTIONS;
    sContent = this->GetOptionsHTML();
    sPageName = "Options";
  }
  else if(ToLower(pMessage->GetRequest()).compare("/presentation/options.html?db=update") == 0) {
    CSharedConfig::Shared()->Refresh();
    if(!CContentDatabase::Shared()->IsRebuilding() 
#ifdef HAVE_VFOLDER
      && !CVirtualContainerMgr::Shared()->IsRebuilding()
#endif
      )
      CContentDatabase::Shared()->UpdateDB();

    nPresentationPage = PRESENTATION_PAGE_OPTIONS;
    sContent = this->GetOptionsHTML();
    sPageName = "Options";
  }
	else if(ToLower(pMessage->GetRequest()).compare("/presentation/options.html?vcont=rebuild") == 0) {
    CSharedConfig::Shared()->Refresh();
    if(!CContentDatabase::Shared()->IsRebuilding() 
#ifdef HAVE_VFOLDER
      && !CVirtualContainerMgr::Shared()->IsRebuilding()
#endif
      )
#ifdef HAVE_VFOLDER
      CVirtualContainerMgr::Shared()->RebuildContainerList();
#endif

    nPresentationPage = PRESENTATION_PAGE_OPTIONS;
    sContent = this->GetOptionsHTML();
    sPageName = "Options";
  }
  else if(ToLower(pMessage->GetRequest()).compare("/presentation/status.html") == 0) {
    nPresentationPage = PRESENTATION_PAGE_STATUS;
    sContent = this->GetStatusHTML();
    sPageName = "Status";
  }
  else if(ToLower(pMessage->GetRequest()).compare("/presentation/config.html") == 0) {
    nPresentationPage = PRESENTATION_PAGE_STATUS;
    sContent = this->GetConfigHTML(pMessage);
    sPageName = "Configuration";
  }
  else if(ToLower(pMessage->GetRequest()).compare("/presentation/jstest.html") == 0) {
    nPresentationPage = PRESENTATION_PAGE_JSTEST;
    sContent = this->GetJsTestHTML();
    sPageName = "Configuration";
  }

#warning todo add http cache fields to the response header
  
  // device icons
  else if(ToLower(pMessage->GetRequest()).compare("/presentation/fuppes-icon-50x50.png") == 0) {
    nPresentationPage = PRESENTATION_BINARY_IMAGE;
		pResult->LoadContentFromFile(CSharedConfig::Shared()->dataDir() + "fuppes-icon-50x50.png");
    pResult->SetContentType("image/png");
  }
  else if(ToLower(pMessage->GetRequest()).compare("/presentation/fuppes-icon-50x50.jpg") == 0) {
    nPresentationPage = PRESENTATION_BINARY_IMAGE;
		pResult->LoadContentFromFile(CSharedConfig::Shared()->dataDir() + "fuppes-icon-50x50.jpg");
    pResult->SetContentType("image/jpeg");
  }
  // webinterface images
  else if(ToLower(pMessage->GetRequest()).compare("/presentation/fuppes-logo.png") == 0) {
    nPresentationPage = PRESENTATION_BINARY_IMAGE;
		pResult->LoadContentFromFile(CSharedConfig::Shared()->dataDir() + "fuppes-logo.png");
    pResult->SetContentType("image/png");
  }
  else if(ToLower(pMessage->GetRequest()).compare("/presentation/header-gradient.png") == 0) {
    nPresentationPage = PRESENTATION_BINARY_IMAGE;
		pResult->LoadContentFromFile(CSharedConfig::Shared()->dataDir() + "header-gradient.png");
    pResult->SetContentType("image/png");
  }  
  else if(ToLower(pMessage->GetRequest()).compare("/presentation/header-gradient-small.png") == 0) {
    nPresentationPage = PRESENTATION_BINARY_IMAGE;
		pResult->LoadContentFromFile(CSharedConfig::Shared()->dataDir() + "header-gradient-small.png");
    pResult->SetContentType("image/png");
  }

  // mootools
  else if(ToLower(pMessage->GetRequest()).compare("/presentation/mootools-1.2.4-core-yc.js") == 0) {
    nPresentationPage = PRESENTATION_JAVASCRIPT;
		pResult->LoadContentFromFile(CSharedConfig::Shared()->dataDir() + "mootools-1.2.4-core-yc.js");
  }
  else if(ToLower(pMessage->GetRequest()).compare("/presentation/fuppes.js") == 0) {
    nPresentationPage = PRESENTATION_JAVASCRIPT;
		pResult->LoadContentFromFile(CSharedConfig::Shared()->dataDir() + "fuppes.js");
  }
  
  if(nPresentationPage == PRESENTATION_BINARY_IMAGE) {
    pResult->SetMessageType(HTTP_MESSAGE_TYPE_200_OK);    
  }
	else if(nPresentationPage == PRESENTATION_STYLESHEET) {
    pResult->SetMessageType(HTTP_MESSAGE_TYPE_200_OK);    
    pResult->SetContentType("text/css");
  }
  else if(nPresentationPage == PRESENTATION_JAVASCRIPT) {
    pResult->SetMessageType(HTTP_MESSAGE_TYPE_200_OK);    
    pResult->SetContentType("text/javascript");
  }
  else if((nPresentationPage != PRESENTATION_BINARY_IMAGE) && (nPresentationPage != PRESENTATION_PAGE_UNKNOWN))
  {   
    stringstream sResult;   
    
    sResult << GetXHTMLHeader();
    sResult << GetPageHeader(nPresentationPage, sPageName);
    sResult << sContent;    
    sResult << GetPageFooter(nPresentationPage);
    
    pResult->SetMessageType(HTTP_MESSAGE_TYPE_200_OK);    
    pResult->SetContentType("text/html; charset=\"utf-8\""); // HTTP_CONTENT_TYPE_TEXT_HTML
    pResult->SetContent(sResult.str());    
  }  
  else if(nPresentationPage == PRESENTATION_PAGE_UNKNOWN) 
  {
    pResult->SetMessageType(HTTP_MESSAGE_TYPE_404_NOT_FOUND); 
    pResult->SetContentType("text/html");
  }
}


std::string CPresentationHandler::GetPageHeader(PRESENTATION_PAGE /*p_nPresentationPage*/, std::string p_sPageName)
{
  std::stringstream sResult; 
	 
  /* header */
  sResult << "<html xmlns=\"http://www.w3.org/1999/xhtml\">\n";
  sResult << "<head>";  
  sResult << "<title>" << CSharedConfig::Shared()->GetAppName() << " - " << CSharedConfig::Shared()->GetAppFullname() << " " << CSharedConfig::Shared()->GetAppVersion();
  sResult << " (" << CSharedConfig::Shared()->networkSettings->GetHostname() << ")";
  sResult << "</title>" << endl;

	//sResult << "<meta http-equiv=\"Content-Script-Type\" content=\"text/javascript\">" << endl;
	sResult << "<meta http-equiv=\"content-type\" content=\"text/html; charset=UTF-8\">" << endl;
  sResult << "<meta http-equiv=\"Content-Style-Type\" content=\"text/css\">" << endl; 
	sResult << "<link href=\"/presentation/style.css\" rel=\"stylesheet\" type=\"text/css\" media=\"screen\" />" << endl;

	sResult << "<script type=\"text/javascript\" src=\"/presentation/mootools-1.2.4-core-yc.js\"></script>" << endl;
	sResult << "<script type=\"text/javascript\" src=\"/presentation/fuppes.js\"></script>" << endl;
  
  sResult << "</head>";
  /* header end */
  
    
  sResult << "<body>";
  
  //pFuppes->GetRemoteDevices().size()
  
  /* title */
  sResult << "<div id=\"header\">" << endl;

  sResult << "<div id=\"logo\"></div>" << endl;

  sResult << "<p>" << endl <<
    "FUPPES - Free UPnP Entertainment Service<br />" << endl <<
    "<span>" <<
    "Version: " << CSharedConfig::Shared()->GetAppVersion() << " &bull; " <<
    "Host: "    << CSharedConfig::Shared()->networkSettings->GetHostname() << " &bull; " <<
    "Address: " << CSharedConfig::Shared()->networkSettings->GetIPv4Address() <<
    "</span>"   << endl <<
    "</p>" << endl;
  
  sResult << "</div>" << endl;
  /* title end */
  
  /* menu */
  sResult << "<div id=\"menu\">" << endl;
  sResult << "<div id=\"framehead\">Menu</div>" << endl;    

  sResult << 
    "<ul>" <<
      "<li><a href=\"/index.html\">Start</a></li>" <<
      "<li><a href=\"/presentation/options.html\">Options</a></li>" <<
      "<li><a href=\"/presentation/status.html\">Status</a></li>" <<
      "<li><a href=\"/presentation/config.html\">Configuration</a></li>" <<
    "</ul>";
  
  sResult << "</div>" << endl;  
  /* menu end */
  

  sResult << "<div id=\"mainframe\">" << endl;
  sResult << "<div id=\"framehead\">" << p_sPageName << "</div>" << endl;
  
  sResult << "<div id=\"content\">" << endl;
  
  return sResult.str().c_str();
}


std::string CPresentationHandler::GetPageFooter(PRESENTATION_PAGE /*p_nPresentationPage*/)
{
  std::stringstream sResult;
  
  sResult << "<p style=\"padding-top: 20pt; text-align: center;\"><small>copyright &copy; 2005-2010 Ulrich V&ouml;lkel</small></p>";

  sResult << "</div>" << endl; // #content
  sResult << "</div>" << endl; // #mainframe
  
  sResult << "</body>";
  sResult << "</html>";
  
  return sResult.str().c_str();
}

/* GetXHTMLHeader */
std::string CPresentationHandler::GetXHTMLHeader()
{
  std::stringstream sResult;
  
  sResult << "<?xml version=\"1.0\" encoding=\"UTF-8\"?>" << endl;
  sResult << "<!DOCTYPE html PUBLIC \"-//W3C//DTD XHTML 1.0 Strict//EN\" " << endl;
  sResult << "\"http://www.w3.org/TR/xhtml1/DTD/xhtml1-strict.dtd\">" << endl;
  
  return sResult.str().c_str();
}

/* GetIndexHTML */
std::string CPresentationHandler::GetIndexHTML()
{
  std::stringstream sResult;
  
  sResult << "<h1>system information</h1>" << endl;  
  
  sResult << "<p>" << endl;
  sResult << "Version: " << CSharedConfig::Shared()->GetAppVersion() << "<br />" << endl;
  sResult << "Hostname: " << CSharedConfig::Shared()->networkSettings->GetHostname() << "<br />" << endl;
  sResult << "OS: " << CSharedConfig::Shared()->GetOSName() << " " << CSharedConfig::Shared()->GetOSVersion() << "<br />" << endl;
  //sResult << "SQLite: " << CContentDatabase::Shared()->GetLibVersion() << endl;
  sResult << "</p>" << endl;
  
  sResult << "<p>" << endl;
  sResult << "build at: " << __DATE__ << " - " << __TIME__ << "<br />" << endl;
  sResult << "build with: " << __VERSION__ << endl;
  sResult << "</p>" << endl;
  
  sResult << "<p>" << endl;
  sResult << "<a href=\"http://sourceforge.net/projects/fuppes/\">http://sourceforge.net/projects/fuppes/</a><br />" << endl;
  sResult << "</p>" << endl;


  // uptime
  fuppes::DateTime now = fuppes::DateTime::now();
  int uptime = now.toInt() - CSharedConfig::Shared()->GetFuppesInstance(0)->startTime().toInt();


  int days;
  int hours;
  int minutes;
  int seconds;

  seconds = uptime % 60;
  minutes = uptime / 60;
  hours = minutes / 60;
  minutes = minutes % 60;
  days = hours / 24;
  hours = hours % 24;

  
  sResult << "<p>" << endl;
  sResult << "uptime: " << days << " days " << hours << " hours " << minutes << " minutes " << seconds << " seconds" << "<br />" << endl;
  sResult << "</p>" << endl;
  
  sResult << "<h1>remote devices</h1>";
  sResult << BuildFuppesDeviceList(CSharedConfig::Shared()->GetFuppesInstance(0));
  
  return sResult.str().c_str();
  
  //sResult << "<h2>Start</h2>" << endl;
  //*p_psImgPath = "Start";
  
  /*for (unsigned int i = 0; i < m_vFuppesInstances.size(); i++)
  {
    sResult << "FUPPES Instance No. " << i + 1 << "<br />";    
    sResult << "IP-Address: " << ((CFuppes*)m_vFuppesInstances[i])->GetIPAddress() << "<br />";
    sResult << "HTTP-Server URL: " << ((CFuppes*)m_vFuppesInstances[i])->GetHTTPServerURL() << "<br />";    
    sResult << "<br />";
    sResult << "<br />";    
    
  }*/
}

std::string CPresentationHandler::GetAboutHTML()
{
  std::stringstream sResult;
  
  //sResult << "<h2>About</h2>" << endl;
  //*p_psImgPath = "About";

  
  return sResult.str().c_str();
}

std::string CPresentationHandler::GetOptionsHTML()
{
  std::stringstream sResult;
  
  //sResult << "<h2>Options</h2>" << endl;
  //*p_psImgPath = "Options";
  /*sResult << "<a href=\"http://sourceforge.net/projects/fuppes/\">http://sourceforge.net/projects/fuppes/</a><br />" << endl; */

  //((CFuppes*)m_vFuppesInstances[0])->GetContentDirectory()->BuildDB();
  sResult << "<h1>database options</h1>" << endl;
  if(!CContentDatabase::Shared()->IsRebuilding() 
#ifdef HAVE_VFOLDER
    && !CVirtualContainerMgr::Shared()->IsRebuilding()
#endif
    )  {
    sResult << "<a href=\"/presentation/options.html?db=rebuild\">rebuild database</a><br />" << endl;
    sResult << "<a href=\"/presentation/options.html?db=update\">update database</a><br />" << endl;
#ifdef HAVE_VFOLDER
		sResult << "<a href=\"/presentation/options.html?vcont=rebuild\">rebuild virtual container</a>" << endl;
#endif
  }
  else {
		if(CContentDatabase::Shared()->IsRebuilding())
			sResult << "database rebuild/update in progress" << endl;
#ifdef HAVE_VFOLDER
		else if(CVirtualContainerMgr::Shared()->IsRebuilding())
			sResult << "virtual container rebuild in progress" << endl;
#endif
	}
  
  return sResult.str().c_str();
}

std::string CPresentationHandler::GetStatusHTML()
{
  std::stringstream sResult;  
 
  // Database status
  sResult << "<h1>database status</h1>" << endl;  
  sResult << buildObjectStatusTable();

  // transcoding  
  string sTranscoding;
  sResult << "<h1>transcoding</h1>";
  CTranscodingMgr::Shared()->PrintTranscodingSettings(&sTranscoding);
  sResult << sTranscoding;
  
  // plugins
  sResult << "<h1>plugins</h1>" << CPluginMgr::printInfo(true);
  
  // system status
  sResult << "<h1>system status</h1>" << endl;  
  
  sResult << "<p>" << endl;
  sResult << "UUID: " << CSharedConfig::Shared()->GetFuppesInstance(0)->GetUUID() << "<br />";    
  sResult << "</p>" << endl;
  
  // device settings
  sResult << "<h1>device settings</h1>" << endl;
  string sDeviceSettings;
  CDeviceIdentificationMgr::Shared()->PrintSettings(&sDeviceSettings);
  sResult << sDeviceSettings << endl;
  
  return sResult.str();  
}

std::string CPresentationHandler::buildObjectStatusTable()
{
  OBJECT_TYPE nType = OBJECT_TYPE_UNKNOWN;
  std::stringstream sResult;
  
	SQLQuery qry;
  string sql = qry.build(SQL_GET_OBJECT_TYPE_COUNT, 0);
	qry.select(sql);
  
  sResult << 
    "<table rules=\"all\" style=\"font-size: 10pt; border-style: solid; border-width: 1px; border-color: #000000;\" cellspacing=\"0\" width=\"400\">" << endl <<
      "<thead>" << endl <<
        "<tr>" << endl <<        
          "<th>Type</th>" << 
          "<th>Count</th>" << endl <<
        "</tr>" << endl <<
      "</thead>" << endl << 
      "<tbody>" << endl;  


  while(!qry.eof()) {
    nType = (OBJECT_TYPE)qry.result()->asInt("TYPE");
    
    sResult << "<tr>" << endl;
    sResult << "<td>" << CFileDetails::Shared()->GetObjectTypeAsStr(nType) << "</td>" << endl;
    sResult << "<td>" << qry.result()->asString("VALUE") << "</td>" << endl;    
    sResult << "</tr>" << endl;
    
    qry.next();
  }
    
  sResult <<
      "</tbody>" << endl <<   
    "</table>" << endl;

  return sResult.str();

}

std::string CPresentationHandler::GetConfigHTML(CHTTPMessage* pRequest)
{
  std::stringstream sResult;  

  CSharedConfig* sharedCfg = CSharedConfig::Shared();

  // handle config changes
  if(pRequest->GetMessageType() == HTTP_MESSAGE_TYPE_POST)
  {
  	//cout << pRequest->GetMessage() << endl;
	
		/*CHTTPParser* pParser = new CHTTPParser();
		pParser->ConvertURLEncodeContentToPlain(pRequest);
		delete pParser;	*/
		CHTTPParser::ConvertURLEncodeContentToPlain(pRequest);
	
    // remove shared objects(s)
    stringstream sVar;
    for(int i = sharedCfg->sharedObjects->SharedDirCount() - 1; i >= 0; i--) {
      sVar << "shared_dir_" << i;      
      if(pRequest->PostVarExists(sVar.str()))      
        sharedCfg->sharedObjects->RemoveSharedDirectory(i);      
      sVar.str("");
    }    
    
    for(int i = sharedCfg->sharedObjects->SharedITunesCount() - 1; i >= 0; i--) {
      sVar << "shared_itunes_" << i;      
      if(pRequest->PostVarExists(sVar.str()))      
        sharedCfg->sharedObjects->RemoveSharedITunes(i);      
      sVar.str("");
    }    
    
    // add shared object
    if(pRequest->PostVarExists("new_obj") && (pRequest->GetPostVar("new_obj").length() > 0))
    {     
      if(pRequest->GetPostVar("new_obj_type").compare("dir") == 0) {
        sharedCfg->sharedObjects->AddSharedDirectory(pRequest->GetPostVar("new_obj"));
      }
      else if(pRequest->GetPostVar("new_obj_type").compare("itunes") == 0) {
        sharedCfg->sharedObjects->AddSharedITunes(pRequest->GetPostVar("new_obj"));
      }
    }

    // local_charset
    if(pRequest->PostVarExists("local_charset")) {
      sharedCfg->contentDirectory->SetLocalCharset(pRequest->GetPostVar("local_charset"));
    }    
    
    /* playlist_representation */
    /*if(pRequest->PostVarExists("playlist_representation"))
    {
      CSharedConfig::Shared()->SetPlaylistRepresentation(pRequest->GetPostVar("playlist_representation"));
    }*/
    
    /* max_file_name_length */
    /*if(pRequest->PostVarExists("max_file_name_length") && (pRequest->GetPostVar("max_file_name_length").length() > 0))
    {
      int nMaxFileNameLength = atoi(pRequest->GetPostVar("max_file_name_length").c_str());      
      CSharedConfig::Shared()->SetMaxFileNameLength(nMaxFileNameLength);
    }*/
    
    // ip address
    if(pRequest->PostVarExists("net_interface") && (pRequest->GetPostVar("net_interface").length() > 0)) {
      sharedCfg->networkSettings->SetNetInterface(pRequest->GetPostVar("net_interface"));
    }
    
    // http port
    if(pRequest->PostVarExists("http_port") && (pRequest->GetPostVar("http_port").length() > 0)) {
      int nHTTPPort = atoi(pRequest->GetPostVar("http_port").c_str());      
      sharedCfg->networkSettings->SetHTTPPort(nHTTPPort);
    }
    
    // add allowed ip
    if(pRequest->PostVarExists("new_allowed_ip") && (pRequest->GetPostVar("new_allowed_ip").length() > 0)) {     
      sharedCfg->networkSettings->AddAllowedIP(pRequest->GetPostVar("new_allowed_ip"));
    }
    
    // remove allowed ip
    for(int i = sharedCfg->networkSettings->AllowedIPCount() - 1; i >= 0; i--) {
      sVar << "allowed_ip_" << i;      
      if(pRequest->PostVarExists(sVar.str()))      
        sharedCfg->networkSettings->RemoveAllowedIP(i);      
      sVar.str("");
    }        
  }
  
  
  sResult << "<strong>Device and file settings are currently not configurable via this webinterface.<br />";
  sResult << "Please edit the file: " << sharedCfg->GetConfigFileName() << "</strong>" << endl;
  
  /* show config page */
  sResult << "<h1>ContentDirectory settings</h1>" << endl;
  sResult << "<form method=\"POST\" action=\"/presentation/config.html\" enctype=\"text/plain\" accept-charset=\"UTF-8\">" << endl;  //  
  
  // shared objects
  sResult << "<h2>shared objects</h2>" << endl;
  
  // object list
  sResult << "<p>" << endl <<  
             "<table>" << endl <<
               "<thead>" << endl <<
                 "<tr>" <<
                   "<th>del</th>" <<
                   "<th>type</th>" <<
                   "<th>object</th>" <<
                 "</tr>" <<
               "</thead>" << endl <<
               "<tbody>" << endl;
  
  // dirs
  for(int i = 0; i < sharedCfg->sharedObjects->SharedDirCount(); i++) {
    sResult << "<tr>" << endl;    
    sResult << "<td><input type=\"checkbox\" name=\"shared_dir_" << i << "\" value=\"remove\"></td>" << endl;
    sResult << "<td>dir</td>" << endl;
    sResult << "<td>" << sharedCfg->sharedObjects->GetSharedDir(i) << "</td>" << endl;
    sResult << "</tr>" << endl;
  }
  
  // itunes
  for(int i = 0; i < sharedCfg->sharedObjects->SharedITunesCount(); i++) {
    sResult << "<tr>" << endl;    
    sResult << "<td><input type=\"checkbox\" name=\"shared_itunes_" << i << "\" value=\"remove\"></td>" << endl;   
    sResult << "<td>iTunes</td>" << endl;    
    sResult << "<td>" << sharedCfg->sharedObjects->GetSharedITunes(i) << "</td>" << endl;
    sResult << "</tr>" << endl;
  }  
  
  sResult <<   "</tbody>" << endl <<
             "</table>" << endl <<  
             "</p>" << endl;
  
  // "add new form" controls
  sResult << "<p>" <<  
               "Add objects: <input name=\"new_obj\" /><br />" << endl <<
               "<input type=\"radio\" name=\"new_obj_type\" value=\"dir\" checked=\"checked\" />directory " <<
               "<input type=\"radio\" name=\"new_obj_type\" value=\"itunes\" />iTunes db<br />" <<    
               "<input type=\"submit\" />" << endl <<             
             "</p>" << endl;
  
  // playlist representation
  /*sResult << "<h2>playlist representation</h2>" << endl;
  sResult << "<p>Choose how playlist items are represented. <br />\"file\" sends playlists as real playlist files (m3u, pls or wpl for a full list see the wiki)<br />" <<
             "\"container\" represents playlists as containers including the playlist items.<br />" << endl;
             if(sharedCfg->GetDisplaySettings().bShowPlaylistsAsContainers)  
             {
               sResult << "<input type=\"radio\" name=\"playlist_representation\" value=\"file\"> file<br />" << endl <<
                          "<input type=\"radio\" name=\"playlist_representation\" value=\"container\" checked=\"checked\"> container<br />" << endl;
             }
             else
             {
               sResult << "<input type=\"radio\" name=\"playlist_representation\" value=\"file\" checked=\"checked\"> file<br />" << endl <<
                          "<input type=\"radio\" name=\"playlist_representation\" value=\"container\"> container<br />" << endl;               
             }
             
  sResult << "<input type=\"submit\" />" << endl <<             
             "</p>" << endl; */
  
             
  // charset
  sResult << "<h2>character encoding</h2>" << endl;
  sResult << "<p>Set your local character encoding.<br />" <<
             "<a href=\"http://www.gnu.org/software/libiconv/\" target=\"blank\">http://www.gnu.org/software/libiconv/</a><br />" << endl <<
             "</p>" << endl;
             
  sResult << "<p>" << endl <<
             "<input name=\"local_charset\" value=\"" << sharedCfg->contentDirectory->GetLocalCharset() << "\"/><br />" << endl;
  sResult << "<input type=\"submit\" />" << endl <<             
             "</p>" << endl;              
             
  
  // max filename length
  /*sResult << "<h2>max file name length</h2>" << endl;
  sResult << "<p>The \"max file name length\" option sets the maximum length for file names in the directory listings.<br />" <<
             "some devices can't handle an unlimited length.<br />" << endl <<
             "(e.g. the Telegent TG 100 crashes on receiving file names larger then 101 characters.)<br />" << endl <<
             "0 or empty means unlimited length. a value greater 0 sets the maximum number of characters to be sent.</p>" << endl;
  //sResult << "<p><strong>Max file name length: " << sharedCfg->GetMaxFileNameLength() << "</strong></p>" << endl;  
  
    sResult << "<p>" <<  
               "<input name=\"max_file_name_length\" value=\"" << sharedCfg->GetMaxFileNameLength() << "\" />" << endl <<
               "<br />" << endl <<
               "<input type=\"submit\" />" << endl <<             
             "</p>" << endl;  */
  
  
  sResult << "<h1>Network settings</h1>" << endl;
  
  sResult << "<h2>IP address or network interface name (e.g. eth0, wlan1, ...)/HTTP port</h2>" << endl;  
  sResult << "<p>" <<  
               "<input name=\"net_interface\" value=\"" << sharedCfg->networkSettings->GetNetInterface() << "\" />" << endl <<  
               "<input name=\"http_port\" value=\"" << sharedCfg->networkSettings->GetHTTPPort() << "\" />" << endl <<
               "<br />" << endl <<
               "<input type=\"submit\" />" << endl <<             
             "</p>" << endl;  
  
  sResult << "<h2>Allowed IP-addresses</h2>" << endl;
  
  // allowed ip list
  sResult << "<p>" << endl <<  
             "host address is always allowed to access." << endl <<
             "</p>" << endl <<
             "<p>" << endl <<
             "<table>" << endl <<
               "<thead>" << endl <<
                 "<tr>" <<
                   "<th>Del</th>" <<
                   "<th>IP-Address</th>" <<
                 "</tr>" <<
               "</thead>" << endl <<
               "<tbody>" << endl;
  for(unsigned int i = 0; i < sharedCfg->networkSettings->AllowedIPCount(); i++) {
    sResult << "<tr>" << endl;    
    sResult << "<td><input type=\"checkbox\" name=\"allowed_ip_" << i << "\" value=\"remove\"></td>" << endl;   
    sResult << "<td>" << sharedCfg->networkSettings->GetAllowedIP(i) << "</td>" << endl;
    sResult << "</tr>" << endl;
  }
  
  sResult <<   "</tbody>" << endl <<
             "</table>" << endl <<  
             "</p>" << endl;  
  
  sResult << "<p>" <<  
               "<input name=\"new_allowed_ip\" />" << endl <<
               "<br />" << endl <<
               "<input type=\"submit\" />" << endl <<             
             "</p>" << endl;
  
  sResult << "</form>";
  
  return sResult.str().c_str();  
}

/* BuildFuppesDeviceList */
std::string CPresentationHandler::BuildFuppesDeviceList(CFuppes* pFuppes)
{
  stringstream result;

  // sort the devices
  std::list<CUPnPDevice*> devices;
  for(unsigned int i = 0; i < pFuppes->GetRemoteDevices().size(); i++) {

    CUPnPDevice* pDevice = pFuppes->GetRemoteDevices()[i];
    if(pDevice->descriptionAvailable())
      devices.push_front(pDevice);
    else
      devices.push_back(pDevice);
  }


  std::list<CUPnPDevice*>::iterator iter;
  int count = 0;
  for(iter = devices.begin(); iter != devices.end(); iter++) {
    
    CUPnPDevice* pDevice = *iter;

    result << "<div class=\"remote-device\">" << endl;

    // icon
    result << "<div class=\"remote-device-icon\">";
      if(pDevice->descriptionAvailable()) {
        result << "device icon";
      }
      else {
       result << "icon loading";
      }
    result << "</div>" << endl;

 
    // friendly name    
    result << "<div class=\"remote-device-friendly-name\">";
    result << pDevice->GetFriendlyName();
    result << "</div>" << endl;


    result << "<div class=\"remote-device-info\">";
    result << pDevice->GetUPnPDeviceTypeAsString() << " : " << pDevice->GetUPnPDeviceVersion();    
    result << "</div>" << endl;

    result << "<a href=\"javascript:deviceDetails(" << count << ");\">";
    result << "details";
    result << "</a>" << endl;

    // details
    result << "<div class=\"remote-device-details\" id=\"remote-device-details-" << count << "\">";


    result << "<table>" << endl;

    result << "<tr><th colspan=\"2\">details</th></tr>" << endl;
    
    // uuid
    result << "<tr><td>uuid</td><td>";
    result << pDevice->GetUUID();
    result << "</td></tr>" << endl;
    
    // timeout
    result << "<tr><td>timeout</td><td>";
    result << pDevice->GetTimer()->GetCount() / 60 << "min. " << pDevice->GetTimer()->GetCount() % 60 << "sec.";
    result << "</td></tr>" << endl;
    
    // ip : port

    // presentation url
    result << "<tr><td>presentation</td><td>";
    result << pDevice->presentationUrl() << "<br />";
    result << "</td></tr>" << endl;
    
    // manufacturer
    result << "<tr><td>manufacturer</td><td>";
    result << pDevice->manufacturer() << "<br />";
    result << "</td></tr>" << endl;
    
    // manufacturerUrl
    result << "<tr><td>manufacturerUrl</td><td>";
    result << pDevice->manufacturerUrl() << "<br />";
    result << "</td></tr>" << endl;



    // descriptionUrl
    result << "<tr><td>descriptionUrl</td><td>";
    result << pDevice->descriptionUrl() << "<br />";
    result << "</td></tr>" << endl;
    

    result << "</table>" << endl;
    
    result << "</div>" << endl; //details
    

    result << "</div>" << endl; // remote-device

    count++;
  }

  result << "<div id=\"remote-device-count\" style=\"display: none;\">" << count << "</div>";
  
  return result.str();
}


std::string CPresentationHandler::GetJsTestHTML()
{
  std::stringstream sResult;
  
  sResult << "<h1>JS test</h1>";

  sResult << "<div>";

  sResult << "<a href=\"javascript:browseDirectChildren(0, 0, 20);\">browse</a>"; // objectId, startIdx, requestCnt
  sResult << "<div id=\"browse-result\">";

  sResult << "</div>";
  
  sResult << "</div>";
  
  return sResult.str();
}
