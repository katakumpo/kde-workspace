/* 
   Copyright (c) 2002 Malte Starostik <malte@kde->org>

   This program is free software; you can redistribute it and/or
   modify it under the terms of the GNU General Public
   License as published by the Free Software Foundation; either
   version 2 of the License, or (at your option) any later version->
 
   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE->  See the GNU
   General Public License for more details->
 
   You should have received a copy of the GNU General Public License
   along with this program; see the file COPYING->  If not, write to
   the Free Software Foundation, Inc->, 59 Temple Place - Suite 330,
   Boston, MA 02111-1307, USA->
*/

// $Id$

#include <qapplication.h>
#include <qimage.h>
#include <qpainter.h>
#include <qpixmap.h>
#include <qpixmapcache.h>

#include "pixmaploader.h"

#include "pixmaps.embed"

using namespace Keramik;

PixmapLoader PixmapLoader::s_instance;

PixmapLoader::PixmapLoader()
	: m_cache( 193 ),
	  m_colorize( false ),
	  m_hue( -1 ),
	  m_sat( 0 ),
	  m_val( 228 )
{ 
	QPixmapCache::setCacheLimit( 128 );
	m_cache.setAutoDelete( true );
}

void PixmapLoader::setColor( const QColor& color )
{
	bool colorize = color.isValid() && color.rgb() != qRgb( 228, 228, 228 );
	if ( ( m_colorize = colorize ) )
		color.hsv( &m_hue, &m_sat, &m_val );
	else
	{
		m_hue = -1;
		m_sat = 0;
		m_val = 228;
	}

	m_cache.clear();
	QPixmapCache::clear();
}

void PixmapLoader::colorize( QImage &img )
{
	if ( img.isNull() ) return;

	img = img.copy();
	register Q_UINT32* data = reinterpret_cast< Q_UINT32* >( img.bits() );
	register Q_UINT32* end = data + img.width() * img.height();
	while ( data < end )
	{
		QColor c( *data );
		int h, s, v;
		c.hsv( &h, &s, &v );
		if ( m_hue >= 0 && h >= 0 ) h = ( h - 216 + m_hue ) % 360;
		if ( s ) s += m_sat / 2;
		c.setHsv( h, QMIN( s, 255 ), QMIN( v * m_val / 228, 255 ) );
		*data++ = ( c.rgb() & RGB_MASK ) | ( *data & ~RGB_MASK );
	}
}

void PixmapLoader::makeDisabled( QImage &img )
{
	if ( img.isNull() ) return;

	img = img.copy();
	register Q_UINT32* data = reinterpret_cast< Q_UINT32* >( img.bits() );
	register Q_UINT32* end = data + img.width() * img.height();
	while ( data < end )
	{
		QColor c( *data );
		int h, s, v;
		c.hsv( &h, &s, &v );
		if ( m_hue >= 0 && h >= 0 ) h = ( h - 216 + m_hue ) % 360;
		if ( s ) s += m_sat / 5;
		c.setHsv( h, QMIN( s, 255 ), QMIN( v * m_val / 255, 255 ) );
		*data++ = ( c.rgb() & RGB_MASK ) | ( *data & ~RGB_MASK );
	}
}

QPixmap PixmapLoader::pixmap( const QString& name, bool disabled )
{
	QString cacheName = name;
	if ( disabled ) cacheName += "-disabled";
	QPixmap result;
	if ( QPixmapCache::find( cacheName, result ) )
		return result;

	QImage* img = m_cache[ cacheName ];
	if ( !img ) {
		img = new QImage( qembed_findImage( name ) );
		if ( disabled )
			makeDisabled( *img );
		else if ( m_colorize )
			colorize( *img );
		m_cache.insert( cacheName, img );
	}
	result.convertFromImage( *img );
	QPixmapCache::insert( cacheName, result );
	return result;
}


QPixmap PixmapLoader::scale( const QString& name, int width, int height, bool disabled )
{
	QString key = name + '-' + QString::number( width ) + '-' + QString::number( height );
	if ( disabled ) key += "-disabled";
	QPixmap result;
	if ( QPixmapCache::find( key, result  ) )
		return result;

	QString cacheName = name;
	if ( disabled ) cacheName += "-disabled";
	QImage* img = m_cache[ cacheName ];
	if ( !img ) {
		img = new QImage( qembed_findImage( name ) );
		if ( disabled )
			makeDisabled( *img );
		else if ( m_colorize )
			colorize( *img );
		m_cache.insert( cacheName, img );
	}

	result.convertFromImage( img->scale( width ? width : img->width(), height ? height : img->height() ) );
	QPixmapCache::insert( key, result );
	return result;
}

void TilePainter::draw( QPainter *p, int x, int y, int width, int height, bool disabled )
{
	unsigned int scaledColumns = 0, scaledRows = 0, lastScaledColumn = 0, lastScaledRow = 0;
	int scaleWidth = width, scaleHeight = height;

	for ( unsigned int col = 0; col < columns(); ++col )
		if ( columnMode( col ) != Fixed )
		{
			scaledColumns++;
			lastScaledColumn = col;
		}
		else scaleWidth -= tile( col, 0, disabled ).width();
	for ( unsigned int row = 0; row < rows(); ++row )
		if ( rowMode( row ) != Fixed )
		{
			scaledRows++;
			lastScaledRow = row;
		}
		else scaleHeight -= tile( 0, row, disabled ).height();
	if ( scaleWidth < 0 ) scaleWidth = 0;
	if ( scaleHeight < 0 ) scaleHeight = 0;

	int ypos = y;
	if ( scaleHeight && !scaledRows ) ypos += scaleHeight / 2;
	for ( unsigned int row = 0; row < rows(); ++row )
	{
		int xpos = x;
		if ( scaleWidth && !scaledColumns ) xpos += scaleWidth / 2;
		int h = rowMode( row ) == Fixed ? 0 : scaleHeight / scaledRows;
		if ( scaledRows && row == lastScaledRow ) h += scaleHeight - scaleHeight / scaledRows * scaledRows;
		int realH = h ? h : tile( 0, row, disabled ).height();
		if ( rowMode( row ) == Tiled ) h = 0;

		for ( unsigned int col = 0; col < columns(); ++col )
		{
			int w = columnMode( col ) == Fixed ? 0 : scaleWidth / scaledColumns;
			if ( scaledColumns && col == lastScaledColumn ) w += scaleWidth - scaleWidth / scaledColumns * scaledColumns;
			int realW = w ? w : tile( col, row, disabled ).width();
			if ( columnMode( col ) == Tiled ) w = 0;

			if ( !tile( col, row, disabled ).isNull() )
				if ( w || h )
					p->drawTiledPixmap( xpos, ypos, realW, realH, scale( col, row, w, h, disabled ) );
				else p->drawTiledPixmap( xpos, ypos, realW, realH, tile( col, row, disabled ) );
			xpos += realW;
		}
		ypos += realH;
	}
}

QString TilePainter::absTileName( unsigned int column, unsigned int row ) const
{
	QString name = tileName( column, row );
	if ( name.isEmpty() ) return m_name;
	return m_name + "-" + name;
}

RectTilePainter::RectTilePainter( const QString& name,
                                  bool scaleH, bool scaleV,
                                  unsigned int columns, unsigned int rows )
	: TilePainter( name ),
	  m_scaleH( scaleH ),
	  m_scaleV( scaleV ),
	  m_columns( columns ),
	  m_rows( rows )
{
}

QString RectTilePainter::tileName( unsigned int column, unsigned int row ) const
{
	static QString c = "lcr", r = "tcb";
	return QString( r.mid( row, 1 ) + c.mid( column, 1 ) );
}

TilePainter::TileMode RectTilePainter::columnMode( unsigned int column ) const
{
	if ( column != 1 ) return Fixed;
	return m_scaleH ? Scaled : Tiled;
}

TilePainter::TileMode RectTilePainter::rowMode( unsigned int row ) const
{
	if ( row != 1 ) return Fixed;
	return m_scaleV ? Scaled : Tiled;
}

ActiveTabPainter::ActiveTabPainter( bool bottom )
	: RectTilePainter( QString( "tab-" ) + ( bottom ? "bottom-" : "top-" ) + "active" ),
	  m_bottom( bottom )
{
}

TilePainter::TileMode ActiveTabPainter::rowMode( unsigned int row ) const
{
	if ( m_bottom ) return ( row == 1 ) ? Fixed : Scaled;
	return ( row == 1 ) ? Scaled : Fixed;
}

QString ActiveTabPainter::tileName( unsigned int column, unsigned int row ) const
{
	if ( m_bottom )
		return RectTilePainter::tileName( column, row + 1 );
	return RectTilePainter::tileName( column, row );
}

InactiveTabPainter::InactiveTabPainter( Mode mode, bool bottom )
	: RectTilePainter( QString( "tab-" ) + ( bottom ? "bottom-" : "top-" ) + "inactive" ),
	  m_mode( mode ), m_bottom( bottom )
{
}

unsigned int InactiveTabPainter::columns() const
{
	Mode check = QApplication::reverseLayout() ? First : Last;
	return m_mode == check ? 3 : 2;
}

TilePainter::TileMode InactiveTabPainter::rowMode( unsigned int row ) const
{
	if ( m_bottom ) return ( row == 1 ) ? Fixed : Scaled;
	return ( row == 1 ) ? Scaled : Fixed;
}

QString InactiveTabPainter::tileName( unsigned int column, unsigned int row ) const
{
	Mode check = QApplication::reverseLayout() ? Last : First;
	if ( column == 0 && m_mode != check ) return "separator";
	if ( m_bottom )
		return RectTilePainter::tileName( column, row + 1 );
	return RectTilePainter::tileName( column, row );
}

ScrollBarPainter::ScrollBarPainter( const QString& type, int count, bool horizontal )
	: TilePainter( name( horizontal ) ),
	  m_type( type ),
	  m_count( count ),
	  m_horizontal( horizontal )
{
}

QString ScrollBarPainter::name( bool horizontal )
{
	return QString( "scrollbar-" ) + ( horizontal ? "hbar" : "vbar" );
}

TilePainter::TileMode ScrollBarPainter::columnMode( unsigned int column ) const
{
	if ( !m_horizontal || !( column % 2 ) ) return Fixed;
	return ( m_count == 2 ) ? Scaled : Tiled;
}

TilePainter::TileMode ScrollBarPainter::rowMode( unsigned int row ) const
{
	if ( m_horizontal || !( row % 2 ) ) return Fixed;
	return ( m_count == 2 ) ? Scaled : Tiled;
}

QString ScrollBarPainter::tileName( unsigned int column, unsigned int row ) const
{
	unsigned int num = ( column ? column : row ) + 1;
	if ( m_count == 5 )
		if ( num == 3 ) num = 4;
		else if ( num == 4 ) num = 2;
		else if ( num == 5 ) num = 3;

	return m_type + QString::number( num );
}

TilePainter::TileMode SpinBoxPainter::columnMode( unsigned int column ) const
{
	return column == 1 ? Scaled : Fixed;
}

QString SpinBoxPainter::tileName( unsigned int column, unsigned int ) const
{
	return QString::number( column + 1 );
}

// vim: ts=4 sw=4 noet
