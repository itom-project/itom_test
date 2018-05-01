

#include <QtGui>
#include <qapplication.h>

#include "codeEditor.h"
#include "modes/caretLineHighlight.h"
#include "modes/symbolMatcherMode.h"
#include "modes/occurrences.h"
#include "modes/autoindent.h"
#include "modes/pyAutoIndent.h"
#include "modes/indenter.h"
#include "panels/lineNumber.h"
#include "syntaxHighlighter/pythonSyntaxHighlighter.h"
#include "managers/modesManager.h"
#include "managers/panelsManager.h"

int main(int argv, char **args)
{
    //Q_INIT_RESOURCE(customcompleter);

    QApplication app(argv, args);

    //TextEdit edit;
    CodeEditor editor;

    editor.resize(800, 600);
    editor.modes()->append(Mode::Ptr(new CaretLineHighlighterMode("description of caret line highlighter mode")));
    editor.modes()->append(Mode::Ptr(new PythonSyntaxHighlighter(editor.document(), "description of PythonSyntaxHighlighter")));
    editor.modes()->append(Mode::Ptr(new SymbolMatcherMode("description of SymbolMatcherMode")));
    editor.modes()->append(Mode::Ptr(new OccurrencesHighlighterMode("description of OccurrencesHighlighterMode")));
    editor.modes()->append(Mode::Ptr(new PyAutoIndentMode("description of PyAutoIndentMode")));
    editor.modes()->append(Mode::Ptr(new IndenterMode("description of IndenterMode")));
    
    editor.panels()->append(Panel::Ptr(new LineNumberPanel("description of LineNumberPanel")));
    
    //editor.appendPlainText("\n\n\n\n\n\n\n\n\n\n");
    //editor.appendPlainText("(----(j\njj)\n)");

    editor.setWindowTitle("Code Editor Example");
    editor.show();

    return app.exec();
}

