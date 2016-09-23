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

#ifndef DIALOGTIMERMANAGER
#define DIALOGTIMERMANAGER


#include "ui_dialogTimerManager.h"
#include <qdialog.h>

namespace ito
{
	class DialogTimerManager : public QDialog
	{
		Q_OBJECT

	public:
		DialogTimerManager(QWidget *parent = NULL);
		~DialogTimerManager();
	private:
		Ui::DialogTimerManager ui;
		
		void updateTimerList();

	
	private slots:
		void on_btnStop_clicked();
		void on_btnStart_clicked();
		void on_btnStopAll_clicked();
		void on_listWidget_itemSelectionChanged();


	};
}//end namespace ito
#endif