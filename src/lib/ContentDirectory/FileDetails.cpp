/***************************************************************************
 *            FileDetails.cpp
 *
 *  FUPPES - Free UPnP Entertainment Service
 *
 *  Copyright (C) 2005-2008 Ulrich Völkel <u-voelkel@users.sourceforge.net>
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

#include "FileDetails.h"

#include "../Common/Common.h"
#include "../SharedConfig.h"
#include "../SharedLog.h"
#include "../Transcoding/TranscodingMgr.h"
#include "../DeviceSettings/DeviceIdentificationMgr.h"
#include "../Plugins/Plugin.h"

#include <sstream>
#include <iostream>

using namespace std;

CFileDetails* CFileDetails::m_Instance = 0;

CFileDetails* CFileDetails::Shared()
{
	if (m_Instance == 0)
		m_Instance = new CFileDetails();
	return m_Instance;
}

CFileDetails::CFileDetails()
{
	#ifdef DLNA_SUPPORT
	m_dlna = dlna_init();
  dlna_register_all_media_profiles(m_dlna);
	#endif
}

CFileDetails::~CFileDetails()
{
	#ifdef DLNA_SUPPORT
	dlna_uninit(m_dlna);
	#endif
}

OBJECT_TYPE CFileDetails::GetObjectType(std::string p_sFileName)
{
  string sExt = ExtractFileExt(p_sFileName);
  return CDeviceIdentificationMgr::Shared()->DefaultDevice()->ObjectType(sExt);
}

std::string CFileDetails::GetObjectTypeAsStr(OBJECT_TYPE p_nObjectType)
{
  if(p_nObjectType < ITEM) {
    return GetContainerTypeAsStr(p_nObjectType);
  }
  
  switch(p_nObjectType) {
    
    case ITEM:
      return "object.item";      
    case ITEM_IMAGE_ITEM :
      return "object.item.imageItem";    
    case ITEM_IMAGE_ITEM_PHOTO :
      return "object.item.imageItem.photo";
  
    case ITEM_AUDIO_ITEM :
      return "object.item.audioItem";
    case ITEM_AUDIO_ITEM_MUSIC_TRACK :
      return "object.item.audioItem.musicTrack";
    case ITEM_AUDIO_ITEM_AUDIO_BROADCAST :
      return "object.item.audioItem.audioBroadcast";
  
    case ITEM_VIDEO_ITEM :
      return "object.item.videoItem";
    case ITEM_VIDEO_ITEM_MOVIE :
      return "object.item.videoItem.movie";
    case ITEM_VIDEO_ITEM_VIDEO_BROADCAST :
      return "object.item.videoItem.videoBroadcast";
      
    default:
      return "unknown";
  }
}

std::string CFileDetails::GetContainerTypeAsStr(OBJECT_TYPE p_nContainerType)
{
  switch(p_nContainerType) {
    
    //CONTAINER_PERSON = 4,
    case CONTAINER_PERSON_MUSIC_ARTIST :
      return "object.container.person.musicArtist";
    
    case CONTAINER_PLAYLIST_CONTAINER :
      return "object.container.playlistContainer";
    
    //CONTAINER_ALBUM = 6, 
    
		case CONTAINER_ALBUM_MUSIC_ALBUM :
		  return "object.container.album.musicAlbum";
			
    case CONTAINER_ALBUM_PHOTO_ALBUM :
		  return "object.container.album.photoAlbum";
    
    case CONTAINER_GENRE :
      return "object.container.genre";
    case CONTAINER_GENRE_MUSIC_GENRE :
      return "object.container.genre.musicGenre";    
    /*  CONTAINER_GENRE_MOVIE_GENRE = 701,
      
    CONTAINER_STORAGE_SYSTEM = 8,
    CONTAINER_STORAGE_VOLUME = 9, */
    case CONTAINER_STORAGE_FOLDER :
      return "object.container.storageFolder";
    
    default:
      return "unknown";
  }
}

bool CFileDetails::IsSupportedFileExtension(std::string p_sFileExtension)
{
  p_sFileExtension = ToLower(p_sFileExtension);
  return CDeviceIdentificationMgr::Shared()->DefaultDevice()->Exists(p_sFileExtension);
}

bool CFileDetails::GetMusicTrackDetails(std::string p_sFileName, SAudioItem* pMusicTrack)
{
  string sExt = ExtractFileExt(p_sFileName);
  if(!CDeviceIdentificationMgr::Shared()->DefaultDevice()->FileSettings(sExt)->ExtractMetadata())
    return false;
	
	CMetadataPlugin* audio = NULL;

	if(sExt.compare("m4a") == 0 || sExt.compare("mp4") == 0) {
		audio = CPluginMgr::metadataPlugin("mp4v2");
	}
	else {
		audio = CPluginMgr::metadataPlugin("taglib");
	}
	
	metadata_t metadata;
	if(!audio || !audio->openFile(p_sFileName)) {
		return false;
	}
		
	init_metadata(&metadata);			
	if(!audio->readData(&metadata)) {
		free_metadata(&metadata);	
		return false;
	}

	pMusicTrack->sTitle 					= TrimWhiteSpace(metadata.title);  
	pMusicTrack->sDuration				= metadata.duration;
	pMusicTrack->nNrAudioChannels =	metadata.channels;
	pMusicTrack->nBitrate 				= metadata.bitrate;
	pMusicTrack->nBitsPerSample 	= 0;
	pMusicTrack->nSampleRate 			= metadata.samplerate;
	pMusicTrack->sArtist 					= TrimWhiteSpace(metadata.artist);
	pMusicTrack->sAlbum 					= TrimWhiteSpace(metadata.album);
	pMusicTrack->sGenre 					= TrimWhiteSpace(metadata.genre);
	pMusicTrack->sDescription 		= TrimWhiteSpace(metadata.description);
	pMusicTrack->nOriginalTrackNumber = metadata.track_no;
	//pMusicTrack->sDate 					= sDate.str();

	if(pMusicTrack->sArtist.empty()) {
		pMusicTrack->sArtist = "unknown";
	}			 
	if(pMusicTrack->sAlbum.empty()) {
		pMusicTrack->sAlbum = "unknown";
	}				
	if(pMusicTrack->sGenre.empty()) {
		pMusicTrack->sGenre = "unknown";
	}
	
	audio->closeFile();
	free_metadata(&metadata);
	return true;	
}

bool CFileDetails::GetImageDetails(std::string p_sFileName, SImageItem* pImageItem)
{
	string sExt = ExtractFileExt(p_sFileName);  
  if(!CDeviceIdentificationMgr::Shared()->DefaultDevice()->FileSettings(sExt)->ExtractMetadata())
    return false;
	
	CMetadataPlugin* image = CPluginMgr::metadataPlugin("simage");
	metadata_t metadata;
	if(image && image->openFile(p_sFileName)) {
		
		if(image->readData(&metadata)) {
			pImageItem->nWidth  = metadata.width;
			pImageItem->nHeight = metadata.height;
			
			image->closeFile();
			return true;
		}
	}
	
	image = CPluginMgr::metadataPlugin("magickWand");
	if(image && image->openFile(p_sFileName)) {
				
		if(image->readData(&metadata)) {
			pImageItem->nWidth  = metadata.width;
			pImageItem->nHeight = metadata.height;
			
			image->closeFile();
			return true;
		}
	}
	
	return false;
}

bool CFileDetails::GetVideoDetails(std::string p_sFileName, SVideoItem* pVideoItem)
{
	string sExt = ExtractFileExt(p_sFileName);  
  if(!CDeviceIdentificationMgr::Shared()->DefaultDevice()->FileSettings(sExt)->ExtractMetadata())
    return false;
	
	CMetadataPlugin* video = CPluginMgr::metadataPlugin("libavformat");
	metadata_t metadata;
	if(video && video->openFile(p_sFileName)) {

		init_metadata(&metadata);
		if(video->readData(&metadata)) {
			
			pVideoItem->nWidth  = metadata.width;
			pVideoItem->nHeight = metadata.height;
			
			pVideoItem->nBitrate = metadata.bitrate;
			pVideoItem->sACodec  = metadata.audio_codec;
			pVideoItem->sVCodec  = metadata.video_codec;

			pVideoItem->sDuration = metadata.duration;
			
			video->closeFile();
			free_metadata(&metadata);
			return true;
		}
		free_metadata(&metadata);
	}
	
	return false;
}

std::string CFileDetails::GuessDLNAProfileId(std::string p_sFileName)
{
  #ifdef DLNA_SUPPORT
  string sResult;
  dlna_profile_t *p;
  p = dlna_guess_media_profile(m_dlna, p_sFileName.c_str());
  if(p) {
    sResult = p->id;
  }  
  return sResult;
  #else
  return "";
  #endif
}

/*std::string CFileDetails::GetDLNAString(std::string p_sFileName)
{		
	string sResult = "";	
	dlna_profile_t *p;
  dlna_org_flags_t flags;

  flags = DLNA_ORG_FLAG_STREAMING_TRANSFER_MODE |
    DLNA_ORG_FLAG_BACKGROUND_TRANSFERT_MODE |
    DLNA_ORG_FLAG_CONNECTION_STALL |
    DLNA_ORG_FLAG_DLNA_V15;		
		
	p = dlna_guess_media_profile(dlna, p_sFileName.c_str());
  if(p) {
    char *protocol_info;
  */  
    /*printf ("ID: %s\n", p->id);
    printf ("MIME: %s\n", p->mime);
    printf ("Label: %s\n", p->label);
    printf ("Class: %d\n", p->class);
    printf ("UPnP Object Item: %s\n", dlna_profile_upnp_object_item(p));*/
/*
    protocol_info = dlna_write_protocol_info(DLNA_PROTOCOL_INFO_TYPE_HTTP,
                                             DLNA_ORG_PLAY_SPEED_NORMAL,
                                             DLNA_ORG_CONVERSION_NONE,
                                             DLNA_ORG_OPERATION_RANGE,
                                             flags, p);
    
		//printf ("Protocol Info: %s\n", protocol_info);
		sResult = protocol_info;
    free (protocol_info);
  }
			
	return sResult;
	#endif
}*/
