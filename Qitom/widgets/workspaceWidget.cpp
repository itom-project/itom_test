﻿/* ********************************************************************
    itom software
    URL: http://www.uni-stuttgart.de/ito
    Copyright (C) 2013, Institut für Technische Optik (ITO),
    Universität Stuttgart, Germany

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
#include "workspaceWidget.h"

#include "../AppManagement.h"
#include "../ui/dialogVariableDetail.h"

#include <qstringlist.h>
#include <qdrag.h>

namespace ito
{

//----------------------------------------------------------------------------------------------------------------------------------
/*!
    \class WorkspaceWidget
    \brief the workspaceWidget displays either a global or a local workspace given by a python dictionary. This widget is inherited from QTreeWidget.
*/

//! constructor
/*!
    \param globalNotLocal true: this widget shows a global python dictionary, false: local
    \param parent parent-widget
*/
WorkspaceWidget::WorkspaceWidget(bool globalNotLocal, QWidget* parent) :
    QTreeWidget(parent),
    m_globalNotLocal(globalNotLocal),
    m_workspaceContainer(NULL)
{
    QStringList headers;

    setDragDropMode( QAbstractItemView::DragOnly );
#if QT_VERSION < 0x050000
    this->model()->setSupportedDragActions(Qt::CopyAction);
#endif

    setColumnCount(3);
    setEditTriggers(QAbstractItemView::NoEditTriggers);
    
    if (m_globalNotLocal)
    {
        headers << tr("Globals") << tr("Value") << tr("Type");
    }
    else
    {    
        headers << tr("Locals") << tr("Value") << tr("Type");
    }
    setHeaderLabels(headers);
    setSortingEnabled(true);
    sortByColumn(0,Qt::AscendingOrder);
    setTextElideMode(Qt::ElideMiddle);

    clear();
    m_itemHash.clear();

    m_delimiter = QString( QByteArray::fromHex("AAD791A8") );

    QIcon icon(":/application/icons/preferences-python.png");
    m_dragPixmap = icon.pixmap(22,22);
  
   /* '__', 'NoneType', 'type',\
        'bool', 'int', 'long', 'float', 'complex',\
        'str', 'unicode', 'tuple', 'list',\
        'dict', 'dict-proxy', 'set', 'file', 'xrange',\
        'slice', 'buffer', 'class', 'instance',\
        'instance method', 'property', 'generator',\
        'function', 'builtin_function_or_method', 'code', 'module',\
        'ellipsis', 'traceback', 'frame', 'other']*/

    m_workspaceContainer = new ito::PyWorkspaceContainer(m_globalNotLocal);
    connect(m_workspaceContainer,SIGNAL(updateAvailable(PyWorkspaceItem*,QString,QStringList)),this,SLOT(workspaceContainerUpdated(PyWorkspaceItem*,QString,QStringList)));
    connect(this,SIGNAL(itemDoubleClicked(QTreeWidgetItem*, int)),this,SLOT(itemDoubleClicked(QTreeWidgetItem*, int))); //when double-clicking on an item, its content is showed in DialogVariableDetail-Dialog
    connect(this,SIGNAL(itemExpanded(QTreeWidgetItem*)),this,SLOT(itemExpanded(QTreeWidgetItem*)));
}


//----------------------------------------------------------------------------------------------------------------------------------
#if QT_VERSION >= 0x050000
Qt::DropActions WorkspaceWidget::supportedDragActions() const
{
    return supportedDropActions() | Qt::CopyAction;
}
#endif

//----------------------------------------------------------------------------------------------------------------------------------
//! destructor
WorkspaceWidget::~WorkspaceWidget()
{
    disconnect(m_workspaceContainer,SIGNAL(updateAvailable(PyWorkspaceItem*,QString,QStringList)),this,SLOT(workspaceContainerUpdated(PyWorkspaceItem*,QString,QStringList)));
    m_workspaceContainer->deleteLater();
    disconnect(this,SIGNAL(itemDoubleClicked(QTreeWidgetItem*, int)),this,SLOT(itemDoubleClicked(QTreeWidgetItem*, int)));
    disconnect(this,SIGNAL(itemExpanded(QTreeWidgetItem*)),this,SLOT(itemExpanded(QTreeWidgetItem*)));
}

//----------------------------------------------------------------------------------------------------------------------------------
QStringList WorkspaceWidget::mimeTypes() const
{
    QStringList types = QTreeWidget::mimeTypes();

    if (types.contains("text/plain") == false)
    {
        types.append("text/plain");
    }

    return types;
}

//----------------------------------------------------------------------------------------------------------------------------------
QMimeData * WorkspaceWidget::mimeData(const QList<QTreeWidgetItem *> items) const
{
    QMimeData *mimeData = QTreeWidget::mimeData(items);
    QStringList texts;

    //QByteArray encoded = mimeData->data("application/x-qabstractitemmodeldatalist");
    //QDataStream stream(&encoded, QIODevice::ReadOnly);

    QString name;
    QSharedPointer<QString> tempValue;
    const QTreeWidgetItem *tempItem = NULL;
    QString fullName("empty item");
    QByteArray type;

    foreach(const QTreeWidgetItem *item, items)
    {
        fullName = item->data(0, Qt::UserRole+1).toString();
        type = item->data(0, Qt::UserRole + 3).toByteArray();

        if (item->parent() == NULL)
        {
            name = item->text(0);
        }
        else
        {
            tempItem = item;
            while(tempItem->parent() != NULL)
            {
                if (type[0] == PY_DICT || type[0] == PY_MAPPING || type[0] == PY_LIST)
                {
                    if (type[1] == PY_NUMBER)
                    {
                        name.prepend( "[" + tempItem->text(0) + "]" );
                    }
                    else
                    {
                        name.prepend( "[\"" + tempItem->text(0) + "\"]" );
                    }
                }
                else if (type[0] == PY_ATTR)
                {
                    name.prepend( "." + tempItem->text(0) );
                }
                tempItem = tempItem->parent();
            }
            name.prepend( tempItem->text(0) );
        }

        texts.append(name);
    }

    //while (!stream.atEnd())
    //{
    //    int row, col;
    //    QMap<int,  QVariant> roleDataMap;
    //    stream >> row >> col >> roleDataMap;
    //    texts.append( roleDataMap[0].toString() );
    //}

    mimeData->setData("text/plain", texts.join("\n").toLatin1() );
    return mimeData;
}

//----------------------------------------------------------------------------------------------------------------------------------
int WorkspaceWidget::numberOfSelectedMainItems() const
{
    unsigned int counter = 0;
    QList<QTreeWidgetItem*> items = selectedItems();
    foreach(const QTreeWidgetItem* item, items)
    {
        if (item->parent() == NULL) counter++;
    }
    return counter;
}

//----------------------------------------------------------------------------------------------------------------------------------
void WorkspaceWidget::updateView(QHash<QString,ito::PyWorkspaceItem*> items, QString baseName, QTreeWidgetItem *parent)
{
    QHash<QString,QTreeWidgetItem*>::iterator it;
    QString hashName;
    QTreeWidgetItem *actItem;
    QTreeWidgetItem *tempItem;
    foreach(const ito::PyWorkspaceItem *item, items)
    {
        hashName = baseName + m_delimiter + item->m_key;
        it = m_itemHash.find(hashName);
        if (it != m_itemHash.end())
        {
            actItem = *it;
        }
        else
        {
            actItem = new QTreeWidgetItem(parent,0);
            m_itemHash[hashName] = actItem;
            if (parent == NULL)
            {
                addTopLevelItem(actItem);
                actItem->setFlags(Qt::ItemIsDragEnabled | Qt::ItemIsEditable | Qt::ItemIsSelectable | Qt::ItemIsEnabled);
            }
            else
            {
                actItem->setFlags( Qt::ItemIsDragEnabled | Qt::ItemIsEnabled );
            }
        }

        actItem->setText(0, item->m_name); //name of variable, key-word of dictionary of index number of sequences (list, tuple..)
        actItem->setText(1, item->m_value); //content of variable
        actItem->setText(2, item->m_type); //data type
        actItem->setData(0, Qt::UserRole + 1, hashName);
        actItem->setData(0, Qt::UserRole + 2, item->m_compatibleParamBaseType);
        actItem->setData(0, Qt::UserRole + 3, item->m_key.left(2).toLatin1()); //m_key is ab:name where a is [PY_LIST,PY_MAPPING,PY_DICT,PY_ATTR] and b is [PY_NUMBER or PY_STRING]

        if (item->m_childState == ito::PyWorkspaceItem::stateNoChilds)
        {
            actItem->setChildIndicatorPolicy(QTreeWidgetItem::DontShowIndicator);
            while(actItem->childCount() > 0)
            {
                tempItem = actItem->child(0);
                recursivelyDeleteHash( tempItem->data(0, Qt::UserRole+1).toString() );
                actItem->removeChild( actItem->child(0) );
            }
        }
        else
        {
            actItem->setChildIndicatorPolicy(QTreeWidgetItem::ShowIndicator);
            if (item->m_childs.count() == 0) //item has children, but they are not shown yet
            {
                while(actItem->childCount() > 0)
                {
                    tempItem = actItem->child(0);
                    recursivelyDeleteHash( tempItem->data(0, Qt::UserRole+1).toString() );
                    actItem->removeChild( actItem->child(0) );
                }
            }
            else
            {
                updateView(item->m_childs,hashName,actItem);
            }
        }
    }
}

//----------------------------------------------------------------------------------------------------------------------------------
void WorkspaceWidget::workspaceContainerUpdated(PyWorkspaceItem *rootItem, QString fullNameRoot, QStringList recentlyDeletedFullNames)
{
    QTreeWidgetItem *parent = NULL;
    QTreeWidgetItem *temp;

    if (m_workspaceContainer)
    {
        if (m_workspaceContainer->isRoot(rootItem))
        {
            if (rootItem->m_childs.count() == 0)
            {
                m_itemHash.clear();
                clear();
                recentlyDeletedFullNames.clear();
            }
            parent = NULL;
        }
        else
        {
            QHash<QString, QTreeWidgetItem*>::const_iterator it = m_itemHash.find(fullNameRoot);
            if (it == m_itemHash.constEnd()) return; //error
            parent = *it;
        }

        QHash<QString,QTreeWidgetItem*>::iterator it;
        foreach(const QString& deleteHashName, recentlyDeletedFullNames)
        {
            temp = NULL;
            it = m_itemHash.find(deleteHashName);
            if (it != m_itemHash.end())
            {
                temp = (*it);
            }
            recursivelyDeleteHash(deleteHashName);
            
            delete temp;
        }

        if (m_workspaceContainer->m_accessMutex.tryLock(1000))
        {
            updateView(rootItem->m_childs, fullNameRoot, parent);
            m_workspaceContainer->m_accessMutex.unlock();
        }
    }
}

//----------------------------------------------------------------------------------------------------------------------------------
void WorkspaceWidget::recursivelyDeleteHash(QTreeWidgetItem *item)
{
    if (item)
    {
        QString fullName = item->data(0,Qt::UserRole+1).toString();
        recursivelyDeleteHash(fullName);
    }
}

//----------------------------------------------------------------------------------------------------------------------------------
void WorkspaceWidget::recursivelyDeleteHash(const QString &fullBaseName)
{
    QHash<QString,QTreeWidgetItem*>::iterator it = m_itemHash.find(fullBaseName);
    if (it != m_itemHash.end())
    {
        for(int i=0;i<it.value()->childCount();i++)
        {
            recursivelyDeleteHash(it.value()->child(i));
        }

        m_itemHash.erase(it);
    }
}

//----------------------------------------------------------------------------------------------------------------------------------
//! slot, invoked if item is double-clicked
/*!
    opens instance of DialogVariableDetail to show detailed information about the double-clicked variable

    \param item QTreeWidgetItem which has been clicked
    \sa DialogVariableDetail
*/
void WorkspaceWidget::itemDoubleClicked(QTreeWidgetItem* item, int /*column*/)
{
    QString extendedValue = "";
    QString name;
    QSharedPointer<QString> tempValue;
    QTreeWidgetItem *tempItem = NULL;
    QString fullName("empty item");
    QByteArray type;

	if (item)
    {
		fullName = item->data(0, Qt::UserRole+1).toString();
        type = item->data(0, Qt::UserRole + 3).toByteArray();
        ito::PyWorkspaceItem* item2 = m_workspaceContainer->getItemByFullName( fullName );
        extendedValue = item2->m_extendedValue;

        if (item->parent() == NULL)
        {
            name = item->text(0);
        }
        else
        {
            tempItem = item;
            while(tempItem->parent() != NULL)
            {
                type = tempItem->data(0, Qt::UserRole + 3).toByteArray();

                if (type[0] == PY_DICT || type[0] == PY_MAPPING || type[0] == PY_LIST)
                {
                    if (type[1] == PY_NUMBER)
                    {
                        name.prepend( "[" + tempItem->text(0) + "]" );
                    }
                    else
                    {
                        name.prepend( "[\"" + tempItem->text(0) + "\"]" );
                    }
                }
                else if (type[0] == PY_ATTR)
                {
                    name.prepend( "." + tempItem->text(0) );
                }
                tempItem = tempItem->parent();
            }
            name.prepend( tempItem->text(0) );
        }
    }

    if (extendedValue == "") //ask python to get extendedValue, since this value has been complex such that is hasn't been evaluated at runtime before
    {
        PythonEngine* eng = qobject_cast<PythonEngine*>(AppManagement::getPythonEngine());

        if (eng)
        {
            ItomSharedSemaphoreLocker locker(new ItomSharedSemaphore());
            tempValue = QSharedPointer<QString>(new QString());
            QMetaObject::invokeMethod(eng, "workspaceGetValueInformation", Q_ARG(PyWorkspaceContainer*, m_workspaceContainer), Q_ARG(QString,fullName), Q_ARG(QSharedPointer<QString>, tempValue), Q_ARG(ItomSharedSemaphore*, locker.getSemaphore()));
            
            if (!locker.getSemaphore()->waitAndProcessEvents(1000))
            {
                extendedValue = tr("timeout while asking python for detailed information");
            }
            else
            {
                extendedValue = *tempValue;
            }
        }
    }

    DialogVariableDetail *dlg = new DialogVariableDetail(name, item->text(2), extendedValue); //wItem.m_extendedValue);
    dlg->exec();
    DELETE_AND_SET_NULL(dlg);
}

//----------------------------------------------------------------------------------------------------------------------------------
void WorkspaceWidget::itemExpanded(QTreeWidgetItem* item)
{
    QString fullName = item->data(0, Qt::UserRole + 1).toString();
    m_workspaceContainer->m_accessMutex.lock();
    m_workspaceContainer->m_expandedFullNames.insert(fullName);
    m_workspaceContainer->m_accessMutex.unlock();

    if (item->childCount() == 0) //childs have not been submitted by python yet
    {
        m_workspaceContainer->emitGetChildNodes(m_workspaceContainer, fullName);
    }
}

//----------------------------------------------------------------------------------------------------------------------------------
void WorkspaceWidget::itemCollapsed(QTreeWidgetItem* item)
{
    QString fullName = item->data(0, Qt::UserRole + 1).toString();
    m_workspaceContainer->m_accessMutex.lock();
    m_workspaceContainer->m_expandedFullNames.remove(fullName);
    m_workspaceContainer->m_accessMutex.unlock();
}


//----------------------------------------------------------------------------------------------------------------------------------
void WorkspaceWidget::startDrag(Qt::DropActions supportedActions)
{
    //QTreeWidget::startDrag(supportedActions);
    QList<QTreeWidgetItem*> items = selectedItems();
    if (items.count() > 0) 
    {
        QMimeData *data = mimeData(items);
        if (!data)
            return;
        QRect rect;
        QDrag *drag = new QDrag(this);
        drag->setPixmap(m_dragPixmap);
        drag->setMimeData(data);
        Qt::DropAction defaultDropAction = Qt::IgnoreAction;
        if (this->defaultDropAction() != Qt::IgnoreAction && (supportedActions & this->defaultDropAction()))
            defaultDropAction = this->defaultDropAction();
        else if (supportedActions & Qt::CopyAction && dragDropMode() != QAbstractItemView::InternalMove)
            defaultDropAction = Qt::CopyAction;
        drag->exec(supportedActions, defaultDropAction);
    }
}

} //end namespace ito


