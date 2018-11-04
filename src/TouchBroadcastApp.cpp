#include "cinder/app/App.h"
#include "cinder/app/RendererGl.h"
#include "cinder/gl/gl.h"
#include "cinder/System.h"
#include "cinder/Log.h"

#include <vector>
#include <map>
#include <list>

#include "TUIO/TuioServer.h"
#include "EventToTuio.h"
#include "Text.h"

#define TOUCH_POINTS

#ifdef TOUCH_POINTS
#include "TouchPoint.h"
#endif

using namespace ci;
using namespace ci::app;
using namespace std;

const std::string destinationHost = "127.0.0.1";
const uint16_t destinationPort = 3333;
const uint16_t localPort = 10000;


class TouchBroadcastApp : public App {
	public:

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

		std::shared_ptr<EventToTuio> eventToTuioRef;
		std::shared_ptr<TUIO::TuioServer> tuioServerRef;

		#ifdef TOUCH_POINTS
		std::shared_ptr<TouchPointManager> touchPointManRef = nullptr;
		#endif

		Text helpText;
		// OSC
		// void onSendError( asio::error_code error );
		// Sender	mSender;
		bool	mIsConnected = true;
		bool bDrawHelp = true;
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

void TouchBroadcastApp::setup()
{
	CI_LOG_I("Initialising TuioServer...");
	this->tuioServerRef = std::make_shared<TUIO::TuioServer>(destinationHost.c_str(), destinationPort);
	this->tuioServerRef->setSourceName("TouchBroadcast");
	this->eventToTuioRef = std::make_shared<EventToTuio>(this->tuioServerRef);

	this->helpText.setPos(ci::vec2(10,10));
	this->helpText.setScale(ci::vec2(1,1));
	this->helpText.setFontSize(20);
	this->helpText.setText("TouchBroadcaster"
		"\n==============="
		"\nF - toggle fullscreen"
		"\nT - toggle always-on-top"
		"\nH - draw help overlay"
		"\nV - toggle visualisation"
		"\n \n \n \n "
		"\nDO NOT CLOSE"
		"\n "
		"\nTHIS APPLICATION SENDS TOUCH-SCREEN EVENTS TO THE INTERACTIVE TABLE APPLICATION");

	// #ifdef TOUCH_POINTS
	// 	if (this->touchPointManRef) this->touchPointManRef = std::make_shared<TouchPointManager>();
	// #endif

  setFullScreen(true);
}

vec2 TouchBroadcastApp::normalise(const vec2& vec) {
	return vec2((float)vec.x / getWindowWidth(), (float)vec.y / getWindowHeight());
}

void TouchBroadcastApp::touchesBegan( TouchEvent event )
{
	for(  auto &touch : event.getTouches() ) {
		auto normpos = normalise(touch.getPos());
		this->eventToTuioRef->touchDown(normpos.x, normpos.y);

		#ifdef TOUCH_POINTS
			if (this->touchPointManRef) this->touchPointManRef->add(touch.getId(), touch.getPos());
		#endif
	}
}


void TouchBroadcastApp::touchesMoved( TouchEvent event )
{
	for(  auto &touch : event.getTouches() ) {
		auto normpos = normalise(touch.getPos());
		this->eventToTuioRef->touchMove(normpos.x, normpos.y);

		#ifdef TOUCH_POINTS
			if (this->touchPointManRef) this->touchPointManRef->update(touch.getId(), touch.getPos());
		#endif
	}
}

void TouchBroadcastApp::touchesEnded( TouchEvent event )
{
	for(  auto &touch : event.getTouches() ) {
		auto normpos = normalise(touch.getPos());
		this->eventToTuioRef->touchUp(normpos.x, normpos.y);

		#ifdef TOUCH_POINTS
			if (this->touchPointManRef) this->touchPointManRef->remove(touch.getId());
		#endif
	}
}

void TouchBroadcastApp::mouseDown( MouseEvent event )
{
	auto normpos = normalise(event.getPos());
	this->eventToTuioRef->touchDown(normpos.x, normpos.y);

	#ifdef TOUCH_POINTS
		if (this->touchPointManRef) this->touchPointManRef->update(0, event.getPos());
	#endif
}

void TouchBroadcastApp::mouseDrag( MouseEvent event )
{
	auto normpos = normalise(event.getPos());
	this->eventToTuioRef->touchMove(normpos.x, normpos.y);

	#ifdef TOUCH_POINTS
		if (this->touchPointManRef) this->touchPointManRef->update(0, event.getPos());
	#endif
}

void TouchBroadcastApp::mouseUp( MouseEvent event )
{
	auto normpos = normalise(event.getPos());
	this->eventToTuioRef->touchUp(normpos.x, normpos.y);

	#ifdef TOUCH_POINTS
		if (this->touchPointManRef) this->touchPointManRef->remove(0);
	#endif
}

void TouchBroadcastApp::keyDown( KeyEvent event )
{
	if( event.getChar() == 'f' ) {
		setFullScreen(!isFullScreen());
	}

  if( event.getChar() == 't' ) {
    getWindow()->setAlwaysOnTop(!getWindow()->isAlwaysOnTop());
  }

	if( event.getChar() == 'h' ) {
		bDrawHelp = !bDrawHelp;
	}

	if (event.getCode() == KeyEvent::KEY_ESCAPE) {
		quit();
	}

	if (event.getChar() == 'v') {
		this->touchPointManRef = this->touchPointManRef ? nullptr : std::make_shared<TouchPointManager>();
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

	#ifdef TOUCH_POINTS
		if (this->touchPointManRef) this->touchPointManRef->draw();
	#endif

	// draw yellow circles at the active touch points
	gl::color( Color( 1, 1, 0 ) );
	for( const auto &touch : getActiveTouches() )
		gl::drawStrokedCircle( touch.getPos(), 20 );


	if (bDrawHelp) this->helpText.draw();
}

CINDER_APP( TouchBroadcastApp, RendererGl, prepareSettings )
