//-----------------------------------------------------------------------------
//
// KDE Display screen saver setup module
//
// Copyright (c)  Martin R. Jones 1996
// Copyright (C) Chris Howells 2004
//

#ifndef __SCRNSAVE_H__
#define __SCRNSAVE_H__

#include <QWidget>
//Added by qt3to4:
#include <QMouseEvent>
#include <QLabel>
#include <QResizeEvent>
#include <QKeyEvent>
#include <kcmodule.h>

#include "kssmonitor.h"
#include "saverconfig.h"
#include "testwin.h"
#include "advanceddialog.h"
#include "kssmonitor.h"
#include "saverlist.h"

class QTimer;
class QSpinBox;
class QSlider;
class QCheckBox;
class QLabel;
class Q3ListView;
class Q3ListViewItem;
class QPushButton;
class KIntNumInput;
class KProcess;

//===========================================================================
class KScreenSaver : public KCModule
{
    Q_OBJECT
public:
    KScreenSaver(QWidget *parent, const QStringList &);
    ~KScreenSaver();

    virtual void load();
    virtual void save();
    virtual void defaults();

    void updateValues();
    void readSettings();

protected Q_SLOTS:
    void slotEnable( bool );
    void slotScreenSaver( Q3ListViewItem* );
    void slotSetup();
    void slotAdvanced();
    void slotTest();
    void slotStopTest();
    void slotTimeoutChanged( int );
    void slotLockTimeoutChanged( int );
    void slotDPMS( bool );
    void slotLock( bool );
    void slotSetupDone(KProcess*);
    // when selecting a new screensaver, the old preview will
    // be killed. -- This callback is responsible for restarting the
    // new preview
    void slotPreviewExited(KProcess *);
    void findSavers();

protected:
    void writeSettings();
    void getSaverNames();
    void setMonitor();
    void setDefaults();
    void resizeEvent( QResizeEvent * );
    void mousePressEvent(QMouseEvent *);
    void keyPressEvent(QKeyEvent *);

protected:
    TestWin     *mTestWin;
    KProcess    *mTestProc;
    KProcess    *mSetupProc;
    KProcess    *mPreviewProc;
    KSSMonitor  *mMonitor;
    QPushButton *mSetupBt;
    QPushButton *mTestBt;
    Q3ListView   *mSaverListView;
    QSpinBox	*mWaitEdit;
    QSpinBox    *mWaitLockEdit;
    QCheckBox   *mLockCheckBox;
    QCheckBox   *mStarsCheckBox;
    QCheckBox   *mEnabledCheckBox;
    QCheckBox	*mDPMSDependentCheckBox;
    QLabel      *mMonitorLabel;
    QLabel      *mActivateLbl;
    QLabel      *mLockLbl;
    QStringList mSaverFileList;
    SaverList   mSaverList;
    QTimer      *mLoadTimer;
    Q3GroupBox   *mSaverGroup;
    Q3GroupBox   *mSettingsGroup;

    int         mSelected;
    int         mPrevSelected;
    int		mNumLoaded;
    bool        mChanged;
    bool	mTesting;

    // Settings
    int         mTimeout;
    int         mLockTimeout;
    bool	mDPMS;
    bool        mLock;
    bool        mEnabled;
    QString     mSaver;
    bool        mImmutable;
};

#endif
