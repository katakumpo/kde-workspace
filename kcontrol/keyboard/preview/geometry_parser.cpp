 /*
 *  Copyright (C) 2013 Shivam Makkar (amourphious1992@gmail.com)
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 */

#include "geometry_parser.h"
#include "geometry_components.h"
#include "x11_helper.h"

#include <QtCore/QString>
#include <QtCore/QDebug>
#include <QFileDialog>
#include <QFile>
#include <QtCore/QPair>


#include <fixx11h.h>
#include <config-workspace.h>

namespace grammar{
keywords::keywords(){
    add
       ("shape", 1)
       ("height", 2)
       ("width", 3)
       ("description", 4)
       ("keys", 5)
       ("row", 6)
       ("section", 7)
       ("key", 8)
       ("//", 9)
       ("/*", 10)
       ;
}


template<typename Iterator>
Geometry_parser<Iterator>::Geometry_parser():Geometry_parser::base_type(start){

    using qi::lexeme;
    using qi::char_;
    using qi::lit;
    using qi::_1;
    using qi::_val;
    using qi::int_;
    using qi::double_;
    using qi::eol;


    name = '"'>>+(char_-'"')>>'"';

    ignore =(lit("outline")||lit("overlay")||lit("text"))>>*(char_-lit("};"))>>lit("};")
            ||lit("solid")>>*(char_-lit("};"))>>lit("};")
            ||lit("indicator")>>*(char_-';'-'{')>>';'||'{'>>*(char_-lit("};"))>>lit("};")
            ||lit("indicator")>>'.'>>lit("shape")>>'='>>name>>';';

    comments =lexeme[ lit("//")>>*(char_-eol||kw-eol)>>eol||lit("/*")>>*(char_-lit("*/")||kw-lit("*/"))>>lit("*/") ];

    cordinates = ('['
            >>double_[phx::ref(x)=_1]
            >>','
            >>double_[phx::ref(y)=_1]
            >>']')
            ||'['>>double_>>",">>double_>>']'
            ;

    cordinatea = '['>>double_[phx::ref(ax)=_1]>>",">>double_[phx::ref(ay)=_1]>>']';

    set = '{'>>cordinates>>*(','>>cordinates)>>'}';

    setap = '{'>>cordinatea>>*(','>>cordinatea)>>'}';

    seta = '{'
            >>cordinates[phx::bind(&Geometry_parser::setCord,this)]
            >>*(','>>cordinates[phx::bind(&Geometry_parser::setCord,this)])
            >>'}'
            ;

    description = lit("description")>>'='>>name[phx::bind(&Geometry_parser::getDescription,this,_1)]>>';';

    cornerRadius = (lit("cornerRadius")||lit("corner"))>>'='>>double_;

    shapeDef = lit("shape")
            >>name[phx::bind(&Geometry_parser::getShapeName,this,_1)]
            >>'{'
            >>*(lit("approx")>>'='>>setap[phx::bind(&Geometry_parser::setApprox,this)]>>','||cornerRadius>>','||comments)
            >>seta
            >>*((','>>(set||lit("approx")>>'='>>setap[phx::bind(&Geometry_parser::setApprox,this)]||cornerRadius)||comments))
            >>lit("};")
            ;

    keyName = '<'>>+(char_-'>')>>'>';

    keyShape = *(lit("key."))>>lit("shape")>>'='>>name[phx::bind(&Geometry_parser::setKeyShape,this,_1)]
            ||name[phx::bind(&Geometry_parser::setKeyShape,this,_1)];

    keyColor = lit("color")>>'='>>name;

    keygap = lit("gap")>>'='>>double_[phx::ref(off)=_1]||double_[phx::ref(off)=_1];

    keyDesc = keyName[phx::bind(&Geometry_parser::setKeyNameandShape,this,_1)]
            ||'{'>>(keyName[phx::bind(&Geometry_parser::setKeyNameandShape,this,_1)]||keyShape
                   ||keygap[phx::bind(&Geometry_parser::setKeyOffset,this)]
                   ||keyColor)
            >>*((','
            >>(keyName
            ||keyShape
            ||keygap[phx::bind(&Geometry_parser::setKeyOffset,this)]
            ||keyColor))
            ||comments)
            >>'}';

    keys = lit("keys")
            >>'{'
            >>keyDesc[phx::bind(&Geometry_parser::setKeyCordi,this)]
            >>*((*lit(',')>>keyDesc[phx::bind(&Geometry_parser::setKeyCordi,this)]>>*lit(','))||comments)
            >>lit("};");

    geomShape = ((lit("key.shape")>>'='>>name[phx::bind(&Geometry_parser::setGeomShape,this,_1)])||(lit("key.color")>>'='>>name))>>';';
    geomLeft = lit("section.left")>>'='>>double_[phx::ref(geom.sectionLeft)=_1]>>';';
    geomTop = lit("section.top")>>'='>>double_[phx::ref(geom.sectionTop)=_1]>>';';
    geomRowTop = lit("row.top")>>'='>>double_[phx::ref(geom.rowTop)=_1]>>';';
    geomRowLeft = lit("row.left")>>'='>>double_[phx::ref(geom.rowLeft)=_1]>>';';
    geomGap = lit("key.gap")>>'='>>double_[phx::ref(geom.keyGap)=_1]>>';';
    geomVertical = *lit("row.")>>lit("vertical")>>'='>>(lit("True")||lit("true"))>>';';
    geomAtt = geomLeft||geomTop||geomRowTop||geomRowLeft||geomGap;

    top = lit("top")>>'='>>double_>>';';
    left = lit("left")>>'='>>double_>>';';

    row = lit("row")[phx::bind(&Geometry_parser::rowinit,this)]
            >>'{'
            >>*(top[phx::bind(&Geometry_parser::setRowTop,this,_1)]
            ||left[phx::bind(&Geometry_parser::setRowLeft,this,_1)]
            ||localShape[phx::bind(&Geometry_parser::setRowShape,this,_1)]
            ||localColor
            ||comments
            ||geomVertical[phx::bind(&Geometry_parser::setVerticalRow,this)]
            ||keys
            )
            >>lit("};")||ignore||geomVertical[phx::bind(&Geometry_parser::setVerticalSection,this)];

    angle = lit("angle")>>'='>>double_>>';';

    localShape = lit("key.shape")>>'='>>name[_val=_1]>>';';
    localColor = lit("key.color")>>'='>>name>>';';
    localDimension = (lit("height")||lit("width"))>>'='>>double_>>';';
    priority = lit("priority")>>'='>>double_>>';';

    section = lit("section")[phx::bind(&Geometry_parser::sectioninit,this)]
            >>name[phx::bind(&Geometry_parser::sectionName,this,_1)]
            >>'{'
            >>*(top[phx::bind(&Geometry_parser::setSectionTop,this,_1)]
            ||left[phx::bind(&Geometry_parser::setSectionLeft,this,_1)]
            ||angle[phx::bind(&Geometry_parser::setSectionAngle,this,_1)]
            ||row[phx::bind(&Geometry_parser::addRow,this)]
            ||localShape[phx::bind(&Geometry_parser::setSectionShape,this,_1)]
            ||geomAtt
            ||localColor
            ||localDimension
            ||priority
            ||comments)
            >>lit("};")||geomVertical[phx::bind(&Geometry_parser::setVerticalGeometry,this)];

    shapeC = lit("shape")>>'.'>>cornerRadius>>';';

    shape = shapeDef||shapeC;


    in = '{'
          >>+(width
          ||height
          ||comments
          ||ignore
          ||description
          ||(char_-kw-'}'
          ||shape[phx::bind(&Geometry::addShape,&geom)]
          ||section[phx::bind(&Geometry::addSection,&geom)]
          ||geomAtt
          ||geomShape
          ))
          >>'}';

          width = lit("width")>>'='>>double_[phx::bind(&Geometry::setWidth,&geom,_1)]>>";";
          height = lit("height")>>'='>>double_[phx::bind(&Geometry::setHeight,&geom,_1)]>>";";


          info = in;


          start %= *(lit("default"))
                 >>lit("xkb_geometry")
                 >>name[phx::bind(&Geometry_parser::getName,this,_1)]
                 >>info
                 >>';'>>*(comments||char_-lit("xkb_geometry"));
}

template<typename Iterator>
    void Geometry_parser<Iterator>::setCord(){
        geom.setShapeCord(x, y);
    }


template<typename Iterator>
    void Geometry_parser<Iterator>::setSectionShape(std::string n){
    geom.sectionList[geom.getSectionCount()].setShapeName( QString::fromUtf8(n.data(), n.size()));
}


template<typename Iterator>
    void Geometry_parser<Iterator>::getName(std::string n){
        geom.setName(QString::fromUtf8(n.data(), n.size()));
}


template<typename Iterator>
    void Geometry_parser<Iterator>::getDescription(std::string n){
        geom.setDescription( QString::fromUtf8(n.data(), n.size()));
}


template<typename Iterator>
    void Geometry_parser<Iterator>::getShapeName(std::string n){
        geom.setShapeName( QString::fromUtf8(n.data(), n.size()));
}


template<typename Iterator>
    void Geometry_parser<Iterator>::setGeomShape(std::string n){
        geom.setKeyShape(QString::fromUtf8(n.data(), n.size()));
}


template<typename Iterator>
void Geometry_parser<Iterator>::setRowShape(std::string n){
    geom.sectionList[geom.getSectionCount()].rowList[geom.sectionList[geom.getSectionCount()].getRowCount()].setShapeName(QString::fromUtf8(n.data(), n.size() ));
}


template<typename Iterator>
    void Geometry_parser<Iterator>::setApprox(){
        geom.setShapeApprox(ax, ay);
}


template<typename Iterator>
    void Geometry_parser<Iterator>::addRow(){
        geom.sectionList[geom.getSectionCount()].addRow();
}


template<typename Iterator>
    void Geometry_parser<Iterator>::sectionName(std::string n){
        geom.sectionList[geom.getSectionCount()].setName(QString::fromUtf8(n.data(), n.size()));
}


template<typename Iterator>
    void Geometry_parser<Iterator>::rowinit(){
        geom.sectionList[geom.getSectionCount()].rowList[geom.sectionList[geom.getSectionCount()].getRowCount()].setTop(geom.sectionList[geom.getSectionCount()].getTop());
        geom.sectionList[geom.getSectionCount()].rowList[geom.sectionList[geom.getSectionCount()].getRowCount()].setLeft(geom.sectionList[geom.getSectionCount()].getLeft());
        geom.sectionList[geom.getSectionCount()].rowList[geom.sectionList[geom.getSectionCount()].getRowCount()].setShapeName(geom.sectionList[geom.getSectionCount()].getShapeName());
        cx = geom.sectionList[geom.getSectionCount()].rowList[geom.sectionList[geom.getSectionCount()].getRowCount()].getLeft();
        cy = geom.sectionList[geom.getSectionCount()].rowList[geom.sectionList[geom.getSectionCount()].getRowCount()].getTop();
        geom.sectionList[geom.getSectionCount()].rowList[geom.sectionList[geom.getSectionCount()].getRowCount()].setVertical(geom.sectionList[geom.getSectionCount()].getVertical());
    }


template<typename Iterator>
    void Geometry_parser<Iterator>::sectioninit(){
        geom.sectionList[geom.getSectionCount()].setTop(geom.sectionTop);
        geom.sectionList[geom.getSectionCount()].setLeft(geom.sectionLeft);
        cx = geom.sectionList[geom.getSectionCount()].getLeft();
        cy = geom.sectionList[geom.getSectionCount()].getTop();
        geom.sectionList[geom.getSectionCount()].setShapeName(geom.getKeyShape());
        geom.sectionList[geom.getSectionCount()].setVertical(geom.getVertical());
    }


template<typename Iterator>
    void Geometry_parser<Iterator>::setRowTop(double a){
        geom.sectionList[geom.getSectionCount()].rowList[geom.sectionList[geom.getSectionCount()].getRowCount()].setTop(a + geom.sectionList[geom.getSectionCount()].getTop());
        cy = geom.sectionList[geom.getSectionCount()].rowList[geom.sectionList[geom.getSectionCount()].getRowCount()].getTop();
    }


template<typename Iterator>
    void Geometry_parser<Iterator>::setRowLeft(double a){
        geom.sectionList[geom.getSectionCount()].rowList[geom.sectionList[geom.getSectionCount()].getRowCount()].setLeft(a + geom.sectionList[geom.getSectionCount()].getLeft());
        cx = geom.sectionList[geom.getSectionCount()].rowList[geom.sectionList[geom.getSectionCount()].getRowCount()].getLeft();
    }


template<typename Iterator>
    void Geometry_parser<Iterator>::setSectionTop(double a){
        //qDebug()<<"\nsectionCount"<<geom.sectionCount;
        geom.sectionList[geom.getSectionCount()].setTop(a + geom.sectionTop);
        cy = geom.sectionList[geom.getSectionCount()].getTop();
    }


template<typename Iterator>
    void Geometry_parser<Iterator>::setSectionLeft(double a){
        //qDebug()<<"\nsectionCount"<<geom.sectionCount;
        geom.sectionList[geom.getSectionCount()].setLeft(a + geom.sectionLeft);
        cx = geom.sectionList[geom.getSectionCount()].getLeft();

    }


template<typename Iterator>
    void Geometry_parser<Iterator>::setSectionAngle(double a){
        //qDebug()<<"\nsectionCount"<<geom.sectionCount;
        geom.sectionList[geom.getSectionCount()].setAngle(a);
    }


template<typename Iterator>
    void Geometry_parser<Iterator>::setVerticalRow(){
        geom.sectionList[geom.getSectionCount()].rowList[geom.sectionList[geom.getSectionCount()].getRowCount()].setVertical(1);
    }


template<typename Iterator>
    void Geometry_parser<Iterator>::setVerticalSection(){
        geom.sectionList[geom.getSectionCount()].setVertical(1);
    }


template<typename Iterator>
    void Geometry_parser<Iterator>::setVerticalGeometry(){
        geom.setVertical(1);
    }


template<typename Iterator>
    void Geometry_parser<Iterator>::setKeyName(std::string n){
        int secn = geom.getSectionCount();
        int rown = geom.sectionList[secn].getRowCount();
        int keyn = geom.sectionList[secn].rowList[rown].getKeyCount();
        //qDebug()<<"\nsC: "<<secn<<"\trC: "<<rown<<"\tkn: "<<keyn;
        geom.sectionList[secn].rowList[rown].keyList[keyn].setKeyName(QString::fromUtf8(n.data(), n.size()));
     }


template<typename Iterator>
    void Geometry_parser<Iterator>::setKeyShape(std::string n){
        int secn = geom.getSectionCount();
        int rown = geom.sectionList[secn].getRowCount();
        int keyn = geom.sectionList[secn].rowList[rown].getKeyCount();
        //qDebug()<<"\nsC: "<<secn<<"\trC: "<<rown<<"\tkn: "<<keyn;
        geom.sectionList[secn].rowList[rown].keyList[keyn].setShapeName(QString::fromUtf8(n.data(), n.size()));
    }


template<typename Iterator>
    void Geometry_parser<Iterator>::setKeyNameandShape(std::string n){
        int secn = geom.getSectionCount();
        int rown = geom.sectionList[secn].getRowCount();
        setKeyName(n);
        setKeyShape(geom.sectionList[secn].rowList[rown].getShapeName().toUtf8().constData());
    }


template<typename Iterator>
    void Geometry_parser<Iterator>::setKeyOffset(){
        //qDebug()<<"\nhere\n";
        int secn = geom.getSectionCount();
        int rown = geom.sectionList[secn].getRowCount();
        int keyn = geom.sectionList[secn].rowList[rown].getKeyCount();
        //qDebug()<<"\nsC: "<<secn<<"\trC: "<<rown<<"\tkn: "<<keyn;
        geom.sectionList[secn].rowList[rown].keyList[keyn].setOffset(off);
    }


template<typename Iterator>
    void Geometry_parser<Iterator>::setKeyCordi(){
        int secn = geom.getSectionCount();
        int rown = geom.sectionList[secn].getRowCount();
        int keyn = geom.sectionList[secn].rowList[rown].getKeyCount();
        int vertical = geom.sectionList[secn].rowList[rown].getVertical();

        if(vertical == 0)
            cx+=geom.sectionList[secn].rowList[rown].keyList[keyn].getOffset();
        else
            cy+=geom.sectionList[secn].rowList[rown].keyList[keyn].getOffset();

        geom.sectionList[secn].rowList[rown].keyList[keyn].setKeyPosition(cx, cy);

        QString s = geom.sectionList[secn].rowList[rown].keyList[keyn].getShapeName();
        if ( s.isEmpty() )
            s = geom.getKeyShape();

        GShape t = geom.findShape(s);
        int a = t.size(vertical);

        if(vertical == 0)
            cx+=a+geom.keyGap;
        else
            cy+=a+geom.keyGap;

        geom.sectionList[secn].rowList[rown].addKey();
    }


    QStringList ModelToGeometry :: pcmodels = QStringList() << "pc101" << "pc102" << "pc104" << "pc105" ;
    QStringList ModelToGeometry :: msmodels = QStringList() << "microsoft"<< "microsoft4000"<< "microsoft7000"<< "microsoftpro"<< "microsoftprousb"<< "microsoftprose";
    QStringList ModelToGeometry :: nokiamodels = QStringList() << "nokiasu8w" << "nokiarx44"<< "nokiarx51" ;
    QStringList ModelToGeometry :: pcgeometries = QStringList() << "latitude" ;
    QStringList ModelToGeometry :: macbooks = QStringList() << "macbook78" << "macbook79";
    QStringList ModelToGeometry :: applealu = QStringList() << "applealu_ansi" << "applealu_iso" << "applealu_jis";
    QStringList ModelToGeometry :: macs = QStringList() << "macintosh" << "macintosh_old" << "ibook" << "powerbook" << "macbook78" << "macbook79";

    ModelToGeometry :: ModelToGeometry(QString model){

        geometryFile = "pc";
        geometryName = "pc104";
        kbModel = model;

        if (model == "microsoftelite"){
            geometryFile = "microsoft";
            geometryName = "elite";
        }

        if (msmodels.contains(model)){
            geometryFile = "microsoft";
            geometryName = "natural";
        }

        if (model == "dell101"){
            geometryFile = "dell";
            geometryName = "dell101";
        }

        if (model == "dellm65"){
            geometryFile = "dell";
            geometryName = "dellm65";
        }
        if (model == "latitude"){
            geometryFile = "dell";
            geometryName = "latitude";
        }
        if (model == "flexpro"){
            geometryFile = "keytronic";
            geometryName = "FlexPro";
        }
        if(model == "hp6000" || model == "hpmini110"){
            geometryFile = "hp";
            geometryName = "mini110";
        }
        if(model == "hpdv5"){
            geometryFile = "hp";
            geometryName = "dv5";
        }
        if(model == "omnikey101"){
            geometryFile = "northgate";
            geometryName = "omnikey101";
        }
        if(model == "sanwaskbkg3"){
            geometryFile = "sanwa";
            geometryName = "sanwaskbkg3";
        }
        if(pcmodels.contains(model) || pcgeometries.contains(model)){
            geometryFile = "pc";
            geometryName = model;
        }
        if(model == "everex"){
            geometryFile = "everex";
            geometryName = "STEPnote";
        }
        if(model.contains("thinkpad")){
            geometryFile = "thinkpad";
            geometryName = "60";
        }
        if(model == "winbook"){
            geometryFile = "winbook";
            geometryName = "XP5";
        }
        if(model == "pc98"){
            geometryFile = "nec";
            geometryName = "pc98";
        }
        if(model == "hhk"){
            geometryFile = "hhk";
            geometryName = "basic";
        }
        if(model == "kinesis"){
            geometryFile = "kinesis";
            geometryName = "model100";
        }
        if(nokiamodels.contains(model)){
            geometryFile = "nokia";
            geometryName = model;
        }
        if(macs.contains(model) || macbooks.contains(model) || applealu.contains(model)){
            geometryFile = "macintosh";
            geometryName = model;
        }

    }


    ModelToGeometry :: ModelToGeometry(){
        geometryFile = "pc";
        geometryName = "pc104";
        kbModel = "pc104";
    }

    Geometry parseGeometry(QString model){
        using boost::spirit::iso8859_1::space;
        typedef std::string::const_iterator iterator_type;
        typedef grammar::Geometry_parser<iterator_type> Geometry_parser;
        Geometry_parser g;
        ModelToGeometry m2g = ModelToGeometry(model);

        //qDebug()<<geometry;
        QString geometryFile = m2g.getGeometryFile();
        QString geometryName = m2g.getGeometryName();
        //qDebug()<< geometryFile << geometryName;

        QString xkbParentDir = findGeometryBaseDir();
        geometryFile.prepend(xkbParentDir);
        QFile gfile(geometryFile);
         if (!gfile.open(QIODevice::ReadOnly | QIODevice::Text)){
             qDebug()<<"unable to open the file";
             return g.geom;
        }

        QString gcontent = gfile.readAll();
        gfile.close();

        QStringList gcontentList = gcontent.split("xkb_geometry");

        int i = 1;
        while(g.geom.getName()!=geometryName && i < gcontentList.size() ){
            g.geom = Geometry();
            QString input = gcontentList.at(i);
            input.prepend("xkb_geometry");
            //qDebug()<<input;
            std::string xyz = input.toUtf8().constData();

            std::string::const_iterator iter = xyz.begin();
            std::string::const_iterator end = xyz.end();

            bool r = phrase_parse(iter, end, g, space);
            /*if (r && iter == end){
                std::cout << "-------------------------\n";
                std::cout << "Parsing succeeded\n";
                std::cout << "\n-------------------------\n";
            }
            else{
                std::cout << "-------------------------\n";
                std::cout << "Parsing failed\n";
                std::cout << "-------------------------\n";
            }*/
            i++;

        }
        //g.geom.display();
        return g.geom;
    }

    QString findGeometryBaseDir()
    {
        QString xkbDir = X11Helper::findXkbDir();
        return QString("%1/geometry/").arg(xkbDir);
    }

}
