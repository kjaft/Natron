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


#include "StringAnimationManager.h"

#include <set>
#include <cmath>
#include <QMutex>

#include "Engine/Knob.h"

struct StringKeyFrame
{
    std::string value;
    int time;
};

struct StringKeyFrame_compare_time
{
    bool operator() (const StringKeyFrame & lhs,
                     const StringKeyFrame & rhs) const
    {
        return lhs.time < rhs.time;
    }
};

typedef std::set<StringKeyFrame,StringKeyFrame_compare_time> Keyframes;


struct StringAnimationManagerPrivate
{
    StringAnimationManager::customParamInterpolationV1Entry_t customInterpolation;
    void* ofxParamHandle;
    mutable QMutex keyframesMutex;
    Keyframes keyframes;
    const KnobI* knob;

    StringAnimationManagerPrivate(const KnobI* knob)
        : customInterpolation(NULL)
          , ofxParamHandle(NULL)
          , keyframesMutex()
          , keyframes()
          , knob(knob)
    {
    }
};

StringAnimationManager::StringAnimationManager(const KnobI* knob)
    : _imp( new StringAnimationManagerPrivate(knob) )
{
}

StringAnimationManager::~StringAnimationManager()
{
}

bool
StringAnimationManager::hasCustomInterp() const
{
    return _imp->customInterpolation != NULL;
}

void
StringAnimationManager::setCustomInterpolation(customParamInterpolationV1Entry_t func,
                                               void* ofxParamHandle)
{
    _imp->customInterpolation = func;
    _imp->ofxParamHandle = ofxParamHandle;
}

bool
StringAnimationManager::customInterpolation(double time,
                                            std::string* ret) const
{
    QMutexLocker l(&_imp->keyframesMutex);

    assert(_imp->customInterpolation);
    ///if there's a single keyframe, return it

    if ( _imp->keyframes.empty() ) {
        return false;
    }

    if (_imp->keyframes.size() == 1) {
        *ret = _imp->keyframes.begin()->value;

        return true;
    }

    /// get the keyframes surrounding the time
    Keyframes::const_iterator upper = _imp->keyframes.end();
    Keyframes::const_iterator lower = _imp->keyframes.end();
    for (Keyframes::const_iterator it = _imp->keyframes.begin(); it != _imp->keyframes.end(); ++it) {
        if (it->time > time) {
            upper = it;
            break;
        } else if (it->time == time) {
            ///if there's a keyframe exactly at this time, return its value
            *ret = it->value;

            return true;
        }
    }

    if ( upper == _imp->keyframes.end() ) {
        ///if the time is greater than the time of all keyframes return the last

        --upper;
        *ret = upper->value;

        return true;
    } else if ( upper == _imp->keyframes.begin() ) {
        ///if the time is lesser than the time of all keyframes, return the first
        *ret = upper->value;

        return true;
    } else {
        ///general case, we're in-between 2 keyframes
        lower = upper;
        --lower;
    }

    OFX::Host::Property::PropSpec inArgsSpec[] = {
        { kOfxPropName,    OFX::Host::Property::eString, 1, true, "" },
        { kOfxPropTime,    OFX::Host::Property::eDouble, 1, true, "" },
        { kOfxParamPropCustomValue,    OFX::Host::Property::eString, 2, true, ""},
        { kOfxParamPropInterpolationTime,    OFX::Host::Property::eDouble, 2, true, "" },
        { kOfxParamPropInterpolationAmount,    OFX::Host::Property::eDouble, 1, true, "" },
        OFX::Host::Property::propSpecEnd
    };
    OFX::Host::Property::Set inArgs(inArgsSpec);
    inArgs.setStringProperty( kOfxPropName, _imp->knob->getName() );
    inArgs.setDoubleProperty(kOfxPropTime, time);

    inArgs.setStringProperty(kOfxParamPropCustomValue, lower->value,0);
    inArgs.setStringProperty(kOfxParamPropCustomValue, upper->value,1);
    inArgs.setDoubleProperty(kOfxParamPropInterpolationTime, lower->time,0);
    inArgs.setDoubleProperty(kOfxParamPropInterpolationTime, upper->time,1);
    inArgs.setDoubleProperty( kOfxParamPropInterpolationAmount, (time - lower->time) / (double)(upper->time - lower->time) );


    OFX::Host::Property::PropSpec outArgsSpec[] = {
        { kOfxParamPropCustomValue,    OFX::Host::Property::eString, 1, false, ""},
        OFX::Host::Property::propSpecEnd
    };
    OFX::Host::Property::Set outArgs(outArgsSpec);

    l.unlock();
    _imp->customInterpolation( _imp->ofxParamHandle,inArgs.getHandle(),outArgs.getHandle() );

    *ret = outArgs.getStringProperty(kOfxParamPropCustomValue,0).c_str();

    return true;
} // customInterpolation

void
StringAnimationManager::insertKeyFrame(int time,
                                       const std::string & v,
                                       double* index)
{
    StringKeyFrame k;

    k.time = time;
    k.value = v;
    QMutexLocker l(&_imp->keyframesMutex);
    std::pair<Keyframes::iterator,bool> ret = _imp->keyframes.insert(k);
    if (!ret.second) {
        _imp->keyframes.erase(ret.first);
        ret = _imp->keyframes.insert(k);
        assert(ret.second);
    }
    *index = std::distance(_imp->keyframes.begin(), ret.first);
}

void
StringAnimationManager::removeKeyFrame(int time)
{
    QMutexLocker l(&_imp->keyframesMutex);

    for (Keyframes::iterator it = _imp->keyframes.begin(); it != _imp->keyframes.end(); ++it) {
        if (it->time == time) {
            _imp->keyframes.erase(it);

            return;
        }
    }
}

void
StringAnimationManager::clearKeyFrames()
{
    QMutexLocker l(&_imp->keyframesMutex);

    _imp->keyframes.clear();
}

void
StringAnimationManager::stringFromInterpolatedIndex(double interpolated,
                                                    std::string* returnValue) const
{
    int index = std::floor(interpolated + 0.5);
    QMutexLocker l(&_imp->keyframesMutex);

    if ( _imp->keyframes.empty() ) {
        return;
    }

    ///if the index is not in the range, just return the last
    if ( index >= (int)_imp->keyframes.size() ) {
        Keyframes::const_iterator it = _imp->keyframes.end();
        --it;
        *returnValue = it->value;

        return;
    }

    int i = 0;
    for (Keyframes::const_iterator it = _imp->keyframes.begin(); it != _imp->keyframes.end(); ++it) {
        if (i == index) {
            *returnValue = it->value;

            return;
        }
        ++i;
    }
}

void
StringAnimationManager::clone(const StringAnimationManager & other)
{
    QMutexLocker l(&_imp->keyframesMutex);
    QMutexLocker l2(&other._imp->keyframesMutex);

    _imp->keyframes = other._imp->keyframes;
}

void
StringAnimationManager::clone(const StringAnimationManager & other,
                              SequenceTime offset,
                              const RangeD* range)
{
    // The range=[0,0] case is obviously a bug in the spec of paramCopy() from the parameter suite:
    // it prevents copying the value of frame 0.
    bool copyRange = range != NULL /*&& (range->min != 0 || range->max != 0)*/;
    QMutexLocker l(&_imp->keyframesMutex);

    _imp->keyframes.clear();
    QMutexLocker l2(&other._imp->keyframesMutex);
    for (Keyframes::const_iterator it = other._imp->keyframes.begin(); it != other._imp->keyframes.end(); ++it) {
        if ( copyRange && ( (it->time < range->min) || (it->time > range->max) ) ) {
            continue;
        }
        StringKeyFrame k;
        k.time = it->time + offset;
        k.value = it->value;
        _imp->keyframes.insert(k);
    }
}

void
StringAnimationManager::load(const std::map<int,std::string> & keyframes)
{
    QMutexLocker l(&_imp->keyframesMutex);

    assert( _imp->keyframes.empty() );
    for (std::map<int,std::string>::const_iterator it = keyframes.begin(); it != keyframes.end(); ++it) {
        StringKeyFrame k;
        k.time = it->first;
        k.value = it->second;
        std::pair<Keyframes::iterator,bool> ret = _imp->keyframes.insert(k);
        assert(ret.second);
    }
}

void
StringAnimationManager::save(std::map<int,std::string>* keyframes) const
{
    QMutexLocker l(&_imp->keyframesMutex);

    for (Keyframes::const_iterator it = _imp->keyframes.begin(); it != _imp->keyframes.end(); ++it) {
        std::pair<std::map<int,std::string>::iterator, bool> success = keyframes->insert( std::make_pair(it->time, it->value) );
        assert(success.second);
    }
}

