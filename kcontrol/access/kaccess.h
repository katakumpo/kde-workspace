#ifndef __K_ACCESS_H__
#define __K_ACCESS_H__


#include <qwidget.h>
#include <qcolor.h>


#include <kuniqueapp.h>


#include <X11/Xlib.h>
#include <X11/XKBlib.h>


class KAccessApp : public KUniqueApplication
{
  Q_OBJECT

public:

  KAccessApp(bool allowStyles=true, bool GUIenabled=true);

  bool x11EventFilter(XEvent *event);
  
  int newInstance() { readSettings(); return 0; };


protected:

  void readSettings();

  void xkbBellNotify(XkbBellNotifyEvent *event);


private:

  int xkb_opcode;

  bool    _systemBell, _artsBell, _visibleBell, _visibleBellInvert;
  QString _artsBellFile;
  QColor  _visibleBellColor;
  int     _visibleBellPause;

  QWidget *overlay;

};


class VisualBell : public QWidget
{
  Q_OBJECT

public:

  VisualBell(int pause) 
    : QWidget(0, 0, WStyle_NoBorder|WStyle_Tool|WStyle_Customize), _pause(pause)
    {};

  
protected:
  
  void paintEvent(QPaintEvent *);


private:

  int _pause;

};




#endif
