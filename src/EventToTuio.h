/// This class is inspired by the Tuio2Simulator from the TUIO2 CPP Refernece inmplementation at
/// https://github.com/mkalten/TUIO20_CPP

#pragma once

#include "TUIO2/TuioServer.h"

using namespace std;
using namespace TUIO2;

class EventToTuio {
  public:
    EventToTuio(std::shared_ptr<TUIO2::TuioServer> server);

    // send frame (if one was initialized) and starts new frame
    void update();
    void touchDown(float x, float y);
    void touchMove(float x, float y);
    void touchUp(float x, float y);

    TuioPointer* FindClosest(std::list<TuioPointer*>& list, float x, float y);

  private:
    TuioTime lastCommitTime;
    bool bFrameStarted = false;
    std::shared_ptr<TUIO2::TuioServer> tuioServerRef;

    // std::list<TuioPointer*> stickyPointerList;
    std::list<TuioPointer*> activePointerList;
};
