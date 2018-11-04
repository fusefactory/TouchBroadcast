#include <sstream>
// cinder
#include "cinder/Text.h"
#include "cinder/Font.h"
#include "cinder/gl/gl.h"
// #include "cinder/Log.h"
#include "Text.h"

using namespace std;

void Text::update() {
  this->mTex = Text::createTexture(this->text, this->fontSize);
  // if (verbose)
    // CI_LOG_I("Updated Text component with text" << text << std::to_string(mTex->getWidth()));
}

void Text::draw() {
  if (mTex) {
    const auto& clrScp = ci::gl::ScopedColor(mColor);
    ci::gl::pushMatrices();
    ci::gl::translate(mPos);
    ci::gl::scale(mScale);
    ci::gl::draw(mTex);
    ci::gl::popMatrices();
  }
}

ci::gl::TextureRef Text::createTexture(const std::string& text, float fontSize) {
  ci::TextLayout layout;
  layout.clear( ci::ColorA( 0.0f, 0.0f, 0.0f, 0.0f ) );
  layout.setFont( ci::Font( "AndaleMono", fontSize ) );
  layout.setColor( ci::Color( 1, 1, 1 ) );

  istringstream ss(text);
  string line;
  while (getline(ss, line, '\n')) layout.addLine(line);

  ci::Surface8u rendered = layout.render( true, false /* PREMULT */ );
  return ci::gl::Texture2d::create( rendered );
}
