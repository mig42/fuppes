/***************************************************************************
 *            FileDetails.cpp
 *
 *  FUPPES - Free UPnP Entertainment Service
 *
 *  Copyright (C) 2005 - 2007 Ulrich Völkel <u-voelkel@users.sourceforge.net>
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

#include "FileDetails.h"

#include "../Common/Common.h"
#include "../SharedConfig.h"
#include "../SharedLog.h"
#include "../Transcoding/TranscodingMgr.h"

#ifdef HAVE_CONFIG_H
#include "../../config.h"
#endif

#ifdef HAVE_TAGLIB
#include <fileref.h>
#include <tstring.h>
#include <tfile.h>
#include <tag.h>
#endif

#ifdef HAVE_IMAGEMAGICK
#include <wand/MagickWand.h>
#endif

#ifdef HAVE_LIBAVFORMAT
#include <avformat.h>
#include <avcodec.h>
#endif

#include <sstream>
#include <iostream>

using namespace std;

/** default file settings */
struct FileType_t FileTypes[] = 
{
  /* audio types */
  {"mp3" , ITEM_AUDIO_ITEM_MUSIC_TRACK, "audio/mpeg"},
  {"ogg" , ITEM_AUDIO_ITEM_MUSIC_TRACK, "application/octet-stream"},
  {"mpc" , ITEM_AUDIO_ITEM_MUSIC_TRACK, "application/octet-stream"},
  {"flac", ITEM_AUDIO_ITEM_MUSIC_TRACK, "audio/x-flac"},  
  
  /* image types */
  {"jpeg", ITEM_IMAGE_ITEM_PHOTO, "image/jpeg"},
  {"jpg" , ITEM_IMAGE_ITEM_PHOTO, "image/jpeg"},
  {"bmp" , ITEM_IMAGE_ITEM_PHOTO, "image/bmp"},
  {"png" , ITEM_IMAGE_ITEM_PHOTO, "image/png"},
  {"gif" , ITEM_IMAGE_ITEM_PHOTO, "image/gif"},
  
  /* video types */
  {"mpeg", ITEM_VIDEO_ITEM_MOVIE, "video/mpeg"},
  {"mpg" , ITEM_VIDEO_ITEM_MOVIE, "video/mpeg"},
  {"avi" , ITEM_VIDEO_ITEM_MOVIE, "video/x-msvideo"},
  {"wmv" , ITEM_VIDEO_ITEM_MOVIE, "video/x-ms-wmv"},
  {"vob" , ITEM_VIDEO_ITEM_MOVIE, "video/x-ms-vob"},
  {"vdr" , ITEM_VIDEO_ITEM_MOVIE, "application/x-extension-vdr"},
  
  /* playlist types */
  {"m3u", CONTAINER_PLAYLIST_CONTAINER, "audio/x-mpegurl"},
  {"pls", CONTAINER_PLAYLIST_CONTAINER, "audio/x-scpls"},
  
  /* empty entry to mark the list's end */
  {"", OBJECT_TYPE_UNKNOWN, ""}
};


struct TranscodingSetting_t TranscodingSettings[] =
{
  /* audio */
  {"ogg" , "mp3", "audio/mpeg", ITEM_AUDIO_ITEM_MUSIC_TRACK},
  {"mpc" , "mp3", "audio/mpeg", ITEM_AUDIO_ITEM_MUSIC_TRACK},
  {"flac", "mp3", "audio/mpeg", ITEM_AUDIO_ITEM_MUSIC_TRACK},
  
  /* video */
  {"vdr", "vob", "video/x-ms-vob", ITEM_VIDEO_ITEM_MOVIE},
  
  /* empty entry to mark the list's end */
  {"", "", "", OBJECT_TYPE_UNKNOWN}
};
 

CFileDetails* CFileDetails::m_Instance = 0;

CFileDetails* CFileDetails::Shared()
{
	if (m_Instance == 0)
		m_Instance = new CFileDetails();
	return m_Instance;
}

CFileDetails::CFileDetails()
{
}

OBJECT_TYPE CFileDetails::GetObjectType(std::string p_sFileName)
{
  string sExt = ToLower(ExtractFileExt(p_sFileName));
  struct FileType_t* pType;   
  
  pType = FileTypes;
  while(!pType->sExt.empty())
  {
    if(pType->sExt.compare(sExt) == 0)
    {
      return pType->nObjectType;
      break;
    } 
    
    pType++;
  }

  CSharedLog::Shared()->Log(L_WARNING, "unhandled file extension: " + sExt, __FILE__, __LINE__);  
  return OBJECT_TYPE_UNKNOWN;  
}

std::string CFileDetails::GetMimeType(std::string p_sFileName, bool p_bTranscodingMimeType)
{
  string sExt = ToLower(ExtractFileExt(p_sFileName));
  struct FileType_t* pType; 
  struct TranscodingSetting_t* pTranscoding;  
  
  pType = FileTypes;
  while(!pType->sExt.empty())
  {
    if(pType->sExt.compare(sExt) == 0)
    {
      
      // check for transcoding settings
      if(p_bTranscodingMimeType && CTranscodingMgr::Shared()->IsTranscodingExtension(sExt))
      {
        pTranscoding = TranscodingSettings;
        while(!pTranscoding->sExt.empty())
        {
          if(pTranscoding->sExt.compare(sExt) == 0)                      
            return pTranscoding->sTargetMimeType;
          
          pTranscoding++;
        }
      }
      
      // return default mime type
      return pType->sMimeType;      
    } // if
    
    pType++;
  } // while !sExt.empty
  
  
  CSharedLog::Shared()->Log(L_EXTENDED_WARN, "unhandled file extension: " + sExt, __FILE__, __LINE__);  
  return "";
}

std::string CFileDetails::GetObjectTypeAsString(unsigned int p_nObjectType)
{
  switch(p_nObjectType)
  {
    case OBJECT_TYPE_UNKNOWN :
      return "unknown";
    
    case ITEM_IMAGE_ITEM :
      return "imageItem";    
    case ITEM_IMAGE_ITEM_PHOTO :
      return "imageItem.photo";
  
    case ITEM_AUDIO_ITEM :
      return "audioItem";
    case ITEM_AUDIO_ITEM_MUSIC_TRACK :
      return "audioItem.musicTrack";
    case ITEM_AUDIO_ITEM_AUDIO_BROADCAST :
      return "audioItem.audioBroadcast";
    //ITEM_AUDIO_ITEM_AUDIO_BOOK      = 202,*/
  
    case ITEM_VIDEO_ITEM :
      return "videoItem";
    case ITEM_VIDEO_ITEM_MOVIE :
      return "videoItem.movie";
    case ITEM_VIDEO_ITEM_VIDEO_BROADCAST :
      return "videoItem.videoBroadcast";
    //ITEM_VIDEO_ITEM_MUSIC_VIDEO_CLIP = 302,  
  
    /*CONTAINER_PERSON = 4,
      CONTAINER_PERSON_MUSIC_ARTIST = 400,*/
    
    case CONTAINER_PLAYLIST_CONTAINER :
      return "container.playlistContainer";
    
    /*CONTAINER_ALBUM = 6,
    
      CONTAINER_ALBUM_MUSIC_ALBUM = 600,
      CONTAINER_ALBUM_PHOTO_ALBUM = 601,
      
    CONTAINER_GENRE = 7,
      CONTAINER_GENRE_MUSIC_GENRE = 700,
      CONTAINER_GENRE_MOVIE_GENRE = 701,
      
    CONTAINER_STORAGE_SYSTEM = 8,
    CONTAINER_STORAGE_VOLUME = 9, */
    case CONTAINER_STORAGE_FOLDER :
      return "container";
    
    default :
      return "CFileDetails::GetObjectTypeAsString() :: unhandled type (please send a bugreport)";
  }
}

bool CFileDetails::IsTranscodingExtension(std::string p_sExt)
{
  TranscodingSetting_t* pTranscoding;
  pTranscoding = TranscodingSettings;
  while(!pTranscoding->sExt.empty())
  {
    if(pTranscoding->sExt.compare(p_sExt) == 0)    
      return CTranscodingMgr::Shared()->IsTranscodingExtension(p_sExt);
    
    pTranscoding++;
  }
  return false;
}

std::string CFileDetails::GetTargetExtension(std::string p_sExt)
{
  string sResult = p_sExt;
  
  TranscodingSetting_t* pTranscoding;
  pTranscoding = TranscodingSettings;
  while(!pTranscoding->sExt.empty())
  {
    if(pTranscoding->sExt.compare(p_sExt) == 0)
    {
      sResult = pTranscoding->sTargetExt;
      break;
    }    
    pTranscoding++;
  }
  
  return sResult;
}

bool CFileDetails::GetMusicTrackDetails(std::string p_sFileName, SMusicTrack* pMusicTrack)
{
  #ifdef HAVE_TAGLIB  
  TagLib::FileRef pFile(p_sFileName.c_str());
  
	if (pFile.isNull()) {
	  CSharedLog::Shared()->Log(L_EXTENDED_ERR, "taglib error", __FILE__, __LINE__);
		return false;
	}
	
	TagLib::String sTmp;
	uint nTmp;
	
	// title
	sTmp = pFile.tag()->title();
  pMusicTrack->mAudioItem.sTitle = sTmp.to8Bit(true);  
  
	//string duration = "H+:MM:SS(.00)";
	//pFile.audioProperties()->ReadStyle = TagLib::Accurate;
	long length = pFile.audioProperties()->length();
  char s[4];
  stringstream sDuration;
	sDuration << length/(60*60) << ":";
  sprintf(s, "%02ld:", length/60);
  s[3] = '\0';
  sDuration << s;
  sprintf(s, "%02ld", (length - length/60));
  s[2] = '\0';
  sDuration << s;
	
  cout << p_sFileName << endl << "DURATION:" << endl;
 	cout << length << endl;
	cout << length/(60*60) << ":" << length/60 << ":" << (length - length/60) << endl;
  cout << sDuration.str() << endl << endl;
  
  pMusicTrack->mAudioItem.sDuration = sDuration.str();
	
	// channels
	pMusicTrack->mAudioItem.nNrAudioChannels = pFile.audioProperties()->channels();
	
	// bitrate
	pMusicTrack->mAudioItem.nBitrate = pFile.audioProperties()->bitrate();
	
	// samplerate
	pMusicTrack->mAudioItem.nSampleRate = pFile.audioProperties()->sampleRate();
	
	// artist
  sTmp = pFile.tag()->artist();
  pMusicTrack->sArtist = sTmp.to8Bit(true);  
    
  // album
  sTmp = pFile.tag()->album();
  pMusicTrack->sAlbum = sTmp.to8Bit(true);  
  
  // genre
	sTmp = pFile.tag()->genre();
  pMusicTrack->mAudioItem.sGenre = sTmp.to8Bit(true);   

  // description/comment
	sTmp = pFile.tag()->comment();
	pMusicTrack->mAudioItem.sDescription = sTmp.to8Bit(true);

  // track no.
	nTmp = pFile.tag()->track();
	pMusicTrack->nOriginalTrackNumber = nTmp;

  // date/year
	nTmp = pFile.tag()->year();
  stringstream sDate;
  sDate << nTmp;
  pMusicTrack->sDate = sDate.str();

  return true;
  #else
	return false;
	#endif
}

bool CFileDetails::GetImageDetails(std::string p_sFileName, SImageItem* pImageItem)
{
  #ifdef HAVE_IMAGEMAGICK
	return false;
	
	MagickWand* pWand;
	MagickBooleanType bStatus;
	unsigned long nHeight;
	unsigned long nWidth;
	
	//MagickWandGenesis();
	pWand = NewMagickWand();
	bStatus = MagickReadImage(pWand, p_sFileName.c_str());
	if (bStatus == MagickFalse) {
    DestroyMagickWand(pWand);
		return false;
	}
	
	// width/height
	nWidth  = MagickGetImageWidth(pWand);
	nHeight = MagickGetImageHeight(pWand);
		
	pImageItem->nWidth  = nWidth;
	pImageItem->nHeight = nHeight;
	
	DestroyMagickWand(pWand);
	//MagickWandTerminus();
	return true;
	#else
	return false;
	#endif
}

bool CFileDetails::GetVideoDetails(std::string p_sFileName, SVideoItem* pVideoItem)
{
  #ifdef HAVE_LIBAVFORMAT
	av_register_all();
	AVFormatContext *pFormatCtx;
	
	if(av_open_input_file(&pFormatCtx, p_sFileName.c_str(), NULL, 0, NULL) != 0) {
	  cout << "error open av file: " << p_sFileName << endl;
		return false;
	}
		
	if(av_find_stream_info(pFormatCtx) < 0) {
	  cout << "error reading stream info: " << p_sFileName << endl;
		av_close_input_file(pFormatCtx);
	  return false;
	}
		
	// duration
	if(pFormatCtx->duration != AV_NOPTS_VALUE) {
  	cout << pFormatCtx->duration << endl;
 
	  int hours, mins, secs, us;
		secs = pFormatCtx->duration / AV_TIME_BASE;
		us   = pFormatCtx->duration % AV_TIME_BASE;
		mins = secs / 60;
		secs %= 60;
		hours = mins / 60;
		mins %= 60;
	
		char szDuration[11];
	  sprintf(szDuration, "%02d:%02d:%02d.%01d", hours, mins, secs, (10 * us) / AV_TIME_BASE);
	  szDuration[10] = '\0';
		
	  pVideoItem->sDuration = szDuration;
	}
	else {
	  pVideoItem->sDuration = "NULL";
	}
	
	// bitrate
	if(pFormatCtx->bit_rate)
  	pVideoItem->nBitrate = pFormatCtx->bit_rate / 8;
	else
	  pVideoItem->nBitrate = 0;

  // filesize
	pVideoItem->nSize = pFormatCtx->file_size;
	
	
	for(int i = 0; i < pFormatCtx->nb_streams; i++) 
  {
	  AVStream* pStream = pFormatCtx->streams[i];
	  AVCodec* pCodec;
				
		pCodec = avcodec_find_decoder(pStream->codec->codec_id);
		if(pCodec)
		{			
			switch(pStream->codec->codec_type)
			{
			  case CODEC_TYPE_VIDEO:
					pVideoItem->nWidth  = pStream->codec->width;
					pVideoItem->nHeight = pStream->codec->height;
					break;
			  case CODEC_TYPE_AUDIO:
					break;
				case CODEC_TYPE_DATA:
					break;
				case CODEC_TYPE_SUBTITLE:
					break;
				default:
					break;
			}
		}
	}
	
	//dump_avformat(pFormatCtx, 0, p_sFileName.c_str(), false);
	av_close_input_file(pFormatCtx);
	
	return true;
	#else
	return false;
	#endif
}


/*void dump_avformat(AVFormatContext *ic,
                 int index,
                 const char *url,
                 int is_output)
{
    int i, flags;
    char buf[256];


  cout << "DUMP AV FORMAT" << endl;

    av_log(NULL, AV_LOG_INFO, "%s #%d, %s, %s '%s':\n",
            is_output ? "Output" : "Input",
            index,
            is_output ? ic->oformat->name : ic->iformat->name,
            is_output ? "to" : "from", url);
    if (!is_output) {
        av_log(NULL, AV_LOG_INFO, "  Duration: ");
        if (ic->duration != AV_NOPTS_VALUE) {
            int hours, mins, secs, us;
            secs = ic->duration / AV_TIME_BASE;
            us = ic->duration % AV_TIME_BASE;
            mins = secs / 60;
            secs %= 60;
            hours = mins / 60;
            mins %= 60;
            av_log(NULL, AV_LOG_INFO, "%02d:%02d:%02d.%01d", hours, mins, secs,
                   (10 * us) / AV_TIME_BASE);
        } else {
            av_log(NULL, AV_LOG_INFO, "N/A");
        }
        if (ic->start_time != AV_NOPTS_VALUE) {
            int secs, us;
            av_log(NULL, AV_LOG_INFO, ", start: ");
            secs = ic->start_time / AV_TIME_BASE;
            us = ic->start_time % AV_TIME_BASE;
            av_log(NULL, AV_LOG_INFO, "%d.%06d",
                   secs, (int)av_rescale(us, 1000000, AV_TIME_BASE));
        }
        av_log(NULL, AV_LOG_INFO, ", bitrate: ");
        if (ic->bit_rate) {
            av_log(NULL, AV_LOG_INFO,"%d kb/s", ic->bit_rate / 1000);
        } else {
            av_log(NULL, AV_LOG_INFO, "N/A");
        }
        av_log(NULL, AV_LOG_INFO, "\n");
    }
    
    cout << "NO STREAMS: " << ic->nb_streams << endl;
    
    for(i=0;i<ic->nb_streams;i++) {
        AVStream *st = ic->streams[i];
        int g= ff_gcd(st->time_base.num, st->time_base.den);
        avcodec_string(buf, sizeof(buf), st->codec, is_output);
        av_log(NULL, AV_LOG_INFO, "  Stream #%d.%d", index, i);
        // the pid is an important information, so we display it 
        // XXX: add a generic system
        if (is_output)
            flags = ic->oformat->flags;
        else
            flags = ic->iformat->flags;
        if (flags & AVFMT_SHOW_IDS) {
            av_log(NULL, AV_LOG_INFO, "[0x%x]", st->id);
        }
        if (strlen(st->language) > 0) {
            av_log(NULL, AV_LOG_INFO, "(%s)", st->language);
        }
        av_log(NULL, AV_LOG_DEBUG, ", %d/%d", st->time_base.num/g, st->time_base.den/g);
        av_log(NULL, AV_LOG_INFO, ": %s", buf);
        if(st->codec->codec_type == CODEC_TYPE_VIDEO){
            if(st->r_frame_rate.den && st->r_frame_rate.num)
                av_log(NULL, AV_LOG_INFO, ", %5.2f fps(r)", av_q2d(st->r_frame_rate));
//            else if(st->time_base.den && st->time_base.num)
//                av_log(NULL, AV_LOG_INFO, ", %5.2f fps(m)", 1/av_q2d(st->time_base));
            else
                av_log(NULL, AV_LOG_INFO, ", %5.2f fps(c)", 1/av_q2d(st->codec->time_base));
        }
        av_log(NULL, AV_LOG_INFO, "\n");
    }


  cout << endl << endl;
  fuppesSleep(1000);
} */

/*
void avcodec_string(char *buf, int buf_size, AVCodecContext *enc, int encode)
01088 {
01089     const char *codec_name;
01090     AVCodec *p;
01091     char buf1[32];
01092     char channels_str[100];
01093     int bitrate;
01094 
01095     if (encode)
01096         p = avcodec_find_encoder(enc->codec_id);
01097     else
01098         p = avcodec_find_decoder(enc->codec_id);
01099 
01100     if (p) {
01101         codec_name = p->name;
01102         if (!encode && enc->codec_id == CODEC_ID_MP3) {
01103             if (enc->sub_id == 2)
01104                 codec_name = "mp2";
01105             else if (enc->sub_id == 1)
01106                 codec_name = "mp1";
01107         }
01108     } else if (enc->codec_id == CODEC_ID_MPEG2TS) {
01109         // fake mpeg2 transport stream codec (currently not registered) 
01111         codec_name = "mpeg2ts";
01112     } else if (enc->codec_name[0] != '\0') {
01113         codec_name = enc->codec_name;
01114     } else {
01115         // output avi tags 
01116         if(   isprint(enc->codec_tag&0xFF) && isprint((enc->codec_tag>>8)&0xFF)
01117            && isprint((enc->codec_tag>>16)&0xFF) && isprint((enc->codec_tag>>24)&0xFF)){
01118             snprintf(buf1, sizeof(buf1), "%c%c%c%c / 0x%04X",
01119                      enc->codec_tag & 0xff,
01120                      (enc->codec_tag >> 8) & 0xff,
01121                      (enc->codec_tag >> 16) & 0xff,
01122                      (enc->codec_tag >> 24) & 0xff,
01123                       enc->codec_tag);
01124         } else {
01125             snprintf(buf1, sizeof(buf1), "0x%04x", enc->codec_tag);
01126         }
01127         codec_name = buf1;
01128     }
01129 
01130     switch(enc->codec_type) {
01131     case CODEC_TYPE_VIDEO:
01132         snprintf(buf, buf_size,
01133                  "Video: %s%s",
01134                  codec_name, enc->mb_decision ? " (hq)" : "");
01135         if (enc->pix_fmt != PIX_FMT_NONE) {
01136             snprintf(buf + strlen(buf), buf_size - strlen(buf),
01137                      ", %s",
01138                      avcodec_get_pix_fmt_name(enc->pix_fmt));
01139         }
01140         if (enc->width) {
01141             snprintf(buf + strlen(buf), buf_size - strlen(buf),
01142                      ", %dx%d",
01143                      enc->width, enc->height);
01144             if(av_log_get_level() >= AV_LOG_DEBUG){
01145                 int g= ff_gcd(enc->time_base.num, enc->time_base.den);
01146                 snprintf(buf + strlen(buf), buf_size - strlen(buf),
01147                      ", %d/%d",
01148                      enc->time_base.num/g, enc->time_base.den/g);
01149             }
01150         }
01151         if (encode) {
01152             snprintf(buf + strlen(buf), buf_size - strlen(buf),
01153                      ", q=%d-%d", enc->qmin, enc->qmax);
01154         }
01155         bitrate = enc->bit_rate;
01156         break;
01157     case CODEC_TYPE_AUDIO:
01158         snprintf(buf, buf_size,
01159                  "Audio: %s",
01160                  codec_name);
01161         switch (enc->channels) {
01162             case 1:
01163                 strcpy(channels_str, "mono");
01164                 break;
01165             case 2:
01166                 strcpy(channels_str, "stereo");
01167                 break;
01168             case 6:
01169                 strcpy(channels_str, "5:1");
01170                 break;
01171             default:
01172                 snprintf(channels_str, sizeof(channels_str), "%d channels", enc->channels);
01173                 break;
01174         }
01175         if (enc->sample_rate) {
01176             snprintf(buf + strlen(buf), buf_size - strlen(buf),
01177                      ", %d Hz, %s",
01178                      enc->sample_rate,
01179                      channels_str);
01180         }
01181 
01182         // for PCM codecs, compute bitrate directly
01183         switch(enc->codec_id) {
01184         case CODEC_ID_PCM_S32LE:
01185         case CODEC_ID_PCM_S32BE:
01186         case CODEC_ID_PCM_U32LE:
01187         case CODEC_ID_PCM_U32BE:
01188             bitrate = enc->sample_rate * enc->channels * 32;
01189             break;
01190         case CODEC_ID_PCM_S24LE:
01191         case CODEC_ID_PCM_S24BE:
01192         case CODEC_ID_PCM_U24LE:
01193         case CODEC_ID_PCM_U24BE:
01194         case CODEC_ID_PCM_S24DAUD:
01195             bitrate = enc->sample_rate * enc->channels * 24;
01196             break;
01197         case CODEC_ID_PCM_S16LE:
01198         case CODEC_ID_PCM_S16BE:
01199         case CODEC_ID_PCM_U16LE:
01200         case CODEC_ID_PCM_U16BE:
01201             bitrate = enc->sample_rate * enc->channels * 16;
01202             break;
01203         case CODEC_ID_PCM_S8:
01204         case CODEC_ID_PCM_U8:
01205         case CODEC_ID_PCM_ALAW:
01206         case CODEC_ID_PCM_MULAW:
01207             bitrate = enc->sample_rate * enc->channels * 8;
01208             break;
01209         default:
01210             bitrate = enc->bit_rate;
01211             break;
01212         }
01213         break;
01214     case CODEC_TYPE_DATA:
01215         snprintf(buf, buf_size, "Data: %s", codec_name);
01216         bitrate = enc->bit_rate;
01217         break;
01218     case CODEC_TYPE_SUBTITLE:
01219         snprintf(buf, buf_size, "Subtitle: %s", codec_name);
01220         bitrate = enc->bit_rate;
01221         break;
01222     default:
01223         snprintf(buf, buf_size, "Invalid Codec type %d", enc->codec_type);
01224         return;
01225     }
01226     if (encode) {
01227         if (enc->flags & CODEC_FLAG_PASS1)
01228             snprintf(buf + strlen(buf), buf_size - strlen(buf),
01229                      ", pass 1");
01230         if (enc->flags & CODEC_FLAG_PASS2)
01231             snprintf(buf + strlen(buf), buf_size - strlen(buf),
01232                      ", pass 2");
01233     }
01234     if (bitrate != 0) {
01235         snprintf(buf + strlen(buf), buf_size - strlen(buf),
01236                  ", %d kb/s", bitrate / 1000);
01237     }
01238 }
*/

/*
typedef struct AVFormatContext {
    const AVClass *av_class;


    struct AVInputFormat *iformat;
    struct AVOutputFormat *oformat;
    void *priv_data;
    ByteIOContext pb;
    int nb_streams;
    AVStream *streams[MAX_STREAMS];
    char filename[1024]; 
    
		// stream info 
    int64_t timestamp;
    char title[512];
    char author[512];
    char copyright[512];
    char comment[512];
    char album[512];
    int year;  // ID3 year, 0 if none 
    int track; // track number, 0 if none 
    char genre[32]; // ID3 genre 

   
    // decoding: position of the first frame of the component, in
      // AV_TIME_BASE fractional seconds. NEVER set this value directly:
       //it is deduced from the AVStream values.  
    int64_t start_time;
    // decoding: duration of the stream, in AV_TIME_BASE fractional
    //   seconds. NEVER set this value directly: it is deduced from the
      // AVStream values. 
    int64_t duration;
    // decoding: total file size. 0 if unknown 
    int64_t file_size;
    // decoding: total stream bitrate in bit/s, 0 if not
    //   available. Never set it directly if the file_size and the
     //  duration are known as ffmpeg can compute it automatically. 
    int bit_rate;

    // av_read_frame() support 
    AVStream *cur_st;
    const uint8_t *cur_ptr;
    int cur_len;
    AVPacket cur_pkt;

    // av_seek_frame() support 
    int64_t data_offset; // offset of the first packet 
    int index_built;

    int mux_rate;
    int packet_size;
    int preload;
    int max_delay;

#define AVFMT_NOOUTPUTLOOP -1
#define AVFMT_INFINITEOUTPUTLOOP 0
    // number of times to loop output in formats that support it 
    int loop_output;

    int flags;
#define AVFMT_FLAG_GENPTS       0x0001 ///< generate pts if missing even if it requires parsing future frames
#define AVFMT_FLAG_IGNIDX       0x0002 ///< ignore index

    int loop_input;
    // decoding: size of data to probe; encoding unused 
    unsigned int probesize;
} AVFormatContext;
*/
