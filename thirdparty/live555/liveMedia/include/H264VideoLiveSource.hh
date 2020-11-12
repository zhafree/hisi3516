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
// Framed File Sources
// C++ header

#ifndef _H264_VIDEO_LIVE_SOURCE_HH
#define _H264_VIDEO_LIVE_SOURCE_HH

#ifndef _FRAMED_SOURCE_HH
#include "FramedSource.hh"
#endif

class H264VideoLiveSource: public FramedSource {
public:
  class DeviceParameters {
  public:
    //%%% TO BE WRITTEN %%%
    unsigned sessionId;
    unsigned estBitrate;
    unsigned channelId;
  };

public:
  static H264VideoLiveSource* createNew(UsageEnvironment& env, DeviceParameters params);

protected:
  H264VideoLiveSource(UsageEnvironment& env, DeviceParameters params);
  // called only by createNew(), or by subclass constructors
  virtual ~H264VideoLiveSource();

private:
  virtual void doGetNextFrame();

private:
  static unsigned referenceCount; // used to count how many instances of this class currently exist

private:
  DeviceParameters fDeviceParams;
};

#endif
