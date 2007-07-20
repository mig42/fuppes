/***************************************************************************
 *            FFmpegWrapper.h
 *
 *  FUPPES - Free UPnP Entertainment Service
 *
 *  Copyright (C) 2007 Ulrich Völkel <u-voelkel@users.sourceforge.net>
 ****************************************************************************/

/*
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License version 2 as 
 *  published by the Free Software Foundation.
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

#ifndef FFMPEGWRAPPER_H
#define FFMPEGWRAPPER_H

#ifdef HAVE_CONFIG_H
#include "../../config.h"
#endif

#include "WrapperBase.h"

#ifdef HAVE_LIBAVFORMAT

#include "ffmpeg/ffmpeg.h"

class CFFmpegWrapper: public CTranscoderBase
{
  public:
    virtual ~CFFmpegWrapper() {};
    bool Transcode(std::string p_sInFileParams, std::string p_sInFile, std::string p_sOutFileParams, std::string* p_psOutFile);
};

#endif // HAVE_LIBAVFORMAT

#endif // FFMPEGWRAPPER_H