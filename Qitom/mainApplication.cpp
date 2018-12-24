/* ********************************************************************
    itom software
    URL: http://www.uni-stuttgart.de/ito
    Copyright (C) 2018, Institut fuer Technische Optik (ITO),
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
#define ITOM_IMPORT_API
#include "../common/apiFunctionsInc.h"
#undef ITOM_IMPORT_API
#include "mainApplication.h"
#include "global.h"
#include "version.h"
#include "AppManagement.h"

#include "widgets/abstractDockWidget.h"
#include "../AddInManager/addInManager.h"
#include "./models/UserModel.h"
#include "organizer/userOrganizer.h"
#include "widgets/scriptDockWidget.h"
#include "./ui/dialogSelectUser.h"
#include "ui/dialogPipManager.h"
#include "ui/dialogCloseItom.h"
#include "DataObject/dataobj.h"

#include <qsettings.h>
#include <qstringlist.h>

#include <qdir.h>
#include <qtextcodec.h>
#include <qsplashscreen.h>
#include <qstylefactory.h>
#include <qmessagebox.h>
#include <qpainter.h>
#include <qlibraryinfo.h>
#include <qresource.h>
#include <qfileinfo.h>

#if WIN32
#include <Windows.h>
#endif

namespace ito
{

#ifdef WIN32
    class CPUID {
      ito::uint32 regs[4];

    public:
      void load(unsigned i) {
    #ifndef _MSC_VER
        asm volatile
          ("cpuid" : "=a" (regs[0]), "=b" (regs[1]), "=c" (regs[2]), "=d" (regs[3])
           : "a" (i), "c" (0));
        // ECX is set to zero for CPUID function 4    
    #else
        __cpuid((ito::int32 *)regs, (ito::int32)i); //Microsoft specific for x86 and x64
    #endif
      }

      const ito::uint32 &EAX() const {return regs[0];}
      const ito::uint32 &EBX() const {return regs[1];}
      const ito::uint32 &ECX() const {return regs[2];}
      const ito::uint32 &EDX() const {return regs[3];}
    };
#endif

/*!
    \class MainApplication
    \brief The MainApplication class is the basic management class for the entire application
*/

//! static instance pointer initialization
MainApplication* MainApplication::mainApplicationInstance = NULL;

//----------------------------------------------------------------------------------------------------------------------------------
/*!
    getter-method for static instance pointer
*/
MainApplication* MainApplication::instance()
{
    Q_ASSERT(MainApplication::mainApplicationInstance);
    return MainApplication::mainApplicationInstance;
}

//----------------------------------------------------------------------------------------------------------------------------------
//! constructor
/*!

    \param guiType Type of the desired GUI (normal, console, no)
    \sa tGuiType
*/
MainApplication::MainApplication(tGuiType guiType) : 
    m_pyThread(NULL), 
    m_pyEngine(NULL), 
    m_scriptEditorOrganizer(NULL), 
    m_mainWin(NULL), 
    m_paletteOrganizer(NULL),
    m_uiOrganizer(NULL), 
    m_designerWidgetOrganizer(NULL),
    m_processOrganizer(NULL),
    m_splashScreen(NULL),
    m_pQout(NULL),
    m_pQerr(NULL)
{
    m_guiType = guiType;
    MainApplication::mainApplicationInstance = this;

    //qDebug() << QLibraryInfo::location(QLibraryInfo::BinariesPath);

    AppManagement::setMainApplication(qobject_cast<QObject*>(this));

    //global settings: the settings file will be stored in itomSettings/{organization}/{applicationName}.ini
    QCoreApplication::setOrganizationName("ito");
    QCoreApplication::setApplicationName("itom");
    QCoreApplication::setApplicationVersion(ITOM_VERSION_STR);
}

//----------------------------------------------------------------------------------------------------------------------------------
//! destructor
/*!
    shutdown of whole application, including PythonEngine

    \sa PythonEngine
*/
MainApplication::~MainApplication()
{
    AppManagement::setMainApplication(NULL);
    MainApplication::mainApplicationInstance = NULL;
}

//----------------------------------------------------------------------------------------------------------------------------------
void MainApplication::registerMetaObjects()
{
    qRegisterMetaTypeStreamOperators<ito::ScriptEditorStorage>("ito::ScriptEditorStorage");
    qRegisterMetaTypeStreamOperators<QList<ito::ScriptEditorStorage> >("QList<ito::ScriptEditorStorage>");

    qRegisterMetaTypeStreamOperators<ito::BreakPointItem>("BreakPointItem");

    qRegisterMetaType<ito::tStreamMessageType>("ito::tStreamMessageType");
}

//----------------------------------------------------------------------------------------------------------------------------------
QString getSplashScreenFileName()
{
    QString fileName;

    QDate currentDate = QDate::currentDate();
    int currentMonth = currentDate.month();
    int currentYear = currentDate.year();

    /*easter date calculation*/
    uint easterMonth, easterDay;

    uint aE = currentYear % 19;
    uint bE = currentYear % 4;
    uint cE = currentYear % 7;

    int kE = currentYear / 100;
    int qE = kE / 4;
    int pE = ((8 * kE) + 13) / 25;
    uint EgzE = (38 - (kE - qE) + pE) % 30;
    uint ME = (53 - EgzE) % 30;
    uint NE = (4 + kE - qE) % 7;

    uint dE = ((19 * aE) + ME) % 30;
    uint eE = ((2 * bE) + (4 * cE) + (6 * dE) + NE) % 7;

    // Calculation of Easter date:
    if ((22 + dE + eE) <= 31)
    {
        easterDay = 22 + dE + eE;
        easterMonth = 3;
    }
    else
    {
        easterDay = dE + eE - 9;
        easterMonth = 4;

        // Consider two exceptions:
        if (easterDay == 26)
            easterDay = 19;
        else if ((easterDay == 25) && (dE == 28) && (aE > 10))
            easterDay = 18;
    }
    /*easter date calculation*/

	qint64 daysDiffToEaster = currentDate.toJulianDay() - QDate(currentYear, easterMonth, easterDay).toJulianDay();

    if (currentMonth == 12)
    {
        //Christmas splashScreen whole december of each year
        fileName = ":/application/icons/itomicon/splashScreen2Christmas.png";
    }
    else if (qAbs(daysDiffToEaster) <= 7)
    {
        //Easter splashScreen one week before and after easter day
        fileName = ":/application/icons/itomicon/splashScreen2Easter.png";
    }
    else //default splashScreen
    {
        fileName = ":/application/icons/itomicon/splashScreen2.png";
    }

    return fileName;
}


//----------------------------------------------------------------------------------------------------------------------------------
//! setup of application
/*!
    starts PythonEngine, MainWindow (dependent on gui-type) and all necessary managers and organizers.
    Builds import connections between MainWindow and PythonEngine as well as ScriptEditorOrganizer.

    \sa PythonEngine, MainWindow, ScriptEditorOrganizer
*/
void MainApplication::setupApplication(const QStringList &scriptsToOpen)
{
    RetVal retValue = retOk;
    RetVal pyRetValue;
    QStringList startupScripts;
    QSharedPointer<QVariantMap> infoMessages(new QVariantMap());

    registerMetaObjects();

#ifdef USEGIMMICKS
    QString spashScreenFileName = getSplashScreenFileName(); // get the fileName of splashScreen. Different at easter and christmas time 
#else
    QString spashScreenFileName = ":/application/icons/itomicon/splashScreen2.png"; //only default splashScreen
#endif // USEUSEGIMMICKS  

    QPixmap pixmap(spashScreenFileName);

    QString text;
    
    if (sizeof(void*) > 4) //was before a check using QT_POINTER_SIZE
    {
        text = QString(tr("Version %1\n%2")).arg(ITOM_VERSION_STR).arg(tr("64 bit (x64)"));
    }
    else
    {
        text = QString(tr("Version %1\n%2")).arg(ITOM_VERSION_STR).arg(tr("32 bit (x86)"));
    }

#if USING_GIT == 1
    text.append(QString("\nRev. %1").arg(GIT_HASHTAG_ABBREV));
#endif
    QPainter p;
    p.begin(&pixmap);
    p.setPen(Qt::black);
    QRectF rect(250 /*311*/,217 /*115*/,200,100); //position of the version text within the image
    p.drawText(rect, Qt::AlignLeft, text);
    p.end();

    m_splashScreen = new QSplashScreen(pixmap);
    
    m_splashScreen->show();
    QCoreApplication::processEvents();

    //load std::cout and std::cerr stream redirections
    m_pQout = new QDebugStream(std::cout, ito::msgStreamOut);
    m_pQerr = new QDebugStream(std::cerr, ito::msgStreamErr);
    AppManagement::setStdCoutCerrStreamRedirections(m_pQout, m_pQerr);

    QSettings *settings = new QSettings(AppManagement::getSettingsFile(), QSettings::IniFormat);

    //add further folders to path-variable

    //you can add further pathes to the application-internal PATH variable by adding the following lines to the ini-file:
    /*[Application]
    searchPathes\size=1 ->add here the number of pathes
    searchPathes\1\path=PathToAdd -> for each path add one line like this where you auto-increment the number from \1\ up to your total number*/
    settings->beginGroup("Application");

    int s = settings->beginReadArray("searchPathes");
    QStringList prependPathes;
    QStringList appendPathes;
    for (int i = 0; i < s; ++i)
    {
        settings->setArrayIndex(i);
        if (settings->value("prepend", true).toBool())
        {
            prependPathes.append(QDir::toNativeSeparators(settings->value("path", "").toString()));
        }
        else
        {
            appendPathes.append(QDir::toNativeSeparators(settings->value("path", "").toString()));
        }
    }

    settings->endArray();
    settings->endGroup();

#ifdef WIN32
    if (appendPathes.size() > 0 || prependPathes.size() > 0)
    {
        QByteArray oldpath = getenv("path");
        QByteArray prepend = prependPathes.size() > 0 ? prependPathes.join(";").toLatin1() + ";" : "";
        QByteArray append = appendPathes.size() > 0 ? ";" + appendPathes.join("; ").toLatin1() : "";
        QByteArray newpath = "path=" + prepend + oldpath + append; //set libDir at the beginning of the path-variable
        _putenv(newpath.data());
    }
#else // (defined linux) && (defined _APPLE_)
    if (appendPathes.size() > 0 || prependPathes.size() > 0)
    {
        QByteArray oldpath = getenv("path");
        QByteArray prepend = prependPathes.size() > 0 ? prependPathes.join(";").toLatin1() + ";" : "";
        QByteArray append = appendPathes.size() > 0 ? ";" + appendPathes.join("; ").toLatin1() : "";
        QByteArray newpath = "path=" + prepend + oldpath + append; //set libDir at the beginning of the path-variable
        setenv("PATH", newpath.data(), 1);
    }
#endif

#ifdef WIN32
    //This check is done since the KMP_AFFINITY feature of OpenMP
    //is only available on Intel CPUs and lead to a severe warning
    //on other CPUs.
    CPUID cpuID;
    cpuID.load(0); // Get CPU vendor

    QByteArray vendor("");
    vendor.append((const char *)&cpuID.EBX(), 4);
    vendor.append((const char *)&cpuID.EDX(), 4);
    vendor.append((const char *)&cpuID.ECX(), 4);
    
    if (strcmp(vendor.data(), "GenuineIntel") != 0)
    {
        _putenv_s("KMP_AFFINITY","none");
    }
#else
    // \todo check for Intel/AMD and set KMP_AFFINITY if not Intel
#endif


    settings->beginGroup("Language");
    QString language = settings->value("language", "en").toString();
    QByteArray codec =  settings->value("codec", "ISO 8859-1").toByteArray(); //latin1 is default
    bool setCodecForLocal = settings->value("setCodecForLocale", false).toBool();
    settings->endGroup();
    settings->sync();

    //load timeouts
    settings->beginGroup("Application");
    AppManagement::timeouts.pluginInitClose = settings->value("timeoutInitClose", 10000).toInt();
    AppManagement::timeouts.pluginGeneral = settings->value("timeoutGeneral", PLUGINWAIT).toInt();
    AppManagement::timeouts.pluginFileSaveLoad = settings->value("timeoutFileSaveLoad", 60000).toInt();
    settings->endGroup();

    QLocale local = QLocale(language); //language can be "language[_territory][.codeset][@modifier]"
    QString itomTranslationFolder = QCoreApplication::applicationDirPath() + "/translation";

    //load translation files
    m_splashScreen->showMessage(tr("load translations..."), Qt::AlignRight | Qt::AlignBottom);
    QCoreApplication::processEvents();

    //1. try to load qt-translations from qt-folder
    m_qtTranslator.load("qt_" + local.name(), QLibraryInfo::location(QLibraryInfo::TranslationsPath));
    if (m_qtTranslator.isEmpty())
    {
        //qt-folder is not available, then try itom translation folder
        m_qtTranslator.load("qt_" + local.name(), itomTranslationFolder);
    }
    QCoreApplication::instance()->installTranslator(&m_qtTranslator);

    //2. load itom-specific translation file
    m_translator.load("qitom_" + local.name(), itomTranslationFolder);
    QCoreApplication::instance()->installTranslator(&m_translator);

    m_commonQtTranslator.load("itomCommonQtLib_" + local.name(), itomTranslationFolder);
    QCoreApplication::instance()->installTranslator(&m_commonQtTranslator);

    m_commonPlotTranslator.load("itomCommonPlotLib_" + local.name(), itomTranslationFolder);
    QCoreApplication::instance()->installTranslator(&m_commonPlotTranslator);

    m_widgetsTranslator.load("itomWidgets_" + local.name(), itomTranslationFolder);
    QCoreApplication::instance()->installTranslator(&m_widgetsTranslator);

    m_widgetsTranslator.load("addinmanager_" + local.name(), itomTranslationFolder);
    QCoreApplication::instance()->installTranslator(&m_widgetsTranslator);

    //3. set default encoding codec
    QTextCodec *textCodec = QTextCodec::codecForName(codec);
    if (textCodec == NULL)
    {
        textCodec = QTextCodec::codecForName("ISO 8859-1"); //latin1 is default
    }
    if (!textCodec)
    {
        textCodec = QTextCodec::codecForLocale();
    }

    AppManagement::setScriptTextCodec(textCodec);

    // None of these two is available in Qt5 and according to
    // Qt docu it should not have been used anyway. So 
    // we need to find another solution here
    // QTextCodec::setCodecForCStrings(textCodec);
    if (setCodecForLocal && textCodec)
    {
        QTextCodec::setCodecForLocale(textCodec);
    }

    if (m_guiType == standard || m_guiType == console)
    {
        m_splashScreen->showMessage(tr("load themes and styles..."), Qt::AlignRight | Qt::AlignBottom);
        QCoreApplication::processEvents();

        //set styles (if available)
        settings->beginGroup("ApplicationStyle");
        QString styleName = settings->value("style", "").toString();
        QString cssFile = settings->value("cssFile", "").toString();
        QString rccFile = settings->value("rccFile", "").toString();
        QString iconTheme = settings->value("iconTheme", "bright").toString();
        settings->endGroup();

#if QT_VERSION >= 0x050000
        QDir iconThemeDir(QCoreApplication::applicationDirPath());
        QString iconThemeFile = "iconThemeBright.rcc";
        if (iconTheme.compare("dark", Qt::CaseInsensitive) == 0)
        {
            iconThemeFile = "iconThemeDark.rcc";
        }

        if (!QResource::registerResource(iconThemeDir.absoluteFilePath(iconThemeFile)))
        {
            qDebug() << "error loading the icon theme file " << iconThemeDir.absoluteFilePath(iconThemeFile);
        }
#endif

        if (styleName != "")
        {
            QStringList styles = QStyleFactory::keys();

            if (styles.contains(styleName, Qt::CaseInsensitive))
            {
                QApplication::setStyle(styleName);
            }
            else
            {
                qDebug() << "style " << styleName << "is not available. Available styles are " << styles;
            }
        }

        if (rccFile != "")
        {
            QDir appFolder = QCoreApplication::applicationDirPath();
            QDir dir(rccFile);
            if (dir.isRelative())
            {
                rccFile = QDir::cleanPath(appFolder.absoluteFilePath(rccFile));
            }
            else
            {
                rccFile = QDir::cleanPath(rccFile);
            }

            if (appFolder.exists(rccFile))
            {
                if (!QResource::registerResource(rccFile))
                {
                    qDebug() << "error loading the resource-file " << rccFile;
                }
            }
            else
            {
                qDebug() << "resource-file " << rccFile << " does not exist";
            }
            
        }

        if (cssFile != "")
        {
            QDir appFolder = QCoreApplication::applicationDirPath();
            QDir dir(cssFile);
            if (dir.isRelative())
            {
                cssFile = QDir::cleanPath(appFolder.absoluteFilePath(cssFile));
            }
            else
            {
                cssFile = QDir::cleanPath(cssFile);
            }

            if (appFolder.exists(cssFile))
            {
                QFile css(appFolder.filePath(cssFile));
                if (css.open(QFile::ReadOnly))
                {
                    QString cssContent(css.readAll());
                    qApp->setStyleSheet(cssContent);
                    css.close();
                }
            }
            else
            {
                qDebug() << "style-file " << cssFile << " does not exist";
            }
        }
    }

    DELETE_AND_SET_NULL(settings);

	/*set new seed for random generator of OpenCV. 
	This is required to have real random values for any randn or randu command.
	The seed must be set in every thread. This is for the main thread.
	*/
	cv::theRNG().state = (uint64)cv::getCPUTickCount();
	/*seed is set*/

    //starting ProcessOrganizer for external processes like QtDesigner, QtAssistant, ...
    m_splashScreen->showMessage(tr("load process organizer..."), Qt::AlignRight | Qt::AlignBottom);
    QCoreApplication::processEvents();

    m_processOrganizer = new ProcessOrganizer();
    AppManagement::setProcessOrganizer(qobject_cast<QObject*>(m_processOrganizer));

    qDebug("MainApplication::setupApplication");

    // starting AddInManager
    m_splashScreen->showMessage(tr("scan and load plugins..."), Qt::AlignRight | Qt::AlignBottom);
    QCoreApplication::processEvents();

    //AddInManager *AIM = NULL;
    AddInManager *AIM = AddInManager::createInstance(AppManagement::getSettingsFile(), ito::ITOM_API_FUNCS_GRAPH, AppManagement::getMainWindow(), AppManagement::getMainApplication());
    ito::ITOM_API_FUNCS = AIM->getItomApiFuncsPtr();
    AppManagement::setAddInManager(AIM);
    AIM->setTimeOuts(AppManagement::timeouts.pluginInitClose, AppManagement::timeouts.pluginGeneral);
    connect(AIM, SIGNAL(splashLoadMessage(const QString&, int, const QColor &)), m_splashScreen, SLOT(showMessage(const QString&, int, const QColor &)));
    retValue += AIM->scanAddInDir("");

    qDebug("..plugins loaded");

    m_splashScreen->showMessage(tr("start python..."), Qt::AlignRight | Qt::AlignBottom);
    QCoreApplication::processEvents();

    m_pyEngine = new PythonEngine();
    AppManagement::setPythonEngine(qobject_cast<QObject*>(m_pyEngine));

    qDebug("..python engine started");

    m_pyThread = new QThread();
    m_pyEngine->moveToThread(m_pyThread);
    m_pyThread->start();
    QMetaObject::invokeMethod(m_pyEngine, "pythonSetup", Qt::BlockingQueuedConnection, Q_ARG(ito::RetVal*, &pyRetValue), Q_ARG(QSharedPointer<QVariantMap>, infoMessages));

    qDebug("..python engine moved to new thread");

    retValue += pyRetValue;
    if (pyRetValue.containsError())
    {
        if (pyRetValue.hasErrorMessage())
        {
            qDebug() << "..python engine destroyed since python could not be properly initialized. Reason:" << pyRetValue.errorMessage();
        }
        else
        {
            qDebug() << "..python engine destroyed since python could not be properly initialized. Unknown reason";
        }
        DELETE_AND_SET_NULL(m_pyEngine);
        AppManagement::setPythonEngine(NULL);
    }

    if (m_guiType == standard || m_guiType == console)
    {
        m_splashScreen->showMessage(tr("load main window..."), Qt::AlignRight | Qt::AlignBottom);
        QCoreApplication::processEvents();

        m_mainWin = new MainWindow();
        AppManagement::setMainWindow(qobject_cast<QObject*>(m_mainWin));

        if (m_mainWin && infoMessages->size() > 0)
        {
            QMapIterator<QString, QVariant> it(*infoMessages);
            while (it.hasNext()) 
            {
                it.next();
                m_mainWin->showInfoMessageLine(it.value().toString(), it.key());
            }
        }

        m_splashScreen->showMessage(tr("load ui organizer..."), Qt::AlignRight | Qt::AlignBottom);
        QCoreApplication::processEvents();

        m_uiOrganizer = new UiOrganizer(retValue);
        AppManagement::setUiOrganizer(qobject_cast<QObject*>(m_uiOrganizer));

        m_splashScreen->showMessage(tr("scan and load designer widgets..."), Qt::AlignRight | Qt::AlignBottom);
        QCoreApplication::processEvents();

        m_designerWidgetOrganizer = new DesignerWidgetOrganizer(retValue);
        AppManagement::setDesignerWidgetOrganizer(qobject_cast<QObject*>(m_designerWidgetOrganizer));
        if (AIM)
        {
            AIM->setMainWindow(m_mainWin);
        }
    }
    else
    {
        m_mainWin = NULL;
    }

    qDebug("..main window started");

    m_paletteOrganizer = new PaletteOrganizer();
    AppManagement::setPaletteOrganizer(qobject_cast<QObject*>(m_paletteOrganizer));

    qDebug("..palette organizer started");

    m_splashScreen->showMessage(tr("load script editor organizer..."), Qt::AlignRight | Qt::AlignBottom);
    QCoreApplication::processEvents();

    m_scriptEditorOrganizer = new ScriptEditorOrganizer(m_mainWin != NULL);
    AppManagement::setScriptEditorOrganizer(m_scriptEditorOrganizer); //qobject_cast<QObject*>(scriptEditorOrganizer);

    qDebug("..script editor started");

    if (m_mainWin != NULL)
    {
        connect(m_scriptEditorOrganizer, SIGNAL(addScriptDockWidgetToMainWindow(AbstractDockWidget*,Qt::DockWidgetArea)), m_mainWin, SLOT(addAbstractDock(AbstractDockWidget*, Qt::DockWidgetArea)));
        connect(m_scriptEditorOrganizer, SIGNAL(removeScriptDockWidgetFromMainWindow(AbstractDockWidget*)), m_mainWin, SLOT(removeAbstractDock(AbstractDockWidget*)));
        connect(m_mainWin, SIGNAL(mainWindowCloseRequest()), this, SLOT(mainWindowCloseRequest()));

        if (m_scriptEditorOrganizer)
        {
            m_scriptEditorOrganizer->restoreScriptState();

            foreach(const QString &script, scriptsToOpen)
            {
                QFileInfo info(script);
                if (info.exists())
                {
                    m_scriptEditorOrganizer->openScript(script);
                }
            }
        }
    }

    qDebug("..starting load settings");
    settings = new QSettings(AppManagement::getSettingsFile(), QSettings::IniFormat); //reload settings, since all organizers can load their own instances, that might lead to an unwanted read/write mixture.

    //the current directory is set after having loaded all plugins and designerPlugins
    //Reason: There is a crazy bug, if starting itom in Visual Studio, Debug-Mode. If the current directory
    //is a network drive, no plugins can be loaded any more using Window's loadLibrary command!
    settings->beginGroup("CurrentStatus");
    QDir dir(settings->value("currentDir",QDir::currentPath()).toString());
    if (dir.exists())
    {
        QDir::setCurrent(settings->value("currentDir",QDir::currentPath()).toString());
    }
    settings->endGroup();
    settings->sync();

//This block is currently not perfectly working since it has too much negative side-influences...
//#ifdef WIN32
//    //For Windows: add the append and prepend pathes to the search directories for subsequent LoadLibrary commands. This is done after
//    //having loaded all the plugins, since the 'SetDefaultDllDirectories' command will let some plugins not beeing loaded.
//    if (appendPathes.length() > 0 || prependPathes.length() > 0)
//    {
//#ifdef WINVER
//#if WINVER >= 0x0602 
//        //this is optional and only valid for Windows 8 or higher (at least the Windows SDK must be compatibel to this).
//        //the 'lib' directory is already added to the default search pathes for LoadLibrary commands in main.cpp.
//        //However, further pathes have to be added with AddDllDirectory, which is only available for Windows SDKs >= Win8!
//        //
//        if (QSysInfo::windowsVersion() >= QSysInfo::WV_WINDOWS7) //sometimes the win8 SDK has also be propagated to Windows 7. Therefore, let Win7 be accepted, too.
//        {
//            SetDefaultDllDirectories(LOAD_LIBRARY_SEARCH_DEFAULT_DIRS);
//#if UNICODE
//            //sometimes LoadLibrary commands in plugins with files that are located in the lib folder cannot be loaded
//            //even if the lib folder is add to the path variable in this funtion, too. The SetDllDirectory
//            //is another approach to reach this (only available since Win XP).
//            foreach(const QString &path, prependPathes + appendPathes)
//            {
//                wchar_t *lib_path = new wchar_t[path.size() + 5];
//                memset(lib_path, 0, (path.size() + 5) * sizeof(wchar_t));
//                path.toWCharArray(lib_path);
//                AddDllDirectory(lib_path);
//                delete lib_path;
//#else
//            AddDllDirectory(path.toLatin1().data());
//#endif
//            }
//        }
//#endif
//#endif
//    }
//#endif

    //try to execute startup-python scripts
    m_splashScreen->showMessage(tr("execute startup scripts..."), Qt::AlignRight | Qt::AlignBottom);
    QCoreApplication::processEvents();

    settings->beginGroup("Python");

    int size = settings->beginReadArray("startupFiles");
    QFileInfo startupScript;
    QDir baseDir(QCoreApplication::applicationDirPath());

    for (int i = 0; i < size; ++i)
    {
        settings->setArrayIndex(i);
        startupScript = QFileInfo(baseDir, settings->value("file", QString()).toString()); //if "file" is absolute, baseDir is disregarded
        if (startupScript.isFile())
        {
            startupScripts.append(startupScript.absoluteFilePath());
        }
    }

    settings->endArray();
    settings->endGroup();
    settings->sync();

    if (startupScripts.count() > 0)
    {
        QMetaObject::invokeMethod(m_pyEngine, "pythonRunFile", Q_ARG(QString, startupScripts.join(";")));
    }
    QMetaObject::invokeMethod(m_pyEngine, "pythonGetClearAllValues");

    settings->beginGroup("CurrentStatus");
    QString currentDir = (settings->value("currentDir", QDir::currentPath()).toString());
    settings->endGroup();
    DELETE_AND_SET_NULL(settings);

    m_splashScreen->showMessage(tr("scan and run scripts in autostart folder..."), Qt::AlignRight | Qt::AlignBottom);
    QCoreApplication::processEvents();

    //force python to scan and run files in autostart folder in itom-packages folder
    QMetaObject::invokeMethod(m_pyEngine, "scanAndRunAutostartFolder", Q_ARG(QString, currentDir));

    ////since autostart-files could have changed current directory, re-change it to the value of the settings-file
    //settings.beginGroup("CurrentStatus");
    //QDir::setCurrent(settings.value("currentDir",QDir::currentPath()).toString());
    //settings.endGroup();

    if (retValue.containsError())
    {
        if (retValue.hasErrorMessage())
        {
            std::cerr << "Error when starting the application: \n" << retValue.errorMessage() << std::endl;
        }
        else
        {
            std::cerr << "An unspecified error occurred when starting the application." << std::endl;
        }
    }
    else if (retValue.containsWarning())
    {
        if (retValue.hasErrorMessage())
        {
            std::cout << "Warning when starting the application: \n" << retValue.errorMessage() << std::endl;
        }
        else
        {
            std::cout << "An unspecified warning occurred when starting the application." << std::endl;
        }
    }

    qDebug("..load settings done");
    qDebug("MainApplication::setupApplication .. done");

    //std::cout << "\n    Welcome to itom program!\n\n";
    //std::cout << "    Please report bugs under:\n        https://bitbucket.org/itom/itom/issues\n    Cheers your itom team\n" << std::endl;

    if (m_mainWin)
    {
        m_splashScreen->finish(m_mainWin);
    }
    else
    {
        m_splashScreen->close();
    }
}

//----------------------------------------------------------------------------------------------------------------------------------
//! setup of application
/*!
    stops PythonEngine, MainWindow (dependent on gui-type) and all necessary managers and organizers.
    Closes import connections between MainWindow and PythonEngine as well as ScriptEditorOrganizer.

    \sa PythonEngine, MainWindow, ScriptEditorOrganizer
*/
void MainApplication::finalizeApplication()
{
    DELETE_AND_SET_NULL(m_splashScreen);

    DELETE_AND_SET_NULL(m_scriptEditorOrganizer);
    AppManagement::setScriptEditorOrganizer(NULL);

    DELETE_AND_SET_NULL(m_paletteOrganizer);
    AppManagement::setPaletteOrganizer(NULL);

    DELETE_AND_SET_NULL(m_designerWidgetOrganizer);
    AppManagement::setDesignerWidgetOrganizer(NULL);

    DELETE_AND_SET_NULL(m_uiOrganizer);
    AppManagement::setUiOrganizer(NULL);

    DELETE_AND_SET_NULL(m_mainWin);
    AppManagement::setMainWindow(NULL);

    if (m_pyEngine)
    {
        ItomSharedSemaphore *waitCond = new ItomSharedSemaphore();
        QMetaObject::invokeMethod(m_pyEngine, "pythonShutdown", Q_ARG(ItomSharedSemaphore*, waitCond));
        waitCond->waitAndProcessEvents(-1);

        //call further objects, which have been marked by "deleteLater" during this finalize method (partI)
        QCoreApplication::sendPostedEvents();
        QCoreApplication::sendPostedEvents(NULL,QEvent::DeferredDelete); //these events are not sent by the line above, since the event-loop already has been stopped.
        QCoreApplication::processEvents();

        waitCond->deleteSemaphore();
        waitCond = NULL;
    }

    DELETE_AND_SET_NULL(m_pyEngine);
    AppManagement::setPythonEngine(NULL);

    if (m_pyThread)
    {
        m_pyThread->quit();
        m_pyThread->wait();
    }
    DELETE_AND_SET_NULL(m_pyThread);

    AddInManager::closeInstance();

    DELETE_AND_SET_NULL(m_processOrganizer);
    AppManagement::setProcessOrganizer(NULL);

    //call further objects, which have been marked by "deleteLater" during this finalize method (partII)
    QCoreApplication::sendPostedEvents();
    QCoreApplication::sendPostedEvents(NULL,QEvent::DeferredDelete); //these events are not sent by the line above, since the event-loop already has been stopped.
    QCoreApplication::processEvents();

    QString settingsName(AppManagement::getSettingsFile());
    QSettings *settings = new QSettings(settingsName, QSettings::IniFormat);
    settings->beginGroup("CurrentStatus");
    settings->setValue("currentDir",QDir::currentPath());
    settings->endGroup();

    //save timeouts
    settings->beginGroup("Application");
    settings->setValue("timeoutInitClose", AppManagement::timeouts.pluginInitClose);
    settings->setValue("timeoutGeneral", AppManagement::timeouts.pluginGeneral);
    settings->endGroup();

    delete settings;


    //close std::cout and std::cerr stream redirection
    DELETE_AND_SET_NULL(m_pQout);
    DELETE_AND_SET_NULL(m_pQerr);
}

//----------------------------------------------------------------------------------------------------------------------------------
//! slot invoked if user wants to close application
/*!
    \sa MainWindow
*/
void MainApplication::mainWindowCloseRequest()
{
    RetVal retValue(retOk);

    QSettings *settings = new QSettings(AppManagement::getSettingsFile(), QSettings::IniFormat);
    settings->beginGroup("MainWindow");

	bool pythonStopped = false;
	bool closeRequestCanceled = false;
	int dialogRequest = -1;

	if (m_pyEngine != NULL && m_pyEngine->isPythonBusy())
	{
		DialogCloseItom *dialog = new DialogCloseItom(NULL);

		int dialogRequest = dialog->exec();

		if (dialogRequest == QDialog::Accepted)
		{
			pythonStopped = true;
		}
		else if (dialogRequest == QDialog::Rejected)
		{
			closeRequestCanceled = true;
		}

		DELETE_AND_SET_NULL(dialog);
	}
	else if (settings->value("askBeforeClose", true).toBool() && !pythonStopped)
	{
		QMessageBox msgBox;
		msgBox.setText(tr("Do you really want to exit the application?"));
		msgBox.setStandardButtons(QMessageBox::Ok | QMessageBox::Cancel);
		msgBox.setDefaultButton(QMessageBox::Ok);
		msgBox.setIcon(QMessageBox::Question);
		int ret = msgBox.exec();

		if (ret == QMessageBox::Cancel)
		{
			settings->endGroup();
			delete settings;
			return;
		}
	}

	if (!retValue.containsError() && !closeRequestCanceled)
	{
		settings->endGroup();
		delete settings;
    
		if (retValue.containsError()) return;

		//saves the state of all opened scripts to the settings file
		if (m_scriptEditorOrganizer)
		{
            m_scriptEditorOrganizer->saveScriptState();

            retValue += m_scriptEditorOrganizer->closeAllScripts(true);

            if (retValue.containsError())
            {
                //The user was asked how to proceed with unsaved scripts. In this case, the user cancelled this request... do not close itom!
                return;
            }
		}
	
		if (m_mainWin)
		{
			m_mainWin->hide();
		}
		QApplication::instance()->quit();
    }
}

//----------------------------------------------------------------------------------------------------------------------------------


//----------------------------------------------------------------------------------------------------------------------------------
//! exececution of the main event loop
/*!
    \return value passed to exit() method, which finishes the exec()-loop, 0 if finished by quit()
*/
int MainApplication::exec()
{

    if (m_guiType == standard)
    {
        m_mainWin->show();
        return QApplication::instance()->exec();
    }
    else if (m_guiType == console)
    {
        m_mainWin->show();
        return QApplication::instance()->exec();
    }
    else
    {
        return QApplication::instance()->exec();
    }
}

//----------------------------------------------------------------------------------------------------------------------------------
int MainApplication::execPipManagerOnly()
{
    DialogPipManager manager(NULL, true);
    return manager.exec();
}

} //end namespace ito
