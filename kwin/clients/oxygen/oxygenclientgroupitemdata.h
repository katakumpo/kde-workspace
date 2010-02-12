#ifndef oxygenclientgroupitemdata_h
#define oxygenclientgroupitemdata_h

//////////////////////////////////////////////////////////////////////////////
// oxygenclientgroupitemdata.h
// -------------------
//
// Copyright (c) 2009 Hugo Pereira Da Costa <hugo.pereira@free.fr>
// Copyright (c) 2003, 2004 David Johnson <david@usermode.org>
// Copyright (c) 2006, 2007 Riccardo Iaconelli <ruphy@fsfe.org>
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to
// deal in the Software without restriction, including without limitation the
// rights to use, copy, modify, merge, publish, distribute, sublicense, and/or
// sell copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
// FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
// IN THE SOFTWARE.
//////////////////////////////////////////////////////////////////////////////

#include "oxygenbutton.h"
#include "lib/oxygenanimation.h"

#include <QtCore/QList>
#include <QtCore/QWeakPointer>
#include <QtCore/QRect>

namespace Oxygen
{

  class OxygenClient;

  //! animation type
  enum AnimationType
  {
    AnimationNone = 0,
    AnimationEnter = 1<<0,
    AnimationMove = 1<<1,
    AnimationLeave = 1<<2,
    AnimationSameTarget = 1<<3
  };

  Q_DECLARE_FLAGS(AnimationTypes, AnimationType)

  //! tab data
  class ClientGroupItemData
  {

    public:

    //! constructor
    explicit ClientGroupItemData( void )
    {}

    //! destructor
    virtual ~ClientGroupItemData( void )
    {}

    //! reset all rects to argument
    void reset( const QRect& rect )
    {
      refBoundingRect_ = rect;
      startBoundingRect_ = rect;
      endBoundingRect_ = rect;
      boundingRect_ = rect;
    }

    //! tab active rect
    QRect activeRect_;

    //! reference bounding rect
    /*! it is usually identical to activeRect unless there is only one tab in window */
    QRect refBoundingRect_;

    //! tab drawing rect
    QRect startBoundingRect_;

    //! tab drawing rect
    QRect endBoundingRect_;

    //! tab drawing rect
    QRect boundingRect_;

    //! tab button
    typedef QWeakPointer<OxygenButton> ButtonPointer;
    ButtonPointer closeButton_;

  };

  class ClientGroupItemDataList: public QObject, public QList<ClientGroupItemData>
  {

    Q_OBJECT

    //! declare animation progress property
    Q_PROPERTY( qreal progress READ progress WRITE setProgress )

    public:

    //! invalid item index
    enum { NoItem = -1 };

    //! constructor
    ClientGroupItemDataList( OxygenClient* parent );

    //! dirty state
    void setDirty( const bool& value )
    { dirty_ = value; }

    //! dirty state
    bool isDirty( void ) const
    { return dirty_; }

    //! enable animations
    void setAnimationsEnabled( bool value )
    { animationsEnabled_ = value; }

    //! animations enabled
    bool animationsEnabled( void ) const
    { return animationsEnabled_; }

    //! true if being animated
    bool isAnimated( void ) const
    { return animationType_ != AnimationNone; }

    //! animation type
    AnimationTypes animationType( void ) const
    { return animationType_; }

    //! return item index matching QPoint, or -1 if none
    int itemAt( const QPoint&, bool ) const;

    //! returns true if index is target
    bool isTarget( int index ) const
    { return index == targetItem_; }

    //! start animation
    /* might need to add the side of the target here */
    void animate( AnimationTypes, int = NoItem );

    //! true if animation is in progress
    bool isAnimationRunning( void ) const
    { return animation().data()->isRunning(); }

    //! update button activity
    void updateButtonActivity( int visibleItem ) const;

    //! update buttons
    void updateButtons( bool alsoUpdate ) const;

    //! target rect
    const QRect& targetRect( void ) const
    { return targetRect_; }

    //!@name animation progress
    //@{

    //! return animation object
    virtual const Animation::Pointer& animation() const
    { return animation_; }

    void setProgress( qreal value )
    { progress_ = value; }

    qreal progress( void ) const
    { return progress_; }

    //@}

    protected slots:

    //! update bounding rects
    void updateBoundingRects( bool alsoUpdate = true );

    private:

    //! client
    OxygenClient& client_;

    //! dirty flag
    /* used to trigger update at next paintEvent */
    bool dirty_;

    //! true if animations are enabled
    bool animationsEnabled_;

    //! animation
    Animation::Pointer animation_;

    //! last animation type
    AnimationTypes animationType_;

    //! animation progress
    qreal progress_;

    //! dragged item
    int draggedItem_;

    //! target item
    int targetItem_;

    //! target rect
    QRect targetRect_;

  };

  Q_DECLARE_OPERATORS_FOR_FLAGS(Oxygen::AnimationTypes)

}

#endif
