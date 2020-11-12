/**********
This library is free software; you can redistribute it and/or modify it under
the terms of the GNU Lesser General Public License as published by the
Free Software Foundation; either version 3 of the License, or (at your
option) any later version. (See <http://www.gnu.org/copyleft/lesser.html>.)

This library is distributed in the hope that it will be useful, but WITHOUT
ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
FOR A PARTICULAR PURPOSE.  See the GNU Lesser General Public License for
more details.

You should have received a copy of the GNU Lesser General Public License
along with this library; if not, write to the Free Software Foundation, Inc.,
51 Franklin Street, Fifth Floor, Boston, MA 02110-1301  USA
**********/
// "liveMedia"
// Copyright (c) 1996-2019 Live Networks, Inc.  All rights reserved.
// A source object for AAC audio files in ADTS format
// Implementation

#include "ADTSAudioLiveSource.hh"
#include "InputFile.hh"
#include "himpp/include/sample_comm.h"
#include <GroupsockHelper.hh>

////////// ADTSAudioLiveSource //////////

static unsigned const samplingFrequencyTable[16] = {
  96000, 88200, 64000, 48000,
  44100, 32000, 24000, 22050,
  16000, 12000, 11025, 8000,
  7350, 0, 0, 0
};

unsigned ADTSAudioLiveSource::referenceCount = 0;

ADTSAudioLiveSource*
ADTSAudioLiveSource
::createNew(UsageEnvironment& env, DeviceParameters deviceParams) {
  return new ADTSAudioLiveSource(env, deviceParams);
}

ADTSAudioLiveSource
::ADTSAudioLiveSource(UsageEnvironment& env, DeviceParameters deviceParams)
  : FramedSource(env) {

  u_int8_t samplingFrequencyIndex = 0;
  while (samplingFrequencyTable[samplingFrequencyIndex] != 0
    && samplingFrequencyTable[samplingFrequencyIndex] != deviceParams.samplingFrequency) {
      ++samplingFrequencyIndex;
  }
  ADTSAudioLiveSource(env, 0, samplingFrequencyIndex, 0);
}

ADTSAudioLiveSource
::ADTSAudioLiveSource(UsageEnvironment& env, u_int8_t profile,
		      u_int8_t samplingFrequencyIndex, u_int8_t channelConfiguration)
  : FramedSource(env) {
  if (referenceCount == 0) {
    // Any global initialization of the device would be done here:
    if (HI_SUCCESS != SAMPLE_AENC_Init((hiAUDIO_SAMPLE_RATE_E)fSamplingFrequency, fNumChannels)) {
      SAMPLE_PRT("Start Venc failed!\n");
      SAMPLE_VENC_DeInit();
    } else {
      SAMPLE_PRT("Start Venc success!\n");
    }
  }
  ++referenceCount;

  fSamplingFrequency = samplingFrequencyTable[samplingFrequencyIndex];
  fNumChannels = channelConfiguration == 0 ? 2 : channelConfiguration;
  fuSecsPerFrame = (1024/*samples-per-frame*/*1000000) / fSamplingFrequency/*samples-per-second*/;

  // Construct the 'AudioSpecificConfig', and from it, the corresponding ASCII string:
  unsigned char audioSpecificConfig[2];
  u_int8_t const audioObjectType = profile + 1;
  audioSpecificConfig[0] = (audioObjectType<<3) | (samplingFrequencyIndex>>1);
  audioSpecificConfig[1] = (samplingFrequencyIndex<<7) | (channelConfiguration<<3);
  sprintf(fConfigStr, "%02X%02x", audioSpecificConfig[0], audioSpecificConfig[1]);
}

ADTSAudioLiveSource::~ADTSAudioLiveSource() {
  --referenceCount;
  if (referenceCount == 0) {
    SAMPLE_AENC_DeInit();
  }
}

// Note: We should change the following to use asynchronous file reading, #####
// as we now do with ByteStreamFileSource. #####
void ADTSAudioLiveSource::doGetNextFrame() {
  // No new data is immediately available to be delivered.  We don't do anything more here.
  // Instead, our event trigger must be called (e.g., from a separate thread) when new data becomes available.
  AUDIO_STREAM_S stStream;
  if (HI_SUCCESS != SAMPLE_AENC_PeekStream(&stStream)) {
    return;
  }

  /*******************************************************
   step 2.5 : save frame
  *******************************************************/
  unsigned newFrameSize = stStream.u32Len;
  printf("newFrameSize = %u, fMaxSize = %u\n", newFrameSize, fMaxSize);
  if (newFrameSize> fMaxSize) {
    fFrameSize = fMaxSize;
    fNumTruncatedBytes = newFrameSize - fMaxSize;
  } else {
    fFrameSize = newFrameSize;
  }

  memmove(fTo, stStream.pStream, newFrameSize);
  SAMPLE_AENC_ReleaseStream(&stStream);

  // Begin by reading the 7-byte fixed_variable headers:

  // Set the 'presentation time':
  if (fPresentationTime.tv_sec == 0 && fPresentationTime.tv_usec == 0) {
    // This is the first frame, so use the current time:
    gettimeofday(&fPresentationTime, NULL);
  } else {
    // Increment by the play time of the previous frame:
    unsigned uSeconds = fPresentationTime.tv_usec + fuSecsPerFrame;
    fPresentationTime.tv_sec += uSeconds/1000000;
    fPresentationTime.tv_usec = uSeconds%1000000;
  }

  fDurationInMicroseconds = fuSecsPerFrame;

  // Switch to another task, and inform the reader that he has data:
  nextTask() = envir().taskScheduler().scheduleDelayedTask(0,
				(TaskFunc*)FramedSource::afterGetting, this);
}
