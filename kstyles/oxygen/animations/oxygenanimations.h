#ifndef oxygenanimations_h
#define oxygenanimations_h

//////////////////////////////////////////////////////////////////////////////
// oxygenanimations.h
// container for all animation engines
// -------------------
//
// Copyright (c) 2009 Hugo Pereira Da Costa <hugo.pereira@free.fr>
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

#include "oxygendockseparatorengine.h"
#include "oxygenmenubarengine.h"
#include "oxygenmenuengine.h"
#include "oxygenprogressbarengine.h"
#include "oxygenscrollbarengine.h"
#include "oxygensliderengine.h"
#include "oxygenspinboxengine.h"
#include "oxygentabbarengine.h"
#include "oxygentoolbarengine.h"
#include "oxygenwidgetstateengine.h"

#include <QtCore/QObject>
#include <QtCore/QList>

namespace Oxygen
{

    //! stores engines
    class Animations: public QObject
    {

        Q_OBJECT

        public:

        //! constructor
        explicit Animations( QObject* );

        //! destructor
        virtual ~Animations( void )
        {}

        //! register animations corresponding to given widget, depending on its type.
        void registerWidget( QWidget* widget ) const;

        /*! unregister all animations associated to a widget */
        void unregisterWidget( QWidget* widget ) const;

        //! enability engine
        WidgetStateEngine& widgetEnabilityEngine( void ) const
        { return *widgetEnabilityEngine_; }

        //! abstractButton engine
        WidgetStateEngine& widgetStateEngine( void ) const
        { return *widgetStateEngine_; }

        //! editable combobox arrow hover engine
        WidgetStateEngine& comboBoxEngine( void ) const
        { return *comboBoxEngine_; }

        //! Tool buttons arrow hover engine
        WidgetStateEngine& toolButtonEngine( void ) const
        { return *toolButtonEngine_; }

        //! lineEdit engine
        WidgetStateEngine& lineEditEngine( void ) const
        { return *lineEditEngine_; }

        //! dock separators engine
        DockSeparatorEngine& dockSeparatorEngine( void ) const
        { return *dockSeparatorEngine_; }

        //! progressbar engine
        ProgressBarEngine& progressBarEngine( void ) const
        { return *progressBarEngine_; }

        //! menubar engine
        MenuBarBaseEngine& menuBarEngine( void ) const
        { return *menuBarEngine_; }

        //! menu engine
        MenuBaseEngine& menuEngine( void ) const
        { return *menuEngine_; }

        //! scrollbar engine
        ScrollBarEngine& scrollBarEngine( void ) const
        { return *scrollBarEngine_; }

        //! slider engine
        SliderEngine& sliderEngine( void ) const
        { return *sliderEngine_; }

        //! spinbox engine
        SpinBoxEngine& spinBoxEngine( void ) const
        { return *spinBoxEngine_; }

        //! tabbar
        TabBarEngine& tabBarEngine( void ) const
        { return *tabBarEngine_; }

        //! toolbar
        ToolBarEngine& toolBarEngine( void ) const
        { return *toolBarEngine_; }

        //! setup engines
        void setupEngines( void );

        protected slots:

        //! enregister engine
        void unregisterEngine( QObject* );
        private:

        //! register new engine
        void registerEngine( BaseEngine* engine );

        //! dock separator handle hover effect
        DockSeparatorEngine* dockSeparatorEngine_;

        //! widget enability engine
        WidgetStateEngine* widgetEnabilityEngine_;

        //! abstract button engine
        WidgetStateEngine* widgetStateEngine_;

        //! editable combobox arrow hover effect
        WidgetStateEngine* comboBoxEngine_;

        //! mennu toolbutton arrow hover effect
        WidgetStateEngine* toolButtonEngine_;

        //! line editor engine
        WidgetStateEngine* lineEditEngine_;

        //! progressbar engine
        ProgressBarEngine* progressBarEngine_;

        //! menubar engine
        MenuBarBaseEngine* menuBarEngine_;

        //! menu engine
        MenuBaseEngine* menuEngine_;

        //! scrollbar engine
        ScrollBarEngine* scrollBarEngine_;

        //! slider engine
        SliderEngine* sliderEngine_;

        //! spinbox engine
        SpinBoxEngine* spinBoxEngine_;

        //! tabbar engine
        TabBarEngine* tabBarEngine_;

        //! toolbar engine
        ToolBarEngine* toolBarEngine_;

        //! keep list of existing engines
        QList< BaseEngine::Pointer > engines_;

    };

}

#endif
