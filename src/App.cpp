#include "cinder/app/App.h"
#include "cinder/app/RendererGl.h"
#include "cinder/gl/gl.h"
#include "cinder/System.h"
#include "cinder/Rand.h"
#include "cinder/Log.h"

#include <vector>
#include <map>
#include <list>

#include "cinder/osc/Osc.h"

using namespace ci;
using namespace ci::app;
using namespace std;

#define USE_UDP 1

#if USE_UDP
using Sender = osc::SenderUdp;
#else
using Sender = osc::SenderTcp;
#endif

const std::string destinationHost = "127.0.0.1";
const uint16_t destinationPort = 3335;
const uint16_t localPort = 10000;

struct TouchPoint {
	TouchPoint() {}
	TouchPoint( const vec2 &initialPt, const Color &color ) : mColor( color ), mTimeOfDeath( -1.0 )
	{
		mLine.push_back( initialPt );
	}

	void addPoint( const vec2 &pt ) { mLine.push_back( pt ); }

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

	PolyLine2f		mLine;
	Color			mColor;
	float			mTimeOfDeath;
};

class MultiTouchApp : public App {
 public:
	MultiTouchApp();
	void	mouseDown( MouseEvent event ) override;
  void	mouseMove( MouseEvent event ) override;
  void	mouseDrag( MouseEvent event ) override;
  void	mouseUp( MouseEvent event ) override;
  void submitFakeTuio(const string &addr, int id, const ivec2 &pos);
  void sendFakeTuio(const string &addr, int id, const ivec2 &pos);

	void	touchesBegan( TouchEvent event ) override;
	void	touchesMoved( TouchEvent event ) override;
	void	touchesEnded( TouchEvent event ) override;

  void keyDown( KeyEvent event ) override;
	void	setup() override;
	void	draw() override;

  private:

	map<uint32_t,TouchPoint>	mActivePoints;
	list<TouchPoint>			mDyingPoints;

	// OSC
	void onSendError( asio::error_code error );
	Sender	mSender;
	bool	mIsConnected;
};

void prepareSettings( MultiTouchApp::Settings *settings )
{
	// By default, multi-touch is disabled on desktop and enabled on mobile platforms.
	// You enable multi-touch from the SettingsFn that fires before the app is constructed.
	settings->setMultiTouchEnabled( true );

	// On mobile, if you disable multitouch then touch events will arrive via mouseDown(), mouseDrag(), etc.
//	settings->setMultiTouchEnabled( false );
}

MultiTouchApp::MultiTouchApp()
: mSender( localPort, destinationHost, destinationPort ), mIsConnected( false )
{
}

void MultiTouchApp::setup()
{
	CI_LOG_I( "MT: " << System::hasMultiTouch() << " Max points: " << System::getMaxMultiTouchPoints() );

  try {
    // Bind the sender to the endpoint. This function may throw. The exception will
    // contain asio::error_code information.
    mSender.bind();
  }
  catch ( const osc::Exception &ex ) {
    CI_LOG_E( "Error binding: " << ex.what() << " val: " << ex.value() );
    quit();
  }

#if ! USE_UDP
  mSender.connect(
  // Set up the OnConnectFn. If there's no error, you can consider yourself connected to
  // the endpoint supplied.
  [&]( asio::error_code error ){
    if( error ) {
      CI_LOG_E( "Error connecting: " << error.message() << " val: " << error.value() );
      quit();
    }
    else {
      CI_LOG_V( "Connected" );
      mIsConnected = true;
    }
  });
#else
  // Udp doesn't "connect" the same way Tcp does. If bind doesn't throw, we can
  // consider ourselves connected.
  mIsConnected = true;
#endif

  setFullScreen(true);
}

// Unified error handler. Easiest to have a bound function in this situation,
// since we're sending from many different places.
void MultiTouchApp::onSendError( asio::error_code error )
{
	if( error ) {
		CI_LOG_E( "Error sending: " << error.message() << " val: " << error.value() );
		// If you determine that this error is fatal, make sure to flip mIsConnected. It's
		// possible that the error isn't fatal.
		mIsConnected = false;
		try {
#if ! USE_UDP
			// If this is Tcp, it's recommended that you shutdown before closing. This
			// function could throw. The exception will contain asio::error_code
			// information.
			mSender.shutdown();
#endif
			// Close the socket on exit. This function could throw. The exception will
			// contain asio::error_code information.
			mSender.close();
		}
		catch( const osc::Exception &ex ) {
			CI_LOG_EXCEPTION( "Cleaning up socket: val -" << ex.value(), ex );
		}
		quit();
	}
}
void MultiTouchApp::submitFakeTuio(const string &addr, int id, const ivec2 &pos) {
  // TODO; check if another message with the same addr/id combination is already queued,
  // if so, remove that message (deprecated by this new message)
  this->sendFakeTuio(addr, id, pos);
}

void MultiTouchApp::sendFakeTuio(const string &addr, int id, const ivec2 &pos) {
  osc::Message msg( addr );
  // msg.append( "set" );
  msg.append( (int)id );
  msg.append( (float)pos.x / getWindowWidth() );
  msg.append( (float)pos.y / getWindowHeight() );
  msg.append( (float)0 ); // velX
  msg.append( (float)0 ); // velY
  msg.append( (float)0 ); // motionAccel

  // Send the msg and also provide an error handler. If the message is important you
  // could store it in the error callback to dispatch it again if there was a problem.
  mSender.send( msg, std::bind( &MultiTouchApp::onSendError,
                  this, std::placeholders::_1 ) );
}

void MultiTouchApp::touchesBegan( TouchEvent event )
{
  for( const auto &touch : event.getTouches() ) {
		Color newColor( CM_HSV, Rand::randFloat(), 1, 1 );
		mActivePoints.insert( make_pair( touch.getId(), TouchPoint( touch.getPos(), newColor ) ) );
	}


  // Make sure you're connected before trying to send.
  if( ! mIsConnected )
    return;

  for( const auto &touch : event.getTouches() ) {
    int touchid = touch.getId();
    ivec2 pos = touch.getPos();
    this->submitFakeTuio("/fakeTuio/down", touchid, pos);
  }
}


void MultiTouchApp::touchesMoved( TouchEvent event )
{
	//CI_LOG_I( event );
	for( const auto &touch : event.getTouches() ) {
		mActivePoints[touch.getId()].addPoint( touch.getPos() );
	}

  // Make sure you're connected before trying to send.
  if( ! mIsConnected )
    return;

  for( const auto &touch : event.getTouches() ) {
    int touchid = touch.getId();
    ivec2 pos = touch.getPos();
    this->submitFakeTuio("/fakeTuio/move", touchid, pos);
  }
}

void MultiTouchApp::touchesEnded( TouchEvent event )
{
	//CI_LOG_I( event );
	for( const auto &touch : event.getTouches() ) {
		mActivePoints[touch.getId()].startDying();
		mDyingPoints.push_back( mActivePoints[touch.getId()] );
		mActivePoints.erase( touch.getId() );
	}

  // Make sure you're connected before trying to send.
  if( ! mIsConnected )
    return;

  for( const auto &touch : event.getTouches() ) {
    int touchid = touch.getId();
    ivec2 pos = touch.getPos();

    this->submitFakeTuio("/fakeTuio/up", touchid, pos);
  }
}

void MultiTouchApp::mouseMove( cinder::app::MouseEvent event )
{

}

void MultiTouchApp::mouseDown( MouseEvent event )
{
	// Make sure you're connected before trying to send.
	if( ! mIsConnected )
		return;

    osc::Message msg( "/fakeTuio/down" );
    // msg.append( "set" );
    msg.append( (int)0 );
  	msg.append( (float)event.getPos().x / getWindowWidth() );
  	msg.append( (float)event.getPos().y / getWindowHeight() );
    msg.append( (float)0 ); // velX
    msg.append( (float)0 ); // velY
    msg.append( (float)0 ); // motionAccel
  	// Send the msg and also provide an error handler. If the message is important you
  	// could store it in the error callback to dispatch it again if there was a problem.
  	mSender.send( msg, std::bind( &MultiTouchApp::onSendError,
  								  this, std::placeholders::_1 ) );
}

void MultiTouchApp::mouseDrag( MouseEvent event )
{
  // Make sure you're connected before trying to send.
  if( ! mIsConnected )
    return;

  osc::Message msg( "/fakeTuio/move" );
  // msg.append( "set" );
  msg.append( (int)0 );
  msg.append( (float)event.getPos().x / getWindowWidth() );
  msg.append( (float)event.getPos().y / getWindowHeight() );
  msg.append( (float)0 ); // velX
  msg.append( (float)0 ); // velY
  msg.append( (float)0 ); // motionAccel
  // Send the msg and also provide an error handler. If the message is important you
  // could store it in the error callback to dispatch it again if there was a problem.
  mSender.send( msg, std::bind( &MultiTouchApp::onSendError,
                  this, std::placeholders::_1 ) );
}

void MultiTouchApp::mouseUp( MouseEvent event )
{
	// Make sure you're connected before trying to send.
	if( ! mIsConnected )
		return;

	osc::Message msg( "/fakeTuio/up" );
  msg.append( (int)0 );
	msg.append( (float)event.getPos().x / getWindowWidth() );
	msg.append( (float)event.getPos().y / getWindowHeight() );
	// Send the msg and also provide an error handler. If the message is important you
	// could store it in the error callback to dispatch it again if there was a problem.
	mSender.send( msg, std::bind( &MultiTouchApp::onSendError,
								  this, std::placeholders::_1 ) );
}

void MultiTouchApp::keyDown( KeyEvent event )
{
	if( event.getChar() == 'f' ) {
		setFullScreen(!isFullScreen());
	}
}

void MultiTouchApp::draw()
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

CINDER_APP( MultiTouchApp, RendererGl, prepareSettings )
