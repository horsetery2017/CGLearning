#ifndef TEXTURE_H
#define TEXTURE_H

#include "glsupport.h"

class Texture {
public:
    // Must return one of GL_SAMPLER_1D, GL_SAMPLER_2D, GL_SAMPLER_3D, GL_SAMPLER_CUBE,
    // GL_SAMPLER_1D_SHADOW, or GL_SAMPLER_2D_SHADOW, as its intended usage by GLSL shader
    virtual GLenum getSamplerType() const = 0;
    
    // Binds the texture. (The caller is responsible for setting the active texture unit)
    //基础知识点：shader在执行texture api方法时，使用texture unit为基础索引。
    //具体，sampler uniform variables会被指定一个texture unit的值，
    //比如激活GL_TEXTURE0，则sampler变量则被设置为0，激活GL_TEXTURE2,则sampler被设置为2
    virtual void bind() const = 0;
        
    virtual ~Texture() {}
};

//----------------------------------------
// One concrete implementation of Texture
//----------------------------------------

class ImageTexture : public Texture {
    GlTexture tex;
    
public:
    // Loades a PPM image files with three channels, and create
    // a 2D texture off it. if `srgb' is true, the image is assumed
    // to be in SRGB color space
    ImageTexture(const std::string& ppmFileName, bool srgb); // implemented in texture.cpp
    
    virtual GLenum getSamplerType() const {
        return GL_SAMPLER_2D;
    }
    
    virtual void bind() const {
        glBindTexture(GL_TEXTURE_2D, tex);
    }
};



#endif
