/***************************************************************************
 *   Copyright 2007 by Enrico Ros <enrico.ros@gmail.com>                   *
 *   Copyright 2007 by Riccardo Iaconelli <ruphy@kde.org>                  *
 *   Copyright 2008 by Davide Bettio <davide.bettio@kdemail.net>           *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; if not, write to the                         *
 *   Free Software Foundation, Inc.,                                       *
 *   51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA .        *
 ***************************************************************************/


#include "resultitem.h"

#include <math.h>

#include <QtCore/QTimeLine>
#include <QtCore/QDebug>
#include <QtCore/QtGlobal>
#include <QtCore/QTimer>
#include <QtGui/QApplication>
#include <QtGui/QPainter>
#include <QtGui/QStyleOptionGraphicsItem>
#include <QtGui/QGraphicsItemAnimation>
#include <QtGui/QGraphicsLinearLayout>
#include <QtGui/QGraphicsScene>
#include <QtGui/QGraphicsView>

#include <KDebug>
#include <KIcon>

#include <Plasma/PaintUtils>
#include <Plasma/Plasma>
#include <Plasma/RunnerManager>
#include <Plasma/PaintUtils>

//#define NO_GROW_ANIM

void shadowBlur(QImage &image, int radius, const QColor &color);

int ResultItem::s_fontHeight = 0;

ResultItem::ResultItem(const Plasma::QueryMatch &match, QGraphicsWidget *parent)
    : QGraphicsWidget(parent),
      m_match(0),
      m_highlight(0),
      m_index(-1),
      m_highlightTimerId(0),
      m_innerHeight(DEFAULT_ICON_SIZE)
{
    setFlag(QGraphicsItem::ItemIsFocusable);
    setFlag(QGraphicsItem::ItemIsSelectable);
    setAcceptHoverEvents(true);
    setFocusPolicy(Qt::TabFocus);
    setCacheMode(DeviceCoordinateCache);
    setZValue(0);

    if (s_fontHeight < 1) {
        //FIXME: reset when the application font changes
        QFontMetrics fm(font());
        s_fontHeight = fm.height();
        //kDebug() << "font height is: " << s_fontHeight;
    }

    setMatch(match);
}

ResultItem::~ResultItem()
{
}

void ResultItem::setMatch(const Plasma::QueryMatch &match)
{
    m_match = match;
    m_icon = KIcon(match.icon());
    calculateInnerHeight();
    update();
}

QString ResultItem::id() const
{
    return m_match.id();
}

bool ResultItem::compare(const ResultItem *one, const ResultItem *other)
{
    return other->m_match < one->m_match;
}

bool ResultItem::operator<(const ResultItem &other) const
{
    return m_match < other.m_match;
}

QString ResultItem::name() const
{
    return m_match.text();
}

QString ResultItem::description() const
{
    return m_match.subtext();
}

QString ResultItem::data() const
{
    return m_match.data().toString();
}

QIcon ResultItem::icon() const
{
    return m_icon;
}

Plasma::QueryMatch::Type ResultItem::group() const
{
    return m_match.type();
}

bool ResultItem::isQueryPrototype() const
{
    //FIXME: pretty lame way to decide if this is a query prototype
    return m_match.runner() == 0;
}

qreal ResultItem::priority() const
{
    // TODO, need to fator in more things than just this
    return m_match.relevance();
}

void ResultItem::setIndex(int index)
{
    if (m_index == index) {
        return;
    }

    m_index = qMax(-1, index);
}

int ResultItem::index() const
{
    return m_index;
}

void ResultItem::remove()
{
    deleteLater();
}

void ResultItem::run(Plasma::RunnerManager *manager)
{
    manager->run(m_match);
}

void ResultItem::paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget)
{
    Q_UNUSED(widget);

    bool oldClipping = painter->hasClipping();
    painter->setClipping(false);

    QRect iRect = contentsRect().toRect();
    QSize iconSize = iRect.size().boundedTo(QSize(32, 32));

    painter->setRenderHint(QPainter::Antialiasing);
    bool drawMixed = false;

    if (hasFocus() || isSelected()) {
        // here's what the next line means:
        // we check to see if the scene has focus, but that's overridden by the mouse hovering an
        // item ... or unless we are over 2 ticks into the higlight anim. complex but it works
        if (((scene() && !scene()->views().isEmpty() && !scene()->views()[0]->hasFocus()) &&
            !(option->state & QStyle::State_MouseOver)) || m_highlight > 2) {
            painter->drawPixmap(iRect.topLeft(), m_icon.pixmap(iconSize, QIcon::Active));
        } else {
            drawMixed = true;
            ++m_highlight;

            if (!m_highlightTimerId) {
                m_highlightTimerId = startTimer(TIMER_INTERVAL);
            }
        }
    } else if (m_highlight > 0) {
        drawMixed = true;

        --m_highlight;

        if (!m_highlightTimerId) {
            m_highlightTimerId = startTimer(TIMER_INTERVAL);
        }
    } else {
        painter->drawPixmap(iRect.topLeft(), m_icon.pixmap(iconSize, QIcon::Disabled));
    }

    if (drawMixed) {
        qreal factor = .2;

        if (m_highlight == 1) {
            factor = .4;
        } else if (m_highlight == 2) {
            factor = .6;
        } else if (m_highlight > 2) {
            factor = .8;
        }

        qreal activeOpacity = painter->opacity() * factor;

        painter->setOpacity(painter->opacity() * (1 - factor));
        painter->drawPixmap(iRect.topLeft(), m_icon.pixmap(iconSize, QIcon::Disabled));
        painter->setOpacity(activeOpacity);
        painter->drawPixmap(iRect.topLeft(), m_icon.pixmap(iconSize, QIcon::Active));
        painter->setOpacity(1);
    }

    QRect textRect(iRect.topLeft() + QPoint(iconSize.width() + TEXT_MARGIN, 0),
                   iRect.bottomRight());

    // Draw the text on a pixmap
    const QColor textColor = Plasma::Theme::defaultTheme()->color(Plasma::Theme::TextColor);
    const int width = option->fontMetrics.width(name());
    QPixmap pixmap(textRect.size());
    pixmap.fill(Qt::transparent);

    QPainter p(&pixmap);
    p.setPen(textColor);
    //TODO: add subtext, make bold, etc...
    p.drawText(pixmap.rect(), Qt::AlignLeft, name());
    QFont italics = p.font();
    QFontMetrics italicMetrics(italics);
    int fontHeight = italicMetrics.height();
    italics.setItalic(true);
    p.setFont(italics);
    p.drawText(pixmap.rect().adjusted(0, fontHeight, 0, 0), Qt::AlignLeft | Qt::TextWordWrap, description());

    // Fade the pixmap out at the end
    if (width > pixmap.width()) {
        if (m_fadeout.isNull() || m_fadeout.height() != pixmap.height()) {
            QLinearGradient g(0, 0, 20, 0);
            g.setColorAt(0, layoutDirection() == Qt::LeftToRight ? Qt::white : Qt::transparent);
            g.setColorAt(1, layoutDirection() == Qt::LeftToRight ? Qt::transparent : Qt::white);
            m_fadeout = QPixmap(20, textRect.height());
            m_fadeout.fill(Qt::transparent);
            QPainter p(&m_fadeout);
            p.setCompositionMode(QPainter::CompositionMode_Source);
            p.fillRect(m_fadeout.rect(), g);
        }
        const QRect r = QStyle::alignedRect(layoutDirection(), Qt::AlignRight, m_fadeout.size(), pixmap.rect());
        p.setCompositionMode(QPainter::CompositionMode_DestinationIn);
        p.drawPixmap(r.topLeft(), m_fadeout);
    }
    p.end();

    // Draw a drop shadow if we have a bright text color
    if (qGray(textColor.rgb()) > 192) {
        const int blur = 2;
        const QPoint offset(1, 1);

        QImage shadow(pixmap.size() + QSize(blur * 2, blur * 2), QImage::Format_ARGB32_Premultiplied);
        p.begin(&shadow);
        p.setCompositionMode(QPainter::CompositionMode_Source);
        p.fillRect(shadow.rect(), Qt::transparent);
        p.drawPixmap(blur, blur, pixmap);
        p.end();

        Plasma::PaintUtils::shadowBlur(shadow, blur, Qt::black);

        // Draw the shadow
        painter->drawImage(textRect.topLeft() - QPoint(blur, blur) + offset, shadow);
    }

    // Draw the text
    painter->drawPixmap(textRect.topLeft(), pixmap);
    painter->setClipping(oldClipping);
}

void ResultItem::hoverLeaveEvent(QGraphicsSceneHoverEvent *e)
{
    QGraphicsItem::hoverLeaveEvent(e);
    //update();
    //emit hoverLeave(this);
    setFocus(Qt::MouseFocusReason);
}

void ResultItem::hoverEnterEvent(QGraphicsSceneHoverEvent *e)
{
    QGraphicsItem::hoverEnterEvent(e);
    setFocus(Qt::MouseFocusReason);
    scene()->clearSelection();
    setSelected(true);
}

void ResultItem::timerEvent(QTimerEvent *e)
{
    Q_UNUSED(e)

    killTimer(m_highlightTimerId);
    m_highlightTimerId = 0;

    update();
}

void ResultItem::mouseReleaseEvent(QGraphicsSceneMouseEvent *)
{
    emit activated(this);
}

void ResultItem::focusInEvent(QFocusEvent * event)
{
    QGraphicsWidget::focusInEvent(event);
    setZValue(1);

    scene()->clearSelection();
    setSelected(true);

    if (!m_highlightTimerId) {
        m_highlightTimerId = startTimer(TIMER_INTERVAL);
    }

    emit hoverEnter(this);
}

void ResultItem::focusOutEvent(QFocusEvent * event)
{
    QGraphicsWidget::focusOutEvent(event);
    setZValue(0);

    if (!m_highlightTimerId) {
        m_highlightTimerId = startTimer(TIMER_INTERVAL);
    }

    emit hoverLeave(this);
}

void ResultItem::keyPressEvent(QKeyEvent *event)
{
    if (event->key() == Qt::Key_Enter || event->key() == Qt::Key_Return) {
        emit activated(this);
    } else {
        QGraphicsWidget::keyPressEvent(event);
    }
}

QVariant ResultItem::itemChange(GraphicsItemChange change, const QVariant &value)
{
    if (change == QGraphicsItem::ItemSceneHasChanged && scene()) {
        calculateSize();
    }

    return QGraphicsWidget::itemChange(change, value);
}

void ResultItem::changeEvent(QEvent *event)
{
    QGraphicsWidget::changeEvent(event);

    if (event->type() == QEvent::ContentsRectChange) {
        setSize();
    }
}

void ResultItem::setSize()
{
    qreal left, top, right, bottom;
    getContentsMargins(&left, &top, &right, &bottom);
    resize(scene() ? scene()->width() : geometry().width(), m_innerHeight + top + bottom);
    //kDebug() << m_innerHeight << geometry().size();
}

void ResultItem::calculateInnerHeight()
{
    QFontMetrics fm(font());
    const int maxHeight = fm.height() * 4;
    const int minHeight = DEFAULT_ICON_SIZE;

    QString text = name() + "\n" + description();
    QRect textBounds(contentsRect().toRect());
    textBounds.adjust(DEFAULT_ICON_SIZE + TEXT_MARGIN, 0, 0, 0);

    if (maxHeight > textBounds.height()) {
        textBounds.setHeight(maxHeight);
    }

    int height = fm.boundingRect(textBounds, Qt::AlignLeft | Qt::TextWordWrap, text).height();
    //kDebug() << (QObject*)this << text;// << m_match.text();
    //kDebug() << fm.height() << maxHeight << textBounds << height << minHeight << qMax(height, minHeight);
    m_innerHeight = qMax(height, minHeight);
}

void ResultItem::calculateSize()
{
    calculateInnerHeight();
    setSize();
}

#include "resultitem.moc"

