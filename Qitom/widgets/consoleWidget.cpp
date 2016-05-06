/* ********************************************************************
    itom software
    URL: http://www.uni-stuttgart.de/ito
    Copyright (C) 2016, Institut fuer Technische Optik (ITO),
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
*********************************************************************** */

#include "../python/pythonEngineInc.h"
#include "consoleWidget.h"
#include "../global.h"
#include "../AppManagement.h"

#include <qmessagebox.h>
#include <qfile.h>
#include <qmimedata.h>
#include <qurl.h>
#include <qsettings.h>
#include <qfileinfo.h>
#include <qregexp.h>
#include <QClipboard>

#include "../organizer/userOrganizer.h"
#include "../organizer/scriptEditorOrganizer.h"

namespace ito
{

//!< constants
const QString ConsoleWidget::lineBreak = QString("\n");

//----------------------------------------------------------------------------------------------------------------------------------
ConsoleWidget::ConsoleWidget(QWidget* parent) :
    AbstractPyScintillaWidget(parent),
    m_startLineBeginCmd(-1),
    m_canCopy(false),
    m_canCut(false),
    m_waitForCmdExecutionDone(false),
    m_pythonBusy(false),
    m_pCmdList(NULL),
    m_pQout(NULL),
    m_pQerr(NULL),
    m_inputStreamWaitCond(NULL),
    m_inputStartLine(0),
    m_markErrorLine(-1),
    m_markCurrentLine(-1),
    m_markInputLine(-1)
{
    qDebug("console widget start constructor");
    initEditor();

    loadSettings();

    connect(AppManagement::getMainApplication(), SIGNAL(propertiesChanged()), this, SLOT(reloadSettings()));

    qRegisterMetaType<ito::QDebugStream::MsgStreamType>("ito::QDebugStream::MsgStreamType");

    //redirect cout and cerr to this console
    m_pQout = new QDebugStream(std::cout, QDebugStream::msgStreamOut);
    m_pQerr = new QDebugStream(std::cerr, QDebugStream::msgStreamErr);
    
    connect(this, SIGNAL(wantToCopy()), SLOT(copy()));
    connect(this, SIGNAL(selectionChanged()), SLOT(selChanged()));
    connect(this, SIGNAL(SCN_DOUBLECLICK(int,int,int)), SLOT(textDoubleClicked(int,int,int)));

    if (m_pQout)
    {
        connect(m_pQout, SIGNAL(flushStream(QString, ito::QDebugStream::MsgStreamType)), this, SLOT(receiveStream(QString, ito::QDebugStream::MsgStreamType)));
    }
    if (m_pQerr)
    {
        connect(m_pQerr, SIGNAL(flushStream(QString, ito::QDebugStream::MsgStreamType)), this, SLOT(receiveStream(QString, ito::QDebugStream::MsgStreamType)));
    }

    const QObject *pyEngine = AppManagement::getPythonEngine(); //PythonEngine::getInstance();

    if (pyEngine)
    {
        connect(this, SIGNAL(pythonExecuteString(QString)), pyEngine, SLOT(pythonRunString(QString)));
        connect(pyEngine, SIGNAL(pythonStateChanged(tPythonTransitions)), this, SLOT(pythonStateChanged(tPythonTransitions)));
    }

    m_pCmdList = new DequeCommandList(20);
    QString settingsName(AppManagement::getSettingsFile());
    QSettings *settings = new QSettings(settingsName, QSettings::IniFormat);
    settings->beginGroup("ConsoleDequeCommandList");
    int size = settings->beginReadArray("LastCommandList");
    for (int i = size - 1; i > -1; --i)
    {
        settings->setArrayIndex(i);
        m_pCmdList->add(settings->value("cmd", "").toString());
    }
    settings->endArray();
    settings->endGroup();
    delete settings;

    //!< empty queue
    while (!m_cmdQueue.empty())
    {
        m_cmdQueue.pop();
    }

    startNewCommand(true);

    /*freopen ("D:\\test.txt","w",stdout);
    fprintf(stdout, "Test");
    fclose(stdout);*/
}

//----------------------------------------------------------------------------------------------------------------------------------
ConsoleWidget::~ConsoleWidget()
{
    m_pCmdList->moveLast();
    QString settingsName(AppManagement::getSettingsFile());
    QSettings *settings = new QSettings(settingsName, QSettings::IniFormat);
    settings->beginGroup("ConsoleDequeCommandList");
    settings->beginWriteArray("LastCommandList");
    int i = 0;
    QString cmd = m_pCmdList->getPrevious(); 
    while (cmd != "")
    {
        settings->setArrayIndex(i);
        settings->setValue("cmd", cmd);
        cmd = m_pCmdList->getPrevious();
        ++i;
    }
    settings->endArray();
    settings->endGroup();
    delete settings;

    const QObject *pyEngine = AppManagement::getPythonEngine(); //PythonEngine::getInstance();
    if (pyEngine)
    {
        disconnect(this, SIGNAL(pythonExecuteString(QString)), pyEngine, SLOT(pythonRunString(QString)));
        disconnect(pyEngine, SIGNAL(pythonStateChanged(tPythonTransitions)), this, SLOT(pythonStateChanged(tPythonTransitions)));
    }

    DELETE_AND_SET_NULL(m_pCmdList);
    DELETE_AND_SET_NULL(m_pQout);
    DELETE_AND_SET_NULL(m_pQerr);
}

//----------------------------------------------------------------------------------------------------------------------------------
RetVal ConsoleWidget::initEditor()
{
    setPaper(QColor(1, 81, 107));

    setFolding(QsciScintilla::NoFoldStyle);
    autoAdaptLineNumberColumnWidth(); //setMarginWidth(1,25);
    setMarginSensitivity(1, false);
    setMarginLineNumbers(1, true);

    setWrapMode(QsciScintilla::WrapWord);
    setWrapVisualFlags(QsciScintilla::WrapFlagByBorder, QsciScintilla::WrapFlagNone , 2);

    //with some QScintilla versions, there is a bug if the markerBackgroundColor contains transparancy. Then
    //the marker gets a black border. Problem: PlatQt.cpp of QScintilla:
    //
    //no transparancy: void SurfaceImpl::RoundedRectangle is called -> sets painter->setPen(convertQColor(fore)) or setPen(NoPen)
    //with transparancy: void SurfaceImpl::AlphaRectangle is called -> no impact on setPen, uses the lastly used settings

    m_markErrorLine = markerDefine(QsciScintilla::Background) ;
    setMarkerBackgroundColor(QColor(255, 192, 192), m_markErrorLine); //has been (255,0,0,25) -> equal to (255,192,192) on white background

    m_markCurrentLine = markerDefine(QsciScintilla::Background);
    setMarkerBackgroundColor(QColor(255, 255, 128), m_markCurrentLine); //has been (255, 255, 0, 50) -> equal to (255,255,128) on white background

    m_markInputLine = markerDefine(QsciScintilla::Background);
    setMarkerBackgroundColor(QColor(179, 222, 171), m_markInputLine); //has been (255, 255, 0, 50) -> equal to (255,255,128) on white background

    return RetVal(retOk);
}

//----------------------------------------------------------------------------------------------------------------------------------
void ConsoleWidget::loadSettings()
{
    QSettings settings(AppManagement::getSettingsFile(), QSettings::IniFormat);
    settings.beginGroup("PyScintilla");

    bool ok = false;
    QsciScintilla::WrapVisualFlag start, end;

    int wrapMode = settings.value("WrapMode", 0).toInt(&ok);
    if (!ok)
    {
        wrapMode = 0;
    }

    switch (wrapMode)
    {
        case 0: setWrapMode(QsciScintilla::WrapNone); break;
        case 1: setWrapMode(QsciScintilla::WrapWord); break;
        case 2: setWrapMode(QsciScintilla::WrapCharacter); break;
    };

    QString flagStart = settings.value("WrapFlagStart", "NoFlag").toString();
    if (flagStart == "NoFlag")
    {
        start = QsciScintilla::WrapFlagNone;
    }
    if (flagStart == "FlagText")
    {
        start = QsciScintilla::WrapFlagByText;
    }
    if (flagStart == "FlagBorder")
    {
        start = QsciScintilla::WrapFlagByBorder;
    }

    QString flagEnd = settings.value("WrapFlagEnd", "NoFlag").toString();
    if (flagEnd == "NoFlag")
    {
        end = QsciScintilla::WrapFlagNone;
    }
    if (flagEnd == "FlagText")
    {
        end = QsciScintilla::WrapFlagByText;
    }
    if (flagEnd == "FlagBorder")
    {
        end = QsciScintilla::WrapFlagByBorder;
    }

    int indent = settings.value("WrapIndent", 2).toInt(&ok);
    if (!ok)
    {
        indent = 2;
    }

    setWrapVisualFlags(end, start, indent);

    int indentMode = settings.value("WrapIndentMode", 0).toInt(&ok);
    if (!ok)
    {
        indentMode = 0;
    }
    switch (indentMode)
    {
        case 0: setWrapIndentMode(QsciScintilla::WrapIndentFixed); break;
        case 1: setWrapIndentMode(QsciScintilla::WrapIndentSame); break;
        case 2: setWrapIndentMode(QsciScintilla::WrapIndentIndented); break;
    };

    settings.endGroup();

    AbstractPyScintillaWidget::loadSettings();
}

//----------------------------------------------------------------------------------------------------------------------------------
void ConsoleWidget::pythonStateChanged(tPythonTransitions pyTransition)
{
    switch (pyTransition)
    {
    case pyTransBeginRun:
    case pyTransBeginDebug:
    case pyTransDebugContinue:
    case pyTransDebugExecCmdBegin:
        if (!m_waitForCmdExecutionDone)
        {
            //this part is only executed if a script or other python code is executed but
            //not from the command line. Then, the text that is not executed yet, is
            //temporarily removed and finally added again when python has been finished

            //copy text from m_startLineBeginCmd on to m_temporaryRemovedCommands
            QStringList temp;

            for (int i = m_startLineBeginCmd; i <= lines() - 1; i++)
            {
                temp.push_back(text(i));
            }
            m_temporaryRemovedCommands = temp.join("");

            setSelection(m_startLineBeginCmd, 0, lines() - 1, lineLength(lines() - 1));

            removeSelectedText();
        }
        else
        {
            //m_temporaryRemovedCommands = "";
        }

        m_pythonBusy = true;
        break;
    case pyTransEndRun:
    case pyTransEndDebug:
    case pyTransDebugWaiting:
    case pyTransDebugExecCmdEnd:

        if (!m_waitForCmdExecutionDone)
        {
            m_startLineBeginCmd = lines() - 1;
            append(m_temporaryRemovedCommands);
            m_temporaryRemovedCommands = "";
            moveCursorToEnd();
        }
        else
        {
            executeCmdQueue();
        }

        m_pythonBusy = false;

        break;
    }
}

//----------------------------------------------------------------------------------------------------------------------------------
RetVal ConsoleWidget::clearEditor()
{
    startNewCommand(true);
    return RetVal(retOk);
}

//----------------------------------------------------------------------------------------------------------------------------------
//!< new command is a new line starting with ">>"
RetVal ConsoleWidget::startNewCommand(bool clearEditorFirst)
{
    if (clearEditorFirst)
    {
        markerDeleteAll(m_markErrorLine);
        clear();
    }

    if (text() == "")
    {
        //!< empty editor, just start new command
        append(">>");
        moveCursorToEnd();
        m_startLineBeginCmd = lines() - 1;
    }
    else
    {
        //!< append at the end of the existing text
        if (lineLength(lines() - 1) > 0)
        {
            append(ConsoleWidget::lineBreak);
        }
        append(">>");
        moveCursorToEnd();
        m_startLineBeginCmd = lines() - 1;
    }

    return RetVal(retOk);
}

//----------------------------------------------------------------------------------------------------------------------------------
void ConsoleWidget::autoAdaptLineNumberColumnWidth()
{
    int l = lines();
    QString s; //make the width always a little bit bigger than necessary

    if (l < 10)
    {
        s = QString::number(10);
    }
    else if (l < 100)
    {
        s = QString::number(100);
    }
    else if (l < 1000)
    {
        s = QString::number(1000);
    }
    else if (l < 10000)
    {
        s = QString::number(10000);
    }
    else
    {
        s = QString::number(100000);
    }

    setMarginWidth(1, s);
}

//----------------------------------------------------------------------------------------------------------------------------------
RetVal ConsoleWidget::useCmdListCommand(int dir)
{
    QString cmd("");
    int lineFrom, lineTo, indexFrom, indexTo;

    if (m_startLineBeginCmd >= 0)
    {
        if (dir==1)
        {
            cmd = m_pCmdList->getPrevious();
        }
        else
        {
            cmd = m_pCmdList->getNext();
        }

        //delete possible commands after m_startLineBeginCmd:
        lineFrom = m_startLineBeginCmd;
        lineTo = lines() - 1;
        indexFrom = 2;
        indexTo = lineLength(lineTo);
        setSelection(lineFrom, indexFrom, lineTo, indexTo);
        removeSelectedText();
        setCursorPosition(lineFrom, indexFrom);
        append(cmd);

        moveCursorToEnd();
        //m_startLineBeginCmd = lines()-1;
    }

    return RetVal(retOk);
}

//----------------------------------------------------------------------------------------------------------------------------------
//!> reimplementation to process the keyReleaseEvent
void ConsoleWidget::keyPressEvent(QKeyEvent* event)
{
    int key = event->key();
    Qt::KeyboardModifiers modifiers = event->modifiers();
    int lineFrom, lineTo, indexFrom, indexTo;
    bool acceptEvent = false;
    bool forwardEvent = false;

    if (key == Qt::Key_F5 && (modifiers & Qt::ShiftModifier))
    {
        if (m_inputStreamWaitCond)
        {
            markerDeleteAll(m_markInputLine);
            m_inputStreamBuffer->clear();
            m_inputStreamWaitCond->release();
            m_inputStreamWaitCond->deleteSemaphore();
            m_inputStreamWaitCond = NULL;
            append(ConsoleWidget::lineBreak);
        }
        else
        {
            PythonEngine *pyeng = qobject_cast<PythonEngine*>(AppManagement::getPythonEngine());
            if (pyeng)
            {
                pyeng->pythonInterruptExecution();
            }
        }

        acceptEvent = false; //!< no action necessary
        forwardEvent = false;
    }
    else if (hasFocus() && (m_inputStreamWaitCond != NULL || (!m_waitForCmdExecutionDone && !m_pythonBusy)))
    {
        switch (key)
        {
        case Qt::Key_Up:

            if (isCallTipActive() || isListActive())
            {
                acceptEvent = true;
                forwardEvent = true;
            }
            else
            {
                Qt::KeyboardModifiers modifiers = event->modifiers();
                if ((modifiers &  Qt::ShiftModifier) || (modifiers &  Qt::ControlModifier))
                {
                    acceptEvent = true;
                    forwardEvent = true;
                }
                else
                {
                    getSelection(&lineFrom, &indexFrom, &lineTo, &indexTo);

                    if (lineFrom == -1)
                    {
                        getCursorPosition(&lineFrom, &indexFrom);
                    }

                    if (lineFrom <= m_startLineBeginCmd)
                    {
                        acceptEvent = true;
                        forwardEvent = false;
                        useCmdListCommand(1);
                    }
                    else
                    {
                        acceptEvent = true;
                        forwardEvent = true;
                    }
                }
            }
            break;

        case Qt::Key_Down:
            if (isCallTipActive() || isListActive())
            {
                acceptEvent = true;
                forwardEvent = true;
            }
            else
            {
                Qt::KeyboardModifiers modifiers = event->modifiers();
                if ((modifiers &  Qt::ShiftModifier) || (modifiers &  Qt::ControlModifier))
                {
                    acceptEvent = true;
                    forwardEvent = true;
                }
                else
                {
                    getSelection(&lineFrom, &indexFrom, &lineTo, &indexTo);

                    if (lineFrom == -1)
                    {
                        getCursorPosition(&lineFrom, &indexFrom);
                    }

                    if (lineFrom == lines() - 1 || lineFrom < m_startLineBeginCmd)
                    {
                        acceptEvent = true;
                        forwardEvent = false;
                        useCmdListCommand(-1);
                    }
                    else
                    {
                        acceptEvent = true;
                        forwardEvent = true;
                    }
                }
            }
            break;

        case Qt::Key_Left:
        case Qt::Key_Right:
        case Qt::Key_NumLock:
        case Qt::Key_Print:
        case Qt::Key_Pause:
        case Qt::Key_Insert:
        case Qt::Key_End:
        case Qt::Key_PageUp:
        case Qt::Key_PageDown:
        case Qt::Key_CapsLock:
        //case Qt::Key_Escape:
            acceptEvent = true;
            forwardEvent = true;
            break;
        
        // clears the current input or interrupts an input
        case Qt::Key_Escape:
            if (isListActive() == false)
            {
                if (!m_inputStreamWaitCond)
                {
                    lineTo = lines() - 1;
                    indexTo = lineLength(lineTo);

                    setSelection(m_startLineBeginCmd, 2, lineTo, indexTo);
                    removeSelectedText();
                }
                else
                {
                    markerDeleteAll(m_markInputLine);
                    m_inputStreamBuffer->clear();
                    m_inputStreamWaitCond->release();
                    m_inputStreamWaitCond->deleteSemaphore();
                    m_inputStreamWaitCond = NULL;
                    append(ConsoleWidget::lineBreak);
                }

                if (isCallTipActive())
                {
                    SendScintilla(SCI_CALLTIPCANCEL);
                }
                acceptEvent = true;
                forwardEvent = false;
            }
            else
            {
                acceptEvent = true;
                forwardEvent = true;
            }
            break;
        
        case Qt::Key_Home: //Pos1
            getCursorPosition(&lineFrom, &indexFrom);

            if (m_inputStreamWaitCond && lineFrom == m_inputStartLine && indexFrom >= m_inputStartCol)
            {
                if (modifiers & Qt::ShiftModifier)
                {
                    setSelection(m_inputStartLine, m_inputStartCol, lineFrom, indexFrom);
                }
                else
                {
                    setCursorPosition(m_inputStartLine, m_inputStartCol);
                }
            }
            else if (lineFrom == m_startLineBeginCmd && indexFrom >= 2)
            {
                if (modifiers & Qt::ShiftModifier)
                {
                    setSelection(m_startLineBeginCmd,2,lineFrom,indexFrom);
                }
                else
                {
                    setCursorPosition(m_startLineBeginCmd, 2);
                }
            }
            else
            {
                if (modifiers & Qt::ShiftModifier)
                {
                    setSelection(lineFrom,0,lineFrom,indexFrom);
                }
                else
                {
                    setCursorPosition(lineFrom, 0);
                }
            }
            acceptEvent = true;
            forwardEvent = false;
            break;

        case Qt::Key_Return:
        case Qt::Key_Enter:
            if ((modifiers & Qt::ShiftModifier) == 0)
            {
                //!> return pressed
                if (m_startLineBeginCmd >= 0 && !m_pythonBusy)
                {
                    m_waitForCmdExecutionDone = true;
                    //!< new line for possible error or message
                    append(ConsoleWidget::lineBreak);

                    execCommand(m_startLineBeginCmd, lines() - 2);
                    acceptEvent = true;
                    forwardEvent = false;

                    //!< do not emit keyPressEvent in QsciScintilla!!
                }
                else if (m_inputStreamWaitCond) //startInputCommandLine was called before by pythonStream.cpp to wait for a string input (python command 'input(...)'). The semaphore m_inputStreamWaitCond is blocked until the input is obtained.
                {
                    QStringList texts(text(m_inputStartLine).mid(m_inputStartCol));
                    for (int i = m_inputStartLine + 1; i < lines(); i++)
                    {
                        texts.append(text(i));
                    }

                    QByteArray ba = texts.join("").toLatin1().data();
                    if (m_inputStreamBuffer->size() == 0)
                    {
                        *m_inputStreamBuffer = ba;
                    }
                    else
                    {
                        *m_inputStreamBuffer = ba.left(m_inputStreamBuffer->size());
                    }

                    markerDeleteAll(m_markInputLine);

                    m_inputStreamWaitCond->release();
                    m_inputStreamWaitCond->deleteSemaphore();
                    m_inputStreamWaitCond = NULL;

                    append(ConsoleWidget::lineBreak);
                    acceptEvent = true;
                    forwardEvent = false;
                    //!< do not emit keyPressEvent in QsciScintilla!!
                }
                else
                {
                    acceptEvent = false;
                    forwardEvent = false;
                    //!< do not emit keyPressEvent in QsciScintilla!!
                }
                SendScintilla(SCI_CALLTIPCANCEL);
                SendScintilla(SCI_AUTOCCANCEL);
            }
            else
            {
                moveCursorToValidRegion();
                acceptEvent = true;
                forwardEvent = true;
            }
            break;

        case Qt::Key_Backspace:
            //!< check that del and backspace is only pressed in valid cursor context
            getSelection(&lineFrom, &indexFrom, &lineTo, &indexTo);
            if (lineFrom == -1) //!< no selection
            {
                getCursorPosition(&lineFrom, &indexFrom);

                if (lineFrom < m_startLineBeginCmd || (lineFrom == m_startLineBeginCmd && indexFrom <= 2))
                {
                    acceptEvent = false;
                    forwardEvent = false;
                }
                else if (m_inputStreamWaitCond && (lineFrom < m_inputStartLine || (lineFrom == m_inputStartLine && indexFrom <= m_inputStartCol)))
                {
                    acceptEvent = false;
                    forwardEvent = false;
                }
                else
                {
                    acceptEvent = true;
                    forwardEvent = true;
                }
            }
            else
            {
                if (m_inputStreamWaitCond)
                {
                    if (lineFrom > m_inputStartLine || (lineFrom == m_inputStartLine && indexFrom > m_inputStartCol))
                    {
                        acceptEvent = true;
                        forwardEvent = true;
                    }
                    else if ((lineTo == m_inputStartLine && indexTo > m_inputStartCol) || (lineTo > m_inputStartLine))
                    {
                        setSelection(m_inputStartLine, m_inputStartCol, lineTo, indexTo);
                        acceptEvent = true;
                        forwardEvent = true;
                    }
                    else
                    {
                        acceptEvent = false;
                        forwardEvent = false;
                    }
                }
                else
                {
                    if (lineFrom > m_startLineBeginCmd || (lineFrom == m_startLineBeginCmd && indexFrom >= 2))
                    {
                        acceptEvent = true;
                        forwardEvent = true;
                    }
                    else if ((lineTo == m_startLineBeginCmd && indexTo > 2) || (lineTo > m_startLineBeginCmd))
                    {
                        setSelection(m_startLineBeginCmd, 2, lineTo, indexTo);
                        acceptEvent = true;
                        forwardEvent = true;
                    }
                    else
                    {
                        acceptEvent = false;
                        forwardEvent = false;
                    }
                }
            }
            break;

        case Qt::Key_Delete:
            //!< check that del and backspace is only pressed in valid cursor context
            getSelection(&lineFrom, &indexFrom, &lineTo, &indexTo);
            if (lineFrom == -1)
            {
                getCursorPosition(&lineFrom, &indexFrom);

                if (lineFrom < m_startLineBeginCmd || (lineFrom == m_startLineBeginCmd && indexFrom < 2))
                {
                    acceptEvent = false;
                    forwardEvent = false;
                }
                else if (m_inputStreamWaitCond && (lineFrom < m_inputStartLine || (lineFrom == m_inputStartLine && indexFrom < m_inputStartCol)))
                {
                    acceptEvent = false;
                    forwardEvent = false;
                }
                else
                {
                    acceptEvent = true;
                    forwardEvent = true;
                }
            }
            else
            {
                if (m_inputStreamWaitCond)
                {
                    if (lineFrom > m_inputStartLine || (lineFrom == m_inputStartLine && indexFrom > m_inputStartCol))
                    {
                        acceptEvent = true;
                        forwardEvent = true;
                    }
                    else if ((lineTo == m_inputStartLine && indexTo > m_inputStartCol) || (lineTo > m_inputStartLine))
                    {
                        setSelection(m_inputStartLine, m_inputStartCol, lineTo, indexTo);
                        acceptEvent = true;
                        forwardEvent = true;
                    }
                    else
                    {
                        acceptEvent = false;
                        forwardEvent = false;
                    }
                }
                else
                {
                    if (lineFrom > m_startLineBeginCmd || (lineFrom == m_startLineBeginCmd && indexFrom >= 2))
                    {
                        acceptEvent = true;
                        forwardEvent = true;
                    }
                    else if ((lineTo == m_startLineBeginCmd && indexTo > 2) || (lineTo > m_startLineBeginCmd))
                    {
                        setSelection(m_startLineBeginCmd, 2, lineTo, indexTo);
                        acceptEvent = true;
                        forwardEvent = true;
                    }
                    else
                    {
                        acceptEvent = false;
                        forwardEvent = false;
                    }
                }
            }
            break;

         case Qt::Key_C:
            if ((modifiers & Qt::ControlModifier))
            {
                copy();
                acceptEvent = true;
                forwardEvent = false;
            }
            else
            {
                moveCursorToValidRegion();
                acceptEvent = true;
                forwardEvent = true;
            }
            break;

        case Qt::Key_X:
            if ((modifiers & Qt::ControlModifier))
            {
                cut();
                acceptEvent = true;
                forwardEvent = false;
            }
            else
            {
                moveCursorToValidRegion();
                acceptEvent = true;
                forwardEvent = true;
            }
            break;

        case Qt::Key_V:
            if ((modifiers & Qt::ControlModifier))
            {
                paste();
                acceptEvent = true;
                forwardEvent = false;
            }
            else
            {
                moveCursorToValidRegion();
                acceptEvent = true;
                forwardEvent = true;
            }
            break;

        case Qt::Key_Control:
        case Qt::Key_Shift:
        case Qt::Key_Alt:
        case Qt::Key_AltGr:
            acceptEvent = false; //!< no action necessary
            forwardEvent = false;
            break;

        default:
            moveCursorToValidRegion();
            acceptEvent = true;
            forwardEvent = true;
            break;
        }

        if (acceptEvent && forwardEvent)
        {
            AbstractPyScintillaWidget::keyPressEvent(event);
        }
        else if (!acceptEvent)
        {
            event->ignore();
        }
        else if (acceptEvent && !forwardEvent)
        {
            event->accept();
        }
    }
    else
    {
        event->ignore();
    }
}

//----------------------------------------------------------------------------------------------------------------------------------
void ConsoleWidget::textDoubleClicked(int position, int line, int modifiers)
{
    if (modifiers == 0)
    {
        QString selectedText = text(line);

        //check for the following style '  File "x:\...py", line xxx, in ... and if found open the script at the given line to jump to the indicated error location in the script
        if (selectedText.startsWith("  File \""))
        {
            QRegExp rx("^  File \"(.*\\.[pP][yY])\", line (\\d+)(, in )?.*$");
            if (rx.indexIn(selectedText) >= 0)
            {
                ScriptEditorOrganizer *seo = qobject_cast<ScriptEditorOrganizer*>(AppManagement::getScriptEditorOrganizer());
                if (seo)
                {
                    bool ok;
                    int line = rx.cap(2).toInt(&ok);
                    if (ok)
                    {
                        seo->openScript(rx.cap(1), NULL, line - 1, true);
                    }
                }
            }
        }
    }
}

//----------------------------------------------------------------------------------------------------------------------------------
void ConsoleWidget::clearCommandLine()
{
    clear();
    m_startLineBeginCmd = -1;
}

//----------------------------------------------------------------------------------------------------------------------------------
void ConsoleWidget::startInputCommandLine(QSharedPointer<QByteArray> buffer, ItomSharedSemaphore *inputWaitCond)
{
    m_inputStreamWaitCond = inputWaitCond;
    m_inputStreamBuffer = buffer;
    m_inputStartLine = lines() - 1;
    m_inputStartCol = text(m_inputStartLine).size();
    markerAdd(m_inputStartLine, m_markInputLine);
    setFocus();
}

//----------------------------------------------------------------------------------------------------------------------------------
RetVal ConsoleWidget::executeCmdQueue()
{
    cmdQueueStruct value;

    if (m_cmdQueue.empty())
    {
        markerDeleteAll(m_markCurrentLine);
        if (m_waitForCmdExecutionDone)
        {
            m_waitForCmdExecutionDone = false;
            startNewCommand(false);

            //if further text has been removed within execCommand, it is appended now.
            //text, that is removed due to another run of python (not invoked by this command line),
            //is added in the pythonStateChanged method
            append(m_temporaryRemovedCommands);
            m_temporaryRemovedCommands = "";
            moveCursorToEnd();
        }
    }
    else
    {
        m_waitForCmdExecutionDone = true;
        m_canCut = false;
        m_canCopy = false;

        value = m_cmdQueue.front();
        m_cmdQueue.pop();

        markerDeleteAll(m_markCurrentLine);
        for (int i = 0; i < value.m_nrOfLines; i++)
        {
            markerAdd(value.m_lineBegin + i,m_markCurrentLine);
        }

        if (value.singleLine == "")
        {
            //do nothing, emit end of command
            executeCmdQueue();
        }
        else if (value.singleLine == "clc" || value.singleLine == "clear")
        {
            clear();
            m_startLineBeginCmd = -1;
            m_pCmdList->add(value.singleLine);
            executeCmdQueue();
            emit sendToLastCommand(value.singleLine);
        }
        else
        {
            //emit pythonExecuteString(value.singleLine);

            QObject *pyEngine = AppManagement::getPythonEngine(); //qobject_cast<PythonEngine*>(AppManagement::getPythonEngine());
            if (pyEngine)
            {
                QMetaObject::invokeMethod(pyEngine, "pythonExecStringFromCommandLine", Q_ARG(QString, value.singleLine));
            }
            else
            {
                QMessageBox::critical(this, tr("script execution"), tr("Python is not available"));
            }

            //connect(this, SIGNAL(pythonExecuteString(QString)), pyEngine, SLOT(pythonRunString(QString)));

            //pyThread->pythonInterruptExecution();

            m_pCmdList->add(value.singleLine);
            emit sendToLastCommand(value.singleLine);
        }

        autoAdaptLineNumberColumnWidth();
    }

    return RetVal(retOk);
}

//----------------------------------------------------------------------------------------------------------------------------------
RetVal ConsoleWidget::execCommand(int beginLine, int endLine)
{
    if (endLine<beginLine)
    {
        return RetVal(retError);
    }

    QString singleLine;
    QStringList buffer;

    if (beginLine == endLine)
    {
        singleLine = text(beginLine);
        if (singleLine.endsWith('\n'))
        {
            singleLine.chop(1);
        }
        if (singleLine.startsWith(">>"))
        {
            singleLine.remove(0, 2);
        }
        m_cmdQueue.push(cmdQueueStruct(singleLine, beginLine, 1));
    }
    else
    {
        for (int i = beginLine; i <= endLine; i++)
        {
            singleLine = text(i);
            if (singleLine.endsWith('\n'))
            {
                singleLine.chop(1);
            }
            if (singleLine.startsWith(">>"))
            {
                singleLine.remove(0, 2);
            }

            buffer.append(singleLine);
        }

        const PythonEngine *pyEng = PythonEngine::getInstance();
        QStringList temp;
        QByteArray encoding;
        singleLine = buffer.join("\n");
        QList<int> lines = pyEng->parseAndSplitCommandInMainComponents(singleLine.toLatin1().data(), encoding); //clc command will be accepted and parsed as single command -> this leads to our desired behaviour

        //if lines is empty, a syntax error occurred in the file and the python error indicator is set.
        //This will be checked in subsequent call of run-string or debug-string method.
        if (lines.length() == 0 || (encoding.length() > 0 && lines.length() == 1)) //probably error while execution, execute it in one block
        {
            if (encoding.length() > 0)
            {
                m_cmdQueue.push(cmdQueueStruct(singleLine, beginLine, 2));
            }
            else
            {
                m_cmdQueue.push(cmdQueueStruct(singleLine, beginLine, buffer.length()));
            }
        }
        else
        {
            lines.append(buffer.length() + 1); //append last index
            for (int i = 0; i < lines.length() - 1; i++)
            {
                temp = buffer.mid(lines[i] - 1 , lines[i+1] - lines[i]);

                //remove empty (besides whitechars) lines at the end of each block, else an error can occur if the block is indented
                while (temp.size() > 1)
                {
                    if (temp.last().trimmed() == "")
                    {
                        temp.removeLast();
                    }
                    else
                    {
                        //there is a line with content.
                        break;
                    }
                }

                singleLine = temp.join("\n");

                if (encoding.length() > 0)
                {
                    singleLine.prepend("#coding=" + encoding + "\n");
                }

                m_cmdQueue.push(cmdQueueStruct(singleLine, beginLine + lines[i] - 1, temp.length()));
            }
        }
    }

    //if endLine does not correspond to last line in command line, remove this part
    //and add it to m_temporaryRemovedCommands. It is added again after that python has been finished
    QStringList temp;

    for (int i = endLine + 1; i < lines(); i++)
    {
        temp.push_back(text(i));
    }
    m_temporaryRemovedCommands = temp.join("");
    setSelection(endLine + 1, 0, lines() - 1, lineLength(lines() - 1));
    removeSelectedText();

    m_waitForCmdExecutionDone = true;
    executeCmdQueue();

    return RetVal(retOk);
}

//----------------------------------------------------------------------------------------------------------------------------------
void ConsoleWidget::receiveStream(QString text, ito::QDebugStream::MsgStreamType msgType)
{
    int fromLine, toLine;

    switch (msgType)
    {
    case ito::QDebugStream::msgStreamErr:
        //case msgReturnError:
        //!> insert msg after last line
        fromLine = lines() - 1;
        append(text);
        toLine = lines() - 1;
        if (lineLength(toLine) == 0)
        {
            toLine--;
        }

        for (int i = fromLine; i <= toLine; ++i)
        {
            markerAdd(i, m_markErrorLine);
        }
        moveCursorToEnd();
        m_startLineBeginCmd = -1;
        if (!m_pythonBusy)
        {
            startNewCommand(false);
        }
        break;

    case ito::QDebugStream::msgStreamOut:
        //case msgReturnInfo:
        //case msgReturnWarning:
        //!> insert msg after last line
        append(text);
        moveCursorToEnd();
        m_startLineBeginCmd = -1;

        if (!m_pythonBusy)
        {
            startNewCommand(false);
        }
        break;

        //case msgTextInfo:
        //case msgTextWarning:
        //case msgTextError:
        //    //!> insert msg before last line containing ">>" (m_startLineBeginCmd)
        //    insertAt(totalMsg, m_startLineBeginCmd, 0);
        //    m_startLineBeginCmd += msg.length();
        //    moveCursorToEnd();
        //    break;
    }

    autoAdaptLineNumberColumnWidth();
}

//----------------------------------------------------------------------------------------------------------------------------------
RetVal ConsoleWidget::moveCursorToEnd()
{
    int lastLine = lines() - 1;
    setCursorPosition(lastLine, lineLength(lastLine));
    return RetVal(retOk);
}

//----------------------------------------------------------------------------------------------------------------------------------
void ConsoleWidget::dropEvent(QDropEvent * event)
{
    const QMimeData *md = event->mimeData();

    //check if a local python file will be dropped -> allow this
//    if (md->hasText() == false && (md->hasFormat("FileName") || md->hasFormat("text/uri-list")))
    if ((md->hasFormat("FileName") || md->hasFormat("text/uri-list")))
    {
        QObject *sew = AppManagement::getScriptEditorOrganizer();
        ito::UserOrganizer *uOrg = (UserOrganizer*)AppManagement::getUserOrganizer();

        if (uOrg->hasFeature(featDeveloper))
        {
            foreach (const QUrl &url, md->urls())
            {
                if (!url.isLocalFile() || !url.isValid())
                {
                    break;
                }

                QFileInfo file(url.toLocalFile());
                QString suffix = file.suffix().toLower();
                if (suffix != "py")
                {
                    break;
                }

                if (sew != NULL)
                {
                    QMetaObject::invokeMethod(sew, "openScript", Q_ARG(QString,QString(file.absoluteFilePath())), Q_ARG(ItomSharedSemaphore*,NULL));
                }
            }   
        }
    }
    else
    {
        QsciScintilla::dropEvent(event);
    }
    setFocus(); //set focus to this widget such that a key-press (e.g. return) after a drop is directly executed (useful if code from callstack is dropped)
}

//----------------------------------------------------------------------------------------------------------------------------------
void ConsoleWidget::dragEnterEvent(QDragEnterEvent * event)
{
    const QMimeData *md = event->mimeData();

    if (md->hasText())
    {
        event->acceptProposedAction();
    }
    else
    {
        ito::UserOrganizer *uOrg = (UserOrganizer*)AppManagement::getUserOrganizer();
        if (uOrg->hasFeature(featDeveloper))
        {
            //check if a local python file will be dropped -> allow this
            if (md->hasText() == false && (md->hasFormat("FileName") || md->hasFormat("text/uri-list")))
            {
                foreach (const QUrl &url, md->urls())
                {
                    if (!url.isLocalFile() || !url.isValid())
                    {
                        return; //not good
                    }

                    QString suffix = QFileInfo(url.toString()).suffix().toLower();
                    if (suffix != "py")
                    {
                        return; //not good
                    }
                }

                event->acceptProposedAction();
            }
        }
    }
}

//----------------------------------------------------------------------------------------------------------------------------------
void ConsoleWidget::dragMoveEvent(QDragMoveEvent * event)
{
    const QMimeData *md = event->mimeData();

    //check if a local python file will be dropped -> allow this
    if ((md->hasFormat("FileName") || md->hasFormat("text/uri-list")))
    {
        event->accept();
    }
    else
    {
        QsciScintilla::dragMoveEvent(event);

        //!< if text selected in this widget, starting point before valid region and move action -> ignore
        int lineFrom, lineTo, indexFrom, indexTo;

        //!< check, that selections are only in valid area
        getSelection(&lineFrom, &indexFrom, &lineTo, &indexTo);

        bool dragFromConsole = (event->source() == this);

        if (dragFromConsole && (lineFrom < m_startLineBeginCmd || (lineFrom == m_startLineBeginCmd && indexFrom < 2)))
        {
            if (event->dropAction() & Qt::MoveAction)
            {
                event->ignore();
            }
        }
        else
        {
            int res = checkValidDropRegion(event->pos());

            if (res==0)
            {
                event->ignore();
            }
            else
            {
                event->accept();
            }
        }
    }
}

//----------------------------------------------------------------------------------------------------------------------------------
/*
@return 0: pos invalid, 1: pos valid, 2: pos below last line
*/
int ConsoleWidget::checkValidDropRegion(const QPoint &pos)
{
    if (m_waitForCmdExecutionDone || m_pythonBusy)
    {
        return 0;
    }
    else
    {
        long position;
        int line, index;
        QPoint pos2 = pos;

        int margin = marginWidth(1) + marginWidth(2) + marginWidth(3) + marginWidth(4);

        pos2.setX(1+ margin);

        position = SendScintilla(SCI_POSITIONFROMPOINT, pos2.x(), pos2.y());
        if (position>=0)
        {
            lineIndexFromPosition(position, &line, &index);
        }
        else
        {
            line = -1;
        }

        if (line == -1)
        {
            //!< pos is below last line
            return 2;
        }
        else if (line == m_startLineBeginCmd)
        {
            if (pos.x() <= margin)
            {
                //!< mouse over margin left
                return 0;
            }
            else
            {
                position = SendScintilla(SCI_POSITIONFROMPOINT, pos.x(),pos.y());

                if (position == -1)
                {
                    return 2; //!< mouse at the end of this line
                }
                else
                {
                    lineIndexFromPosition(position, &line, &index);

                    if (index<2)
                    {
                        return 0;
                    }
                    else
                    {
                        return 1;
                    }
                }
            }
        }
        else if (line > m_startLineBeginCmd)
        {
            return 1;
        }
    }

    return 0;
}

//----------------------------------------------------------------------------------------------------------------------------------
void ConsoleWidget::selChanged()
{
    if (m_waitForCmdExecutionDone)
    {
        m_canCut = false;
        m_canCopy = false;
    }
    else
    {
        int lineFrom, lineTo, indexFrom, indexTo;
        getSelection(&lineFrom, &indexFrom, &lineTo, &indexTo);

        if (lineFrom == -1) //nothing selected
        {
            m_canCut = false;
            m_canCopy = false;
        }
        else if (lineFrom < m_startLineBeginCmd)
        {
            m_canCut = false;
            m_canCopy = true;
        }
        else if (lineFrom == m_startLineBeginCmd && indexFrom < 2)
        {
            m_canCut = false;
            m_canCopy = true;
        }
        else
        {
            m_canCut = true;
            m_canCopy = true;
        }
    }
}

//----------------------------------------------------------------------------------------------------------------------------------
void ConsoleWidget::copy()
{
    if (m_canCopy)
    {
        QsciScintilla::copy();

        QSettings settings(AppManagement::getSettingsFile(), QSettings::IniFormat);
        settings.beginGroup("PyScintilla");
        bool formatCopyCode = settings.value("formatCopyCode", "false").toBool();
        settings.endGroup();

        if (formatCopyCode)
        {
            QClipboard *clipboard = QApplication::clipboard();

            if (clipboard->mimeData()->hasText()) 
            {
                clipboard->setText(formatConsoleCodePart(clipboard->text()));
            }
        }
    }
}

//----------------------------------------------------------------------------------------------------------------------------------
void ConsoleWidget::paste()
{
    moveCursorToValidRegion();

    QSettings settings(AppManagement::getSettingsFile(), QSettings::IniFormat);
    settings.beginGroup("PyScintilla");
    bool formatPastCode = settings.value("formatPastCode", "false").toBool();
    settings.endGroup();

    QClipboard *clipboard = QApplication::clipboard();
    QString clipboardSave = "";

    if (formatPastCode)
    {
        if (clipboard->mimeData()->hasText()) 
        {
            clipboardSave = clipboard->text();
            int lineCount;
            clipboard->setText(formatPhytonCodePart(clipboard->text(), lineCount));
        }
    }

    QsciScintilla::paste();

    if (clipboardSave != "")
    {
        clipboard->setText(clipboardSave);
    }
}

//----------------------------------------------------------------------------------------------------------------------------------
void ConsoleWidget::cut()
{
    if (m_canCut)
    {
        QsciScintilla::cut();
    }
}

//----------------------------------------------------------------------------------------------------------------------------------
RetVal ConsoleWidget::moveCursorToValidRegion()
{
    int lineFrom, lineTo, indexFrom, indexTo;

    //!< check, that selections are only in valid area
    getSelection(&lineFrom, &indexFrom, &lineTo, &indexTo);
    if (lineFrom == -1)
    {
        getCursorPosition(&lineFrom, &indexFrom);
    }

    if (lineFrom == -1)
    {
        //do nothing
    }
    else if (lineFrom < m_startLineBeginCmd)
    {
        moveCursorToEnd();
    }
    else if (lineFrom == m_startLineBeginCmd && indexFrom < 2)
    {
        moveCursorToEnd();
    }

    return RetVal(retOk);
}

//----------------------------------------------------------------------------------------------------------------------------------
void ConsoleWidget::pythonRunSelection(QString selectionText)
{
    // we have to remove the indent
    if (selectionText.length() > 0)
    {
        int lineCount = 0;
        selectionText = formatPhytonCodePart(selectionText, lineCount);

        if (selectionText.endsWith("\n"))
        {
            insertAt(selectionText, m_startLineBeginCmd, 2);
        }
        else
        {
            insertAt(selectionText + "\n", m_startLineBeginCmd, 2);
        }

        execCommand(m_startLineBeginCmd, m_startLineBeginCmd + lineCount - 1);
    }
}

//----------------------------------------------------------------------------------------------------------------------------------
//----------------------------------------------------------------------------------------------------------------------------------
DequeCommandList::DequeCommandList(int maxLength)
{
    m_maxItems = maxLength;
    m_cmdList.clear();
    m_cmdList.push_back(QString());
    m_rit = m_cmdList.rbegin();
}

//----------------------------------------------------------------------------------------------------------------------------------
DequeCommandList::~DequeCommandList()
{
    m_cmdList.clear();
}

//----------------------------------------------------------------------------------------------------------------------------------
RetVal DequeCommandList::add(const QString &cmd)
{
    moveLast();
    *m_rit = cmd;
    m_cmdList.push_back(QString());

    if (static_cast<int>(m_cmdList.size()) > m_maxItems)
    {
        m_cmdList.pop_front();
    }

    moveLast();

    return RetVal(retOk);
}

//----------------------------------------------------------------------------------------------------------------------------------
RetVal DequeCommandList::moveLast()
{
    m_rit = m_cmdList.rbegin();
    return RetVal(retOk);
}

//----------------------------------------------------------------------------------------------------------------------------------
QString DequeCommandList::getPrevious()
{
    if (m_cmdList.size() > 1)
    {
        if (m_rit < m_cmdList.rend())
        {
            if ((++m_rit) < m_cmdList.rend())
            {
                return *m_rit;
            }
        }
        else
        {
            moveLast();
            return getPrevious();
        }
    }

    return QString();
}

//----------------------------------------------------------------------------------------------------------------------------------
QString DequeCommandList::getNext()
{
    if (m_cmdList.size() > 1 && m_rit > m_cmdList.rbegin())
    {
        --m_rit;
        return *m_rit;
    }

    return QString();
}

} //end namespace ito