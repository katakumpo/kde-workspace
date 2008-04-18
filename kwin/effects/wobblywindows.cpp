/*****************************************************************
 KWin - the KDE window manager
 This file is part of the KDE project.

Copyright (C) 2008 Cédric Borgese <cedric.borgese@gmail.com>

You can Freely distribute this program under the GNU General Public
License. See the file "COPYING" for the exact licensing terms.
******************************************************************/


#include "wobblywindows.h"
#include "wobblywindows_constants.h"

#include <kdebug.h>
#include <kconfiggroup.h>
#include <math.h>

#define USE_ASSERT
#ifdef USE_ASSERT
#define ASSERT1 assert
#else
#define ASSERT1
#endif

//#define COMPUTE_STATS

// if you enable it and run kwin in a terminal from the session it manages,
// be sure to redirect the output of kwin in a file or
// you'll propably get deadlocks.
//#define VERBOSE_MODE

#if defined COMPUTE_STATS && !defined VERBOSE_MODE
#   ifdef __GNUC__
#       warning "You enable COMPUTE_STATS without VERBOSE_MODE, computed stats will not be printed."
#   endif
#endif

namespace KWin
{

KWIN_EFFECT(wobblywindows, WobblyWindowsEffect)

WobblyWindowsEffect::WobblyWindowsEffect()
{
    KConfigGroup conf = effects->effectConfig("Wobbly");

    m_raideur = conf.readEntry("Raideur", RAIDEUR);
    m_amortissement = conf.readEntry("Amortissement", AMORTISSEMENT);
    m_move_factor = conf.readEntry("MoveFactor", MOVEFACTOR);

    m_xTesselation = conf.readEntry("XTesselation", XTESSELATION);
    m_yTesselation = conf.readEntry("YTesselation", YTESSELATION);

    m_minVelocity = conf.readEntry("MinVelocity", MINVELOCITY);
    m_maxVelocity = conf.readEntry("MaxVelocity", MAXVELOCITY);
    m_stopVelocity = conf.readEntry("StopVelocity", STOPVELOCITY);
    m_minAcceleration = conf.readEntry("MinAcceleration", MINACCELERATION);
    m_maxAcceleration = conf.readEntry("MaxAcceleration", MAXACCELERATION);
    m_stopAcceleration = conf.readEntry("StopAcceleration", STOPACCELERATION);

    QString velFilter = conf.readEntry("VelocityFilter", VELOCITYFILTER);
    if (velFilter == "NoFilter")
    {
        m_velocityFilter = NoFilter;
    }
    else if (velFilter == "FourRingLinearMean")
    {
        m_velocityFilter = FourRingLinearMean;
    }
    else if (velFilter == "MeanWithMean")
    {
        m_velocityFilter = MeanWithMean;
    }
    else if (velFilter == "MeanWithMedian")
    {
        m_velocityFilter = MeanWithMedian;
    }
    else
    {
        m_velocityFilter = FourRingLinearMean;
        kDebug() << "Unknown config value for VelocityFilter : " << velFilter;
    }


    QString accFilter = conf.readEntry("AccelerationFilter", ACCELERATIONFILTER);
    if (accFilter == "NoFilter")
    {
        m_accelerationFilter = NoFilter;
    }
    else if (accFilter == "FourRingLinearMean")
    {
        m_accelerationFilter = FourRingLinearMean;
    }
    else if (accFilter == "MeanWithMean")
    {
        m_accelerationFilter = MeanWithMean;
    }
    else if (accFilter == "MeanWithMedian")
    {
        m_accelerationFilter = MeanWithMedian;
    }
    else
    {
        m_accelerationFilter = NoFilter;
        kDebug() << "Unknown config value for accelerationFilter : " << accFilter;
    }

#if defined VERBOSE_MODE
    kDebug() << "Parameters :\n" <<
        "grid(" << m_raideur << ", " << m_amortissement << ", " << m_move_factor << ")\n" <<
        "velocity(" << m_minVelocity << ", " << m_maxVelocity << ", " << m_stopVelocity << ")\n" <<
        "acceleration(" << m_minAcceleration << ", " << m_maxAcceleration << ", " << m_stopAcceleration << ")\n" <<
        "tesselation(" << m_xTesselation <<  ", " << m_yTesselation << ")";
#endif
}

WobblyWindowsEffect::~WobblyWindowsEffect()
{
    if (windows.empty())
    {
        // we should be empty at this point...
        // emit a warning and clean the list.
        kDebug() << "Windows list not empty. Left items : " << windows.count();
        QHash< const EffectWindow*,  WindowWobblyInfos >::iterator i;
        for (i = windows.begin(); i != windows.end(); ++i)
        {
            freeWobblyInfo(i.value());
        }
    }
}
void WobblyWindowsEffect::setVelocityThreshold(qreal m_minVelocity)
{
    this->m_minVelocity = m_minVelocity;
}

void WobblyWindowsEffect::setMoveFactor(qreal factor)
{
    m_move_factor = factor;
}

void WobblyWindowsEffect::setRaideur(qreal m_raideur)
{
    this->m_raideur = m_raideur;
}

void WobblyWindowsEffect::setVelocityFilter(GridFilter filter)
{
    m_velocityFilter = filter;
}

void WobblyWindowsEffect::setAccelerationFilter(GridFilter filter)
{
    m_accelerationFilter = filter;
}

WobblyWindowsEffect::GridFilter WobblyWindowsEffect::velocityFilter() const
{
    return m_velocityFilter;
}

WobblyWindowsEffect::GridFilter WobblyWindowsEffect::accelerationFilter() const
{
    return m_accelerationFilter;
}

void WobblyWindowsEffect::setAmortissement(qreal m_amortissement)
{
    this->m_amortissement = m_amortissement;
}

void WobblyWindowsEffect::prePaintScreen(ScreenPrePaintData& data, int time)
{
    // We need to mark the screen windows as transformed. Otherwise the whole
    // screen won't be repainted, resulting in artefacts.
    // Could we just set a subset of the screen to be repainted ?
    if (windows.count() != 0)
    {
        data.mask |= PAINT_SCREEN_WITH_TRANSFORMED_WINDOWS;
    }

    // this set the QRect invalid.
    m_updateRegion.setWidth(0);

    effects->prePaintScreen(data, time);
}
const qreal maxTime = 10.0;
void WobblyWindowsEffect::prePaintWindow(EffectWindow* w, WindowPrePaintData& data, int time)
{
    if (windows.contains(w))
    {
        data.setTransformed();
        data.quads = data.quads.makeRegularGrid(m_xTesselation, m_yTesselation);
        bool stop = false;
        qreal updateTime = time;

        while (!stop && (updateTime > maxTime))
        {
#if defined VERBOSE_MODE
            kDebug() << "loop time " << updateTime << " / " << time;
#endif
            stop = !updateWindowWobblyDatas(w, maxTime);
            updateTime -= maxTime;
        }
        if (!stop && updateTime > 0)
        {
            updateWindowWobblyDatas(w, updateTime);
        }
    }

    effects->prePaintWindow(w, data, time);
}

void WobblyWindowsEffect::paintWindow(EffectWindow* w, int mask, QRegion region, WindowPaintData& data)
{
    if(windows.contains(w))
    {
        WindowWobblyInfos& wwi = windows[w];
        int tx = w->geometry().x();
        int ty = w->geometry().y();
        for (int i = 0; i < data.quads.count(); ++i)
        {
            for(int j = 0; j < 4; ++j)
            {
                WindowVertex& v = data.quads[i][j];
                Pair oldPos = {tx + v.x(), ty + v.y()};
                Pair newPos = computeBezierPoint(wwi, oldPos);
                v.move(newPos.x - tx, newPos.y - ty);
            }
        }
    }

    // Call the next effect.
    effects->paintWindow(w, mask, region, data);
}

void WobblyWindowsEffect::postPaintScreen()
{
    if (!windows.isEmpty())
    {
        effects->addRepaint(m_updateRegion);
    }

    // Call the next effect.
    effects->postPaintScreen();
}

void WobblyWindowsEffect::windowUserMovedResized(EffectWindow* w, bool first, bool last)
{
    if (first && !w->isSpecialWindow())
    {
        if (!windows.contains(w))
        {
            WindowWobblyInfos new_wwi;
            initWobblyInfo(new_wwi, w->geometry());
            windows[w] = new_wwi;
        }

        WindowWobblyInfos& wwi = windows[w];
        wwi.onConstrain = true;
        const QRectF& rect = w->geometry();

        qreal x_increment = rect.width() / (wwi.width-1.0);
        qreal y_increment = rect.height() / (wwi.height-1.0);

        Pair picked = {cursorPos().x(), cursorPos().y()};
        int indx = (picked.x - rect.x()) / x_increment;
        int indy = (picked.y - rect.y()) / y_increment;
        int pickedPointIndex = indy*wwi.width + indx;
        if (pickedPointIndex < 0)
        {
            kDebug() << "Picked index == " << pickedPointIndex << " with (" << cursorPos().x() << "," << cursorPos().y() << ")";
            pickedPointIndex = 0;
        }
        else if (static_cast<unsigned int>(pickedPointIndex) > wwi.count - 1)
        {
            kDebug() << "Picked index == " << pickedPointIndex << " with (" << cursorPos().x() << "," << cursorPos().y() << ")";
            pickedPointIndex = wwi.count - 1;
        }
#if defined VERBOSE_MODE
        kDebug() << "Original Picked point -- x : " << picked.x << " - y : " << picked.y;
#endif
        wwi.constraint[pickedPointIndex] = true;
    }
    else if (last)
    {
        if (windows.contains(w))
        {
            WindowWobblyInfos& wwi = windows[w];
            wwi.onConstrain = false;
        }
    }
}

void WobblyWindowsEffect::windowClosed(EffectWindow* w)
{
    if(windows.contains(w))
    {
        WindowWobblyInfos& wwi = windows[w];
        freeWobblyInfo(wwi);
        windows.remove(w);
    }
}

void WobblyWindowsEffect::initWobblyInfo(WindowWobblyInfos& wwi, QRect geometry) const
{
    wwi.count = 4*4;
    wwi.width = 4;
    wwi.height = 4;

    wwi.bezierWidth = m_xTesselation;
    wwi.bezierHeight = m_yTesselation;
    wwi.bezierCount = m_xTesselation * m_yTesselation;

    wwi.origin = new Pair[wwi.count];
    wwi.position = new Pair[wwi.count];
    wwi.velocity = new Pair[wwi.count];
    wwi.acceleration = new Pair[wwi.count];
    wwi.buffer = new Pair[wwi.count];
    wwi.constraint = new bool[wwi.count];

    wwi.bezierSurface = new Pair[wwi.bezierCount];

    wwi.onConstrain = true;

    qreal x = geometry.x(), y = geometry.y();
    qreal width = geometry.width(), height = geometry.height();

    Pair initValue = {x, y};
    static const Pair nullPair = {0.0, 0.0};

    qreal x_increment = width / (wwi.width-1.0);
    qreal y_increment = height / (wwi.height-1.0);

    for (unsigned int j=0; j<4; ++j)
    {
        for (unsigned int i=0; i<4; ++i)
        {
            unsigned int idx = j*4 + i;
            wwi.origin[idx] = initValue;
            wwi.position[idx] = initValue;
            wwi.velocity[idx] = nullPair;
            wwi.constraint[idx] = false;
            if (i != 4-2) // x grid count - 2, i.e. not the last point
            {
                initValue.x += x_increment;
            }
            else
            {
                initValue.x = width + x;
            }
            initValue.x = initValue.x;
        }
        initValue.x = x;
        initValue.x = initValue.x;
        if (j != 4-2) // y grid count - 2, i.e. not the last point
        {
            initValue.y += y_increment;
        }
        else
        {
            initValue.y = height + y;
        }
        initValue.y = initValue.y;
    }
}

void WobblyWindowsEffect::freeWobblyInfo(WindowWobblyInfos& wwi) const
{
    delete wwi.origin;
    delete wwi.position;
    delete wwi.velocity;
    delete wwi.acceleration;
    delete wwi.buffer;
    delete wwi.constraint;

    delete wwi.bezierSurface;
}

WobblyWindowsEffect::Pair WobblyWindowsEffect::computeBezierPoint(const WindowWobblyInfos& wwi, Pair point) const
{
    // compute the input value
    Pair topleft = wwi.origin[0];
    Pair bottomright = wwi.origin[wwi.count-1];

    ASSERT1(point.x >= topleft.x);
    ASSERT1(point.y >= topleft.y);
    ASSERT1(point.x <= bottomright.x);
    ASSERT1(point.y <= bottomright.y);

    qreal tx = (point.x - topleft.x) / (bottomright.x - topleft.x);
    qreal ty = (point.y - topleft.y) / (bottomright.y - topleft.y);

    ASSERT1(tx >= 0);
    ASSERT1(tx <= 1);
    ASSERT1(ty >= 0);
    ASSERT1(ty <= 1);

    // compute polinomial coeff

    qreal px[4];
    px[0] = (1-tx)*(1-tx)*(1-tx);
    px[1] = 3*(1-tx)*(1-tx)*tx;
    px[2] = 3*(1-tx)*tx*tx;
    px[3] = tx*tx*tx;

    qreal py[4];
    py[0] = (1-ty)*(1-ty)*(1-ty);
    py[1] = 3*(1-ty)*(1-ty)*ty;
    py[2] = 3*(1-ty)*ty*ty;
    py[3] = ty*ty*ty;

    Pair res = {0.0, 0.0};

    for (unsigned int j = 0; j < 4; ++j)
    {
        for (unsigned int i = 0; i < 4; ++i)
        {
            // this assume the grid is 4*4
            res.x += px[i] * py[j] * wwi.position[i + j * wwi.width].x;
            res.y += px[i] * py[j] * wwi.position[i + j * wwi.width].y;
        }
    }

    return res;
}
namespace
{

static inline void fixVectorBounds(WobblyWindowsEffect::Pair& vec, qreal min, qreal max)
{
    if (fabs(vec.x) < min)
    {
        vec.x = 0.0;
    }
    else if (fabs(vec.x) > max)
    {
        if (vec.x > 0.0)
        {
            vec.x = max;
        }
        else
        {
            vec.x = -max;
        }
    }

    if (fabs(vec.y) < min)
    {
        vec.y = 0.0;
    }
    else if (fabs(vec.y) > max)
    {
        if (vec.y > 0.0)
        {
            vec.y = max;
        }
        else
        {
            vec.y = -max;
        }
    }
}

static inline void computeVectorBounds(WobblyWindowsEffect::Pair& vec, WobblyWindowsEffect::Pair& bound)
{
    if (fabs(vec.x) < bound.x)
    {
        bound.x = fabs(vec.x);
    }
    else if (fabs(vec.x) > bound.y)
    {
        bound.y = fabs(vec.x);
    }
    if (fabs(vec.y) < bound.x)
    {
        bound.x = fabs(vec.y);
    }
    else if (fabs(vec.y) > bound.y)
    {
        bound.y = fabs(vec.y);
    }
}

} // close the anonymous namespace

bool WobblyWindowsEffect::updateWindowWobblyDatas(EffectWindow* w, qreal time)
{
    const QRectF& rect = w->geometry();
    WindowWobblyInfos& wwi = windows[w];

    qreal x_length = rect.width() / (wwi.width-1.0);
    qreal y_length = rect.height() / (wwi.height-1.0);

#if defined VERBOSE_MODE
    kDebug() << "time " << time;
    kDebug() << "increment x " << x_length << " // y" <<  y_length;
#endif

    Pair origine = {rect.x(), rect.y()};

    for (unsigned int j=0; j<wwi.height; ++j)
    {
        for (unsigned int i=0; i<wwi.width; ++i)
        {
            wwi.origin[wwi.width*j + i] = origine;
            if (i != wwi.width-2)
            {
                origine.x += x_length;
            }
            else
            {
                origine.x = rect.width() + rect.x();
            }
         }
        origine.x = rect.x();
        if (j != wwi.height-2)
        {
            origine.y += y_length;
        }
        else
        {
            origine.y = rect.height() + rect.y();
        }
    }

    Pair neibourgs[4];
    Pair acceleration;

    qreal acc_sum = 0.0;
    qreal vel_sum = 0.0;

    // compute acceleration, velocity and position for each point

    // for corners

    // top-left

    if (wwi.constraint[0])
    {
        Pair window_pos = wwi.origin[0];
        Pair current_pos = wwi.position[0];
        Pair move = {window_pos.x - current_pos.x, window_pos.y - current_pos.y};
        Pair accel = {move.x*m_raideur, move.y*m_raideur};
        wwi.acceleration[0] = accel;
    }
    else
    {
        Pair& pos = wwi.position[0];
        neibourgs[0] = wwi.position[1];
        neibourgs[1] = wwi.position[wwi.width];

        acceleration.x = ((neibourgs[0].x - pos.x) - x_length)*m_raideur + (neibourgs[1].x - pos.x)*m_raideur;
        acceleration.y = ((neibourgs[1].y - pos.y) - y_length)*m_raideur + (neibourgs[0].y - pos.y)*m_raideur;

        acceleration.x /= 2;
        acceleration.y /= 2;

        wwi.acceleration[0] = acceleration;
    }

    // top-right

    if (wwi.constraint[wwi.width-1])
    {
        Pair window_pos = wwi.origin[wwi.width-1];
        Pair current_pos = wwi.position[wwi.width-1];
        Pair move = {window_pos.x - current_pos.x, window_pos.y - current_pos.y};
        Pair accel = {move.x*m_raideur, move.y*m_raideur};
        wwi.acceleration[wwi.width-1] = accel;
    }
    else
    {
        Pair& pos = wwi.position[wwi.width-1];
        neibourgs[0] = wwi.position[wwi.width-2];
        neibourgs[1] = wwi.position[2*wwi.width-1];

        acceleration.x = (x_length - (pos.x - neibourgs[0].x))*m_raideur + (neibourgs[1].x - pos.x)*m_raideur;
        acceleration.y = ((neibourgs[1].y - pos.y) - y_length)*m_raideur + (neibourgs[0].y - pos.y)*m_raideur;

        acceleration.x /= 2;
        acceleration.y /= 2;

        wwi.acceleration[wwi.width-1] = acceleration;
    }

    // bottom-left

    if (wwi.constraint[wwi.width*(wwi.height-1)])
    {
        Pair window_pos = wwi.origin[wwi.width*(wwi.height-1)];
        Pair current_pos = wwi.position[wwi.width*(wwi.height-1)];
        Pair move = {window_pos.x - current_pos.x, window_pos.y - current_pos.y};
        Pair accel = {move.x*m_raideur, move.y*m_raideur};
        wwi.acceleration[wwi.width*(wwi.height-1)] = accel;
    }
    else
    {
        Pair& pos = wwi.position[wwi.width*(wwi.height-1)];
        neibourgs[0] = wwi.position[wwi.width*(wwi.height-1)+1];
        neibourgs[1] = wwi.position[wwi.width*(wwi.height-2)];

        acceleration.x = ((neibourgs[0].x - pos.x) - x_length)*m_raideur + (neibourgs[1].x - pos.x)*m_raideur;
        acceleration.y = (y_length - (pos.y - neibourgs[1].y))*m_raideur + (neibourgs[0].y - pos.y)*m_raideur;

        acceleration.x /= 2;
        acceleration.y /= 2;

        wwi.acceleration[wwi.width*(wwi.height-1)] = acceleration;
    }

    // bottom-right

    if (wwi.constraint[wwi.count-1])
    {
        Pair window_pos = wwi.origin[wwi.count-1];
        Pair current_pos = wwi.position[wwi.count-1];
        Pair move = {window_pos.x - current_pos.x, window_pos.y - current_pos.y};
        Pair accel = {move.x*m_raideur, move.y*m_raideur};
        wwi.acceleration[wwi.count-1] = accel;
    }
    else
    {
        Pair& pos = wwi.position[wwi.count-1];
        neibourgs[0] = wwi.position[wwi.count-2];
        neibourgs[1] = wwi.position[wwi.width*(wwi.height-1)-1];

        acceleration.x = (x_length - (pos.x - neibourgs[0].x))*m_raideur + (neibourgs[1].x - pos.x)*m_raideur;
        acceleration.y = (y_length - (pos.y - neibourgs[1].y))*m_raideur + (neibourgs[0].y - pos.y)*m_raideur;

        acceleration.x /= 2;
        acceleration.y /= 2;

        wwi.acceleration[wwi.count-1] = acceleration;
    }


    // for borders

    // top border
    for (unsigned int i=1; i<wwi.width-1; ++i)
    {
        if (wwi.constraint[i])
        {
            Pair window_pos = wwi.origin[i];
            Pair current_pos = wwi.position[i];
            Pair move = {window_pos.x - current_pos.x, window_pos.y - current_pos.y};
            Pair accel = {move.x*m_raideur, move.y*m_raideur};
            wwi.acceleration[i] = accel;
        }
        else
        {
            Pair& pos = wwi.position[i];
            neibourgs[0] = wwi.position[i-1];
            neibourgs[1] = wwi.position[i+1];
            neibourgs[2] = wwi.position[i+wwi.width];

            acceleration.x = (x_length - (pos.x - neibourgs[0].x))*m_raideur + ((neibourgs[1].x - pos.x) - x_length)*m_raideur + (neibourgs[2].x - pos.x)*m_raideur;
            acceleration.y = ((neibourgs[2].y - pos.y) - y_length)*m_raideur + (neibourgs[0].y - pos.y)*m_raideur + (neibourgs[1].y - pos.y)*m_raideur;

            acceleration.x /= 3;
            acceleration.y /= 3;

            wwi.acceleration[i] = acceleration;
        }
    }

    // bottom border
    for (unsigned int i=wwi.width*(wwi.height-1)+1; i<wwi.count-1; ++i)
    {
        if (wwi.constraint[i])
        {
            Pair window_pos = wwi.origin[i];
            Pair current_pos = wwi.position[i];
            Pair move = {window_pos.x - current_pos.x, window_pos.y - current_pos.y};
            Pair accel = {move.x*m_raideur, move.y*m_raideur};
            wwi.acceleration[i] = accel;
        }
        else
        {
            Pair& pos = wwi.position[i];
            neibourgs[0] = wwi.position[i-1];
            neibourgs[1] = wwi.position[i+1];
            neibourgs[2] = wwi.position[i-wwi.width];

            acceleration.x = (x_length - (pos.x - neibourgs[0].x))*m_raideur + ((neibourgs[1].x - pos.x) - x_length)*m_raideur + (neibourgs[2].x - pos.x)*m_raideur;
            acceleration.y = (y_length - (pos.y - neibourgs[2].y))*m_raideur + (neibourgs[0].y - pos.y)*m_raideur + (neibourgs[1].y - pos.y)*m_raideur;

            acceleration.x /= 3;
            acceleration.y /= 3;

            wwi.acceleration[i] = acceleration;
        }
    }

    // left border
    for (unsigned int i=wwi.width; i<wwi.width*(wwi.height-1); i+=wwi.width)
    {
        if (wwi.constraint[i])
        {
            Pair window_pos = wwi.origin[i];
            Pair current_pos = wwi.position[i];
            Pair move = {window_pos.x - current_pos.x, window_pos.y - current_pos.y};
            Pair accel = {move.x*m_raideur, move.y*m_raideur};
            wwi.acceleration[i] = accel;
        }
        else
        {
            Pair& pos = wwi.position[i];
            neibourgs[0] = wwi.position[i+1];
            neibourgs[1] = wwi.position[i-wwi.width];
            neibourgs[2] = wwi.position[i+wwi.width];

            acceleration.x = ((neibourgs[0].x - pos.x) - x_length)*m_raideur + (neibourgs[1].x - pos.x)*m_raideur + (neibourgs[2].x - pos.x)*m_raideur;
            acceleration.y = (y_length - (pos.y - neibourgs[1].y))*m_raideur + ((neibourgs[2].y - pos.y) - y_length)*m_raideur + (neibourgs[0].y - pos.y)*m_raideur;

            acceleration.x /= 3;
            acceleration.y /= 3;

            wwi.acceleration[i] = acceleration;
        }
    }

    // right border
    for (unsigned int i=2*wwi.width-1; i<wwi.count-1; i+=wwi.width)
    {
        if (wwi.constraint[i])
        {
            Pair window_pos = wwi.origin[i];
            Pair current_pos = wwi.position[i];
            Pair move = {window_pos.x - current_pos.x, window_pos.y - current_pos.y};
            Pair accel = {move.x*m_raideur, move.y*m_raideur};
            wwi.acceleration[i] = accel;
        }
        else
        {
            Pair& pos = wwi.position[i];
            neibourgs[0] = wwi.position[i-1];
            neibourgs[1] = wwi.position[i-wwi.width];
            neibourgs[2] = wwi.position[i+wwi.width];

            acceleration.x = (x_length - (pos.x - neibourgs[0].x))*m_raideur + (neibourgs[1].x - pos.x)*m_raideur + (neibourgs[2].x - pos.x)*m_raideur;
            acceleration.y = (y_length - (pos.y - neibourgs[1].y))*m_raideur + ((neibourgs[2].y - pos.y) - y_length)*m_raideur + (neibourgs[0].y - pos.y)*m_raideur;

            acceleration.x /= 3;
            acceleration.y /= 3;

            wwi.acceleration[i] = acceleration;
        }
    }

    // for the inner points
    for (unsigned int j=1; j<wwi.height-1; ++j)
    {
        for (unsigned int i=1; i<wwi.width-1; ++i)
        {
            unsigned int index = i+j*wwi.width;

            if (wwi.constraint[index])
            {
            Pair window_pos = wwi.origin[index];
            Pair current_pos = wwi.position[index];
            Pair move = {window_pos.x - current_pos.x, window_pos.y - current_pos.y};
            Pair accel = {move.x*m_raideur, move.y*m_raideur};
            wwi.acceleration[index] = accel;
            }
            else
            {
                Pair& pos = wwi.position[index];
                neibourgs[0] = wwi.position[index-1];
                neibourgs[1] = wwi.position[index+1];
                neibourgs[2] = wwi.position[index-wwi.width];
                neibourgs[3] = wwi.position[index+wwi.width];

                acceleration.x = ((neibourgs[0].x - pos.x) - x_length)*m_raideur +
                                 (x_length - (pos.x - neibourgs[1].x))*m_raideur +
                                 (neibourgs[2].x - pos.x)*m_raideur +
                                 (neibourgs[3].x - pos.x)*m_raideur;
                acceleration.y = (y_length - (pos.y - neibourgs[2].y))*m_raideur +
                                 ((neibourgs[3].y - pos.y) - y_length)*m_raideur +
                                 (neibourgs[0].y - pos.y)*m_raideur +
                                 (neibourgs[1].y - pos.y)*m_raideur;

                acceleration.x /= 4;
                acceleration.y /= 4;

                wwi.acceleration[index] = acceleration;
            }
        }
    }

    switch (m_accelerationFilter)
    {
    case NoFilter:
        break;

    case FourRingLinearMean:
        fourRingLinearMean(&wwi.acceleration, wwi);
        break;

    case MeanWithMean:
        meanWithMean(&wwi.acceleration, wwi);
        break;

    case MeanWithMedian:
        meanWithMedian(&wwi.acceleration, wwi);
        break;

    default:
        ASSERT1(false);
    }

#if defined COMPUTE_STATS
    Pair accBound = {m_maxAcceleration, m_minAcceleration};
    Pair velBound = {m_maxVelocity, m_minVelocity};
#endif

    // compute the new velocity of each vertex.
    for (unsigned int i = 0; i < wwi.count; ++i)
    {
        Pair acc = wwi.acceleration[i];
        fixVectorBounds(acc, m_minAcceleration, m_maxAcceleration);

#if defined COMPUTE_STATS
        computeVectorBounds(acc, accBound);
#endif

        Pair& vel = wwi.velocity[i];
        vel.x = acc.x*time + vel.x*m_amortissement;
        vel.y = acc.y*time + vel.y*m_amortissement;

        acc_sum += fabs(acc.x) + fabs(acc.y);
    }


    switch (m_velocityFilter)
    {
    case NoFilter:
        break;

    case FourRingLinearMean:
        fourRingLinearMean(&wwi.velocity, wwi);
        break;

    case MeanWithMean:
        meanWithMean(&wwi.velocity, wwi);
        break;

    case MeanWithMedian:
        meanWithMedian(&wwi.velocity, wwi);
        break;

    default:
        ASSERT1(false);
    }

    Pair topLeftCorner = {-10000.0, -10000.0};
    Pair bottomRightCorner = {10000.0, 10000.0};

    // compute the new pos of each vertex.
    for (unsigned int i = 0; i < wwi.count; ++i)
    {
        Pair& pos = wwi.position[i];
        Pair& vel = wwi.velocity[i];

        fixVectorBounds(vel, m_minVelocity, m_maxVelocity);
#if defined COMPUTE_STATS
        computeVectorBounds(vel, velBound);
#endif

        pos.x += vel.x*time*m_move_factor;
        pos.y += vel.y*time*m_move_factor;

        if (pos.x < topLeftCorner.x)
        {
            topLeftCorner.x = pos.x;
        }
        if (pos.x > bottomRightCorner.x)
        {
            bottomRightCorner.x = pos.x;
        }
        if (pos.y < topLeftCorner.y)
        {
            topLeftCorner.y = pos.y;
        }
        if (pos.y > bottomRightCorner.y)
        {
            bottomRightCorner.y = pos.y;
        }

        vel_sum += fabs(vel.x) + fabs(vel.y);

#if defined VERBOSE_MODE
        if (wwi.constraint[i])
        {
            kDebug() << "Constraint point ** vel : " << vel.x << "," << vel.y << " ** move : " << vel.x*time << "," << vel.y*time;
        }
#endif
    }

#if defined VERBOSE_MODE
#   if defined COMPUTE_STATS
        kDebug() << "Acceleration bounds (" << accBound.x << ", " << accBound.y << ")";
        kDebug() << "Velocity bounds (" << velBound.x << ", " << velBound.y << ")";
#   endif
    kDebug() << "sum_acc : " << acc_sum << "  ***  sum_vel :" << vel_sum;
#endif

    if (!wwi.onConstrain && acc_sum < m_stopAcceleration && vel_sum < m_stopVelocity)
    {
        freeWobblyInfo(wwi);
        windows.remove(w);
        return false;
    }

    QRect windowRect(topLeftCorner.x, topLeftCorner.y,
        bottomRightCorner.x - topLeftCorner.x, bottomRightCorner.y - topLeftCorner.y);
    if (m_updateRegion.isValid())
    {
        m_updateRegion = m_updateRegion.united(windowRect);
    }
    else
    {
        m_updateRegion = windowRect;
    }

    return true;
}

void WobblyWindowsEffect::fourRingLinearMean(Pair** datas_pointer, WindowWobblyInfos& wwi)
{
    Pair* datas = *datas_pointer;
    Pair neibourgs[4];

    // for corners

    // top-left
    {
        Pair& res = wwi.buffer[0];
        Pair vit = datas[0];
        neibourgs[0] = datas[1];
        neibourgs[1] = datas[wwi.width];

        res.x = (neibourgs[0].x + neibourgs[1].x + 2.0*vit.x) / 4.0;
        res.y = (neibourgs[0].y + neibourgs[1].y + 2.0*vit.y) / 4.0;
    }


    // top-right
    {
        Pair& res = wwi.buffer[wwi.width-1];
        Pair vit = datas[wwi.width-1];
        neibourgs[0] = datas[wwi.width-2];
        neibourgs[1] = datas[2*wwi.width-1];

        res.x = (neibourgs[0].x + neibourgs[1].x + 2.0*vit.x) / 4.0;
        res.y = (neibourgs[0].y + neibourgs[1].y + 2.0*vit.y) / 4.0;
    }


    // bottom-left
    {
        Pair& res = wwi.buffer[wwi.width*(wwi.height-1)];
        Pair vit = datas[wwi.width*(wwi.height-1)];
        neibourgs[0] = datas[wwi.width*(wwi.height-1)+1];
        neibourgs[1] = datas[wwi.width*(wwi.height-2)];

        res.x = (neibourgs[0].x + neibourgs[1].x + 2.0*vit.x) / 4.0;
        res.y = (neibourgs[0].y + neibourgs[1].y + 2.0*vit.y) / 4.0;
    }


    // bottom-right
    {
        Pair& res = wwi.buffer[wwi.count-1];
        Pair vit = datas[wwi.count-1];
        neibourgs[0] = datas[wwi.count-2];
        neibourgs[1] = datas[wwi.width*(wwi.height-1)-1];

        res.x = (neibourgs[0].x + neibourgs[1].x + 2.0*vit.x) / 4.0;
        res.y = (neibourgs[0].y + neibourgs[1].y + 2.0*vit.y) / 4.0;
    }


    // for borders

    // top border
    for (unsigned int i=1; i<wwi.width-1; ++i)
    {
        Pair& res = wwi.buffer[i];
        Pair vit = datas[i];
        neibourgs[0] = datas[i-1];
        neibourgs[1] = datas[i+1];
        neibourgs[2] = datas[i+wwi.width];

        res.x = (neibourgs[0].x + neibourgs[1].x + neibourgs[2].x + 3.0*vit.x) / 6.0;
        res.y = (neibourgs[0].y + neibourgs[1].y + neibourgs[2].y + 3.0*vit.y) / 6.0;
    }

    // bottom border
    for (unsigned int i=wwi.width*(wwi.height-1)+1; i<wwi.count-1; ++i)
    {
        Pair& res = wwi.buffer[i];
        Pair vit = datas[i];
        neibourgs[0] = datas[i-1];
        neibourgs[1] = datas[i+1];
        neibourgs[2] = datas[i-wwi.width];

        res.x = (neibourgs[0].x + neibourgs[1].x + neibourgs[2].x + 3.0*vit.x) / 6.0;
        res.y = (neibourgs[0].y + neibourgs[1].y + neibourgs[2].y + 3.0*vit.y) / 6.0;
    }

    // left border
    for (unsigned int i=wwi.width; i<wwi.width*(wwi.height-1); i+=wwi.width)
    {
        Pair& res = wwi.buffer[i];
        Pair vit = datas[i];
        neibourgs[0] = datas[i+1];
        neibourgs[1] = datas[i-wwi.width];
        neibourgs[2] = datas[i+wwi.width];

        res.x = (neibourgs[0].x + neibourgs[1].x + neibourgs[2].x + 3.0*vit.x) / 6.0;
        res.y = (neibourgs[0].y + neibourgs[1].y + neibourgs[2].y + 3.0*vit.y) / 6.0;
    }

    // right border
    for (unsigned int i=2*wwi.width-1; i<wwi.count-1; i+=wwi.width)
    {
        Pair& res = wwi.buffer[i];
        Pair vit = datas[i];
        neibourgs[0] = datas[i-1];
        neibourgs[1] = datas[i-wwi.width];
        neibourgs[2] = datas[i+wwi.width];

        res.x = (neibourgs[0].x + neibourgs[1].x + neibourgs[2].x + 3.0*vit.x) / 6.0;
        res.y = (neibourgs[0].y + neibourgs[1].y + neibourgs[2].y + 3.0*vit.y) / 6.0;
    }

    // for the inner points
    for (unsigned int j=1; j<wwi.height-1; ++j)
    {
        for (unsigned int i=1; i<wwi.width-1; ++i)
        {
            unsigned int index = i+j*wwi.width;

            Pair& res = wwi.buffer[index];
            Pair& vit = datas[index];
            neibourgs[0] = datas[index-1];
            neibourgs[1] = datas[index+1];
            neibourgs[2] = datas[index-wwi.width];
            neibourgs[3] = datas[index+wwi.width];

            res.x = (neibourgs[0].x + neibourgs[1].x + neibourgs[2].x + neibourgs[3].x + 4.0*vit.x) / 8.0;
            res.y = (neibourgs[0].y + neibourgs[1].y + neibourgs[2].y + neibourgs[3].y + 4.0*vit.y) / 8.0;
        }
    }

    Pair* tmp = datas;
    *datas_pointer = wwi.buffer;
    wwi.buffer = tmp;
}

void WobblyWindowsEffect::meanWithMean(Pair** datas_pointer, WindowWobblyInfos& wwi)
{
    Pair* datas = *datas_pointer;

    Pair mean = {0.0, 0.0};
    for (unsigned int i = 0; i < wwi.count; ++i)
    {
        mean.x += datas[i].x;
        mean.y += datas[i].y;
    }

    mean.x /= wwi.count;
    mean.y /= wwi.count;

    for (unsigned int i = 0; i < wwi.count; ++i)
    {
        wwi.buffer[i].x = (datas[i].x + mean.x) / 2.0;
        wwi.buffer[i].y = (datas[i].y + mean.y) / 2.0;
    }

    Pair* tmp = datas;
    *datas_pointer = wwi.buffer;
    wwi.buffer = tmp;
}

void WobblyWindowsEffect::meanWithMedian(Pair** datas_pointer, WindowWobblyInfos& wwi)
{
    Pair* datas = *datas_pointer;

    qreal xmin = datas[0].x, ymin = datas[0].y;
    qreal xmax = datas[0].x, ymax = datas[0].y;
    for (unsigned int i = 1; i < wwi.count; ++i)
    {
        if (datas[i].x < xmin)
        {
            xmin = datas[i].x;
        }
        if (datas[i].x > xmax)
        {
            xmax = datas[i].x;
        }

        if (datas[i].y < ymin)
        {
            ymin = datas[i].y;
        }
        if (datas[i].y > ymax)
        {
            ymax = datas[i].y;
        }
    }

    Pair median = {(xmin + xmax)/2.0, (ymin + ymax)/2.0};

    for (unsigned int i = 0; i < wwi.count; ++i)
    {
        wwi.buffer[i].x = (datas[i].x + median.x) / 2.0;
        wwi.buffer[i].y = (datas[i].y + median.y) / 2.0;
    }

    Pair* tmp = datas;
    *datas_pointer = wwi.buffer;
    wwi.buffer = tmp;
}


} // namespace KWin

