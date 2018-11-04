/// This class is inspired by the Tuio2Simulator from the TUIO2 CPP Refernece inmplementation at
/// https://github.com/mkalten/TUIO20_CPP

#pragma once

#include "TUIO/TuioServer.h"

using namespace std;

class EventToTuio {
  public:
    EventToTuio(std::shared_ptr<TUIO::TuioServer> server);

    /// update, when called sends all touch data collected
    /// since the last call to update.
    void update();

    // touch event input methods
    void touchDown(float x, float y);
    void touchMove(float x, float y);
    void touchUp(float x, float y);

  protected:
    TUIO::TuioCursor* FindClosest(std::list<TUIO::TuioCursor*>& list, float x, float y);

  private:
    bool bFrameStarted = false;
    std::shared_ptr<TUIO::TuioServer> tuioServerRef;
    std::list<TUIO::TuioCursor*> activePointerList;
};
