#pragma once

// cinder
#include "cinder/Text.h"
#include "cinder/Font.h"
#include "cinder/gl/gl.h"

class Text {
  public: // instance interface
    void update();
    void draw();

    void setScale(const ci::vec2& v, bool update=false) { mScale = v; if (update) this->update(); }
    void setPos(const ci::vec2& v, bool update=false) { mPos = v; if (update) this->update(); }
    void setFontSize(float size, bool update=false) { fontSize = size; if (update) this->update(); }
    void setText(const std::string& text, bool update=true) { this->text = text; if (update) this->update(); }

  public: // static interface
    static ci::gl::TextureRef createTexture(const std::string& text, float fontSize);

  private:
    ci::gl::TextureRef mTex=nullptr;
    float fontSize = 14;
    std::string text = "";
    ci::vec2 mScale = ci::vec2(1.0f);
    ci::vec2 mPos = ci::vec2(10.0f);
    ci::Color mColor = ci::ColorA(0.7f, 0.7f, 0.7f);
};
