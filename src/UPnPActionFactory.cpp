/***************************************************************************
 *            UPnPActionFactory.cpp
 *
 *  FUPPES - Free UPnP Entertainment Service
 *
 *  Copyright (C) 2005, 2006 Ulrich Völkel <u-voelkel@users.sourceforge.net>
 *  Copyright (C) 2005 Thomas Schnitzler <tschnitzler@users.sourceforge.net>
 ****************************************************************************/

/*
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License version 2 as
 *  published by the Free Software Foundation.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU Library General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 */

/*===============================================================================
 INCLUDES
===============================================================================*/

#include "Common.h"
#include "UPnPActionFactory.h"
#include "RegEx.h"

#include <iostream>
#include <libxml/parser.h>
#include <libxml/tree.h>

using namespace std;

/*===============================================================================
 CLASS CUPnPActionFactory
===============================================================================*/

/* <PUBLIC> */

/*===============================================================================
 UPNP ACTIONS
===============================================================================*/

CUPnPAction* CUPnPActionFactory::BuildActionFromString(std::string p_sContent)
{  
  //BOOL_CHK_RET_POINTER(pAction);  
  //cout << p_sContent << endl;
  
  xmlDocPtr pDoc = NULL;
  pDoc = xmlReadMemory(p_sContent.c_str(), p_sContent.length(), "", NULL, 0);
  if(!pDoc)
    cout << "error parsing action" << endl;
   
  xmlNode* pRootNode = NULL;  
  xmlNode* pTmpNode  = NULL;   
  pRootNode = xmlDocGetRootElement(pDoc);   
  
  pTmpNode = pRootNode->children->next;
  //string sName = (char*)pTmpNode->name;
  //cout << sName << endl;

  pTmpNode = pTmpNode->children->next;
  string sName = (char*)pTmpNode->name;
  //cout << sName << endl;
  
  
  CUPnPAction* pAction = NULL;
   
  
  
  if(sName.compare("Browse") == 0)
  {
    pAction = new CUPnPBrowse(p_sContent);
    ParseBrowseAction((CUPnPBrowse*)pAction);
  }
  else if(sName.compare("GetSearchCapabilities") == 0)
  {
    pAction = new CUPnPAction(UPNP_ACTION_TYPE_CONTENT_DIRECTORY_GET_SEARCH_CAPABILITIES, p_sContent);
  }
  else if(sName.compare("GetSortCapabilities") == 0)
  {
    pAction = new CUPnPAction(UPNP_ACTION_TYPE_CONTENT_DIRECTORY_GET_SORT_CAPABILITIES, p_sContent);
  }
  else if(sName.compare("GetSystemUpdateID") == 0)
  {
    pAction = new CUPnPAction(UPNP_ACTION_TYPE_CONTENT_DIRECTORY_GET_SYSTEM_UPDATE_ID, p_sContent);
  }  
  else if(sName.compare("GetProtocolInfo") == 0)
  {
    pAction = new CUPnPAction(UPNP_ACTION_TYPE_CONTENT_DIRECTORY_GET_PROTOCOL_INFO, p_sContent);
  }
  
  /* Target */
  if(pAction)
  {
    string sNs = (char*)pTmpNode->nsDef->href;
    //cout << sNs << endl;
      
    if(sNs.compare("urn:schemas-upnp-org:service:ContentDirectory:1") == 0)
    {    
      pAction->m_nTargetDevice = UPNP_DEVICE_TYPE_CONTENT_DIRECTORY;                                                                      
    }
    else if(sNs.compare("urn:schemas-upnp-org:service:ConnectionManager:1") == 0)
    {
      pAction->m_nTargetDevice = UPNP_DEVICE_TYPE_CONNECTION_MANAGER;
    }  
  }


  /*cout << "[UPnPActionFactory] Browse Action:" << endl;
  cout << "\tObjectID: " << ((CUPnPBrowse*)pResult)->m_sObjectID << endl;
  cout << "\tStartingIndex: " << ((CUPnPBrowse*)pResult)->m_nStartingIndex << endl;
  cout << "\tRequestedCount: " << ((CUPnPBrowse*)pResult)->m_nRequestedCount << endl;*/  
  
  xmlFreeDoc(pDoc);
  xmlCleanupParser();  
  
  return pAction;
}

/* <\PUBLIC> */

bool CUPnPActionFactory::ParseBrowseAction(CUPnPBrowse* pAction)
{
/*<?xml version="1.0" encoding="utf-8"?>
  <s:Envelope s:encodingStyle="http://schemas.xmlsoap.org/soap/encoding/" xmlns:s="http://schemas.xmlsoap.org/soap/envelope/">
  <s:Body>
  <u:Browse xmlns:u="urn:schemas-upnp-org:service:ContentDirectory:1">
  <ObjectID>0</ObjectID>
  <BrowseFlag>BrowseDirectChildren</BrowseFlag>
  <Filter>*</Filter>
  <StartingIndex>0</StartingIndex>
  <RequestedCount>10</RequestedCount>
  <SortCriteria />
  </u:Browse>
  </s:Body>
  </s:Envelope>*/
  
  /* Object ID */
  RegEx rxObjId("<ObjectID>(.+)</ObjectID>");
  if (rxObjId.Search(pAction->m_sMessage.c_str()))
    pAction->m_sObjectID = rxObjId.Match(1);
  
  /* Browse flag */
  pAction->m_nBrowseFlag = UPNP_BROWSE_FLAG_UNKNOWN;    
  RegEx rxBrowseFlag("<BrowseFlag>(.+)</BrowseFlag>");
  if(rxBrowseFlag.Search(pAction->m_sMessage.c_str()))
  {
    string sMatch = rxBrowseFlag.Match(1);
    if(sMatch.compare("BrowseMetadata") == 0)
      pAction->m_nBrowseFlag = UPNP_BROWSE_FLAG_METADATA;
    else if(sMatch.compare("BrowseDirectChildren") == 0)
      pAction->m_nBrowseFlag = UPNP_BROWSE_FLAG_DIRECT_CHILDREN;
  }  
  
  /* Filter */
  RegEx rxFilter("<Filter>(.+)</Filter>");
  if(rxFilter.Search(pAction->m_sMessage.c_str()))  
    pAction->m_sFilter = rxFilter.Match(1);  
  else
    pAction->m_sFilter = "*";

  /* Starting index */
  RegEx rxStartIdx("<StartingIndex>(.+)</StartingIndex>");
  if(rxStartIdx.Search(pAction->m_sMessage.c_str()))  
    pAction->m_nStartingIndex = atoi(rxStartIdx.Match(1));

  /* Requested count */
  RegEx rxReqCnt("<RequestedCount>(.+)</RequestedCount>");
  if(rxReqCnt.Search(pAction->m_sMessage.c_str()))  
    pAction->m_nRequestedCount = atoi(rxReqCnt.Match(1));  
  
  /* Sort */
  pAction->m_sSortCriteria = "";

  return true;     
}
