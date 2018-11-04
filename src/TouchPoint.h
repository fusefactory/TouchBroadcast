#include "cinder/gl/gl.h"
#include "cinder/app/RendererGl.h"
#include "cinder/app/App.h"
#include "cinder/Log.h"

using namespace ci;
using namespace std;

struct TouchPoint {
	TouchPoint() {}
	TouchPoint( const vec2 &initialPt, const Color &color ) : mColor( color ), mTimeOfDeath( -1.0 )
	{
		mLine.push_back( initialPt );
		lastPos = initialPt;
	}

	void addPoint( const vec2 &pt ) { mLine.push_back( pt ); lastPos = pt; }

	void draw() const
	{
		if( mTimeOfDeath > 0 ) // are we dying? then fade out
			gl::color( ColorA( mColor, ( mTimeOfDeath - cinder::app::getElapsedSeconds() ) / 2.0f ) );
		else
			gl::color( mColor );

		gl::draw( mLine );
	}

	void startDying() { mTimeOfDeath = cinder::app::getElapsedSeconds() + 2.0f; } // two seconds til dead

	bool isDead() const { return cinder::app::getElapsedSeconds() > mTimeOfDeath; }

	vec2 getLastPos() { return lastPos; }

	PolyLine2f		mLine;
	Color			mColor;
	float			mTimeOfDeath;
	vec2 lastPos;
};

class TouchPointManager {

public:
	TouchPointManager() {
		CI_LOG_I( "MT: " << System::hasMultiTouch() << " Max points: " << System::getMaxMultiTouchPoints() );
	}

	void add(int id, vec2 pos) {
		Color newColor( CM_HSV, Rand::randFloat(), 1, 1 );
		mActivePoints.insert( make_pair( id, TouchPoint( pos, newColor ) ) );
	}

	void update(int id, vec2 pos) {
		mActivePoints[id].addPoint( pos );
	}

	void remove(int id) {
		mActivePoints[id].startDying();
		mDyingPoints.push_back( mActivePoints[id] );
		mActivePoints.erase( id );
	}

	void draw() {
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

	private:
		map<uint32_t,TouchPoint>	mActivePoints;
		list<TouchPoint>			mDyingPoints;
};
