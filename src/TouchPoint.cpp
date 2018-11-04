#include "cinder/gl/gl.h"
#include "cinder/app/RendererGl.h"
#include "cinder/app/App.h"
#include "cinder/Log.h"
#include "cinder/Rand.h"

#include "TouchPoint.h"
using namespace ci;
using namespace std;


TouchPoint::TouchPoint( const vec2 &initialPt, const Color &color ) : mColor( color ), mTimeOfDeath( -1.0 )
{
	mLine.push_back( initialPt );
	lastPos = initialPt;
}

void TouchPoint::addPoint( const vec2 &pt ) { mLine.push_back( pt ); lastPos = pt; }

void TouchPoint::draw() const
{
	if( mTimeOfDeath > 0 ) // are we dying? then fade out
		gl::color( ColorA( mColor, ( mTimeOfDeath - cinder::app::getElapsedSeconds() ) / 2.0f ) );
	else
		gl::color( mColor );

	gl::draw( mLine );
}

void TouchPoint::startDying() { mTimeOfDeath = cinder::app::getElapsedSeconds() + 2.0f; } // two seconds til dead

bool TouchPoint::isDead() const { return cinder::app::getElapsedSeconds() > mTimeOfDeath; }

vec2 TouchPoint::getLastPos() const { return lastPos; }


TouchPointManager::TouchPointManager() {
	CI_LOG_I( "MT: " << System::hasMultiTouch() << " Max points: " << System::getMaxMultiTouchPoints() );
}

void TouchPointManager::add(int id, vec2 pos) {
	Color newColor( CM_HSV, Rand::randFloat(), 1, 1 );
	mActivePoints.insert( make_pair( id, TouchPoint( pos, newColor ) ) );
}

void TouchPointManager::update(int id, vec2 pos) {
	mActivePoints[id].addPoint( pos );
}

void TouchPointManager::remove(int id) {
	mActivePoints[id].startDying();
	mDyingPoints.push_back( mActivePoints[id] );
	mActivePoints.erase( id );
}

void TouchPointManager::draw() {
	for( const auto &activePoint : mActivePoints ) {
		activePoint.second.draw();
	}

	for( auto dyingIt = mDyingPoints.begin(); dyingIt != mDyingPoints.end(); ) {
		dyingIt->draw();
		if( dyingIt->isDead() )
			dyingIt = mDyingPoints.erase( dyingIt );
		else
			++dyingIt;
	}
}
