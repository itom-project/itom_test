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

#include "dialogTimerManager.h"
#include "../AppManagement.h"
#include "../organizer/uiOrganizer.h"

namespace ito
{
	DialogTimerManager::DialogTimerManager(QWidget *parent /*= NULL*/) :
		QDialog(parent)
	{
		ui.setupUi(this);
		connect(ui.btnOk, SIGNAL(clicked()), this, SLOT(close()));
		updateTimerList();
		on_listWidget_itemSelectionChanged();
	}
	//----------------------------------------------------------------------------------------------------------------------------------
	DialogTimerManager::~DialogTimerManager()
	{

	}
	//----------------------------------------------------------------------------------------------------------------------------------
	void DialogTimerManager::updateTimerList()
	{

		ui.listWidget->clear();
		UiOrganizer *uiOrg = (UiOrganizer*)AppManagement::getUiOrganizer();
		QList<TimerContainer> list(uiOrg->getRegisteredTimers());
		int i;
		for (i = 0; i<list.length(); ++i)
		{
			if (!list.at(i).timer->isSingleShot())
			{
				if (list.at(i).timer->isActive())
				{
					ui.listWidget->addItem(new QListWidgetItem(QIcon(":/application/icons/timerRun.png"), QString("TimerID: %1; Interval: %2 ms").arg(list.at(i).name).arg(list.at(i).timer.data()->interval())));
				}
				else
				{
					ui.listWidget->addItem(new QListWidgetItem(QIcon(":/application/icons/timerStop.png"), QString("TimerID: %1; Interval: %2 ms").arg(list.at(i).name).arg(list.at(i).timer.data()->interval())));
				}
				}
			else
			{
				if (list.at(i).timer->isActive())
				{
					ui.listWidget->addItem(new QListWidgetItem(QIcon(":/application/icons/timerRun.png"), QString("TimerID: %1; Interval: %2 ms (single-shot)").arg(list.at(i).name).arg(list.at(i).timer.data()->interval())));
				}
				else
				{
					ui.listWidget->addItem(new QListWidgetItem(QIcon(":/application/icons/timerStop.png"), QString("TimerID: %1; Interval: %2 ms (single-shot)").arg(list.at(i).name).arg(list.at(i).timer.data()->interval())));
				}

				}
		}


	}
//----------------------------------------------------------------------------------------------------------------------------------
	void DialogTimerManager::on_btnStop_clicked()
	{
		QList<QListWidgetItem*> selection = ui.listWidget->selectedItems();
		QListWidgetItem* item;
		int row;
		UiOrganizer *uiOrg = (UiOrganizer*)AppManagement::getUiOrganizer();
		QList<TimerContainer> list(uiOrg->getRegisteredTimers());
		foreach(item, selection)
		{
			row = ui.listWidget->row(item);
			list.at(row).timer->stop();
			item->setData(Qt::DecorationRole, QIcon((":/application/icons/timerStop.png")));
			
		}
		ui.btnStart->setEnabled(true);
		ui.btnStop->setEnabled(false);

	}
//----------------------------------------------------------------------------------------------------------------------------------
	void DialogTimerManager::on_btnStart_clicked()
	{
		QList<QListWidgetItem*> selection = ui.listWidget->selectedItems();
		QListWidgetItem* item;
		int row;
		UiOrganizer *uiOrg = (UiOrganizer*)AppManagement::getUiOrganizer();
		QList<TimerContainer> list(uiOrg->getRegisteredTimers());
		foreach(item, selection)
		{

			row = ui.listWidget->row(item);
			QMetaObject::invokeMethod(list.at(row).timer.data(), "start");
			item->setData(Qt::DecorationRole, QIcon((":/application/icons/timerRun.png")));
		}
		ui.btnStart->setEnabled(false);
		ui.btnStop->setEnabled(true);
	}
//----------------------------------------------------------------------------------------------------------------------------------
	void DialogTimerManager::on_btnStopAll_clicked()
	{
		UiOrganizer *uiOrg = (UiOrganizer*)AppManagement::getUiOrganizer();
		QList<TimerContainer> list(uiOrg->getRegisteredTimers());
		int i;
		for (i = 0; i < list.length(); ++i)
		{
			list.at(i).timer -> stop();
			ui.listWidget->item(i)->setData(Qt::DecorationRole, QIcon((":/application/icons/timerStop.png")));

		}
		ui.btnStart->setEnabled(true);
		ui.btnStop->setEnabled(false);
	}
//----------------------------------------------------------------------------------------------------------------------------------
	void DialogTimerManager::on_listWidget_itemSelectionChanged()
	{

		bool sameState(true); //marks if the whole selection has the same state
		bool first(false);
		bool state;
		UiOrganizer *uiOrg = (UiOrganizer*)AppManagement::getUiOrganizer();
		QList<QListWidgetItem*> selection(ui.listWidget->selectedItems());
		QList<TimerContainer> list(uiOrg->getRegisteredTimers());
		QListWidgetItem* item;

		if (selection.size() > 0)
		{
			foreach(item, selection)
			{
				if (first)
				{
					if (list.at(ui.listWidget->row(item)).timer->isActive() != state)
					{
						sameState = false;
					}
				}
				first = true;
				state = list.at(ui.listWidget->row(item)).timer->isActive();
			}

			if (sameState && first)//first as arguument to avoid crash if no item is selected
			{
				if (state)
				{
					ui.btnStart->setEnabled(false);
					ui.btnStop->setEnabled(true);
				}
				else
				{
					ui.btnStart->setEnabled(true);
					ui.btnStop->setEnabled(false);

				}
			}
			else
			{
				ui.btnStart->setEnabled(true);
				ui.btnStop->setEnabled(true);
			}
		}
		else
		{
			ui.btnStart->setEnabled(false);
			ui.btnStop->setEnabled(false);
				
		}

		ui.btnStopAll->setEnabled(list.size() > 0);
	}
	
}

