/* -*- Mode: C; indent-tabs-mode: nil; c-basic-offset: 2; tab-width: 2 -*- */
/***************************************************************************
 *            fuppes_plugin.h
 *
 *  FUPPES - Free UPnP Entertainment Service
 *
 *  Copyright (C) 2008-2009 Ulrich Völkel <u-voelkel@users.sourceforge.net>
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

#ifdef __cplusplus
extern "C" {
#endif

#ifndef _FUPPES_PLUGIN_H
#define _FUPPES_PLUGIN_H
	
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include "fuppes_plugin_types.h"


static inline int set_value_old(char** out, const char* in)
{	
	if(strlen(in) == 0) {
		return 0;
	}
  
	int size = sizeof(char) * (strlen(in) + 1);  
	*out = (char*)realloc(*out, size);	
	strncpy(*out, in, size - 1);
	out[size - 1] = '\0';
  return size;
}


static inline int set_value(char out[], const char* in)
{	
	int size = strlen(in);
  if(size == 0) {
    out[size] = '\0';
    return 0;
  }

  //printf("SIZEOF SET VALUE NEW %lu*\n", sizeof(out));
  
	//*out = (char*)realloc(*out, size + 1);	
	strncpy(out, in, size);
	out[size] = '\0';
  return size;
}


static inline arg_list_t* arg_list_create() {
	
	arg_list_t* list = (arg_list_t*)malloc(sizeof(arg_list_t));
	
	list->key = (char*)malloc(sizeof(char));
	list->key = '\0';
	list->value = (char*)malloc(sizeof(char));
	list->value = '\0';	

	list->next = NULL;
	
	return list;
}

static inline void arg_list_set_values(arg_list_t* list, const char* key, const char* value)
{
	set_value_old(&list->key, key);
	set_value_old(&list->value, value);	
}

static inline arg_list_t* arg_list_append(arg_list_t* list) {
	while(list->next) {
		list = (arg_list_t*)list->next;
	}
	list->next = arg_list_create();
	return (arg_list_t*)list->next;
}

static inline void arg_list_free(arg_list_t* list) {
	free(list->key);
	free(list->value);
	if(list->next) {
		arg_list_free((arg_list_t*)list->next);
	}
	free(list);
}


static inline result_set_t* result_set_create() {
	result_set_t* set = (result_set_t*)malloc(sizeof(result_set_t));
	set->arg = arg_list_create();
	set->next = NULL;
	return set;
}

static inline void result_set_free(result_set_t* set) {
	arg_list_free(set->arg);
	if(set->next) {
		result_set_free((result_set_t*)set->next);
	}
	free(set);
}

static inline result_set_t* result_set_append(result_set_t* set) {
	while(set->next) {
		set = (result_set_t*)set->next;
	}
	set->next = result_set_create();
	return (result_set_t*)set->next;
}
	

// METADATA

static inline void init_metadata(struct metadata_t* metadata)
{
	metadata->type = MD_NONE;

	//metadata->title = (char*)malloc(sizeof(char));
	set_value(metadata->title, "");

	//metadata->publisher = (char*)malloc(sizeof(char));
	set_value(metadata->publisher, "unknown");

  //metadata->date = (char*)malloc(sizeof(char));
	set_value(metadata->date, "");

  //metadata->description = (char*)malloc(sizeof(char));
	set_value(metadata->description, "");

  //metadata->long_description = (char*)malloc(sizeof(char));
	set_value(metadata->long_description, "");
  
  //metadata->genre = (char*)malloc(sizeof(char));
	set_value(metadata->genre, "unknown");

  //metadata->language = (char*)malloc(sizeof(char));
	set_value(metadata->language, "");

  //metadata->artist = (char*)malloc(sizeof(char));
	set_value(metadata->artist, "unknown");

  //metadata->album = (char*)malloc(sizeof(char));
	set_value(metadata->album, "unknown");

  metadata->track_number = 0;

  //metadata->contributor = (char*)malloc(sizeof(char));
	set_value(metadata->contributor, "");

  //metadata->producer = (char*)malloc(sizeof(char));
	set_value(metadata->producer, "");

  //metadata->actors = (char*)malloc(sizeof(char));
	set_value(metadata->actors, "");
  
  //metadata->director = (char*)malloc(sizeof(char));
	set_value(metadata->director, "");

  //metadata->series_title = (char*)malloc(sizeof(char));
	set_value(metadata->series_title, "");

  //metadata->program_title = (char*)malloc(sizeof(char));
	set_value(metadata->program_title, "");
  
  metadata->episode_nr = 0;

  metadata->episode_count = 0;
  

  // res
  metadata->size                    = 0;
  metadata->duration_ms             = 0;
  metadata->nr_audio_channels       = 0;
  metadata->audio_bitrate           = 0;
  metadata->audio_bits_per_sample   = 0;
  metadata->audio_sample_frequency  = 0;
  metadata->video_bitrate           = 0;
  metadata->width                   = 0;
  metadata->height                  = 0;
  metadata->color_depth             = 0;
  
	// other
  metadata->year = 0;

  //metadata->composer = (char*)malloc(sizeof(char));
	set_value(metadata->composer, "unknown");

  //metadata->audio_codec = (char*)malloc(sizeof(char));
	set_value(metadata->audio_codec, "");

  //metadata->video_codec = (char*)malloc(sizeof(char));
	set_value(metadata->video_codec, "");

  metadata->has_image = 0;

  //metadata->image_mime_type = (char*)malloc(sizeof(char));
	set_value(metadata->image_mime_type, "");
}

static inline void free_metadata(struct metadata_t* metadata)
{
  /*
  free(metadata->title);
  free(metadata->publisher);
  free(metadata->date);
  free(metadata->description);
  free(metadata->long_description);
  free(metadata->genre);
  free(metadata->language);
  free(metadata->artist);
  free(metadata->album);
  free(metadata->contributor);
  free(metadata->producer);
  free(metadata->actors);
  free(metadata->director);
  free(metadata->series_title);
  free(metadata->program_title);
  free(metadata->composer);
  free(metadata->audio_codec);
  free(metadata->video_codec);
  free(metadata->image_mime_type);
  */
}


// AUDIO

static inline void init_audio_settings(audio_settings_t* settings)
{
	settings->channels = 0;
	settings->samplerate = 0;	
	settings->bitrate = 0;
	settings->num_samples = 0;
}

static inline void free_audio_settings(audio_settings_t* settings) // __attribute__((unused))
{
}

	
	
#endif //_FUPPES_PLUGIN_H

#ifdef __cplusplus
}
#endif
