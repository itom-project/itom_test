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

#ifndef ALGOINTERFACEVALIDATOR_H
#define ALGOINTERFACEVALIDATOR_H

#include <qobject.h>
#include "../../common/addInInterface.h"
#include "../../common/sharedStructures.h"

#include <qvector.h>
#include <qvariant.h>

namespace ito {

class AlgoInterfaceValidator : public QObject
{
public:
    AlgoInterfaceValidator(ito::RetVal &retValue);
    ~AlgoInterfaceValidator();

    ito::RetVal addInterface(ito::AddInAlgo::tAlgoInterface iface, QVector<ito::Param> &mandParams, QVector<ito::Param> &outParams, size_t maxNumMand, size_t maxNumOpt, size_t maxNumOut);
    bool isValidFilter(const ito::AddInAlgo::FilterDef &filter, ito::RetVal &ret, QStringList &tags) const;
    bool isValidWidget(const ito::AddInAlgo::AlgoWidgetDef &widget, ito::RetVal &ret, QStringList &tags) const;
    ito::RetVal getInterfaceParameters(ito::AddInAlgo::tAlgoInterface iface, QVector<ito::ParamBase> &mandParams, QVector<ito::ParamBase> &outParams) const;

protected:
    struct AlgoInterface
    {
        AlgoInterface() : maxNumMand(0), maxNumOpt(0), maxNumOut(0) {}
        QVector<ito::Param> mandParams;
        QVector<ito::Param> outParams;
        size_t maxNumMand;
        size_t maxNumOpt;
        size_t maxNumOut;
    };

    enum tCompareResult { tCmpEqual, tCmpCompatible, tCmpFailed };

    QMap<int,AlgoInterface> m_interfaces;

    ito::RetVal init(void);
    bool isValid(const ito::AddInAlgo::tAlgoInterface iface, const ito::AddInAlgo::t_filterParam filterParamFunc, ito::RetVal &ret) const;
    bool getTags(const ito::AddInAlgo::tAlgoInterface iface, const QString &metaInformation, QStringList &tags) const;
    tCompareResult compareParam(const ito::Param &paramTemplate, const ito::Param &param, ito::RetVal &ret) const;
    tCompareResult compareMetaParam(const ito::ParamMeta *metaTemplate, const ito::ParamMeta *meta, const char* nameTemplate, const char *name, ito::RetVal &ret) const;

private:
};

} //end namespace ito

#endif
