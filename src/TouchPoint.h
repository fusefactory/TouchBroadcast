#include "cinder/gl/gl.h"

using namespace ci;
using namespace std;

struct TouchPoint {
	public:
		TouchPoint(){}
		TouchPoint( const vec2 &initialPt, const Color &color );

		void addPoint( const vec2 &pt );
		void draw() const;
		void startDying();
		bool isDead() const;
		vec2 getLastPos() const;

	private:
		PolyLine2f		mLine;
		Color			mColor;
		float			mTimeOfDeath;
		vec2 lastPos;
};

class TouchPointManager {

	public:
		TouchPointManager();
		void draw();

		void add(int id, vec2 pos);
		void update(int id, vec2 pos);
		void remove(int id);

	private:
		map<uint32_t,TouchPoint>	mActivePoints;
		list<TouchPoint>			mDyingPoints;
};
