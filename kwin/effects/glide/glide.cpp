/********************************************************************
 KWin - the KDE window manager
 This file is part of the KDE project.

Copyright (C) 2007 Philip Falkner <philip.falkner@gmail.com>
Copyright (C) 2009 Martin Gräßlin <kde@martin-graesslin.com>
Copyright (C) 2010 Alexandre Pereira <pereira.alex@gmail.com>

This program is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation; either version 2 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program.  If not, see <http://www.gnu.org/licenses/>.
*********************************************************************/

#include "glide.h"

#include <kconfiggroup.h>

// Effect is based on fade effect by Philip Falkner

namespace KWin
{

KWIN_EFFECT( glide, GlideEffect )
KWIN_EFFECT_SUPPORTED( glide, GlideEffect::supported() )

static const int IsGlideWindow = 0x22A982D4;

GlideEffect::GlideEffect()
    {
    reconfigure( ReconfigureAll );
    }

bool GlideEffect::supported()
    {
    return effects->compositingType() == OpenGLCompositing;
    }

void GlideEffect::reconfigure( ReconfigureFlags )
    {
    KConfigGroup conf = effects->effectConfig( "Glide" );
    duration = animationTime( conf, "AnimationTime", 350 );
    effect = (EffectStyle) conf.readEntry( "GlideEffect", 0 );
    angle = conf.readEntry( "GlideAngle", -90 );
    }

void GlideEffect::prePaintScreen( ScreenPrePaintData& data, int time )
    {
    if( !windows.isEmpty() )
        data.mask |= PAINT_SCREEN_WITH_TRANSFORMED_WINDOWS;
    effects->prePaintScreen( data, time );
    }

void GlideEffect::prePaintWindow( EffectWindow* w, WindowPrePaintData& data, int time )
    {
    InfoHash::iterator info = windows.find( w );
    if( info != windows.end() )
        {
        data.setTransformed();
        if( info->added )
            info->timeLine.addTime( time );
        else if( info->closed )
            {
            info->timeLine.removeTime( time );
            if( info->deleted )
                w->enablePainting( EffectWindow::PAINT_DISABLED_BY_DELETE );
            }
        }
    
    effects->prePaintWindow( w, data, time );
    
    // if the window isn't to be painted, then let's make sure
    // to track its progress
    if( info != windows.end() && !w->isPaintingEnabled() && !effects->activeFullScreenEffect() )
        w->addRepaintFull();
    }

void GlideEffect::paintWindow( EffectWindow* w, int mask, QRegion region, WindowPaintData& data )
    {
    InfoHash::const_iterator info = windows.constFind( w );
    if( info != windows.constEnd() )
        {
        const double progress = info->timeLine.value();
        RotationData rot;
        rot.axis = RotationData::XAxis;
        rot.angle = angle * ( 1 - progress );
        data.rotation = &rot;
        data.opacity *= progress;
        switch ( effect )
            {
            default:
            case GlideInOut:
                if( info->added )
                    glideIn( w, data );
                else if( info->closed )
                    glideOut( w, data );
                break;
            case GlideOutIn:
                if( info->added )
                    glideOut( w, data );
                if( info->closed )
                    glideIn( w, data );
                break;
            case GlideIn: glideIn( w, data ); break;
            case GlideOut: glideOut( w, data ); break;
            }
        }
    effects->paintWindow( w, mask, region, data );
    }

void GlideEffect::glideIn(EffectWindow* w, WindowPaintData& data )
    {
    InfoHash::const_iterator info = windows.constFind( w );
    if ( info == windows.constEnd() )
        return;
    const double progress = info->timeLine.value();
    data.xScale *= progress;
    data.yScale *= progress;
    data.zScale *= progress;
    data.xTranslate += int( w->width() / 2 * ( 1 - progress ) );
    data.yTranslate += int( w->height() / 2 * ( 1 - progress ) );
    }

void GlideEffect::glideOut(EffectWindow* w, WindowPaintData& data )
    {
    InfoHash::const_iterator info = windows.constFind( w );
    if ( info == windows.constEnd() )
        return;
    const double progress = info->timeLine.value();
    data.xScale *= ( 2 - progress );
    data.yScale *= ( 2 - progress );
    data.zScale *= ( 2 - progress );
    data.xTranslate -= int( w->width() / 2 * ( 1 - progress ) );
    data.yTranslate -= int( w->height() / 2 * ( 1 - progress ) );
    }

void GlideEffect::postPaintWindow( EffectWindow* w )
    {
    InfoHash::iterator info = windows.find( w );
    if( info != windows.end() )
        {
        if( info->added && info->timeLine.value() == 1.0 )
            {
            windows.remove( w );
            effects->addRepaintFull();
            }
        else if( info->closed && info->timeLine.value() == 0.0 )
            {
            info->closed = false;
            if( info->deleted )
                {
                windows.remove( w );
                w->unrefWindow();
                }
            effects->addRepaintFull();
            }
        if( info->added || info->closed )
            w->addRepaintFull();
        }
    effects->postPaintWindow( w );
    }

void GlideEffect::windowAdded( EffectWindow* w )
    {
    if( !isGlideWindow( w ) )
        return;
    w->setData( IsGlideWindow, true );
    const void *addGrab = w->data( WindowAddedGrabRole ).value<void*>();
    if ( addGrab && addGrab != this )
        return;
    w->setData( WindowAddedGrabRole, QVariant::fromValue( static_cast<void*>( this )));
    
    InfoHash::iterator it = windows.find( w );
    WindowInfo *info = ( it == windows.end() ) ? &windows[w] : &it.value();
    info->added = true;
    info->closed = false;
    info->deleted = false;
    info->timeLine.setDuration( duration );
    info->timeLine.setCurveShape( TimeLine::EaseOutCurve );
    w->addRepaintFull();
    }

void GlideEffect::windowClosed( EffectWindow* w )
    {
    if ( !isGlideWindow( w ) )
        return;
    const void *closeGrab = w->data( WindowClosedGrabRole ).value<void*>();
    if ( closeGrab && closeGrab != this )
        return;
    w->refWindow();
    w->setData( WindowClosedGrabRole, QVariant::fromValue( static_cast<void*>( this )));
    
    InfoHash::iterator it = windows.find( w );
    WindowInfo *info = ( it == windows.end() ) ? &windows[w] : &it.value();
    info->added = false;
    info->closed = true;
    info->deleted = true;
    info->timeLine.setDuration( duration );
    info->timeLine.setCurveShape( TimeLine::EaseInCurve );
    info->timeLine.setProgress( 1.0 );
    w->addRepaintFull();
    }

void GlideEffect::windowDeleted( EffectWindow* w )
    {
    windows.remove( w );
    }

bool GlideEffect::isGlideWindow( EffectWindow* w )
    {
    if ( effects->activeFullScreenEffect() )
        return false;
    if ( w->data( IsGlideWindow ).toBool() )
        return true;
    if ( w->hasDecoration() )
        return true;
    if ( !w->isManaged() || w->isMenu() ||  w->isNotification() || w->isDesktop() || 
         w->isDock() ||  w->isSplash() || w->isTopMenu() || w->isToolbar() ||
         w->windowClass() == "dashboard dashboard" )
        return false;
    return true;
    }
} // namespace
