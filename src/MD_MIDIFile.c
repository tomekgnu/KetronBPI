/*
  MD_MIDIFile.cpp - An Arduino library for processing Standard MIDI Files (SMF).
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
#include <sys/time.h>
#include "MD_MIDIFile.h"
#include "MD_MIDIHelper.h"
/**
 * \file
 * \brief Main file for the MD_MIDIFile class implementation
 */

void initialise(struct MD_MIDIFile *m,int fd)
{
  m->_trackCount = 0;            // number of tracks in file
  m->_format = 0;
  m->_tickTime = 0;
  m->_lastTickError = 0;
  m->_syncAtStart = FALSE;
  m->_paused = m->_looping = FALSE;
  
  setUartFd(m,fd);
  setMidiHandler(m,NULL);
  setSysexHandler(m,NULL);
  setMetaHandler(m,NULL);

  // File handling
  setFilename(m,"");
  
  // Set MIDI defaults
  setTicksPerQuarterNote(m,48); // 48 ticks per quarter note
  setTempo(m,120);              // 120 beats per minute
  setTempoAdjust(m,0);          // 0 beats per minute adjustment
  setMicrosecondPerQuarterNote(m,500000);  // 500,000 microseconds per quarter note
  setTimeSignature(m,4, 4);     // 4/4 time
}

void setMidiHandler(struct MD_MIDIFile *m,void (*mh)(int fd,midi_event *pev)) {
	m->_midiHandler = mh; 
}

void setUartFd(struct MD_MIDIFile *m,int fd){
	m->_uart = fd;
}
void setMetaHandler(struct MD_MIDIFile *m,void (*mh)(const meta_event *mev)) { 
	m->_metaHandler = mh; 
}

void setSysexHandler(struct MD_MIDIFile *m,void (*sh)(sysex_event *pev)) { 
	m->_sysexHandler = sh; 
}
			
void synchTracks(struct MD_MIDIFile *m)
{
	uint8_t i;
	for (i=0; i<m->_trackCount; i++)
    syncTime(&m->_track[i]);

  m->_lastTickCheckTime = getMicros();
}


void closeMIDIFile(struct MD_MIDIFile *m)
// Close out - should be ready for the next file
{
	uint8_t i;
	for (i = 0; i<m->_trackCount; i++)
  {
    closeTrack(&m->_track[i]);
  }
  m->_trackCount = 0;
  m->_syncAtStart = FALSE;
  m->_paused = FALSE;

  setFilename(m,"");
  fclose(m->_fd);
  m->_fileOpen = FALSE;
}

void setTempoAdjust(struct MD_MIDIFile *m, int16_t t)
{
  if ((t + m->_tempo) > 0) m->_tempoDelta = t;
  calcTickTime(m);
}

void setTempo(struct MD_MIDIFile *m,uint16_t t)
{
  if ((m->_tempoDelta + t) > 0) m->_tempo = t;
  calcTickTime(m);
}

void setTimeSignature(struct MD_MIDIFile *m,uint8_t n, uint8_t d)
{
  m->_timeSignature[0] = n;
  m->_timeSignature[1] = d;
  calcTickTime(m);
}

void setTicksPerQuarterNote(struct MD_MIDIFile *m,uint16_t ticks) 
{
  m->_ticksPerQuarterNote = ticks;
  calcTickTime(m);
}

void setMicrosecondPerQuarterNote(struct MD_MIDIFile *mf,uint32_t m)
// This is the value given in the META message setting tempo
{
  // work out the tempo from the delta by reversing the calcs in
  // calctickTime - m is already per quarter note
  mf->_tempo = (60 * 1000000L) / m;
  calcTickTime(mf);
}

void calcTickTime(struct MD_MIDIFile *m) 
// 1 tick = microseconds per beat / ticks per Q note
// The variable "microseconds per beat" is specified by a MIDI event carrying 
// the set tempo meta message. If it is not specified then it is 500,000 microseconds 
// by default, which is equivalent to 120 beats per minute. 
// If the MIDI time division is 60 ticks per beat and if the microseconds per beat 
// is 500,000, then 1 tick = 500,000 / 60 = 8333.33 microseconds.
{
  if ((m->_tempo + m->_tempoDelta != 0) && m->_ticksPerQuarterNote != 0 && m->_timeSignature[1] != 0)
  {
    m->_tickTime = (60 * 1000000L) / (m->_tempo + m->_tempoDelta); // microseconds per beat
//    _tickTime = (_tickTime * 4) / (_timeSignature[1] * _ticksPerQuarterNote); // microseconds per tick
    m->_tickTime /= m->_ticksPerQuarterNote;
  }
}

BOOL isEOF(struct MD_MIDIFile *m)
{
  BOOL bEof = TRUE;
  uint8_t i;
  // check if each track has finished
  for (i=0; i<m->_trackCount && bEof; i++)
  {
    bEof = (getEndOfTrack(&m->_track[i]) && bEof);  // breaks at first false
  }
  
  if (bEof) DUMPS("\n! EOF");

  // if looping and all tracks done, reset to the start
  if (bEof && m->_looping)
  {
    restart(m);
    bEof = FALSE;
  }

  return(bEof);
}

void pauseMIDIFile(struct MD_MIDIFile *m,BOOL bMode)
// Start pause when true and restart when false
{
  m->_paused = bMode;

  if (!m->_paused)           // restarting so ..
    m->_syncAtStart = FALSE; // .. force a time resynch when next processing events
}

void restart(struct MD_MIDIFile *m)
// Reset the file to the start of all tracks
{
  // track 0 contains information that does not need to be reloaded every time, 
  // so if we are looping, ignore restarting that track. The file may have one 
  // track only and in this case always sync from track 0.
	uint8_t i;
	for (i=(m->_looping && m->_trackCount>1 ? 1 : 0); i<m->_trackCount; i++)
    restartTrack(&m->_track[i]);

  m->_syncAtStart = FALSE;   // force a time resych
}

uint16_t tickClock(struct MD_MIDIFile *m)
// check if enough time has passed for a MIDI tick and work out how many!
{
  uint32_t  elapsedTime;
  uint16_t  ticks = 0;
  uint32_t uc = getMicros();   
  elapsedTime = m->_lastTickError + uc - m->_lastTickCheckTime;
   
  if (elapsedTime >= m->_tickTime)
  {
    ticks = elapsedTime/m->_tickTime;
    m->_lastTickError = elapsedTime - (m->_tickTime * ticks);
    m->_lastTickCheckTime = getMicros();    // save for next round of checks
  }
	
  return(ticks);
}

BOOL getNextEvent(struct MD_MIDIFile *m)
{
  uint16_t  ticks;

  // if we are paused we are paused!
  if (m->_paused) 
    return FALSE;

  // sync start all the tracks if we need to
  if (!m->_syncAtStart)
  {
    synchTracks(m);
    m->_syncAtStart = TRUE;
  }

  // check if enough time has passed for a MIDI tick
  if ((ticks = tickClock(m)) == 0)
    return FALSE;

  processEvents(m,ticks);

  return(TRUE);
}

void processEvents(struct MD_MIDIFile *m,uint16_t ticks)
{
  uint8_t n;

  if (m->_format != 0) 
  {
    DUMP("\n-- [", ticks); 
    DUMPS("] TRK "); 
  }

#if TRACK_PRIORITY
  // process all events from each track first - TRACK PRIORITY
  for (uint8_t i = 0; i < m->_trackCount; i++)
  {
    if (m->_format != 0) DUMPX("", i);
    // Limit n to be a sensible number of events in the loop counter
    // When there are no more events, just break out
    // Other than the first event, the other have an elapsed time of 0 (ie occur simultaneously)
    for (n=0; n < 100; n++)
    {
      if (!getNextTrackEvent(m,&m->_track[i], (n==0 ? ticks : 0)))
        break;
    }

    if ((n > 0) && (m->_format != 0))
      DUMPS("\n-- TRK "); 
  }
#else // EVENT_PRIORITY
  // process one event from each track round-robin style - EVENT PRIORITY
  BOOL doneEvents;

  // Limit n to be a sensible number of events in the loop counter
  for (n = 0; (n < 100) && (!doneEvents); n++)
  {
    doneEvents = FALSE;
    uint8_t i;
    for (i = 0; i < m->_trackCount; i++) // cycle through all
    {
      BOOL b;

      if (m->_format != 0) DUMPX("", i);

      // Other than the first event, the other have an elapsed time of 0 (ie occur simultaneously)
      b = getNextTrackEvent(m,&m->_track[i], (n==0 ? ticks : 0));
      if (b && (m->_format != 0))
        DUMPS("\n-- TRK "); 
      doneEvents = (doneEvents || b);
    }

    // When there are no more events, just break out
    if (!doneEvents)
      break;
  } 
#endif // EVENT/TRACK_PRIORITY
}

int loadMIDIFile(struct MD_MIDIFile *m) 
// Load the MIDI file into memory ready for processing
{
  uint32_t dat32;
  uint16_t dat16;
  uint8_t i;
  char    h[MTHD_HDR_SIZE+1]; // Header characters + nul
  
  if (m->_fileName[0] == '\0')  
    return(0);

  // open the file for reading:
  if((m->_fd = fopen(m->_fileName,"r")) == NULL)
    return(2);

  // Read the MIDI header
  // header chunk = "MThd" + <header_length:4> + <format:2> + <num_tracks:2> + <time_division:2>
    
  	  fread(&dat32,MTHD_HDR_SIZE,1,m->_fd);
    //f_read(&m->_fd,h,MTHD_HDR_SIZE,(UINT *)&dat32);
	
    h[MTHD_HDR_SIZE] = '\0';

    if (strcmp(h, MTHD_HDR) != 0)
    {
      fclose(m->_fd);
      return(3);
    }  

  // read header size
  dat32 = readMultiByte(m->_fd, MB_LONG);
  if (dat32 != 6)   // must be 6 for this header
  {
	fclose(m->_fd);
    return(4);
  }
  
  // read file type
  dat16 = readMultiByte(m->_fd, MB_WORD);
  if ((dat16 != 0) && (dat16 != 1))
  {
    fclose(m->_fd);
    return(5);
  }
  m->_format = dat16;
 
   // read number of tracks
  dat16 = readMultiByte(m->_fd, MB_WORD);
  if ((m->_format == 0) && (dat16 != 1)) 
  {
    fclose(m->_fd);
    return(6);
  }
  if (dat16 > MIDI_MAX_TRACKS)
  {
    fclose(m->_fd);
    return(7);
  }
  m->_trackCount = dat16;

   // read ticks per quarter note
  dat16 = readMultiByte(m->_fd, MB_WORD);
  if (dat16 & 0x8000) // top bit set is SMTE format
  {
    int framespersecond = (dat16 >> 8) & 0x00ff;
    int resolution      = dat16 & 0x00ff;

    switch (framespersecond) 
    {
      case 232:  framespersecond = 24; break;
      case 231:  framespersecond = 25; break;
      case 227:  framespersecond = 29; break;
      case 226:  framespersecond = 30; break;
      default:   fclose(m->_fd); return(7);
    }
    dat16 = framespersecond * resolution;
  } 
  m->_ticksPerQuarterNote = dat16;
  calcTickTime(m);  // we may have changed from default, so recalculate

  // load all tracks
  for (i = 0; i<m->_trackCount; i++)
  {
    int err;

    if ((err = loadTrack(&m->_track[i],i,m )) != -1)
    {
      fclose(m->_fd);
      return((10*(i+1))+err);
    }
   }

  return(-1);
}


const char * getFilename(struct MD_MIDIFile *m) 
{ 
	return(m->_fileName); 
}

void setFilename(struct MD_MIDIFile *m,const char* aname) 
{ 
	if (aname != NULL) strcpy(m->_fileName, aname); 
}	

inline uint32_t getMicros(){
	struct timeval tv;
	struct timezone tz;
	gettimeofday(&tv, &tz);

	return tv.tv_usec;
}

inline uint32_t getTickTime(struct MD_MIDIFile *m) { return (m->_tickTime); }
inline uint16_t getTempo(struct MD_MIDIFile *m) { return(m->_tempo); }
inline int16_t getTempoAdjust(struct MD_MIDIFile *m) { return(m->_tempoDelta); }
inline uint16_t getTicksPerQuarterNote(struct MD_MIDIFile *m) { return(m->_ticksPerQuarterNote); }
inline uint16_t getTimeSignature(struct MD_MIDIFile *m) { return((m->_timeSignature[0]<<8) + m->_timeSignature[1]); };
inline uint8_t getFormat(struct MD_MIDIFile *m) { return(m->_format); }
inline uint8_t getTrackCount(struct MD_MIDIFile *m) { return (m->_trackCount); };
inline void looping(struct MD_MIDIFile *m, BOOL bMode) { m->_looping = bMode; };


#if DUMP_DATA
void dumpFile(struct MD_MIDIFile *m)
{
  DUMP("\nFile Name:\t", getFilename(m)); getFilename
  DUMP("\nFile format:\t", getFormat(m));
  DUMP("\nTracks:\t\t", getTrackCount(m));
  DUMP("\nTime division:\t", getTicksPerQuarterNote(m));
  DUMP("\nTempo:\t\t", getTempo(m));
  DUMP("\nMicrosec/tick:\t", getTickTime(m));
  DUMP("\nTime Signature:\t", getTimeSignature(m)>>8);
  DUMP("/", getTimeSignature(m) & 0xf);
  DUMPS("\n");
 
  for (uint8_t i=0; i<m->_trackCount; i++)
  {
    //dumpTrack(m->_track[i])
    DUMPS("\n");
  }
}
#endif // DUMP_DATA
