//-----------------------------------------------------------------------------
//
// KDE Display screen saver setup module
//
// Copyright (c)  Martin R. Jones 1996,1999
//
// Converted to a kcc module by Matthias Hoelzer 1997
//


#include <config.h>

#include <stdio.h>
#include <stdlib.h>
#include <signal.h>

#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif

#include <sys/types.h>
#include <sys/wait.h>

// X11 headers
#undef Above
#undef Below
#undef None

#include <qbuttongroup.h>
#include <qlabel.h>
#include <qpixmap.h>
#include <qpushbutton.h>
#include <qradiobutton.h>
#include <qcheckbox.h>
#include <qslider.h>
#include <qlayout.h>
#include <qtextstream.h>
#include <qwhatsthis.h>

#include <kapp.h>
#include <kprocess.h>
#include <ksimpleconfig.h>
#include <knuminput.h>
#include <kstddirs.h>
#include <klocale.h>
#include <kdesktopfile.h>
#include <kcolordlg.h>
#include <kiconloader.h>
#include <kcmodule.h>
#include <kglobal.h>
#include <dcopclient.h>

#include <X11/Xlib.h>

#include "scrnsave.h"

template class QList<SaverConfig>;

//===========================================================================
// DLL Interface for kcontrol

extern "C" {
    KCModule *create_screensaver(QWidget *parent, const char *name) {
	KGlobal::locale()->insertCatalogue("kcmdisplay");
	return new KScreenSaver(parent, name);
    }
}

//===========================================================================
//
//
SaverConfig::SaverConfig()
{
}

bool SaverConfig::read(QString file)
{
    KDesktopFile config(file, true);
    mExec = config.readEntry("Exec");
    mName = config.readEntry("Name");

    if (config.hasActionGroup("Setup"))
    {
      config.setActionGroup("Setup");
      mSetup = config.readEntry("Exec");
    }

    if (config.hasActionGroup("InWindow"))
    {
      config.setActionGroup("InWindow");
      mSaver = config.readEntry("Exec");
    }
    
    int indx = file.findRev('/');
    if (indx >= 0) {
        mFile = file.mid(indx+1);
    }

    return !mSaver.isEmpty();
}

//===========================================================================
//
int SaverList::compareItems(QCollection::Item item1, QCollection::Item item2)
{
    SaverConfig *s1 = (SaverConfig *)item1;
    SaverConfig *s2 = (SaverConfig *)item2;

    return s1->name().compare(s2->name());
}

//===========================================================================
//
TestWin::TestWin()
    : QWidget(0, 0, WStyle_Customize | WStyle_NoBorder)
{
    setFocusPolicy(StrongFocus);
}

void TestWin::mousePressEvent(QMouseEvent *)
{
    emit stopTest();
}

void TestWin::keyPressEvent(QKeyEvent *)
{
    emit stopTest();
}


//===========================================================================
//
KScreenSaver::KScreenSaver(QWidget *parent, const char *name)
	: KCModule(parent, name)
{
    mSetupProc = 0;
    mPreviewProc = 0;
    mTestWin = 0;
    mTestProc = 0;
    mPrevSelected = -1;
    mMonitor = 0;
     
    // Add non-KDE path
    KGlobal::dirs()->addResourceType("scrsav",
                                     KGlobal::dirs()->kde_default("apps") +
                                     "apps/ScreenSavers/");

    // Add KDE specific screensaver path
    KGlobal::dirs()->addResourceType("scrsav",
                                     KGlobal::dirs()->kde_default("apps") +
                                     "System/ScreenSavers/");
    
    readSettings();
	
    mSetupProc = new KProcess;
    connect(mSetupProc, SIGNAL(processExited(KProcess *)),
            this, SLOT(slotSetupDone(KProcess *)));
    
    mPreviewProc = new KProcess;
    connect(mPreviewProc, SIGNAL(processExited(KProcess *)),
            this, SLOT(slotPreviewExited(KProcess *)));
    
    findSavers();
    
    QBoxLayout *topLayout = new QHBoxLayout(this, 10, 10);

    // left column
    QBoxLayout *vLayout = new QVBoxLayout(this, 0, 10);
    topLayout->addLayout(vLayout);

    mEnableCheckBox = new QCheckBox( i18n("&Enable screensaver"), this );
    mEnableCheckBox->setChecked( mEnabled );
    connect( mEnableCheckBox, SIGNAL( toggled( bool ) ), 
	     this, SLOT( slotEnable( bool ) ) );
    vLayout->addWidget(mEnableCheckBox);
    QWhatsThis::add( mEnableCheckBox, i18n("Check this box if you would like"
      " to enable a screen saver. If you have power saving features enabled"
      " for your display, you may still enable a screen saver.") );
    
    QGroupBox *group = new QGroupBox(i18n("Screen Saver"), this );
    vLayout->addWidget(group);
    QBoxLayout *groupLayout = new QVBoxLayout( group, 10 );
    groupLayout->addSpacing(10);		
    
    mSaverListBox = new QListBox( group );
    /* mSaverListBox->setFixedHeight(120); */

    SaverConfig *saver;
    mSelected = 0;
    for (saver = mSaverList.first(); saver != 0; saver = mSaverList.next())
    {
        mSaverListBox->insertItem(saver->name());
        if (saver->file() == mSaver) 
            mSelected = mSaverListBox->count()-1;
    }

    mSaverListBox->setCurrentItem(mSelected);
    mSaverListBox->setTopItem(mSaverListBox->currentItem());
    mSaverListBox->setEnabled(mEnabled);
    mSelected = mSaverListBox->currentItem();
    connect( mSaverListBox, SIGNAL( highlighted( int ) ),
             this, SLOT( slotScreenSaver( int ) ) );
    groupLayout->addWidget( mSaverListBox, 10 );
    QWhatsThis::add( mSaverListBox, i18n("This is a list of the available"
      " screen savers. Select the one you want to use.") );

    QBoxLayout* hlay = new QHBoxLayout(groupLayout, 10);
    mSetupBt = new QPushButton(  i18n("&Setup ..."), group );
    connect( mSetupBt, SIGNAL( clicked() ), SLOT( slotSetup() ) );
    mSetupBt->setEnabled(mEnabled &&
                         !mSaverList.at(mSelected)->setup().isEmpty());
    hlay->addWidget( mSetupBt );
    QWhatsThis::add( mSetupBt, i18n("If the screen saver you selected has"
      " customizable features, you can set them up by clicking this button.") );
    
    mTestBt = new QPushButton(  i18n("&Test"), group );
    connect( mTestBt, SIGNAL( clicked() ), SLOT( slotTest() ) );
    mTestBt->setEnabled(mEnabled);
    hlay->addWidget( mTestBt );
    QWhatsThis::add( mTestBt, i18n("You can try out the screen saver by clicking"
      " this button. (Also, the preview image shows you what the screen saver"
      " will look like.)") );

    // right column
    vLayout = new QVBoxLayout(this, 0, 10);
    topLayout->addLayout(vLayout);

    mMonitorLabel = new QLabel( this );
    mMonitorLabel->setAlignment( AlignCenter );
    mMonitorLabel->setPixmap( QPixmap(locate("data",
					     "kcontrol/pics/monitor.png")));
    vLayout->addWidget(mMonitorLabel, 0);
    QWhatsThis::add( mMonitorLabel, i18n("Here you can see a preview of the selected screen saver.") );

    group = new QGroupBox( i18n("Settings"), this );
    vLayout->addWidget( group );
    groupLayout = new QVBoxLayout( group, 10, 10 );
    groupLayout->addSpacing(10);

    QBoxLayout *hbox = new QHBoxLayout();
    groupLayout->addLayout(hbox);
    QLabel *lbl = new QLabel(i18n("&Wait for"), group);
    hbox->addWidget(lbl);
    mWaitEdit = new QSpinBox(group);
    mWaitEdit->setSteps(1, 10);
    mWaitEdit->setRange(1, 120);
    mWaitEdit->setSuffix(i18n(" min."));
    mWaitEdit->setValue(mTimeout/60);
    mWaitEdit->setEnabled(mEnabled);
    connect(mWaitEdit, SIGNAL(valueChanged(int)), SLOT(slotTimeoutChanged(int)));
    lbl->setBuddy(mWaitEdit);
    hbox->addWidget(mWaitEdit);
    QString wtstr = i18n("Choose the period of inactivity (from 1"
      " to 120 minutes) after which the screen saver should start.");
    QWhatsThis::add( lbl, wtstr );
    QWhatsThis::add( mWaitEdit, wtstr );

    groupLayout->addStretch(1);

    mLockCheckBox = new QCheckBox( i18n("&Require password"), group );
    mLockCheckBox->setChecked( mLock );
    mLockCheckBox->setEnabled( mEnabled );
    connect( mLockCheckBox, SIGNAL( toggled( bool ) ), 
	     this, SLOT( slotLock( bool ) ) );
    groupLayout->addWidget(mLockCheckBox);
    QWhatsThis::add( mLockCheckBox, i18n("If you check this option, the display"
      " will be locked when the screen saver starts. To restore the display,"
      " enter your account password at the prompt.") );

    mStarsCheckBox = new QCheckBox( i18n("Show p&assword as stars"), group );
    mStarsCheckBox->setChecked(mPasswordStars);
    mStarsCheckBox->setEnabled(mEnabled);
    connect( mStarsCheckBox, SIGNAL( toggled( bool ) ), 
	     this, SLOT( slotStars( bool ) ) );
    groupLayout->addWidget(mStarsCheckBox);
    QWhatsThis::add( mStarsCheckBox, i18n("If you check this option and the one"
      " above, each character you type in the password will be echoed as an"
      " asterisk (*) symbol. Otherwise, there will be no visual feedback.") );

    groupLayout->addStretch(1);

    QGridLayout *gl = new QGridLayout(groupLayout, 2, 4);
    gl->setColStretch( 2, 10 );
    
    lbl = new QLabel(i18n("&Priority"), group);
    gl->addWidget(lbl, 0, 0);

    mPrioritySlider = new QSlider(QSlider::Horizontal, group);
    mPrioritySlider->setRange(0, 19);
    mPrioritySlider->setSteps(1, 5);
    mPrioritySlider->setValue(19 - mPriority);
    mPrioritySlider->setEnabled( mEnabled );
    connect(mPrioritySlider, SIGNAL( valueChanged(int)),
	    SLOT(slotPriorityChanged(int)));
    lbl->setBuddy(mPrioritySlider);
    gl->addMultiCellWidget(mPrioritySlider, 0, 0, 1, 3);
    QWhatsThis::add( mPrioritySlider, i18n("Use this slider to change the"
      " processing priority for the screen saver over other jobs that are"
      " being executed in the background. For a processor-intensive screen"
      " saver, setting a higher priority may make the display smoother at"
      " the expense of other jobs.") );

#ifndef HAVE_SETPRIORITY
    lbl->setEnabled(false);
    mPrioritySlider->setEnabled(false);
#endif
    lbl = new QLabel(i18n("Low Priority", "Low"), group);
    gl->addWidget(lbl, 1, 1);
    
    lbl = new QLabel(i18n("High Priority", "High"), group);
    gl->addWidget(lbl, 1, 3);
    
#ifndef HAVE_SETPRIORITY
    lbl->setEnabled(false);
#endif

    groupLayout->addStretch(1);

    topLayout->activate();

    setMonitor();
}

//---------------------------------------------------------------------------
//
void KScreenSaver::resizeEvent( QResizeEvent * )
{
  
  if (mMonitor)
    {
      mMonitor->setGeometry( (mMonitorLabel->width()-200)/2+20,
			     (mMonitorLabel->height()-160)/2+10, 157, 111 );
    }
}

//---------------------------------------------------------------------------
//
int KScreenSaver::buttons()
{
    return KCModule::Help | KCModule::Default | KCModule::Reset |
	   KCModule::Cancel | KCModule::Apply | KCModule::Ok;
}

//---------------------------------------------------------------------------
//
KScreenSaver::~KScreenSaver()
{
    if (mPreviewProc)
    {
        if (mPreviewProc->isRunning())
        {
            int pid = mPreviewProc->getPid();  
            mPreviewProc->kill( );
            waitpid(pid, (int *) 0,0);
        }
        delete mPreviewProc;
    }

    delete mTestProc;
    delete mSetupProc;
    delete mTestWin;
}

//---------------------------------------------------------------------------
//
void KScreenSaver::load()
{
    readSettings();

    SaverConfig *saver;
    mSelected = 0;
    for (saver = mSaverList.first(); saver != 0; saver = mSaverList.next()) {
        if (saver->file() == mSaver) 
            mSelected = mSaverListBox->count()-1;
    }
    mSaverListBox->setCurrentItem(mSelected);
    slotScreenSaver(mSelected);

    updateValues();
    emit changed(false);
}
    
//---------------------------------------------------------------------------
//
void KScreenSaver::readSettings()
{
    KConfig *config = new KConfig( "kdesktoprc");
    config->setGroup( "ScreenSaver" );

    mEnabled = config->readBoolEntry("Enabled", false);
    mLock = config->readBoolEntry("Lock", false);
    mTimeout = config->readNumEntry("Timeout", 300);
    mPriority = config->readNumEntry("Priority", 19);
    mPasswordStars = config->readBoolEntry("PasswordAsStars", true);
    mSaver = config->readEntry("Saver");

    if (mPriority < 0) mPriority = 0;
    if (mPriority > 19) mPriority = 19;
    if (mTimeout < 60) mTimeout = 60;

    mChanged = false;
    delete config;
}

//---------------------------------------------------------------------------
//
void KScreenSaver::updateValues()
{
    mWaitEdit->setValue(mTimeout/60);
    mLockCheckBox->setChecked(mLock);
    mStarsCheckBox->setChecked(mPasswordStars);
    mPrioritySlider->setValue(mPriority);
    mEnableCheckBox->setChecked( mEnabled );
}

//---------------------------------------------------------------------------
//
void KScreenSaver::defaults()
{
    slotScreenSaver( 0 );
    mSaverListBox->setCurrentItem( 0 );
    mSaverListBox->centerCurrentItem();
    slotEnable( false );
    slotTimeoutChanged( 1 );
    slotPriorityChanged( 0 );
    slotLock( false );
    slotStars( true );
    updateValues();

    emit changed(true);
}

//---------------------------------------------------------------------------
//
void KScreenSaver::save()
{
    if ( !mChanged )
	return;

    KConfig *config = new KConfig( "kdesktoprc");
    config->setGroup( "ScreenSaver" );

    config->writeEntry("Enabled", mEnabled);
    config->writeEntry("Timeout", mTimeout);
    config->writeEntry("Lock", mLock);
    config->writeEntry("Priority", mPriority);
    config->writeEntry("PasswordAsStars", mPasswordStars);
    config->writeEntry("Saver", mSaver);
    config->sync();
    delete config;

    // TODO (GJ): When you changed anything, these two lines will give a segfault 
    // on exit. I don't know why yet.

    DCOPClient *client = kapp->dcopClient();
    client->send("kdesktop", "KScreensaverIface", "configure()", "");

    mChanged = false;
    emit changed(false);
}

//---------------------------------------------------------------------------
//
void KScreenSaver::findSavers()
{
    QStringList saverFileList = KGlobal::dirs()->findAllResources("scrsav",
						"*.desktop", false, true);

    QStringList::Iterator it = saverFileList.begin();
    for ( ; it != saverFileList.end(); ++it ) {
	SaverConfig *saver = new SaverConfig;
	if (saver->read(*it))
	    mSaverList.append(saver);
	else 
	    delete saver;
    }

    mSaverList.sort();
}

//---------------------------------------------------------------------------
//
void KScreenSaver::setMonitor()
{
    if (mPreviewProc->isRunning())
	// CC: this will automatically cause a "slotPreviewExited"
	// when the viewer exits
	mPreviewProc->kill();
    else
	slotPreviewExited(mPreviewProc);
}

//---------------------------------------------------------------------------
//
void KScreenSaver::slotPreviewExited(KProcess *)
{
    // Ugly hack to prevent continual respawning of savers that crash
    if (mSelected == mPrevSelected)
        return;

    // Some xscreensaver hacks do something nasty to the window that
    // requires a new one to be created (or proper investigation of the
    // problem).
    if (mMonitor)
        delete mMonitor;

    mMonitor = new KSSMonitor(mMonitorLabel);
    mMonitor->setBackgroundColor(black);
    mMonitor->setGeometry((mMonitorLabel->width()-200)/2+20,
                          (mMonitorLabel->height()-160)/2+10, 157, 111);
    mMonitor->show();

    if (mEnabled) {
        mPreviewProc->clearArguments();
        
        QString saver = mSaverList.at(mSelected)->saver();
        QTextStream ts(&saver, IO_ReadOnly);
        
        QString word;
        ts >> word;
        QString path = KStandardDirs::findExe(word);

        if (!path.isEmpty())
        {
            (*mPreviewProc) << path;

            while (!ts.atEnd())
            {
                ts >> word;
                if (word == "%w")
                {
                    word = word.setNum(mMonitor->winId());
                }
                (*mPreviewProc) << word;
            }

            mPreviewProc->start();
        }
    }

    mPrevSelected = mSelected;
}

//---------------------------------------------------------------------------
//
void KScreenSaver::slotEnable(bool e)
{
    if ( !e ) {
	mSetupBt->setEnabled( false );
	mEnabled = false;
    } else {
	if (!mSetupProc->isRunning())
	    mSetupBt->setEnabled(!mSaverList.at(mSelected)->setup().isEmpty());
	mEnabled = true;
    }

    mSaverListBox->setEnabled( e );
    mTestBt->setEnabled( e );
    mWaitEdit->setEnabled( e );
    mLockCheckBox->setEnabled( e );
    mStarsCheckBox->setEnabled( e );
#ifdef HAVE_SETPRIORITY
    mPrioritySlider->setEnabled( e );
#endif

    mPrevSelected = -1;  // see ugly hack in slotPreviewExited()
    setMonitor();
    mChanged = true;
    emit changed(true);
}

//---------------------------------------------------------------------------
//
void KScreenSaver::slotScreenSaver(int indx)
{
    if (!mEnabled)
        return;

    if (!mSetupProc->isRunning())
        mSetupBt->setEnabled(!mSaverList.at(indx)->setup().isEmpty());
    mTestBt->setEnabled(true);
    mSaver = mSaverList.at(indx)->file();
    mEnabled = true;
    
    mSelected = indx;
    
    setMonitor();
    mChanged = true;
    emit changed(true);
}

//---------------------------------------------------------------------------
//
void KScreenSaver::slotSetup()
{
    if ( !mEnabled )
	return;

    if (mSetupProc->isRunning())
	return;

    mSetupProc->clearArguments();

    QString saver = mSaverList.at(mSelected)->setup();
    QTextStream ts(&saver, IO_ReadOnly);

    QString word;
    ts >> word;
    QString path = KStandardDirs::findExe(word);

    if (!path.isEmpty())
    {
        (*mSetupProc) << path;

        while (!ts.atEnd())
        {
            ts >> word;
            (*mSetupProc) << word;
        }

        mSetupBt->setEnabled( FALSE );
        kapp->flushX();

        mSetupProc->start();
    }
}

//---------------------------------------------------------------------------
//
void KScreenSaver::slotTest()
{
    if (!mTestProc) {
	    mTestProc = new KProcess;
    }

    mTestProc->clearArguments();
    QString saver = mSaverList.at(mSelected)->saver();
    QTextStream ts(&saver, IO_ReadOnly);

    QString word;
    ts >> word;
    QString path = KStandardDirs::findExe(word);

    if (!path.isEmpty())
    {
        (*mTestProc) << path;

        if (!mTestWin)
        {
            mTestWin = new TestWin();
            mTestWin->setBackgroundMode(QWidget::NoBackground);
            mTestWin->setGeometry(0, 0, kapp->desktop()->width(),
                                    kapp->desktop()->height());
            connect(mTestWin, SIGNAL(stopTest()), SLOT(slotStopTest()));
        }

        mTestWin->show();
        mTestWin->raise();
        mTestWin->setFocus();
        mTestWin->grabKeyboard();

        mTestBt->setEnabled( FALSE );

        while (!ts.atEnd())
        {
            ts >> word;
            if (word == "%w")
            {
                word = word.setNum(mTestWin->winId());
            }
            (*mTestProc) << word;
        }

        mTestProc->start(KProcess::DontCare);
    }
}

//---------------------------------------------------------------------------
//
void KScreenSaver::slotStopTest()
{
    if (mTestProc->isRunning()) {
        mTestProc->kill();
    }
    mTestWin->releaseKeyboard();
    mTestWin->hide();
    mTestBt->setEnabled(true);
}

//---------------------------------------------------------------------------
//
void KScreenSaver::slotTimeoutChanged(int to )
{
    mTimeout = to * 60;
    mChanged = true;
    emit changed(true);
}

//---------------------------------------------------------------------------
//
void KScreenSaver::slotLock( bool l )
{
    mLock = l;
    mChanged = true;
    emit changed(true);
}

//---------------------------------------------------------------------------
//
void KScreenSaver::slotStars( bool s )
{
    mPasswordStars = s;
    mChanged = true;
    emit changed(true);
}

//---------------------------------------------------------------------------
//
void KScreenSaver::slotPriorityChanged( int val )
{
    if (val == mPriority)
	return;
    
    mPriority = 19 - val;
    if (mPriority > 19)
	mPriority = 19;
    else if (mPriority < 0)
	mPriority = 0;

    mChanged = true;
    emit changed(true);
}

//---------------------------------------------------------------------------
//
void KScreenSaver::slotSetupDone(KProcess *)
{
    mPrevSelected = -1;  // see ugly hack in slotPreviewExited()
    setMonitor();
    mSetupBt->setEnabled( true );
}

//---------------------------------------------------------------------------
//
QString KScreenSaver::quickHelp()
{
    return i18n("<h1>Screen saver</h1> This module allows you to enable and"
       " configure a screen saver. Note that you can enable a screen saver"
       " even if you have power saving features enabled for your display.<p>"
       " Besides providing an endless variety of entertainment and"
       " preventing monitor burn-in, a screen saver also gives you a simple"
       " way to lock your display if you are going to leave it unattended"
       " for a while. If you want the screen saver to lock the screen, make sure you enable"
       " the \"Require password\" feature of the screen saver. If you don't, you can still"
       " explicitly lock the screen using the desktop's \"Lock Screen\" action.");
}

#include "scrnsave.moc"
