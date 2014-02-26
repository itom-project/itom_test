/* ********************************************************************
    itom software
    URL: http://www.uni-stuttgart.de/ito
    Copyright (C) 2013, Institut f�r Technische Optik (ITO),
    Universit�t Stuttgart, Germany

    This file is part of itom.
  
    itom is free software; you can redistribute it and/or modify it
    under the terms of the GNU Library General Public Licence as published by
    the Free Software Foundation; either version 2 of the Licence, or (at
    your option) any later version.

    itom is distributed in the hope that it will be useful, but
    WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU Library
    General Public Licence for more details.

    You should have received a copy of the GNU Library General Public License
    along with itom. If not, see <http://www.gnu.org/licenses/>.
*********************************************************************** */

#ifndef ABSTRACTPROPERTYPAGEWIDGET_H
#define ABSTRACTPROPERTYPAGEWIDGET_H

#include <qwidget.h>

class AbstractPropertyPageWidget : public QWidget
{
    Q_OBJECT

    public:
        AbstractPropertyPageWidget(QWidget * parent = NULL, Qt::WindowFlags f = NULL) : QWidget(parent, f) {}
        ~AbstractPropertyPageWidget() {}

        virtual void readSettings() = 0;  /*!< This method is called at startup of the property dialog. Read the setting file using QSetting in order to initialize your property widget (this method must be overwritten) */
        virtual void writeSettings() = 0; /*!< This method is called at shutdown of the property dialog (only if apply or ok has been clicked). Apply your settings and write it to the setting file using QSetting (this method must be overwritten) */
};

#endif