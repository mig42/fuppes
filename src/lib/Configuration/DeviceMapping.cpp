/* -*- Mode: C++; indent-tabs-mode: nil; c-basic-offset: 2; tab-width: 2 -*- */
/***************************************************************************
 *            DeviceMapping.cpp
 *
 *  FUPPES - Free UPnP Entertainment Service
 *
 *  Copyright (C) 2010 Ulrich Völkel <u-voelkel@users.sourceforge.net>
 *  Copyright (C) 2010 Robert Massaioli <robertmassaioli@gmail.com>
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

#include "DeviceConfigFile.h"
#include "DeviceMapping.h"
#include "../DeviceSettings/DeviceIdentificationMgr.h"
#include "../Configuration/PathFinder.h"
#include "../SharedConfig.h"

using namespace std;
using namespace fuppes;


bool VirtualFolders::Read(void) 
{
  ASSERT(pStart != NULL);

  m_enabled = (pStart->Attribute("enabled") == "true");
  
  CXMLNode* tmp = NULL;
  struct VirtualFolder folder;
  for(int i = 0; i < pStart->ChildCount(); ++i) {
    tmp = pStart->ChildNode(i);
    folder.name = tmp->Attribute("name");
    folder.enabled = (tmp->Attribute("enabled") == "true");
    m_folderSettings.push_back(folder);
  }

  return true;
}

StringList VirtualFolders::getEnabledFolders()
{
  StringList result;
  if(!m_enabled)
    return result;

  std::vector<struct VirtualFolder>::iterator iter;
  for(iter = m_folderSettings.begin();
      iter != m_folderSettings.end();
      ++iter) {
    if(!iter->enabled)
      continue;
    result.push_back(iter->name);
  }

  return result;
}




void PrintSetupDeviceErrorMessages(int error, string deviceName);

bool DeviceMapping::Read(void) 
{
  assert(pStart != NULL);

  CDeviceIdentificationMgr* identMgr = CDeviceIdentificationMgr::Shared();
  CXMLNode* pDevice = NULL;
  CDeviceConfigFile* devConf = new CDeviceConfigFile();
  int error;

  // Setup the default device
  error = devConf->SetupDevice(identMgr->GetSettingsForInitialization("default"), string("default"));
  if (error) {
    // Not being able to load the default device is a fatal error
    PrintSetupDeviceErrorMessages(error, string("default"));
    delete devConf;
    return false;
  }

  string name;
  struct mapping temp;
  for(int i = 0; i < pStart->ChildCount(); ++i) {
    pDevice = pStart->ChildNode(i);
    if(pDevice->type() != CXMLNode::ElementNode)
      continue;

    // setup the mapping
    temp.value = pDevice->Attribute("value");
    // make the right device for it
    string devName = pDevice->Attribute("device");
    // by default if there is no device field then it is the default device.
    if (devName.empty())
      devName = "default";    
    temp.device = identMgr->GetSettingsForInitialization(devName);
    
    temp.vfolder = pDevice->Attribute("vfolder");
    if(temp.vfolder.compare("none") == 0 || !CSharedConfig::virtualFolders()->enabled())
      temp.vfolder = "";
    
    // you have to map to an IP or Mac Addr (and we should add in ranges too)
    if(!temp.value.empty()) {
      // Now load all of the settings for the device
      error = devConf->SetupDevice(temp.device, devName);
      if(error) { // that is meant to be a single equals
        PrintSetupDeviceErrorMessages(error, devName);
      } else {
        // Add it to our lists
        if(pDevice->Name().compare("mac") == 0) {
          macAddrs.push_back(temp);
        } else if(pDevice->Name().compare("ip") == 0) {
          ipAddrs.push_back(temp);
        } else {
          // only 'mac' or 'ip' fields allowed in device_mapping
          CSharedLog::Log(L_NORM, __FILE__, __LINE__, "'%s not loaded: '%s' is an invalid name for a field in device_mapping.",
              devName.c_str(),
              pDevice->Name().c_str());
        }
      }
    }
  }

  delete devConf;

  return true;
}

// The error messages are messy so this prints them out and makes the logic clearer
void PrintSetupDeviceErrorMessages(int error, string deviceName) {
  assert(error > SETUP_DEVICE_SUCCESS);

  string deviceFile = deviceName + DEVICE_EXT;

  if(deviceName.compare("default") == 0) {
    switch (error) {
      case SETUP_DEVICE_NOT_FOUND:
        Log::error(Log::config, Log::normal, __FILE__, __LINE__, "The default config file %s was not found in the search path. This is a fatal error, a default config is required. Try 'locate %s' in a terminal to see where it got to.",
            deviceFile.c_str(), 
            deviceFile.c_str());
        break;
      case SETUP_DEVICE_LOAD_FAILED:
        Log::error(Log::config, Log::normal, __FILE__, __LINE__, "The config file '%s' for '%s' was found but could not be loaded. You need a default config so this error means something is wrong and FUPPES will not run in these conditions. Try 'locate %s' in a terminal to see where it got to.",
        deviceFile.c_str(), 
        deviceName.c_str(),
        deviceFile.c_str());
        break;
      default:
        Log::error(Log::config, Log::normal, __FILE__, __LINE__, "An undefined error ocurred while trying to load the default configuration file. It had the error code: %d",
            error);
        break;
    }
  } else {
    switch (error) {
      case SETUP_DEVICE_NOT_FOUND:
        Log::error(Log::config, Log::normal, __FILE__, __LINE__, "The config file '%s' for '%s' was not found. We will be skipping this file and using the defaults instead. Try 'locate %s' in a terminal to see where it got to.",
          deviceFile.c_str(), 
          deviceName.c_str(),
          deviceFile.c_str());
        break;
      case SETUP_DEVICE_LOAD_FAILED:
        Log::error(Log::config, Log::normal, __FILE__, __LINE__, "The config file '%s' for '%s' was found but could not be loaded. We will be skipping this load and using the defaults instead. Try 'locate %s' in a terminal to see where it got to.",
          deviceFile.c_str(), 
          deviceName.c_str(),
          deviceFile.c_str());
        break;
      default:
        Log::error(Log::config, Log::normal, __FILE__, __LINE__, "An undefined error ocurred while trying to load '%s'. It had the error code: %d",
            deviceFile.c_str(),
            error);
        break;
    }
  }
}

/*
void DeviceMapping::requiredVirtualDevices(set<string>* vfolders) {
  assert(vfolders != NULL);
  
  for(vector<struct mapping>::const_iterator it = macAddrs.begin(); it != macAddrs.end(); ++it) {
     vfolders->insert(it->vfolder);
  }

  for(vector<struct mapping>::const_iterator it = ipAddrs.begin(); it != ipAddrs.end(); ++it) {
     vfolders->insert(it->vfolder);
  }
}
*/