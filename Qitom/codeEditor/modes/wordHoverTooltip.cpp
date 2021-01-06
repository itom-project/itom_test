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

    Further hints:
    ------------------------

    This file belongs to the code editor of itom. The code editor is
    in major parts a fork / rewritten version of the python-based source 
    code editor PyQode from Colin Duquesnoy and others 
    (see https://github.com/pyQode). PyQode itself is licensed under 
    the MIT License (MIT).

    Some parts of the code editor of itom are also inspired by the
    source code editor of the Spyder IDE (https://github.com/spyder-ide),
    also licensed under the MIT License and developed by the Spyder Project
    Contributors. 

*********************************************************************** */

#include "wordHoverTooltip.h"

#include "../codeEditor.h"
#include "../utils/utils.h"
#include "../managers/textDecorationsManager.h"
#include "../managers/panelsManager.h"
#include "../delayJobRunner.h"
#include "../toolTip.h"

#include "common/sharedStructures.h"
#include "../../AppManagement.h"
#include "../../python/pythonEngine.h"
#include "../../widgets/scriptEditorWidget.h"

#include <qdir.h>

namespace ito {

//-------------------------------------------------------------------------------------
/*
*/
WordHoverTooltipMode::WordHoverTooltipMode(const QString &name /*="WordHoverTooltipMode"*/, 
                                           const QString &description /*= ""*/, 
                                           QObject *parent /*= nullptr*/) :
    QObject(parent),
    Mode(name, description),
    m_pTimer(nullptr),
    m_pPythonEngine(AppManagement::getPythonEngine()),
    m_requestCount(0),
    m_tooltipsMaxLength(300)
{
    m_pTimer = new DelayJobRunnerArgTextCursor<WordHoverTooltipMode, void(WordHoverTooltipMode::*)(QTextCursor)>(400);
}

//-------------------------------------------------------------------------------------
/*
*/
WordHoverTooltipMode::~WordHoverTooltipMode()
{
    DELETE_AND_SET_NULL(m_pTimer);
}

//-------------------------------------------------------------------------------------
/*
*/
/*virtual*/ void WordHoverTooltipMode::onStateChanged(bool state)
{
    CodeEditor *edit = editor();

    if (state)
    {
        connect(edit, &CodeEditor::mouseMoved, this, &WordHoverTooltipMode::onMouseMoved);
    }
    else
    {
        disconnect(edit, &CodeEditor::mouseMoved, this, &WordHoverTooltipMode::onMouseMoved);
    }
}


//-------------------------------------------------------------------------------------
/*
mouse moved callback
*/
void WordHoverTooltipMode::onMouseMoved(QMouseEvent *e)
{
    QPoint mousePos = e->pos();
    QTextCursor textCursor;
    
    if (e->modifiers() == Qt::NoModifier)
    {
        textCursor = editor()->cursorForPosition(mousePos);

        //check if textCursor is not behind end of line
        QTextCursor endOfLineCursor(textCursor);
        endOfLineCursor.movePosition(QTextCursor::EndOfLine, QTextCursor::MoveAnchor);

        if (textCursor.position() >= endOfLineCursor.position())
        {
            textCursor = QTextCursor();
        }
        else
        {
            QTextCursor startOfLineCursor(textCursor);
            startOfLineCursor.movePosition(QTextCursor::StartOfLine, QTextCursor::KeepAnchor);
            QString trimmedText = startOfLineCursor.selectedText().trimmed();

            if (trimmedText == "" || trimmedText.startsWith("#"))
            {
                // only whitespaces or tabs are left to the cursor.
                // or the line starts with a comment.
                textCursor = QTextCursor();
            }
        }
    }

    if (textCursor.isNull())
    {
        m_pTimer->cancelRequests();
        ToolTip::hideText();
    }
    else if ((m_cursor.isNull() || textCursor.position() != m_cursor.position()))
    {
        DELAY_JOB_RUNNER_ARGTEXTCURSOR(
            m_pTimer, 
            WordHoverTooltipMode, 
            void(WordHoverTooltipMode::*)(QTextCursor)
        )->requestJob(this, &WordHoverTooltipMode::emitWordHover, textCursor);
    }

    m_cursor = textCursor;
}

//-------------------------------------------------------------------------------------
void WordHoverTooltipMode::emitWordHover(QTextCursor cursor)
{
    // show tooltip
    QRect cursorRect = editor()->cursorRect(cursor);
    QPoint position(
        cursorRect.x() + editor()->panels()->marginSize(ito::Panel::Left),
        cursorRect.y() + 
        editor()->panels()->marginSize(ito::Panel::Top));
    position = editor()->mapToGlobal(position);

    QString loadingText = tr("Loading...");
    ToolTip::showText(position, loadingText, editor(), QRect());

    PythonEngine *pyEng = (PythonEngine*)m_pPythonEngine;

    if (pyEng && (m_requestCount == 0))
    {
        ScriptEditorWidget *sew = qobject_cast<ScriptEditorWidget*>(editor());
        QString filename;
        if (sew)
        {
            filename = sew->getFilename();
        }

        if (filename == "")
        {
            filename = QDir::cleanPath(QDir::current().absoluteFilePath("__temporaryfile__.py"));
        }

        if (pyEng->tryToLoadJediIfNotYetDone())
        {
            m_requestCount += 1;

            ito::JediGetHelpRequest request;
            request.m_callbackFctName = "onJediGetHelpResultAvailable";
            request.m_col = cursor.positionInBlock();
            request.m_line = cursor.block().blockNumber();
            request.m_path = filename;
            request.m_sender = this;
            request.m_source = editor()->toPlainText();

            pyEng->enqueueJediGetHelpRequest(request);
        }
        else
        {
            onStateChanged(false);
        }
    }
}

//--------------------------------------------------------------------
QPair<QStringList, QString>  WordHoverTooltipMode::parseTooltipDocstring(const QString &docstring) const
{
    QStringList lines = Utils::strip(docstring).split("\n");
    QStringList signatures;
    int idx = 0;

    for (; idx < lines.size(); ++idx)
    {
        if (lines[idx] == "" || (lines[idx][0] == ' ' && lines[idx].trimmed() == ""))
        {
            // empty line or line with spaces only. skip. the real docstring comes now.
            break;
        }

        signatures << lines[idx];
    }

    QString docstr = lines.mid(idx + 1).join("\n");

    if (docstr.size() > m_tooltipsMaxLength)
    {
        int idx = docstr.lastIndexOf(' ', m_tooltipsMaxLength);

        if (idx > 0)
        {
            docstr = docstr.left(idx);
        }
        else
        {
            docstr = docstr.left(m_tooltipsMaxLength);
        }

        docstr += tr("...");
    }

    return qMakePair<QStringList, QString>(signatures, docstr);
}

//-------------------------------------------------------------------------------------
void WordHoverTooltipMode::onJediGetHelpResultAvailable(QVector<ito::JediGetHelp> helps)
{
    m_requestCount--;

    if (helps.size() < 1)
    {
        ToolTip::hideText();
        return;
    }

    const ito::JediGetHelp &help = helps[0];
    QList<QPair<QStringList, QString>> tooltips;
    QPair<QStringList, QString> tooltip;

    if (help.m_tooltips.size() > 0)
    {
        foreach(const QString &tt, help.m_tooltips)
        {
            if (tt != "")
            {
                tooltip = parseTooltipDocstring(tt);

                if (tooltips.size() > 0 && tooltips.last().second == tooltip.second)
                {
                    //same docstring -> add signatures to previous
                    tooltips[tooltips.size() - 1].first.append(tooltip.first);
                }
                else
                {
                    tooltips.append(tooltip);
                }
            }
        }
    }

    if (tooltips.size() == 0 && help.m_description != "")
    {
        tooltips.append(parseTooltipDocstring(help.m_description));
    }

    /* tasks: convert tooltip to html, check for the first
    section with the definitions and wrap after maximum line length.
    Make a <hr> after the first section
    */
    const QString br("<br>");
    QStringList styledTooltips;

    foreach(const auto &tip, tooltips)
    {
        QString signatures;
        QString docstring;

        QStringList defs = tip.first;

        // the signature is represented as <code> monospace section.
        // this requires much more space than ordinary letters. 
        // Therefore reduce the maximum line length to 88/2.
        const int maxLength = 44;

        for (int i = 0; i < defs.size(); ++i)
        {
            if (defs[i].size() > maxLength)
            {
                defs[i] = Utils::signatureWordWrap(defs[i], maxLength);
            }
        }

        signatures = defs.join("\n");
        docstring = tip.second;

        signatures = signatures.toHtmlEscaped().replace("    ", "&nbsp;&nbsp;&nbsp;&nbsp;");
        QStringList s = signatures.split('\n', QString::SkipEmptyParts);

        if (s.size() > 0)
        {
            signatures = "<nobr>" + s.join("</nobr><br><nobr>") + "</nobr>";
        }
        else
        {
            signatures = "";
        }

        docstring = docstring.toHtmlEscaped();
        docstring = docstring.replace('\n', br);

        if (signatures != "" && docstring != "")
        {
            styledTooltips.append(QString("<code>%1</code><hr><nobr>%2</nobr>").arg(signatures).arg(docstring));
        }
        else if (signatures != "")
        {
            styledTooltips.append(QString("<code>%1</code>").arg(signatures));
        }
        else if (docstring != "")
        {
            styledTooltips.append(QString("<nobr>%1</nobr>").arg(docstring));
        }
    }

    if (styledTooltips.size() == 0)
    {
        ToolTip::hideText();
        return;
    }

    if (!m_cursor.isNull())
    {
        QRect cursorRect = editor()->cursorRect(m_cursor);
        QPoint position(
            cursorRect.x() + editor()->panels()->marginSize(ito::Panel::Left),
            cursorRect.y() +
            editor()->panels()->marginSize(ito::Panel::Top));
        position = editor()->mapToGlobal(position);

        ToolTip::showText(position, styledTooltips.join("<hr>"), editor(), QRect());
    }
}

} //end namespace ito