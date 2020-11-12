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
// H264 Framed Device Sources
// Implementation

#include "H264VideoLiveSource.hh"
#include "himpp/include/sample_comm.h"

#include <stdlib.h>

#define VENC_CHN_NUM  3

static PIC_SIZE_E gstEnSize[VENC_MAX_CHN_NUM] = {
  PIC_1080P, PIC_720P, PIC_360P};

unsigned H264VideoLiveSource::referenceCount = 0;

H264VideoLiveSource*
H264VideoLiveSource::createNew(UsageEnvironment& env,
			DeviceParameters params) {
  return new H264VideoLiveSource(env, params);
}

H264VideoLiveSource::H264VideoLiveSource(UsageEnvironment& env, DeviceParameters deviceParams)
  : FramedSource(env), fDeviceParams(deviceParams) {
  if (referenceCount == 0) {
    // Any global initialization of the device would be done here:
    //%%% TO BE WRITTEN %%%
    if (HI_SUCCESS != SAMPLE_VENC_Init(gstEnSize, VENC_CHN_NUM)) {
      SAMPLE_PRT("Start Venc failed!\n");
      SAMPLE_VENC_DeInit();
    } else {
      SAMPLE_PRT("Start Venc success!\n");
    }
  }
  ++referenceCount;

  // Any instance-specific initialization of the device would be done here:
  //%%% TO BE WRITTEN %%%

  // We arrange here for our "deliverFrame" member function to be called
  // whenever the next frame of data becomes available from the device.
  //
  // If the device can be accessed as a readable socket, then one easy way to do this is using a call to
  //     envir().taskScheduler().turnOnBackgroundReadHandling( ... )
  // (See examples of this call in the "liveMedia" directory.)
  //
  // If, however, the device *cannot* be accessed as a readable socket, then instead we can implement it using 'event triggers':
  // Create an 'event trigger' for this device (if it hasn't already been done):
}

H264VideoLiveSource::~H264VideoLiveSource() {
  // Any instance-specific 'destruction' (i.e., resetting) of the device would be done here:
  //%%% TO BE WRITTEN %%%

  --referenceCount;
  if (referenceCount == 0) {
    // Any global 'destruction' (i.e., resetting) of the device would be done here:
    //%%% TO BE WRITTEN %%%
    SAMPLE_VENC_DeInit();
    // Reclaim our 'event trigger'
  }
}

void H264VideoLiveSource::doGetNextFrame() {
  // This function is called (by our 'downstream' object) when it asks for new data.

  // No new data is immediately available to be delivered.  We don't do anything more here.
  // Instead, our event trigger must be called (e.g., from a separate thread) when new data becomes available.
  VENC_STREAM_S stStream;
  if (HI_SUCCESS != SAMPLE_VENC_PeekStream(fDeviceParams.channelId, &stStream)) {
    return;
  }

  /*******************************************************
   step 2.5 : save frame
  *******************************************************/
  unsigned newFrameSize = 0;
  for (int i = 0; i < stStream.u32PackCount; ++i) {
    newFrameSize += stStream.pstPack[i].u32Len - stStream.pstPack[i].u32Offset;
  }

  //printf("newFrameSize = %u, fMaxSize = %u\n", newFrameSize, fMaxSize);
  if (newFrameSize> fMaxSize) {
    fFrameSize = fMaxSize;
    fNumTruncatedBytes = newFrameSize - fMaxSize;
  } else {
    fFrameSize = newFrameSize;
  }

  unsigned offset = 0;
  for (int i = 0; i < stStream.u32PackCount; ++i) {
    if (offset + stStream.pstPack[i].u32Len - stStream.pstPack[i].u32Offset < fFrameSize) {
      memmove(fTo + offset, stStream.pstPack[i].pu8Addr + stStream.pstPack[i].u32Offset,
        stStream.pstPack[i].u32Len - stStream.pstPack[i].u32Offset);
      offset += stStream.pstPack[i].u32Len - stStream.pstPack[i].u32Offset;
    } else {
      memmove(fTo + offset, stStream.pstPack[i].pu8Addr + stStream.pstPack[i].u32Offset,
        fFrameSize - offset);
      break;
    }
  }
  SAMPLE_VENC_ReleaseStream(fDeviceParams.channelId, &stStream);

  afterGetting(this);
}
