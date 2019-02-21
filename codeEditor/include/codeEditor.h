#ifndef CODEEDITOR_H
#define CODEEDITOR_H

#include <qplaintextedit.h>
#include <qcolor.h>
#include <qset.h>
#include <qpair.h>
#include <qtextobject.h>
#include <qevent.h>
#include <qpoint.h>

#include "textDecoration.h"
#include "syntaxHighlighter/syntaxHighlighterBase.h"

struct VisibleBlock
{
    int topPosition;
    int lineNumber;
    QTextBlock textBlock;
};

class PanelsManager; //forward declaration
class TextDecorationsManager; //forward declaration
class DelayJobRunnerBase; //forward declaration
class ModesManager; //forward declaration
class SyntaxHighlighterBase; //forward declaration

/*
The editor widget is a simple extension to QPlainTextEdit.
It adds a few utility signals/methods and introduces the concepts of
**Managers, Modes and Panels**.
A **mode/panel** is an editor extension that, once added to a CodeEdit
instance, may modify its behaviour and appearance:
    * **Modes** are simple objects which connect to the editor signals to
    append new behaviours (such as automatic indentation, code completion,
    syntax checking,...)
    * **Panels** are the combination of a **Mode** and a **QWidget**.
    They are displayed in the CodeEdit's content margins.
    When you install a Panel on a CodeEdit, you can choose to install it in
    one of the four following zones:
        .. image:: _static/editor_widget.png
            :align: center
            :width: 600
            :height: 450
A **manager** is an object that literally manage a specific aspect of
:class:`pyqode.core.api.CodeEdit`. There are managers to manage the list of
modes/panels, to open/save file and to control the backend:
    - :attr:`pyqode.core.api.CodeEdit.file`:
        File manager. Use it to open/save files or access the opened file
        attribute.
    - :attr:`pyqode.core.api.CodeEdit.backend`:
        Backend manager. Use it to start/stop the backend or send a work
        request.
    - :attr:`pyqode.core.api.CodeEdit.modes`:
        Modes manager. Use it to append/remove modes on the editor.
    - :attr:`pyqode.core.api.CodeEdit.panels`:
        Modes manager. Use it to append/remove panels on the editor.
Starting from version 2.1, CodeEdit defines the
:attr:`pyqode.core.api.CodeEdit.mimetypes` class attribute that can be used
by IDE to determine which editor to use for a given mime type. This
property is a list of supported mimetypes. An empty list means the
CodeEdit is generic. **Code editors specialised for a specific language
should define the mime types they support!**
*/
class CodeEditor : public QPlainTextEdit
{
    Q_OBJECT
public:
    CodeEditor(QWidget *parent = NULL, bool createDefaultActions = true);
    virtual ~CodeEditor();

    bool useSpacesInsteadOfTabs() const;
    void setUseSpacesInsteadOfTabs(bool value);

    bool selectLineOnCopyEmpty() const;
    void setSelectLineOnCopyEmpty(bool value);

    bool showContextMenu() const;
    void setShowContextMenu(bool value);

    bool showWhitespaces() const;
    void setShowWhitespaces(bool value);

    QString fontName() const;
    void setFontName(const QString& value);

    int zoomLevel() const;
    void setZoomLevel(int value);

    int tabLength() const;
    void setTabLength(int value);

    QColor background() const;
    void setBackground(const QColor &value);

    QColor foreground() const;
    void setForeground(const QColor &value);

    QColor selectionForeground() const;
    void setSelectionForeground(const QColor &value);

    QColor selectionBackground() const;
    void setSelectionBackground(const QColor &value);

    QColor whitespacesForeground() const;
    void setWhitespacesForeground(const QColor &value);

    bool saveOnFocusOut() const;
    void setSaveOnFocusOut(bool value);

    bool edgeLineVisible() const;
    void setEdgeLineVisible(bool value);

    int edgeLineColumn() const;
    void setEdgeLineColumn(int column);

    QColor edgeLineColor() const;
    void setEdgeLineColor(const QColor &color);

    QList<VisibleBlock> visibleBlocks() const;
    bool dirty() const;

    void setMouseCursor(const QCursor &cursor);

    void cursorPosition(int &line, int &column) const;

    void setViewportMargins(int left, int top, int right, int bottom);

    PanelsManager* panels() const;
    TextDecorationsManager* decorations() const;
    ModesManager* modes() const;

    SyntaxHighlighterBase *syntaxHighlighter() const;

    int currentLineNumber() const;
    int currentColumnNumber() const;
    int lineNbrFromPosition(int yPos) const;
    int lineCount() const;
    QTextCursor selectWholeLine(int line = -1, bool applySelection = true);
    QTextCursor selectLines(int start = 0, int end = -1, bool applySelection = true);
    QPair<int,int> selectionRange() const; //start, end
    int linePosFromNumber(int lineNumber) const;

    int lineIndent(int lineNumber = -1) const;
    int lineIndent(const QTextBlock *lineNbr) const;
    QString lineText(int lineNbr) const;
    void markWholeDocDirty();
    void callResizeEvent(QResizeEvent *evt) { resizeEvent(evt); }

    void indent();
    void unindent();

    void cut();
    void copy();

    void resetStylesheet();
    void rehighlight();

    void showTooltip(const QPoint &pos, const QString &tooltip);
    void showTooltip(const QPoint &pos, const QString &tooltip, const TextDecoration::Ptr &senderDeco);

    void setPlainText(const QString &text, const QString &mimeType = "", const QString &encoding = "");

    bool isCommentOrString(const QTextCursor &cursor, const QList<ColorScheme::Keys> &formats = QList<ColorScheme::Keys>());
    bool isCommentOrString(const QTextBlock &block, const QList<ColorScheme::Keys> &formats = QList<ColorScheme::Keys>());

    QTextCursor wordUnderCursor(bool selectWholeWord);

    void callWheelEvent(QWheelEvent *e);

protected:

    CodeEditor &operator =(const CodeEditor &) { return *this; };

    void showTooltipDelayJobRunner(QList<QVariant> args);

    void initSettings();
    void initStyle();
    void initActions(bool createStandardActions);

    QString previousLineText() const;
    QString currentLineText() const;    

    void setWhitespacesFlags(bool show);

    void updateVisibleBlocks();

    void doHomeKey(QEvent *event = NULL, bool select = false);

    QTextCursor moveCursorTo(int line);

    virtual void resizeEvent(QResizeEvent *e);
    virtual void closeEvent(QCloseEvent *e);
    virtual void keyPressEvent(QKeyEvent *e);
    virtual void keyReleaseEvent(QKeyEvent *e);
    virtual void mouseDoubleClickEvent(QMouseEvent *e);
    virtual void mousePressEvent(QMouseEvent *e);
    virtual void mouseReleaseEvent(QMouseEvent *e);
    virtual void mouseMoveEvent(QMouseEvent* e);
    virtual void showEvent(QShowEvent *e);
    virtual void paintEvent(QPaintEvent *e);
    virtual void wheelEvent(QWheelEvent *e);

    virtual void focusInEvent(QFocusEvent *e);
    virtual void focusOutEvent(QFocusEvent *e);

    virtual bool eventFilter(QObject *obj, QEvent *e);

private:
    bool m_showCtxMenu;
    int m_defaultFontSize;
    bool m_useSpacesInsteadOfTabs;
    QColor m_whitespacesForeground; 
    QColor m_selBackground;
    QColor m_selForeground;
    QColor m_background;
    QColor m_foreground;
    bool m_showWhitespaces;
    int m_tabLength;
    int m_zoomLevel;
    int m_fontSize;
    QString m_fontFamily;
    bool m_selectLineOnCopyEmpty;
    QString m_wordSeparators;
    bool m_saveOnFocusOut;
    QPoint m_lastMousePos;
    int m_prevTooltipBlockNbr;

    bool m_edgeLineShow;
    int m_edgeLineColumn;
    QColor m_edgeLineColor;

    //flags/working variables
    bool m_cleaning;
    QSet<int> m_modifiedLines; //(line)
    bool m_dirty;
    QList<VisibleBlock> m_visibleBlocks;

    PanelsManager *m_pPanels;
    TextDecorationsManager *m_pDecorations;
    ModesManager *m_pModes;

    DelayJobRunnerBase *m_pTooltipsRunner;

private slots:
    void emitDirtyChanged(bool state);
    void onTextChanged();

signals:
    void dirtyChanged(bool state); //Signal emitted when the dirty state changed
    void painted(QPaintEvent *e);
    void keyPressed(QKeyEvent *e);
    void keyReleased(QKeyEvent *e);
    void postKeyPressed(QKeyEvent *e); // Signal emitted at the end of the key_pressed event
    void mouseDoubleClicked(QMouseEvent *e); // Signal emitted when a mouse double click event occured
    void mousePressed(QMouseEvent *e); // Signal emitted when a mouse button is pressed
    void mouseReleased(QMouseEvent *e); //Signal emitted when a key is released
    void mouseMoved(QMouseEvent *e); // Signal emitted when the mouse_moved
    void mouseWheelActivated(QWheelEvent *e);

    void focusedIn(QFocusEvent *e); //Signal emitted when focusInEvent is is called

    void indentRequested(); //Signal emitted when the user press the TAB key
    void unindentRequested(); //Signal emitted when the user press the BACK-TAB (Shift+TAB) key

    void blockCountChanged();
    void updateRequest();

    void newTextSet(); //!< Signal emitted when a new text is set on the widget
};

#endif