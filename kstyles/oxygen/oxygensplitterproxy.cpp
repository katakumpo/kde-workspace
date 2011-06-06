//////////////////////////////////////////////////////////////////////////////
// oxygensplitterproxy.cpp
// Extended hit area for Splitters
// -------------------
//
// Copyright (C) 2011 Hugo Pereira Da Costa <hugo@oxygen-icons.org>
//
// Based on Bespin splitterproxy code
// Copyright (C) 2011 Thomas Luebking <thomas.luebking@web.de>
//
//
// This library is free software; you can redistribute it and/or
// modify it under the terms of the GNU Library General Public
// License version 2 as published by the Free Software Foundation.
//
// This library is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
// Library General Public License for more details.
//
// You should have received a copy of the GNU Library General Public License
// along with this library; see the file COPYING.LIB.  If not, write to
// the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
// Boston, MA 02110-1301, USA.
//////////////////////////////////////////////////////////////////////////////

#include "oxygensplitterproxy.h"
#include "oxygenmetrics.h"

#include <QtCore/QCoreApplication>
#include <QtGui/QPainter>

namespace Oxygen
{

    //____________________________________________________________________
    bool SplitterFactory::registerWidget( QWidget *widget )
    {

        // check widget type
        if( qobject_cast<QMainWindow*>( widget ) )
        {

            WidgetMap::iterator iter( _widgets.find( widget ) );
            if( iter == _widgets.end() )
            {
                widget->installEventFilter( &_addEventFilter );
                SplitterProxy* proxy( new SplitterProxy( widget ) );
                widget->removeEventFilter( &_addEventFilter );

                widget->installEventFilter( proxy );

                _widgets.insert( widget, proxy );

            } else {

                widget->removeEventFilter( iter.value() );
                widget->installEventFilter( iter.value() );

            }

            return true;

        } else if( qobject_cast<QSplitterHandle*>( widget ) ) {

            QWidget* window( widget->window() );
            WidgetMap::iterator iter( _widgets.find( window ) );
            if( iter == _widgets.end() )
            {


                window->installEventFilter( &_addEventFilter );
                SplitterProxy* proxy( new SplitterProxy( window ) );
                window->removeEventFilter( &_addEventFilter );

                widget->installEventFilter( proxy );
                _widgets.insert( window, proxy );

            } else {

                widget->removeEventFilter( iter.value() );
                widget->installEventFilter( iter.value() );

            }

            return true;

        } else return false;

    }

    //____________________________________________________________________
    void SplitterFactory::unregisterWidget( QWidget *widget )
    {

        WidgetMap::iterator iter( _widgets.find( widget ) );
        if( iter != _widgets.end() )
        {
            iter.value()->deleteLater();
            _widgets.erase( iter );
        }

    }

    //____________________________________________________________________
    SplitterProxy::SplitterProxy( QWidget* parent ):
        QWidget( parent )
    { hide(); }

    //____________________________________________________________________
    SplitterProxy::~SplitterProxy( void )
    {}

    //____________________________________________________________________
    bool SplitterProxy::event( QEvent *event )
    {
        switch( event->type() )
        {

            case QEvent::Paint:
            {
                QPainter p( this );
                p.setPen( Qt::NoPen );
                p.setBrush( QColor( 255, 0, 0, 100 ) );
                p.drawRect( rect() );
                return true;
            }

            case QEvent::MouseMove:
            case QEvent::MouseButtonPress:
            case QEvent::MouseButtonRelease:
            {

                // check splitter
                if( !_splitter ) return false;

                event->accept();

                // grab on mouse press
                if( event->type() == QEvent::MouseButtonPress) grabMouse();

                parentWidget()->setUpdatesEnabled(false);
                resize(1,1);
                parentWidget()->setUpdatesEnabled(true);

                // cast to mouse event
                QMouseEvent *mouseEvent( static_cast<QMouseEvent*>( event ) );

                // get relevant position to post mouse drag event to application
                const QPoint pos( (event->type() == QEvent::MouseMove) ? _splitter.data()->mapFromGlobal(QCursor::pos()) : _hook );
                QMouseEvent mouseEvent2(
                    mouseEvent->type(), pos,
                    _splitter.data()->mapToGlobal(pos),
                    mouseEvent->button(),
                    mouseEvent->buttons(), mouseEvent->modifiers());

                QCoreApplication::sendEvent( _splitter.data(), &mouseEvent2 );

                // release grab on mouse-Release
                if( event->type() == QEvent::MouseButtonRelease && mouseGrabber() == this )
                { releaseMouse(); }

                return true;

            }

            case QEvent::Leave:
            {

                // leave event and reset splitter
                QWidget::leaveEvent( event );
                if( !rect().contains( mapFromGlobal( QCursor::pos() ) ) )
                { clearSplitter(); }
                return true;

            }

            default:
            return QWidget::event( event );

        }

        // fallback
        return QWidget::event( event );

    }

    //____________________________________________________________________
    bool SplitterProxy::eventFilter( QObject* object, QEvent* event )
    {

        if( mouseGrabber() ) return false;

        switch( event->type() )
        {

            case QEvent::HoverEnter:
            if( !isVisible() )
            {

                // cast to splitter handle
                if( QSplitterHandle* handle = qobject_cast<QSplitterHandle*>( object ) )
                { setSplitter( handle ); }

            }
            return false;

            case QEvent::HoverMove:
            case QEvent::HoverLeave:
            if( isVisible() && object == _splitter.data() )
            { return true; }

            case QEvent::MouseMove:
            case QEvent::Timer:
            case QEvent::Move:

            // just for performance - they can occur really often
            return false;

            case QEvent::CursorChange:
            if( QWidget *window = qobject_cast<QMainWindow*>( object ) )
            {
                if( window->cursor().shape() == Qt::SplitHCursor || window->cursor().shape() == Qt::SplitVCursor )
                { setSplitter( window ); }
            }
            return false;

            case QEvent::MouseButtonRelease:
            clearSplitter();
            return false;

            default:
            return false;

        }

        return false;
    }

    //____________________________________________________________________
    void SplitterProxy::setSplitter( QWidget* widget )
    {

        _splitter = widget;
        _hook = _splitter.data()->mapFromGlobal(QCursor::pos());

        QRect r( 0, 0, 2*Splitter_ExtendedWidth, 2*Splitter_ExtendedWidth );
        r.moveCenter( parentWidget()->mapFromGlobal( QCursor::pos() ) );
        setGeometry(r);
        setCursor( _splitter.data()->cursor().shape() );

        raise();
        show();

    }


    //____________________________________________________________________
    void SplitterProxy::clearSplitter( void )
    {

        // release mouse
        if( mouseGrabber() == this ) releaseMouse();

        // hide
        hide();

        // set hover event
        if( _splitter )
        {
            QHoverEvent hoverEvent(
                qobject_cast<QSplitterHandle*>(_splitter.data()) ? QEvent::HoverLeave : QEvent::HoverMove,
                _splitter.data()->mapFromGlobal(QCursor::pos()), _hook);
            QCoreApplication::sendEvent( _splitter.data(), &hoverEvent );
        }

        _splitter.clear();

    }

}
