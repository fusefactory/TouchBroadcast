#include "cinder/app/App.h"
#include "cinder/app/RendererGl.h"
#include "cinder/gl/gl.h"
#include "cinder/System.h"
#include "cinder/Rand.h"
#include "cinder/Log.h"

#include <vector>
#include <map>
#include <list>

// #include "cinder/osc/Osc.h"
#include "TUIO/TuioServer.h"
#include "EventToTuio.h"

using namespace ci;
using namespace ci::app;
using namespace std;
using namespace TUIO;

// #define USE_UDP 1
//
// #if USE_UDP
// using Sender = osc::SenderUdp;
// #else
// using Sender = osc::SenderTcp;
// #endif

const std::string destinationHost = "127.0.0.1";
const uint16_t destinationPort = 3333;
const uint16_t localPort = 10000;

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
			gl::color( ColorA( mColor, ( mTimeOfDeath - getElapsedSeconds() ) / 2.0f ) );
		else
			gl::color( mColor );

		gl::draw( mLine );
	}

	void startDying() { mTimeOfDeath = getElapsedSeconds() + 2.0f; } // two seconds til dead

	bool isDead() const { return getElapsedSeconds() > mTimeOfDeath; }

	vec2 getLastPos() { return lastPos; }

	PolyLine2f		mLine;
	Color			mColor;
	float			mTimeOfDeath;
	vec2 lastPos;
};

class TouchBroadcastApp : public App {
	public:
		TouchBroadcastApp();

		void	mouseDown( MouseEvent event ) override;
		// void	mouseMove( MouseEvent event ) override {}
		void	mouseDrag( MouseEvent event ) override;
		void	mouseUp( MouseEvent event ) override;

		void	touchesBegan( TouchEvent event ) override;
		void	touchesMoved( TouchEvent event ) override;
		void	touchesEnded( TouchEvent event ) override;

		void keyDown( KeyEvent event ) override;
		void setup() override;
		void update() override;
		void draw() override;

	private:

		vec2 normalise(const vec2& vec);

		map<uint32_t,TouchPoint>	mActivePoints;
		list<TouchPoint>			mDyingPoints;
		list<int> mActiveTouchIds;

		std::shared_ptr<TuioServer> tuioServerRef;
		std::shared_ptr<EventToTuio> eventToTuioRef;

		// OSC
		// void onSendError( asio::error_code error );
		// Sender	mSender;
		bool	mIsConnected = true;
		bool commitOnEvent = true;
		bool commitAtInterval = false; // TODO
};

void prepareSettings( TouchBroadcastApp::Settings *settings )
{
	// By default, multi-touch is disabled on desktop and enabled on mobile platforms.
	// You enable multi-touch from the SettingsFn that fires before the app is constructed.
	settings->setMultiTouchEnabled( true );
  settings->setAlwaysOnTop(true);
  // settings->setDisplay( ci::Display::getDisplays()[0] );

	// On mobile, if you disable multitouch then touch events will arrive via mouseDown(), mouseDrag(), etc.
	//	settings->setMultiTouchEnabled( false );
}

TouchBroadcastApp::TouchBroadcastApp()
//: mSender( localPort, destinationHost, destinationPort ), mIsConnected( false )
{
}

void TouchBroadcastApp::setup()
{
	CI_LOG_I( "MT: " << System::hasMultiTouch() << " Max points: " << System::getMaxMultiTouchPoints() );

	CI_LOG_I("Initialising TuioServer...");
	this->tuioServerRef = std::make_shared<TuioServer>(destinationHost.c_str(), destinationPort);
	this->tuioServerRef->setSourceName("TouchBroadcast");
	this->eventToTuioRef = std::make_shared<EventToTuio>(this->tuioServerRef);

  setFullScreen(true);
}

// Unified error handler. Easiest to have a bound function in this situation,
// since we're sending from many different places.
// void TouchBroadcastApp::onSendError( asio::error_code error )
// {
// 	if( error ) {
// 		CI_LOG_E( "Error sending: " << error.message() << " val: " << error.value() );
// 		// If you determine that this error is fatal, make sure to flip mIsConnected. It's
// 		// possible that the error isn't fatal.
// 		mIsConnected = false;
// 		try {
// #if ! USE_UDP
// 			// If this is Tcp, it's recommended that you shutdown before closing. This
// 			// function could throw. The exception will contain asio::error_code
// 			// information.
// 			mSender.shutdown();
// #endif
// 			// Close the socket on exit. This function could throw. The exception will
// 			// contain asio::error_code information.
// 			mSender.close();
// 		}
// 		catch( const osc::Exception &ex ) {
// 			CI_LOG_EXCEPTION( "Cleaning up socket: val -" << ex.value(), ex );
// 		}
// 		quit();
// 	}
// }
// void TouchBroadcastApp::submitFakeTuio(const string &addr, int id, const ivec2 &pos) {
//   // TODO; check if another message with the same addr/id combination is already queued,
//   // if so, remove that message (deprecated by this new message)
//   this->sendFakeTuio(addr, id, pos);
// }
//
// void TouchBroadcastApp::sendFakeTuio(const string &addr, int id, const ivec2 &pos) {
//
//
//   // osc::Message msg( addr );
//   // // msg.append( "set" );
//   // msg.append( (int)id );
//   // msg.append( (float)pos.x / getWindowWidth() );
//   // msg.append( (float)pos.y / getWindowHeight() );
//   // msg.append( (float)0 ); // velX
//   // msg.append( (float)0 ); // velY
//   // msg.append( (float)0 ); // motionAccel
// 	//
//   // // Send the msg and also provide an error handler. If the message is important you
//   // // could store it in the error callback to dispatch it again if there was a problem.
//   // mSender.send( msg, std::bind( &TouchBroadcastApp::onSendError,
//   //                 this, std::placeholders::_1 ) );
// }

vec2 TouchBroadcastApp::normalise(const vec2& vec) {
	return vec2((float)vec.x / getWindowWidth(), (float)vec.y / getWindowHeight());
}

// bool TouchBroadcastApp::setPos(int id, vec2 pos) {
// 	// find existing
// 	auto findIter = std::find_if(mActivePoints.begin(), mActivePoints.end(), [id](auto p){ return p.first == id; });
// 	bool isNew = (findIter == mActivePoints.end());
//
// 	if (isNew) {
// 			Color newColor( CM_HSV, Rand::randFloat(), 1, 1 );
// 			mActivePoints.insert( make_pair( id, TouchPoint( pos, newColor ) ) );
// 			return true;
// 	}
//
// 	if ((*findIter).second.getLastPos() == pos) return false; // no change
//
// 	(*findIter).second.addPoint(pos);
// 	return true;
// }

void TouchBroadcastApp::touchesBegan( TouchEvent event )
{
	for(  auto &touch : event.getTouches() ) {
		auto normpos = normalise(touch.getPos());
		this->eventToTuioRef->touchDown(normpos.x, normpos.y);
	}

  // for( const auto &touch : event.getTouches() ) {
	// 	Color newColor( CM_HSV, Rand::randFloat(), 1, 1 );
	// 	mActivePoints.insert( make_pair( touch.getId(), TouchPoint( touch.getPos(), newColor ) ) );
	// }
	//
  // // Make sure you're connected before trying to send.
  // if( ! mIsConnected )
  //   return;
	//
  // for( auto &touch : event.getTouches() ) {
  //   // if (touch.isHandled()) continue;
  //   // touch.setHandled();
	// 	int touchid = touch.getId();
	//
	// 	std::list<int>::iterator findIter = std::find(mActiveTouchIds.begin(), mActiveTouchIds.end(), touchid);
	// 	bool isActive = findIter != mActiveTouchIds.end();
	//
	// 	if (!isActive) {
	// 		mActiveTouchIds.push_back(touchid);
	//     ivec2 pos = touch.getPos();
	//     this->submitFakeTuio("/fakeTuio/down", touchid, pos);
	// 	}
  // }
}


void TouchBroadcastApp::touchesMoved( TouchEvent event )
{
	for(  auto &touch : event.getTouches() ) {
		auto normpos = normalise(touch.getPos());
		this->eventToTuioRef->touchMove(normpos.x, normpos.y);
	}

	// //CI_LOG_I( event );
	// for( const auto &touch : event.getTouches() ) {
	// 	mActivePoints[touch.getId()].addPoint( touch.getPos() );
	// }
	//
  // // Make sure you're connected before trying to send.
  // if( ! mIsConnected )
  //   return;
	//
  // for(  auto &touch : event.getTouches() ) {
  //   // if (touch.isHandled()) continue;
  //   // touch.setHandled();
	//
  //   int touchid = touch.getId();
	//
	// 	std::list<int>::iterator findIter = std::find(mActiveTouchIds.begin(), mActiveTouchIds.end(), touchid);
	// 	bool isActive = findIter != mActiveTouchIds.end();
	//
	// 	if (isActive) {
	//
  //   	ivec2 pos = touch.getPos();
	//
	// 		if (this->setPos(touchid, pos)) { // any change?
  //   		this->submitFakeTuio("/fakeTuio/move", touchid, pos);
	// 		}
	// 	}
  // }
}

void TouchBroadcastApp::touchesEnded( TouchEvent event )
{
	for(  auto &touch : event.getTouches() ) {
		auto normpos = normalise(touch.getPos());
		this->eventToTuioRef->touchUp(normpos.x, normpos.y);
	}
	//
	// //CI_LOG_I( event );
	// for( const auto &touch : event.getTouches() ) {
	// 	mActivePoints[touch.getId()].startDying();
	// 	mDyingPoints.push_back( mActivePoints[touch.getId()] );
	// 	mActivePoints.erase( touch.getId() );
	// }
	//
  // // Make sure you're connected before trying to send.
  // if( ! mIsConnected )
  //   return;
	//
  // for(  auto &touch : event.getTouches() ) {
  //   // if (touch.isHandled()) continue;
  //   // touch.setHandled();
	//
  //   int touchid = touch.getId();
	//
	// 	std::list<int>::iterator findIter = std::find(mActiveTouchIds.begin(), mActiveTouchIds.end(), touchid);
	// 	bool isActive = findIter != mActiveTouchIds.end();
	//
	// 	if (isActive) {
	// 		mActiveTouchIds.erase(findIter);
	//
  //   	ivec2 pos = touch.getPos();
  //   	this->submitFakeTuio("/fakeTuio/up", touchid, pos);
	// 	}
  // }
}

void TouchBroadcastApp::mouseDown( MouseEvent event )
{
	auto normpos = normalise(event.getPos());
	this->eventToTuioRef->touchDown(normpos.x, normpos.y);
}

void TouchBroadcastApp::mouseDrag( MouseEvent event )
{
	auto normpos = normalise(event.getPos());
	this->eventToTuioRef->touchMove(normpos.x, normpos.y);
}

void TouchBroadcastApp::mouseUp( MouseEvent event )
{
	auto normpos = normalise(event.getPos());
	this->eventToTuioRef->touchUp(normpos.x, normpos.y);
}

void TouchBroadcastApp::keyDown( KeyEvent event )
{
	if( event.getChar() == 'f' ) {
		setFullScreen(!isFullScreen());
	}

  if( event.getChar() == 't' ) {
    getWindow()->setAlwaysOnTop(!getWindow()->isAlwaysOnTop());
  }
}

void TouchBroadcastApp::update() {
	this->eventToTuioRef->update();
	// this->tuioServerRef->setDimension(getWindowWidth(), getWindowHeight());
}
void TouchBroadcastApp::draw()
{
	gl::enableAlphaBlending();
	gl::clear( Color( 0.1f, 0.1f, 0.1f ) );

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

	// draw yellow circles at the active touch points
	gl::color( Color( 1, 1, 0 ) );
	for( const auto &touch : getActiveTouches() )
		gl::drawStrokedCircle( touch.getPos(), 20 );
}

CINDER_APP( TouchBroadcastApp, RendererGl, prepareSettings )
