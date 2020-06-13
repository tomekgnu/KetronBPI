/*
  MD_MIDITrack.cpp - An Arduino library for processing Standard MIDI Files (SMF).
  Copyright (C) 2012 Marco Colli
  All rights reserved.

  See MD_MIDIFile.h for complete comments

  This library is free software; you can redistribute it and/or
  modify it under the terms of the GNU Lesser General Public
  License as published by the Free Software Foundation; either
  version 2.1 of the License, or (at your option) any later version.

  This library is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
  Lesser General Public License for more details.

  You should have received a copy of the GNU Lesser General Public
  License along with this library; if not, write to the Free Software
  Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
*/
#include <string.h>
#include "MD_MIDIFile.h"
#include "MD_MIDIHelper.h"

/**
 * \file
 * \brief Main file for the MFTrack class implementation
 */

void resetTrack(struct MD_MFTrack *t)
{
  t->_length = 0;        // length of track in bytes
  t->_startOffset = 0;   // start of the track in bytes from start of file
  restartTrack(t);
  t->_trackId = 255;
}


void closeTrack(struct MD_MFTrack *t)
{
  resetTrack(t);
}

uint32_t getLength(struct MD_MFTrack *t)
// size of track in bytes
{
  return t->_length;
}

BOOL getEndOfTrack(struct MD_MFTrack *t)
// true if end of track has been reached
{
  return t->_endOfTrack;
}

void syncTime(struct MD_MFTrack *t)
{
  t->_elapsedTicks = 0;
}

void restartTrack(struct MD_MFTrack *t)
// Start playing the track from the beginning again
{
  t->_currOffset = 0;
  t->_endOfTrack = FALSE;
  t->_elapsedTicks = 0;
}

BOOL getNextTrackEvent(struct MD_MIDIFile *mf,struct MD_MFTrack *t, uint16_t tickCount)
// track_event = <time:v> + [<midi_event> | <meta_event> | <sysex_event>]
{
  uint32_t deltaT;

  // is there anything to process?
  if (t->_endOfTrack)
    return(FALSE);

  // move the file pointer to where we left off
  fseek(mf->_fd,t->_startOffset+t->_currOffset,SEEK_SET);

  // Work out new total elapsed ticks - include the overshoot from
  // last event.
  t->_elapsedTicks += tickCount;

  // Get the DeltaT from the file in order to see if enough ticks have
  // passed for the event to be active.
  deltaT = readVarLen(mf->_fd);

  // If not enough ticks, just return without saving the file pointer and 
  // we will go back to the same spot next time.
  if (t->_elapsedTicks < deltaT)
    return(FALSE);

  // Adjust the total elapsed time to the error against actual DeltaT to avoid 
  // accumulation of errors, as we only check for _elapsedTicks being >= ticks,
  // giving positive biased errors every time.
  t->_elapsedTicks -= deltaT;

  DUMP("\ndT: ", deltaT);
  DUMP(" + ", _elapsedTicks);
  DUMPS("\t");

  parseEvent(mf,t);

  // remember the offset for next time
  t->_currOffset = ftell(mf->_fd) - t->_startOffset;

  // catch end of track when there is no META event  
  t->_endOfTrack = t->_endOfTrack || (t->_currOffset >= t->_length);
  if (t->_endOfTrack) DUMPS(" - OUT OF TRACK");

  return(TRUE);
}

void parseEvent(struct MD_MIDIFile *mf,struct MD_MFTrack *t)
// process the event from the physical file
{
  uint8_t eType;
  uint8_t bVal;
  uint32_t mLen;
  uint8_t i;
  // now we have to process this event
  fread(&eType,1,1,mf->_fd);

  switch (eType)
  {
// ---------------------------- MIDI
    // midi_event = any MIDI channel message, including running status
    // Midi events (status bytes 0x8n - 0xEn) The standard Channel MIDI messages, where 'n' is the MIDI channel (0 - 15).
    // This status byte will be followed by 1 or 2 data bytes, as is usual for the particular MIDI message. 
    // Any valid Channel MIDI message can be included in a MIDI file.
  case 0x80 ... 0xBf: // MIDI message with 2 parameters
  case 0xe0 ... 0xef:
    t->_mev.size = 3;
    t->_mev.data[0] = eType;
    t->_mev.channel = t->_mev.data[0] & 0xf;  // mask off the channel
    t->_mev.data[0] = t->_mev.data[0] & 0xf0; // just the command byte
    fread(&t->_mev.data[1],1,1,mf->_fd);
    fread(&t->_mev.data[2],1,1,mf->_fd);
    DUMP("[MID2] Ch: ", _mev.channel);
    DUMPX(" Data: ", _mev.data[0]);
    DUMPX(" ", _mev.data[1]);
    DUMPX(" ", _mev.data[2]);	
#if !DUMP_DATA
    if (mf->_midiHandler != NULL)
      (mf->_midiHandler)(mf->_uart,&t->_mev);
#endif // !DUMP_DATA
  break;

  case 0xc0 ... 0xdf: // MIDI message with 1 parameter
    t->_mev.size = 2;
    t->_mev.data[0] = eType;
    t->_mev.channel = t->_mev.data[0] & 0xf;  // mask off the channel
    t->_mev.data[0] = t->_mev.data[0] & 0xf0; // just the command byte
    fread(&t->_mev.data[1],1,1,mf->_fd);
    DUMP("[MID1] Ch: ", _mev.channel);
    DUMPX(" Data: ", _mev.data[0]);
    DUMPX(" ", _mev.data[1]);

#if !DUMP_DATA
    if (mf->_midiHandler != NULL)
      (mf->_midiHandler)(mf->_uart,&t->_mev);
#endif
  break;

  case 0x00 ... 0x7f: // MIDI run on message
  {
    // If the first (status) byte is less than 128 (0x80), this implies that MIDI 
    // running status is in effect, and that this byte is actually the first data byte 
    // (the status carrying over from the previous MIDI event). 
    // This can only be the case if the immediately previous event was also a MIDI event, 
    // ie SysEx and Meta events clear running status. This means that the _mev structure 
    // should contain the info from the previous message in the structure's channel member 
    // and data[0] (for the MIDI command). 
    // Hence start saving the data at byte data[1] with the byte we have just read (eType) 
    // and use the size member to determine how large the message is (ie, same as before).
    t->_mev.data[1] = eType;
    for (i = 2; i < t->_mev.size; i++)
    {
      fread(&t->_mev.data[i],1,1,mf->_fd);  // next byte
    } 

    DUMP("[MID+] Ch: ", _mev.channel);
    DUMPS(" Data:");
    for (i = 0; i<t->_mev.size; i++)
    {
      DUMPX(" ", t->_mev.data[i]);
    }

#if !DUMP_DATA
    if (mf->_midiHandler != NULL)
      (mf->_midiHandler)(mf->_uart,&t->_mev);
#endif
  }
  break;

// ---------------------------- SYSEX
  case 0xf0:  // sysex_event = 0xF0 + <len:1> + <data_bytes> + 0xF7 
  case 0xf7:  // sysex_event = 0xF7 + <len:1> + <data_bytes> + 0xF7 
  {
    sysex_event sev;
    uint16_t index = 0;

    // collect all the bytes until the 0xf7 - boundaries are included in the message
    sev.track = t->_trackId;
    mLen = readVarLen(mf->_fd);
    sev.size = mLen;
    if (eType==0xF0)       // add space for 0xF0
    {
      sev.data[index++] = eType;
      sev.size++;
    }
    uint16_t minLen = MIN(sev.size, ARRAY_SIZE(sev.data));
    // The length parameter includes the 0xF7 but not the start boundary.
    // However, it may be bigger than our buffer will allow us to store.
    for (i=index; i<minLen; ++i)
      fread(&sev.data[i],1,1,mf->_fd);
    if (sev.size>minLen)
      fseek(mf->_fd,ftell(mf->_fd) + (sev.size-minLen),SEEK_SET);

#if DUMP_DATA
    DUMPS("[SYSX] Data:");
    for (uint16_t i = 0; i<minLen; i++)
    {
      DUMPX(" ", sev.data[i]);
    }
    if (sev.size>minLen)
      DUMPS("...");
#else
    if (mf->_sysexHandler != NULL)
      (mf->_sysexHandler)(&sev);
#endif
  }
  break;

// ---------------------------- META
  case 0xff:  // meta_event = 0xFF + <meta_type:1> + <length:v> + <event_data_bytes>
  {
    meta_event mev;
    
	fread(&eType,1,1,mf->_fd);
    mLen =  readVarLen(mf->_fd);

    mev.track = t->_trackId;
    mev.size = mLen;
    mev.type = eType;

    //DUMPX("[META] Type: 0x", eType);
    //DUMP("\tLen: ", mLen);
   // DUMPS("\t");

    switch (eType)
    {
      case 0x2f:  // End of track
      {
        t->_endOfTrack = TRUE;
        //DUMPS("END OF TRACK");
      }
      break;

      case 0x51:  // set Tempo - really the microseconds per tick
      {
        uint32_t value = readMultiByte(mf->_fd, MB_TRYTE);
        
        setMicrosecondPerQuarterNote(mf,value);
        
        mev.data[0] = (value >> 16) & 0xFF;
        mev.data[1] = (value >> 8) & 0xFF;
        mev.data[2] = value & 0xFF;
        
        //DUMP("SET TEMPO to ", getTickTime(mf));
        //DUMP(" us/tick or ", getTempo(mf));
        //DUMPS(" beats/min");
      }
      break;

      case 0x58:  // time signature
      {
        uint8_t n,d;
		fread(&n,1,1,mf->_fd);
        fread(&d,1,1,mf->_fd);
        
        setTimeSignature(mf,n, 1 << d);  // denominator is 2^n
        fseek(mf->_fd,ftell(mf->_fd) + (mLen - 2),SEEK_SET);

        mev.data[0] = n;
        mev.data[1] = d;
        mev.data[2] = 0;
        mev.data[3] = 0;

        //DUMP("SET TIME SIGNATURE to ", getTimeSignature(mf) >> 8);
        //DUMP("/", getTimeSignature(mf) & 0xf);
      }
      break;

      case 0x59:  // Key Signature
      {
        int8_t sf,mi;
		//DUMPS("KEY SIGNATURE");
        fread(&sf,1,1,mf->_fd);
        fread(&mi,1,1,mf->_fd);
        const char* aaa[] = {"Cb", "Gb", "Db", "Ab", "Eb", "Bb", "F", "C", "G", "D", "A", "E", "B", "F#", "C#", "G#", "D#", "A#"};

        if (sf >= -7 && sf <= 7) 
        {
          switch(mi)
          {
            case 0:
              strcpy(mev.chars, aaa[sf+7]);
              strcat(mev.chars, "M");
              break;
            case 1:
              strcpy(mev.chars, aaa[sf+10]);
              strcat(mev.chars, "m");
              break;
            default:
              strcpy(mev.chars, "Err"); // error mi
          }
        } else
          strcpy(mev.chars, "Err"); // error sf

        mev.size = strlen(mev.chars); // change META length
        //DUMP(" ", mev.chars);
      }
      break;

      case 0x00:  // Sequence Number
      {
        uint16_t x = readMultiByte(mf->_fd, MB_WORD);

        mev.data[0] = (x >> 8) & 0xFF;
        mev.data[1] = x & 0xFF;

        //DUMP("SEQUENCE NUMBER ", mev.data[0]);
        //DUMP(" ", mev.data[1]);
      }
      break;

      case 0x20:  // Channel Prefix
      mev.data[0] = readMultiByte(mf->_fd, MB_BYTE);
      //DUMP("CHANNEL PREFIX ", mev.data[0]);
      break;

      case 0x21:  // Port Prefix
      mev.data[0] = readMultiByte(mf->_fd, MB_BYTE);
      //DUMP("PORT PREFIX ", mev.data[0]);
      break;

#if SHOW_UNUSED_META
      case 0x01:  // Text
      //DUMPS("TEXT ");
      for (i=0; i<mLen; i++)
        fread(&bVal,1,1,mf->_fd);
      break;

      case 0x02:  // Copyright Notice
      //DUMPS("COPYRIGHT ");
      for (i=0; i<mLen; i++)
    	  fread(&bVal,1,1,mf->_fd);
      break;

      case 0x03:  // Sequence or Track Name
      //DUMPS("SEQ/TRK NAME ");
      for (i=0; i<mLen; i++)
        	fread(&bVal,1,1,mf->_fd);
      break;

      case 0x04:  // Instrument Name
      //DUMPS("INSTRUMENT ");
      for (i=0; i<mLen; i++)
        	fread(&bVal,1,1,mf->_fd);
      break;

      case 0x05:  // Lyric
      //DUMPS("LYRIC ");
      for (i=0; i<mLen; i++)
       	fread(&bVal,1,1,mf->_fd);
      break;

      case 0x06:  // Marker
      //DUMPS("MARKER ");
      for (i=0; i<mLen; i++)
        	fread(&bVal,1,1,mf->_fd);
      break;

      case 0x07:  // Cue Point
      //DUMPS("CUE POINT ");
      for (i=0; i<mLen; i++)
        	fread(&bVal,1,1,mf->_fd);
      break;

      case 0x54:  // SMPTE Offset
      //DUMPS("SMPTE OFFSET");
      for (i=0; i<mLen; i++)
      {
        	fread(&bVal,1,1,mf->_fd);
      }
      break;

      case 0x7F:  // Sequencer Specific Metadata
      //DUMPS("SEQ SPECIFIC");
      for (i=0; i<mLen; i++)
      {
        	fread(&bVal,1,1,mf->_fd);
      }
      break;
#endif // SHOW_UNUSED_META

      default:
      {
        uint8_t minLen = MIN(ARRAY_SIZE(mev.data), mLen);
        
        for (i = 0; i < minLen; ++i)
          	fread(&mev.data[i],1,1,mf->_fd); // read next
		 		  
        mev.chars[minLen] = '\0'; // in case it is a string
        if (mLen > ARRAY_SIZE(mev.data))
          fseek(mf->_fd,ftell(mf->_fd) + (mLen-ARRAY_SIZE(mev.data)),SEEK_SET);
  //    DUMPS("IGNORED");
      }
      break;
    }
    if (mf->_metaHandler != NULL)
      (mf->_metaHandler)(&mev);
  }
  break;
  
// ---------------------------- UNKNOWN
  default:
    // stop playing this track as we cannot identify the eType
    t->_endOfTrack = TRUE;
    DUMPX("[UKNOWN 0x", eType);
    DUMPS("] Track aborted");
    break;
  }
}

int loadTrack(struct MD_MFTrack *t,uint8_t trackId, struct MD_MIDIFile *mf)
{
  uint32_t  dat32;
  //uint16_t  dat16;

  // save the trackid for use later
  t->_trackId = t->_mev.track = trackId;
  
  // Read the Track header
  // track_chunk = "MTrk" + <length:4> + <track_event> [+ <track_event> ...]
  {
    char    h[MTRK_HDR_SIZE+1]; // Header characters + nul
    dat32 = fread(h,MTRK_HDR_SIZE,1,mf->_fd);
    h[MTRK_HDR_SIZE] = '\0';

    if (strcmp(h, MTRK_HDR) != 0)
      return(0);
  }

  // Row read track chunk size and in bytes. This is not really necessary 
  // since the track MUST end with an end of track meta event.
  dat32 = readMultiByte(mf->_fd, MB_LONG);
  t->_length = dat32;

  // save where we are in the file as this is the start of offset for this track
  t->_startOffset = ftell(mf->_fd);
  t->_currOffset = 0;

  // Advance the file pointer to the start of the next track;
  if (fseek(mf->_fd,(t->_startOffset+t->_length),SEEK_SET) == -1)
    return(1);

  return(-1);
}

#if DUMP_DATA
void dumpTrack(struct MD_MFTrack *t)
{
  DUMP("\n[Track ", t->_trackId);
  DUMPS(" Header]");
  DUMP("\nLength:\t\t\t", t->_length);
  DUMP("\nFile Location:\t\t", t->_startOffset);
  DUMP("\nEnd of Track:\t\t", t->_endOfTrack);
  DUMP("\nCurrent buffer offset:\t", t->_currOffset);
}
#endif // DUMP_DATA

