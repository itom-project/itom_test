/* ********************************************************************
    itom software
    URL: http://www.uni-stuttgart.de/ito
    Copyright (C) 2020, Institut fuer Technische Optik (ITO),
    Universitaet Stuttgart, Germany

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

    --------------------------------
    This class is a modified version of the class QToolTip of the
    Qt framework (licensed under LGPL):
    https://code.woboq.org/qt5/qtbase/src/widgets/kernel/qtooltip.cpp.html
*********************************************************************** */

#include "pyCodeFormatter.h"
#include "../python/pythonEngine.h"
#include "../AppManagement.h"

#include <qdebug.h>
#include <iostream>

namespace ito {

//-------------------------------------------------------------------------------------
PyCodeFormatter::PyCodeFormatter(QObject *parent /*= nullptr*/) :
    QObject(parent),
    m_isCancelling(false)
{
    connect(&m_process, &QProcess::errorOccurred,
        this, &PyCodeFormatter::errorOccurred);

    connect(&m_process, &QProcess::readyReadStandardOutput,
        this, &PyCodeFormatter::readyReadStandardOutput);

    connect(&m_process, &QProcess::readyReadStandardError,
        this, &PyCodeFormatter::readyReadStandardError);

    connect(&m_process, SIGNAL(finished(int, QProcess::ExitStatus)),
        this, SLOT(finished(int, QProcess::ExitStatus)));

    connect(&m_process, &QProcess::started,
        this, &PyCodeFormatter::started);
}

//-------------------------------------------------------------------------------------
PyCodeFormatter::~PyCodeFormatter()
{
    if (!m_isCancelling)
    {
        cancelRequested();
    }
}

//-------------------------------------------------------------------------------------
/*
\param cmd is the command <cmd> in the call python -m <cmd> and must allow passing
    the code string via stdin.
*/
ito::RetVal PyCodeFormatter::startFormatting(const QString &cmd, const QString &code, QWidget *dialogParent /*= nullptr*/)
{
    m_isCancelling = false;

    if (m_process.state() != QProcess::NotRunning)
    {
        return ito::RetVal(ito::retError, 0, "process already started.");
    }

    const PythonEngine *pyeng = qobject_cast<PythonEngine*>(AppManagement::getPythonEngine());
    QString pythonPath;

    if (pyeng)
    {
        pythonPath = pyeng->getPythonExecutable();
    }
    else
    {
        return RetVal(ito::retError, 0, "could not get python engine.");
    }

    if (dialogParent)
    {
        m_progressDialog = QSharedPointer<QProgressDialog>(
            new QProgressDialog(
                tr("The code formatter is running..."), 
                tr("Cancel"), 
                0, 100, dialogParent)
            );
        m_progressDialog->setModal(true);
        m_progressDialog->setValue(0);
        connect(m_progressDialog.data(), &QProgressDialog::canceled,
            this, &PyCodeFormatter::cancelRequested);
        m_progressDialog->show();
    }

    m_currentCode = code;
    m_currentError = "";

    QString exec = QString("%1 -m %2").arg(pythonPath).arg(cmd);
    m_process.start(exec);

    return ito::retOk;
}

//-------------------------------------------------------------------------------------
void PyCodeFormatter::cancelRequested()
{
    if (m_process.state() == QProcess::Running || m_process.state() == QProcess::Starting)
    {
        m_isCancelling = true;
        m_process.kill();
        m_process.waitForFinished(2000);
    }

    if (m_progressDialog)
    {
        m_progressDialog->accept();
        m_progressDialog.clear();
    }
}

//-------------------------------------------------------------------------------------
void PyCodeFormatter::errorOccurred(QProcess::ProcessError error)
{
    if (m_progressDialog)
    {
        m_progressDialog->accept();
        m_progressDialog.clear();
    }

    if (!m_isCancelling)
    {
        std::cerr << "PyCodeFormatter error: "
            << error << std::endl;
    }

    emit formattingDone(false, "");
}

//-------------------------------------------------------------------------------------
void PyCodeFormatter::finished(int exitCode, QProcess::ExitStatus exitStatus)
{
    if (m_progressDialog)
    {
        m_progressDialog->setValue(100);
        m_progressDialog->accept();
        m_progressDialog.clear();
    }

    if (exitCode == 0 && !m_isCancelling)
    {
        emit formattingDone(true, m_currentCode);
    }
    else
    {
        emit formattingDone(false, m_currentError);
    }
}

//-------------------------------------------------------------------------------------
void PyCodeFormatter::readyReadStandardError()
{
    m_currentError += QLatin1String(m_process.readAllStandardError());
}

//-------------------------------------------------------------------------------------
void PyCodeFormatter::readyReadStandardOutput()
{
    m_currentCode += QLatin1String(m_process.readAllStandardOutput());
}

//-------------------------------------------------------------------------------------
void PyCodeFormatter::started()
{
    if (m_progressDialog)
    {
        m_progressDialog->setValue(10);
    }

    m_process.write(m_currentCode.toUtf8());
    m_currentCode = "";
    m_process.closeWriteChannel();

    if (m_progressDialog)
    {
        m_progressDialog->setValue(20);
    }
}

} //end namespace ito

