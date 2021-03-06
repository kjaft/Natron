//  Natron
//
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */
/*
 * Created by Alexandre GAUTHIER-FOICHAT on 6/1/2012.
 * contact: immarespond at gmail dot com
 *
 */

#ifndef NATRON_GUI_TEXTURE_H_
#define NATRON_GUI_TEXTURE_H_

#include "Global/Macros.h"
#include "Global/GlobalDefines.h"
#include "Engine/Rect.h"
#include "Engine/TextureRect.h"

class Texture
{
public:
    /*Note that the short datatype is not used currently*/
    enum DataTypeEnum
    {
        eDataTypeNone,
        eDataTypeByte,
        eDataTypeFloat,
        eDataTypeHalf,
    };

public:


    Texture(U32 target,
            int minFilter,
            int magFilter,
            int clamp);

    U32 getTexID() const
    {
        return _texID;
    }

    int w() const
    {
        return _textureRect.w;
    }

    int h() const
    {
        return _textureRect.h;
    }

    DataTypeEnum type() const
    {
        return _type;
    }

    /*allocates the texture*/
    void fillOrAllocateTexture(const TextureRect & texRect, DataTypeEnum type);

    const TextureRect & getTextureRect() const
    {
        return _textureRect;
    }

    virtual ~Texture();

private:

    U32 _texID;
    U32 _target;
    int _minFilter,_magFilter, _clamp;
    TextureRect _textureRect;
    DataTypeEnum _type;
};


#endif /* defined(NATRON_GUI_TEXTURE_H_) */
