/*
 *   Copyright 2008 Aaron Seigo <aseigo@kde.org>
 *
 *   This program is free software; you can redistribute it and/or modify
 *   it under the terms of the GNU Library General Public License as
 *   published by the Free Software Foundation; either version 2, or
 *   (at your option) any later version.
 *
 *   This program is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU General Public License for more details
 *
 *   You should have received a copy of the GNU Library General Public
 *   License along with this program; if not, write to the
 *   Free Software Foundation, Inc.,
 *   51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 */


#ifndef PLASMA__H
#define PLASMA__H

#include <QGraphicsProxyWidget>

class QComboBox;

namespace Plasma
{

class ComboBox : public QGraphicsProxyWidget
{
    Q_OBJECT

    Q_PROPERTY(QGraphicsWidget* parentWidget READ parentWidget)
    Q_PROPERTY(QString text READ text)
    Q_PROPERTY(QString stylesheet READ stylesheet WRITE setStylesheet)
    Q_PROPERTY(QComboBox* nativeWidget READ nativeWidget)

public:
    explicit ComboBox(QGraphicsWidget *parent = 0);
    ~ComboBox();

    /**
     * @return the display text
     */
    QString text() const;

    /**
     * Sets the style sheet used to control the visual display of this ComboBox
     *
     * @arg stylehseet a CSS string
     */
    void setStylesheet(const QString &stylesheet);

    /**
     * @return the stylesheet currently used with this widget
     */
    QString stylesheet();

    /**
     * @return the native widget wrapped by this ComboBox
     */
    QComboBox* nativeWidget() const;

    /**
     * Adds an item to the combobox with the given text. The
     * item is appended to the list of existing items.
     */
    void addItem(const QString &text);

public Q_SLOTS:
    void clear();

Q_SIGNALS:
    void activated(const QString & text);

protected:
    void resizeEvent(QGraphicsSceneResizeEvent *event);

private:
    class Private;
    Private * const d;
};

} // namespace Plasma

#endif // multiple inclusion guard
