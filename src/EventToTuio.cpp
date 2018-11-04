/// This class is inspired by the TUIOSimulator from the TUIO CPP Refernece inmplementation at
/// https://github.com/mkalten/TUIO11_CPP

#include "EventToTuio.h"

using namespace std;
using namespace TUIO;

EventToTuio::EventToTuio(std::shared_ptr<TUIO::TuioServer> server) : tuioServerRef(server), bFrameStarted(false) {
  lastCommitTime = TuioTime::getSystemTime();
}


void EventToTuio::update() {

  if (this->bFrameStarted) {
    this->bFrameStarted = false;
    // tuioServerRef->stopUntouchedMovingCursors();
    tuioServerRef->commitFrame(); // todo; configurable interval?
    lastCommitTime = TuioTime::getSystemTime();
  }

  // start next frame
  tuioServerRef->initFrame(TuioTime::getSystemTime());
  this->bFrameStarted = true;
}

void EventToTuio::touchDown(float x, float y) {
  // auto match = FindClosest(stickyPointerList, x, y);
  //
  // if (match==NULL) {
    auto cursor = this->tuioServerRef->addTuioCursor(x,y);
    activePointerList.push_back(cursor);
  // } else {
    // activePointerList.push_back(match);
  // }
}

void EventToTuio::touchMove(float x, float y) {
  auto pointer = FindClosest(activePointerList, x, y);

  if (pointer==NULL) return;
  if (pointer->getTuioTime()==this->tuioServerRef->getFrameTime()) return;
  this->tuioServerRef->updateTuioCursor(pointer,x,y);
}

void EventToTuio::touchUp(float x, float y) {
  auto pointer = FindClosest(activePointerList, x, y);

  if (pointer != NULL) {
    activePointerList.remove(pointer);
    this->tuioServerRef->removeTuioCursor(pointer);
  }
}

TuioCursor* EventToTuio::FindClosest(std::list<TuioCursor*>& list, float x, float y) {
  TuioCursor *match = NULL;
  float distance;

  for (std::list<TuioCursor*>::iterator iter = list.begin(); iter!=list.end(); iter++) {
    TuioCursor *tptr = (*iter);
    float test = tptr->getDistance(x,y);
    if (match == NULL || test < distance) {
      distance = test;
      match = tptr;
    }
  }

  return match;
}
