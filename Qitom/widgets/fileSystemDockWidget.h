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

#ifndef FILESYSTEMDOCKWIDGET_H
#define FILESYSTEMDOCKWIDGET_H

#include "../helper/IOHelper.h"
#include "abstractDockWidget.h"

#include <qwidget.h>
#include <qaction.h>
#include <qtoolbar.h>

#include <qtextbrowser.h>
#include <qtreeview.h>
#include <qlabel.h>
#include <qfilesystemmodel.h>
#include <qhash.h>
#include <qmutex.h>
#include <qmenu.h>
#include <qcombobox.h>
#include <qprocess.h>
#include <qevent.h>
#include <qurl.h>

#include <qsignalmapper.h>


namespace ito
{
    class FileSystemDockWidget : public AbstractDockWidget
    {
        Q_OBJECT

        public:
            FileSystemDockWidget(const QString &title, QWidget *parent = NULL, bool docked = true, bool isDockAvailable = true, tFloatingStyle floatingStyle = floatingNone, tMovingStyle movingStyle = movingEnabled, const QString &baseDirectory = QDir::currentPath());
            ~FileSystemDockWidget();

        protected:
            class FileSystemTreeView : public QTreeView
            {
            public:
                FileSystemTreeView ( QWidget * parent = 0 ) : QTreeView(parent) {}
                ~FileSystemTreeView () {}

		
                QModelIndexList selectedIndexes() const
                { 
                    return QTreeView::selectedIndexes();
                }

                //void selectionChanged(const QItemSelection &selected, const QItemSelection &deselected)
                //{
                //    QTreeView::selectionChanged(selected, deselected);
                //    AbstractDockWidget *adw = qobject_cast<AbstractDockWidget*>(parent());
                //    if(adw) adw->updateActions();
                //}
		
                //! catches the double click event
                /*!
                    Since the read-only property of the QFileSystemModel is set to false (in order to guarantee rename, copy,... methods),
                    a double click leaded to an edit-box in order to being able to rename the file. This is not the desired behaviour.
                    Therefore we've overwritten the mouseDoubleClickEvent-event, such that this event directly emits the activated signal. This
                    is the same behaviour than in the case, when the read-only property is set to true.

			        => Not necessary anymore, by deactivating the editing trigger a double click do not lead to an edit-box. (2012-09-11 David Fleischle)
                */
		        /*
                void mouseDoubleClickEvent ( QMouseEvent * event )
                {
                    QModelIndexList currents = selectedIndexes();

                    if(currents.size() > 0)
                    {
				        emit activated(currents[0]);
				        event->accept();
                    }
                    else
                    {
                        event->ignore();
                    }
                }
		        */

            };

            void createActions();
            void createMenus();
            void createToolBars();
            void createStatusBar(){}
            void updateActions();
            void updatePythonActions(){ updateActions(); }

        private:
            void fillFilterList();
            void showInGraphicalShell(const QString &filePath);

            QMenu* m_pShowDirListMenu;
            QMenu* m_pFileSystemSettingMenu;
            QMenu* m_pContextMenu;
            QTextBrowser* m_pPathEdit;
            QToolBar* m_pMainToolbar;
            FileSystemTreeView* m_pTreeView;
            QLabel* m_pLblFilter;
            QComboBox* m_pCmbFilter;
            QFileSystemModel* m_pFileSystemModel;
            QSignalMapper *m_newDirSelectedMapper;
            QString baseDirectory;
            QHash<QString,QStringList> defaultFilterPatterns;
            QMutex baseDirChangeMutex;
            QList<QUrl> m_clipboardCutData; //this mime-data has recently be selected by a cut action and is no available in QClipboard
            int *m_pColumnWidth;

            ShortcutAction* m_pActMoveCDUp;
            ShortcutAction* m_pActSelectCD;
            ShortcutAction* m_pActOpenFile;
            ShortcutAction* m_pActExecuteFile;
            ShortcutAction* m_pActLocateOnDisk;
            ShortcutAction* m_pActRenameItem;
            ShortcutAction* m_pActDeleteItems;
            ShortcutAction* m_pActCutItems;
            ShortcutAction* m_pActCopyItems;
            ShortcutAction* m_pActPasteItems;
            ShortcutAction* m_pActNewDir;
            ShortcutAction* m_pActNewPyFile;
            ShortcutAction* m_pViewList;
            ShortcutAction* m_pViewDetails;

        signals:
            void currentDirChanged();

        private slots:
            void mnuMoveCDUp();
            void mnuSelectCD();
            void mnuLocateOnDisk();
            void mnuExecuteFile();
            void mnuOpenFile();
            void mnuRenameItem();
            void mnuDeleteItems();
            void mnuCutItems();
            void mnuCopyItems();
            void mnuPasteItems();
            void mnuNewDir();
            void mnuNewPyFile();
            void showList();
            void showDetails();
            void mnuToggleView();
            void newDirSelected(const QString& text);
            void cmbFilterEditTextChanged(const QString &text);
            void openFile(const QModelIndex& index);
            void treeViewContextMenuRequested(const QPoint &pos);
            void setTreeViewHideColumns(const bool &hide);
            void removeActionFromDirList(const int &pos);
	        void changeDir();

            void pathAnchorClicked(const QUrl &link);

        public slots:
            RetVal changeBaseDirectory(QString dir);
            void processError(QProcess::ProcessError error);
    };

} //end namespace ito

#endif
