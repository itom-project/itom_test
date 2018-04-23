#include "pythonSyntaxHighlighter.h"

#include "codeEditor.h"
#include <qapplication.h>
#include <qtextdocument.h>

#include "managers/modesManager.h"
#include "managers/panelsManager.h"
#include "modes/caretLineHighlight.h"
#include "utils/utils.h"

#include <qdebug.h>

/*static*/ QMap<QString,PythonSyntaxHighlighter::QQRegExp> PythonSyntaxHighlighter::regExpProg = PythonSyntaxHighlighter::makePythonPatterns();
/*static*/ QRegExp PythonSyntaxHighlighter::regExpIdProg = QRegExp("\\s+(\\w+)");
/*static*/ QRegExp PythonSyntaxHighlighter::regExpAsProg = QRegExp(".*?\\b(as)\\b");
/*static*/ PythonSyntaxHighlighter::QQRegExp PythonSyntaxHighlighter::regExpOeComment = PythonSyntaxHighlighter::QQRegExp("^(// ?--[-]+|##[#]+ )[ -]*[^- ]+");


//-------------------------------------------------------------------
PythonSyntaxHighlighter::PythonSyntaxHighlighter(QTextDocument *parent, const QString &description /*= ""*/, const ColorScheme &colorScheme /*=  = ColorScheme()*/) :
    SyntaxHighlighterBase("PythonSyntaxHighlighter", parent, description, colorScheme)
{

}

//-------------------------------------------------------------------
PythonSyntaxHighlighter::~PythonSyntaxHighlighter()
{
}

//-------------------------------------------------------------------
const QTextCharFormat PythonSyntaxHighlighter::getTextCharFormat(const QString &colorName, const QString &style)
{
    QTextCharFormat charFormat;
    QColor color(colorName);
    charFormat.setForeground(color);
    if (style.contains("bold", Qt::CaseInsensitive))
    {
        charFormat.setFontWeight(QFont::Bold);
    }
    if (style.contains("italic", Qt::CaseInsensitive))
    {
        charFormat.setFontItalic(true);
    }
    return charFormat;
}






//-------------------------------------------------------------------
/*
Highlights the block using a pygments lexer.

:param text: text of the block to highlight
:param block: block to highlight
*/
void PythonSyntaxHighlighter::highlight_block(const QString &text, QTextBlock &block)
{
    QTextBlock prev_block = block.previous();
    int prev_state = Utils::TextBlockHelper::getState(prev_block);
    int offset;
    QString text2 = text;

    if (prev_state == InsideDq3String)
    {
        offset = -4;
        text2 = "\"\"\" " + text;
    }
    else if (prev_state == InsideSq3String)
    {
        offset = -4;
        text2 = "''' " + text;
    }
    else if (prev_state == InsideDqString)
    {
        offset = -2;
        text2 = "\" " + text;
    }
    else if (prev_state == InsideSqString)
    {
        offset = -2;
        text2 = "' " + text;
    }
    else
    {
        offset = 0;
    }

    QString import_stmt;
        
    //set docstring dynamic attribute, used by the fold detector.
    TextBlockUserData *userData = static_cast<TextBlockUserData*>(block.userData());
    if (userData == NULL)
    {
        userData = new TextBlockUserData();
        block.setUserData(userData);
    }

    userData->m_docstring = false;

    setFormat(0, text2.size(), getFormatFromStyle(ColorScheme::KeyNormal));

    State state = Normal;

    QMap<QString, QQRegExp>::const_iterator it = PythonSyntaxHighlighter::regExpProg.constBegin();
    QString key;
    int pos = 0;
    int length;
    int start;
    int end;
    QString value;

#if QT_VERSION >= MIN_QT_REGULAREXPRESSION_VERSION
    QQRegExp regExpProg_ = it.value();
    QStringList namedCaptureGroups;
    foreach (const QString &key, regExpProg_.namedCaptureGroups())
    {
        if (key != "")
        {
            namedCaptureGroups << key;
        }
    }

    QRegularExpressionMatch match;
    match = regExpProg_.match(text2);
    while (match.hasMatch())
    {
        pos = 0;
        foreach(const QString &key, namedCaptureGroups)
        {
            start = match.capturedStart(key);
            if (start == -1)
            {
                continue;
            }
            value = match.captured(key);
            end = match.capturedEnd(key);
            pos = std::max(pos, end);
            start = std::max(0, start + offset);
            end = std::max(0, end + offset);
            length = match.capturedLength(key);

#else
    pos = 0;
    bool found = true;
    int pos_;

    while (found && (pos < text2.size()))
    {
        found = false;
        pos_ = pos;
        while (it != PythonSyntaxHighlighter::regExpProg.constEnd())
        {
            key = it.key();
            start = it->indexIn(text2, pos_);
            if (start == -1)
            {
                ++it;
                continue;
            }

            found = true;
            length = it->matchedLength();
            value = text2.mid(start, length);
            end = start + length;
            pos = std::max(pos, end);
            start = std::max(0, start + offset);
            end = std::max(0, end + offset);
            ++it;
#endif

            qDebug() << key << start << end << value.toHtmlEscaped() << QString::number(value.right(1)[0].cell(),16) << QString::number(value.right(1)[0].row(),16);

            if (key == "uf_sq3string")
            {
                setFormat(start, length,
                                getFormatFromStyle(ColorScheme::KeyDocstring));
                userData->m_docstring = true;
                state = InsideSq3String;
            }
            else if (key == "uf_dq3string")
            {
                setFormat(start, length,
                                getFormatFromStyle(ColorScheme::KeyDocstring));
                userData->m_docstring = true;
                state = InsideDq3String;
            }
            else if (key == "uf_sqstring")
            {
                setFormat(start, length,
                                getFormatFromStyle(ColorScheme::KeyString));
                state = InsideSqString;
            }
            else if (key == "uf_dqstring")
            {
                setFormat(start, length,
                                getFormatFromStyle(ColorScheme::KeyString));
                state = InsideDqString;
            }
            else if (key == "builtin_fct")
            {
                //trick to highlight __init__, __add__ and so on with
                //builtin color
                setFormat(start, length,
                                getFormatFromStyle(ColorScheme::KeyConstant));
            }
            else
            {
                if ((value.contains("\"\"\"") || value.contains("'''")) && key != "comment")
                {
                    // highlight docstring with a different color
                    userData->m_docstring = true;
                    setFormat(start, length,
                                    getFormatFromStyle(ColorScheme::KeyDocstring));
                }
                else if (key == "decorator")
                {
                    // highlight decorators
                    setFormat(start, length,
                                    getFormatFromStyle(ColorScheme::KeyDecorator));
                }
                else if (value == "self" || value == "cls")
                {
                    // highlight self attribute
                    setFormat(start, length,
                                    getFormatFromStyle(ColorScheme::KeySelf));
                }
                else if (key == "number")
                {
                    setFormat(start, length,
                                    getFormatFromStyle(ColorScheme::KeyNumber));
                }
                else if (key == "keyword")
                {
                    setFormat(start, length,
                                    getFormatFromStyle(ColorScheme::KeyKeyword));
                }
                else if (key == "comment")
                {
                    setFormat(start, length,
                                    getFormatFromStyle(ColorScheme::KeyComment));
                }
                else if (key == "operator_word")
                {
                    setFormat(start, length,
                                    getFormatFromStyle(ColorScheme::KeyOperatorWord));
                }
                else if (key == "string")
                {
                    setFormat(start, length,
                                    getFormatFromStyle(ColorScheme::KeyString));
                }
                else if (key == "namespace")
                {
                    setFormat(start, length,
                                    getFormatFromStyle(ColorScheme::KeyNamespace));
                }
                else if (key == "builtin")
                {
                    setFormat(start, length,
                                    getFormatFromStyle(ColorScheme::KeyBuiltin));
                }
                else
                {
                    // highlight all other tokens
                    setFormat(start, length,
                                    getFormatFromStyle(ColorScheme::KeyTag /*key*/)); //TODO
                }

                if (key == "keyword")
                {
                    if (value == "def" || value == "class")
                    {
                        int pos2 = PythonSyntaxHighlighter::regExpIdProg.indexIn(text2, end);
                        if (pos2 >= 0)
                        {
                            int start1 = pos2;
                            int end1 = start1 + PythonSyntaxHighlighter::regExpIdProg.matchedLength();
                            QTextCharFormat fmt = getFormatFromStyle(value == "class" ? ColorScheme::KeyDefinition : ColorScheme::KeyFunction);
                            setFormat(start1, end1 - start1, fmt);
                        }
                    }
                }
                else if (key == "namespace")
                {
                    import_stmt = text2.trimmed();
                    // color all the "as" words on same line, except
                    // if in a comment; cheap approximation to the
                    // truth
                    int endpos;
                    if (text2.contains('#'))
                    {
                        endpos = text2.indexOf('#');
                    }
                    else
                    {
                        endpos = text2.size();
                    }

                    int pos3 = 0;
                    while ((pos3 = PythonSyntaxHighlighter::regExpAsProg.indexIn(text2.left(endpos), end)) != -1)
                    {
                        setFormat(pos3, pos3 + PythonSyntaxHighlighter::regExpAsProg.matchedLength(), getFormatFromStyle(ColorScheme::KeyNamespace));
                        pos3 += PythonSyntaxHighlighter::regExpAsProg.matchedLength();
                    }
                }
            }
#if QT_VERSION >= MIN_QT_REGULAREXPRESSION_VERSION
        }

        match = regExpProg_.match(text2, pos);
    }
#else
        }
    }
#endif

    Utils::TextBlockHelper::setState(block, state);

    //update import zone
    if (import_stmt != "")
    {
        //block.import_stmt = import_stmt;
        userData->m_importStmt = true;
        m_importStatements.append(block);
        //block.import_stmt = true;
    }
    else if (userData->m_docstring)
    {
        m_docstrings.append(block);
    }
}



//-------------------------------------------------------------------
/*
Returns a QTextCharFormat for token
*/
QTextCharFormat PythonSyntaxHighlighter::getFormatFromStyle(ColorScheme::Keys token) const
{
    return m_colorScheme[token];
}

//--------------------------------------------------------------------
/*virtual*/ void PythonSyntaxHighlighter::rehighlight()
{
    m_docstrings.clear();
    m_importStatements.clear();
}


//----------------------------------------------------------------
QString any(const QString &name, const QStringList &alternates)
{
#if QT_VERSION >= MIN_QT_REGULAREXPRESSION_VERSION
    //Return a named group pattern matching list of alternates.
    return QString("(?<%1>%2)").arg(name).arg(alternates.join("|"));
#else
    return QString("(%1)").arg(alternates.join("|"));
#endif
}


//----------------------------------------------------------------
/*static*/ QMap<QString, PythonSyntaxHighlighter::QQRegExp> PythonSyntaxHighlighter::makePythonPatterns(const QStringList &additionalKeywords, const QStringList &additionalBuiltins)
{
    QMap<QString, QQRegExp> regExpressions;

    QStringList kwlist = QStringList() << "self" << "False" << "None" << "True" << "assert" << "break" \
        << "class" << "continue" << "def" << "del" << "elif" << "else" << "except" << "finally" << "for" \
        << "global" << "if" << "lambda" << "nonlocal" << "pass" << "raise" << "return" << "try" \
        << "while" << "with" << "yield";

    QStringList kwNamespaceList = QStringList() << "from" << "import" << "as";
    QStringList wordopList = QStringList() << "and" << "or" << "not" << "in" << "is";

    //Strongly inspired from idlelib.ColorDelegator.make_pat
    QString kw = "\\b" + any("keyword", kwlist + additionalKeywords) + "\\b";
    QString kw_namespace = "\\b" + any("namespace", kwNamespaceList) + "\\b";
    QString word_operators = "\\b" + any("operator_word", wordopList) + "\\b";
    
    //TODO: obtain the following list by the following python script:
    /*
    import builtins
    text = ["\"%s\"" % str(name) for name in dir(builtins) if not name.startswith('_')]
    print("QStringList() << " + " << ".join(text) + ";")
    */
    //The following builtins are based on Python 3.4
    QStringList builtinlist = QStringList() << "ArithmeticError" << "AssertionError" << "AttributeError" << \
        "BaseException" << "BlockingIOError" << "BrokenPipeError" << "BufferError" << "BytesWarning" << \
        "ChildProcessError" << "ConnectionAbortedError" << "ConnectionError" << "ConnectionRefusedError" << \
        "ConnectionResetError" << "DeprecationWarning" << "EOFError" << "Ellipsis" << "EnvironmentError" << \
        "Exception" << "False" << "FileExistsError" << "FileNotFoundError" << "FloatingPointError" << "FutureWarning" << \
        "GeneratorExit" << "IOError" << "ImportError" << "ImportWarning" << "IndentationError" << "IndexError" << \
        "InterruptedError" << "IsADirectoryError" << "KeyError" << "KeyboardInterrupt" << "LookupError" << "MemoryError" << \
        "NameError" << "None" << "NotADirectoryError" << "NotImplemented" << "NotImplementedError" << "OSError" << "OverflowError" << \
        "PendingDeprecationWarning" << "PermissionError" << "ProcessLookupError" << "ReferenceError" << "ResourceWarning" << "RuntimeError" << \
        "RuntimeWarning" << "StopIteration" << "SyntaxError" << "SyntaxWarning" << "SystemError" << "SystemExit" << "TabError" << \
        "TimeoutError" << "True" << "TypeError" << "UnboundLocalError" << "UnicodeDecodeError" << "UnicodeEncodeError" << "UnicodeError" << \
        "UnicodeTranslateError" << "UnicodeWarning" << "UserWarning" << "ValueError" << "Warning" << "WindowsError" << "ZeroDivisionError" << \
        "abs" << "all" << "any" << "ascii" << "bin" << "bool" << "bytearray" << "bytes" << "callable" << "chr" << "classmethod" << \
        "compile" << "complex" << "copyright" << "credits" << "delattr" << "dict" << "dir" << "divmod" << "enumerate" << "eval" << \
        "exec" << "exit" << "filter" << "float" << "format" << "frozenset" << "getattr" << "globals" << "hasattr" << "hash" << "help" << \
        "hex" << "id" << "input" << "int" << "isinstance" << "issubclass" << "iter" << "len" << "license" << "list" << "locals" << "map" << \
        "max" << "memoryview" << "min" << "next" << "object" << "oct" << "open" << "ord" << "pow" << "print" << "property" << "quit" << \
        "range" << "repr" << "reversed" << "round" << "set" << "setattr" << "slice" << "sorted" << "staticmethod" << "str" << "sum" << \
        "super" << "tuple" << "type" << "vars" << "zip";
    builtinlist << additionalBuiltins;
    builtinlist.removeAll("None");
    builtinlist.removeAll("True");
    builtinlist.removeAll("False");
    QString builtin = "([^.'\"\\#]\\b|^)" + any("builtin", builtinlist) + "\\b";
    QString builtin_fct = any("builtin_fct", QStringList("_{2}[a-zA-Z_]*_{2}"));
    QString comment = any("comment", QStringList("#[^\\n]*"));
    QString instance = any("instance", QStringList("\\bself\\b") << "\\bcls\\b");
    QString decorator = any("decorator",  QStringList("@\\w*") << ".setter");
    QString number = any("number", QStringList() << \
                 "\\b[+-]?[0-9]+[lLjJ]?\\b" <<
                  "\\b[+-]?0[xX][0-9A-Fa-f]+[lL]?\\b" <<
                  "\\b[+-]?0[oO][0-7]+[lL]?\\b" <<
                  "\\b[+-]?0[bB][01]+[lL]?\\b" <<
                  "\\b[+-]?[0-9]+(?:\\.[0-9]+)?(?:[eE][+-]?[0-9]+)?[jJ]?\\b");
    QString prefix = "r|u|R|U|f|F|fr|Fr|fR|FR|rf|rF|Rf|RF|b|B|br|Br|bR|BR|rb|rB|Rb|RB";
                                                                              //"(\\b(b|u))?'[^'\\\\\\n]*(\\\\.[^'\\\\\\n]*)*'?"
    QString sqstring = QString("(\\b(%1))?'[^'\\\\\\n]*(\\\\.[^'\\\\\\n]*)*'?").arg(prefix); //"(\\b(%1))?'[^'\\\\\\n]*(\\\\.[^'\\\\\\n]*)*'?";
    QString dqstring = QString("(\\b(%1))?\"[^\"\\\\\\n]*(\\\\.[^\"\\\\\\n]*)*\"?").arg(prefix); //"(\\b(%1))?\"[^\"\\\\\\n]*(\\\\.[^\"\\\\\\n]*)*\"?";
    QString uf_sqstring = QString("(\\b(%1))?'[^'\\\\\\n]*(\\\\.[^'\\\\\\n]*)*(\\\\)\\n(?!')$").arg(prefix);
    QString uf_dqstring = QString("(\\b(%1))?\"[^\"\\\\\\n]*(\\\\.[^\"\\\\\\n]*)*(\\\\)\\n(?!\")$").arg(prefix);
    QString sq3string =    QString("(\\b(%1))?'''[^'\\\\]*((\\\\.|'(?!''))[^'\\\\]*)*(''')?").arg(prefix);
    QString dq3string =    QString("(\\b(%1))?\"\"\"[^\"\\\\]*((\\\\.|\"(?!\"\"))[^\"\\\\]*)*(\"\"\")?").arg(prefix);
    QString uf_sq3string = QString("(\\b(%1))?'''[^'\\\\]*((\\\\.|'(?!''))[^'\\\\]*)*(\\\\)?(?!''')$").arg(prefix);
    QString uf_dq3string = QString("(\\b(%1))?\"\"\"[^\"\\\\]*((\\\\.|\"(?!\"\"))[^\"\\\\]*)*(\\\\)?(?!\"\"\")$").arg(prefix);
    QString string = any("string", QStringList() << sq3string << dq3string << sqstring << dqstring);
    //string = any("string", QStringList() << sq3string << sqstring);
    QString ufstring1 = any("uf_sqstring", QStringList(uf_sqstring));
    QString ufstring2 = any("uf_dqstring", QStringList(uf_dqstring));
    QString ufstring3 = any("uf_sq3string", QStringList(uf_sq3string));
    QString ufstring4 = any("uf_dq3string", QStringList(uf_dq3string));

#if QT_VERSION > MIN_QT_REGULAREXPRESSION_VERSION
    QStringList all;
    all << instance << decorator << kw << kw_namespace << builtin << word_operators << builtin_fct << comment;
    all << ufstring1 << ufstring2 << ufstring3 << ufstring4 << string << number << any("SNYC", QStringList("\\n"));
    regExpressions["all"] = QQRegExp(all.join("|"));
#else
    regExpressions["instance"] = QQRegExp(instance);
    regExpressions["decorator"] = QQRegExp(decorator);
    regExpressions["keyword"] = QQRegExp(kw);
    regExpressions["namespace"] = QQRegExp(kw_namespace);
    regExpressions["builtin"] = QQRegExp(builtin);
    regExpressions["operator_word"] = QQRegExp(word_operators);
    regExpressions["builtin_fct"] = QQRegExp(builtin_fct);
    regExpressions["comment"] = QQRegExp(comment);
    regExpressions["uf_sqstring"] = QQRegExp(ufstring1);
    regExpressions["uf_dqstring"] = QQRegExp(ufstring2);
    regExpressions["uf_sq3string"] = QQRegExp(ufstring3);
    regExpressions["uf_dq3string"] = QQRegExp(ufstring4);
    regExpressions["string"] = QQRegExp(string);
    regExpressions["number"] = QQRegExp(number);
    regExpressions["SYNC"] = QQRegExp(any("SYNC", QStringList("\\n")));
#endif

    return regExpressions;
}



