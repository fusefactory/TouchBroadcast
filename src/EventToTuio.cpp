/// This class is inspired by the Tuio2Simulator from the TUIO2 CPP Refernece inmplementation at
/// https://github.com/mkalten/TUIO20_CPP

#include "EventToTuio.h"

using namespace std;
using namespace TUIO2;

EventToTuio::EventToTuio(std::shared_ptr<TUIO2::TuioServer> server) : tuioServerRef(server), bFrameStarted(false) {
  lastCommitTime = TuioTime::getSystemTime();
}

void EventToTuio::prep() {
  // if (this->tuioServerRef->getFrameTime() < lastCommitTime) {
  //   tuioServerRef->initTuioFrame(TuioTime::getSystemTime());
  // }
}

void EventToTuio::update() {

  if (this->bFrameStarted) {
    this->bFrameStarted = false;
    std::cout << "EventToTuio::update - commit" << std::endl;
    // tuioServerRef->stopUntouchedMovingObjects();
    tuioServerRef->commitTuioFrame(); // todo; configurable interval?
    lastCommitTime = TuioTime::getSystemTime();
  }

  // start next frame
  tuioServerRef->initTuioFrame(TuioTime::getSystemTime());
  this->bFrameStarted = true;
}

void EventToTuio::touchDown(float x, float y) {
  std::cout << "EventToTuio::touchDown" << std::endl;

  prep();

  // auto match = FindClosest(stickyPointerList, x, y);
  //
  // if (match==NULL) {
    auto tobj = this->tuioServerRef->createTuioPointer(x,y,0,0,0,0);
    activePointerList.push_back(tobj->getTuioPointer());
  // } else {
    // activePointerList.push_back(match);
  // }
}

void EventToTuio::touchMove(float x, float y) {
  std::cout << "EventToTuio::touchMove" << std::endl;
  prep();
  // TuioPointer *pointer = NULL;
  // float distance;
  //
  // for (std::list<TuioPointer*>::iterator iter = activePointerList.begin(); iter!=activePointerList.end(); iter++) {
  //   TuioPointer *tptr = (*iter);
  //   float test = tptr->getDistance(x,y);
  //   if (pointer == NULL || test < distance) {
  //     distance = test;
  //     pointer = tptr;
  //   }
  // }
  auto pointer = FindClosest(activePointerList, x, y);

  if (pointer==NULL) return;
  if (pointer->getTuioTime()==this->tuioServerRef->getFrameTime()) return;
  this->tuioServerRef->updateTuioPointer(pointer,x,y,0,0,0,0);
}

void EventToTuio::touchUp(float x, float y) {
  std::cout << "EventToTuio::touchUp" << std::endl;
  prep();
  // auto pointer = FindClosest(stickyPointerList, x, y);
  //
  // if (pointer != NULL) {
  //   activePointerList.remove(pointer);
  //   return;
  // }

  auto pointer = FindClosest(activePointerList, x, y);

  if (pointer != NULL) {
    activePointerList.remove(pointer);
    this->tuioServerRef->removeTuioPointer(pointer);
  }
}

TuioPointer* EventToTuio::FindClosest(std::list<TuioPointer*>& list, float x, float y) {
  TuioPointer *match = NULL;
  float distance;

  for (std::list<TuioPointer*>::iterator iter = list.begin(); iter!=list.end(); iter++) {
    TuioPointer *tptr = (*iter);
    float test = tptr->getDistance(x,y);
    if (match == NULL || test < distance) {
      distance = test;
      match = tptr;
    }
  }

  return match;
}
