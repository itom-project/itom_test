/* ********************************************************************
    itom software
    URL: http://www.uni-stuttgart.de/ito
    Copyright (C) 2014, Institut f�r Technische Optik (ITO),
    Universit�t Stuttgart, Germany

    This file is part of itom and its software development toolkit (SDK).

    itom is free software; you can redistribute it and/or modify it
    under the terms of the GNU Library General Public Licence as published by
    the Free Software Foundation; either version 2 of the Licence, or (at
    your option) any later version.
   
    In addition, as a special exception, the Institut f�r Technische
    Optik (ITO) gives you certain additional rights.
    These rights are described in the ITO LGPL Exception version 1.0,
    which can be found in the file LGPL_EXCEPTION.txt in this package.

    itom is distributed in the hope that it will be useful, but
    WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU Library
    General Public Licence for more details.

    You should have received a copy of the GNU Library General Public License
    along with itom. If not, see <http://www.gnu.org/licenses/>.
*********************************************************************** */

#include "pluginThreadCtrl.h"

//#include <qdebug.h>
#include <qelapsedtimer.h>
#include <qvector.h>

namespace ito
{

//----------------------------------------------------------------------------------------------------------------------------------
PluginThreadCtrl::PluginThreadCtrl() :
    m_pPlugin(NULL)
{
}

//----------------------------------------------------------------------------------------------------------------------------------
/*!
    \return (void)
    \sa CameraThreadCtrl
*/
PluginThreadCtrl::PluginThreadCtrl(const ito::ParamBase &pluginParameter, ito::RetVal *retval /*= NULL*/) :
    m_pPlugin(NULL)
{
    void *plugin = pluginParameter.getVal<void *>();

    if (plugin && (reinterpret_cast<ito::AddInBase *>(plugin)->getBasePlugin()->getType() & (ito::typeDataIO | ito::typeActuator)))
    {
        m_pPlugin = (ito::AddInBase *)(plugin);

        m_pPlugin->getBasePlugin()->incRef(m_pPlugin); //increment reference, such that camera is not deleted
    }

    if (m_pPlugin == NULL)
    {
        if (retval)
        {
            (*retval) += ito::RetVal(ito::retError, 0, "no or invalid plugin given.");
        }
        return;
    }
}
    
//----------------------------------------------------------------------------------------------------------------------------------
PluginThreadCtrl::PluginThreadCtrl(ito::AddInBase *plugin, ito::RetVal *retval /*= NULL*/) :
    m_pPlugin(plugin)
{
    if (m_pPlugin == NULL)
    {
        if (retval)
        {
            (*retval) += ito::RetVal(ito::retError, 0, "no or invalid plugin given");
        }
        return;
    }
    else
    {
        m_pPlugin->getBasePlugin()->incRef(m_pPlugin); //increment reference, such that camera is not deleted
    }
}

//----------------------------------------------------------------------------------------------------------------------------------
/*!
    \detail The destructor
*/
PluginThreadCtrl::~PluginThreadCtrl()
{
    if (m_pPlugin)
    {
        m_pPlugin->getBasePlugin()->decRef(m_pPlugin); //decrement reference
    }
}

//----------------------------------------------------------------------------------------------------------------------------------
PluginThreadCtrl::PluginThreadCtrl(const PluginThreadCtrl &other) :
    m_pPlugin(other.m_pPlugin)
{
    if (m_pPlugin)
    {
        m_pPlugin->getBasePlugin()->incRef(m_pPlugin); //increment reference, such that camera is not deleted
    }
}

//----------------------------------------------------------------------------------------------------------------------------------
PluginThreadCtrl& PluginThreadCtrl::operator =(const PluginThreadCtrl &other)
{
    ito::AddInBase *op = other.m_pPlugin;
    if (op)
    {
        op->getBasePlugin()->incRef(op); //increment reference, such that camera is not deleted
    }

    if (m_pPlugin)
    {
        m_pPlugin->getBasePlugin()->decRef(m_pPlugin); //decrement reference
        m_pPlugin = NULL;
    }

    m_pPlugin = op;

    return *this;
}

//----------------------------------------------------------------------------------------------------------------------------------
/*!
    \detail After the invoke-command this thread must wait / be synchronize with the camera-thread.
            Therefore the wait-Function of pMySemaphore is called. If the camera do not answer within timeOutMS and the pMyCamera is not alive anymore, the function returns a timeout.

    \param [in] timeOutMS    TimeOut for the semaphore-wait

    \return retOk or retError
    \sa threadActuator, threadCamera
*/
ito::RetVal PluginThreadCtrl::waitForSemaphore(ItomSharedSemaphore *waitCond, int timeOutMS)
{
    ito::RetVal retval(ito::retOk);
    bool timeout = false;
    

    if (timeOutMS == 0)
    {
        while(!timeout && waitCond->wait(PLUGINWAIT) == false)
        {
            if (m_pPlugin->isAlive() == false)
            {
                retval += ito::RetVal(ito::retError, 0, QObject::tr("Timeout while waiting for answer from camera.").toLatin1().data());
                timeout = true;
            }
        }
    }
    else
    {
        QElapsedTimer timer;
        timer.start();
        int t = std::min(timeOutMS, PLUGINWAIT);

        while(!timeout && waitCond->wait(t) == false)
        {
            if (timer.elapsed() > timeOutMS)
            {
                retval += ito::RetVal(ito::retError, 0, QObject::tr("Timeout while waiting for answer from camera.").toLatin1().data());
                timeout = true;
            }
            else if (m_pPlugin->isAlive() == false)
            {
                retval += ito::RetVal(ito::retError, 0, QObject::tr("Timeout while waiting for answer from camera.").toLatin1().data());
                timeout = true;
            }
        }
    }
        
    if (!timeout)
    {
        retval += waitCond->returnValue;
    }

    return retval;
}

//----------------------------------------------------------------------------------------------------------------------------------
/*!
    \detail Get any parameter of the camera defined by val.name. val must be initialised and name must be correct. After correct execution, val has the correct value.

    \param [in|out] val      Initialised tParam (correct name | in)
    \param [in] timeOutMS    TimeOut for the semaphore-wait

    \return retOk or retError
    \sa threadActuator, threadCamera
*/
ito::RetVal PluginThreadCtrl::getParam(ito::Param &val, int timeOutMS)
{
    if (!m_pPlugin)
    {
        return ito::RetVal(ito::retError, 0, "no camera available");
    }

    ItomSharedSemaphoreLocker locker(new ItomSharedSemaphore());
    QSharedPointer<ito::Param> qsParam(new ito::Param(val));
    if (!QMetaObject::invokeMethod(m_pPlugin, "getParam", Q_ARG(QSharedPointer<ito::Param>, qsParam), Q_ARG(ItomSharedSemaphore*, locker.getSemaphore())))
    {
        return ito::RetVal(ito::retError, 0, "error invoking getParam");
    }

    ito::RetVal retval = waitForSemaphore(locker.getSemaphore(), timeOutMS);

    if (!retval.containsError())
    {
        val = *qsParam;
    }
    return retval;
}

//----------------------------------------------------------------------------------------------------------------------------------
/*!
    \detail Get the parameter of the plugin defined by val.name to the value of val.

    \param [in] val         Initialised tParam (correct name | value)
    \param [in] timeOutMS   TimeOut for the semaphore-wait

    \return retOk or retError
    \sa threadActuator, threadCamera
*/
ito::RetVal PluginThreadCtrl::setParam(ito::ParamBase val, int timeOutMS)
{
    if (!m_pPlugin)
    {
        return ito::RetVal(ito::retError, 0, "no camera available");
    }

    ItomSharedSemaphoreLocker locker(new ItomSharedSemaphore());
    QSharedPointer<ito::ParamBase> qsParam(new ito::ParamBase(val));
    if (!QMetaObject::invokeMethod(m_pPlugin, "setParam", Q_ARG(QSharedPointer<ito::ParamBase>, qsParam), Q_ARG(ItomSharedSemaphore*, locker.getSemaphore())))
    {
        return ito::RetVal(ito::retError, 0, "error invoking setParam");
    }
    
    return waitForSemaphore(locker.getSemaphore(), timeOutMS);
}



//----------------------------------------------------------------------------------------------------------------------------------
DataIOThreadCtrl::DataIOThreadCtrl() :
    PluginThreadCtrl()
{
}

//----------------------------------------------------------------------------------------------------------------------------------
DataIOThreadCtrl::DataIOThreadCtrl(const ito::ParamBase &pluginParameter, ito::RetVal *retval /*= NULL*/) :
    PluginThreadCtrl(pluginParameter, retval)
{
}

//----------------------------------------------------------------------------------------------------------------------------------
DataIOThreadCtrl::DataIOThreadCtrl(ito::AddInDataIO *plugin, ito::RetVal *retval /*= NULL*/) :
    PluginThreadCtrl(plugin, retval)
{
}

//----------------------------------------------------------------------------------------------------------------------------------
DataIOThreadCtrl::DataIOThreadCtrl(const DataIOThreadCtrl &other) : 
    PluginThreadCtrl(other)
{

}

//----------------------------------------------------------------------------------------------------------------------------------
DataIOThreadCtrl::~DataIOThreadCtrl()
{
}

//----------------------------------------------------------------------------------------------------------------------------------
/*!
    \detail Every capture procedure starts with the startDevice() to set the camera active and is ended with stopDevice().

    \param [in] timeOutMS    TimeOut for the semaphore-wait

    \return retOk or retError
    \sa threadActuator, threadCamera, threadCamera::stopDevice, threadCamera::acquire, threadCamera::getVal, threadCamera::copyVal
*/
ito::RetVal DataIOThreadCtrl::startDevice(int timeOutMS)
{
    if (!m_pPlugin)
    {
        return ito::RetVal(ito::retError, 0, "no camera available");
    }

    ItomSharedSemaphoreLocker locker(new ItomSharedSemaphore());
    if (!QMetaObject::invokeMethod(m_pPlugin, "startDevice", Q_ARG(ItomSharedSemaphore*, locker.getSemaphore())))
    {
        return ito::RetVal(ito::retError, 0, "error invoking startDevice");
    }

    return waitForSemaphore(locker.getSemaphore(), timeOutMS);
}

//----------------------------------------------------------------------------------------------------------------------------------
/*!
    \detail Every capture procedure starts with the startDevice() to set the camera active and is ended with stopDevice().

    \param [in] timeOutMS    TimeOut for the semaphore-wait

    \return retOk or retError
    \sa threadActuator, threadCamera, threadCamera::startDevice, threadCamera::acquire, threadCamera::getVal, threadCamera::copyVal
*/
ito::RetVal DataIOThreadCtrl::stopDevice(int timeOutMS)
{
    if (!m_pPlugin)
    {
        return ito::RetVal(ito::retError, 0, "no camera available");
    }

    ItomSharedSemaphoreLocker locker(new ItomSharedSemaphore());
    if (!QMetaObject::invokeMethod(m_pPlugin, "stopDevice", Q_ARG(ItomSharedSemaphore*, locker.getSemaphore())))
    {
        return ito::RetVal(ito::retError, 0, "error invoking stopDevice");
    }

    return waitForSemaphore(locker.getSemaphore(), timeOutMS);
}

//----------------------------------------------------------------------------------------------------------------------------------
/*!
    \detail The acquire()-function triggers a new exposure of the camera and returns afterwards. It can only be executed after startDevice().
            The function does not wait until the exposure is done. This is performed by the getVal or copyVal-method.

    \param [in] trigger     A currently not implemented constant to define trigger-modes during exposure of the camera
    \param [in] timeOutMS   TimeOut for the semaphore-wait

    \return retOk or retError
    \sa threadActuator, threadCamera, threadCamera::stopDevice, threadCamera::startDevice, threadCamera::getVal, threadCamera::copyVal
*/
ito::RetVal DataIOThreadCtrl::acquire(const int trigger /*= 0*/, int timeOutMS)
{
    if (!m_pPlugin)
    {
        return ito::RetVal(ito::retError, 0, "no camera available");
    }

    ItomSharedSemaphoreLocker locker(new ItomSharedSemaphore());
    if (!QMetaObject::invokeMethod(m_pPlugin, "acquire", Q_ARG(int, trigger), Q_ARG(ItomSharedSemaphore*, locker.getSemaphore())))
    {
        return ito::RetVal(ito::retError, 0, "error invoking acquire");
    }

    return waitForSemaphore(locker.getSemaphore(), timeOutMS);
}

//----------------------------------------------------------------------------------------------------------------------------------
/*!
    \detail The getVal function is used to wait until an exposure is finished. Than it gives a shallow copy of the inner dataObject within the grabber to the dObj-argument.
            Before the getVal()-function can be used an acquire() is neccessary.
            If the content of dObj is not deepcopied to another object, the data is lost after the next acquire() - getVal() combination and overwritten by the newly captured image.

    \param [in|out] dObj    IN: an dataObject | OUT: an dataObject containing an shallow copy of the last captured image
    \param [in] timeOutMS   TimeOut for the semaphore-wait

    \return retOk or retError
    \sa threadActuator, threadCamera, threadCamera::stopDevice, threadCamera::acquire, threadCamera::startDevice, threadCamera::copyVal
*/
ito::RetVal DataIOThreadCtrl::getVal(ito::DataObject &dObj, int timeOutMS)   /*! < Get a shallow-copy of the dataObject */
{
    if (!m_pPlugin)
    {
        return ito::RetVal(ito::retError, 0, "no camera available");
    }

    ItomSharedSemaphoreLocker locker(new ItomSharedSemaphore());
    if (!QMetaObject::invokeMethod(m_pPlugin, "getVal", Q_ARG(void*, (void *)&dObj), Q_ARG(ItomSharedSemaphore*, locker.getSemaphore())))
    {
        return ito::RetVal(ito::retError, 0, "error invoking getVal");
    }

    return waitForSemaphore(locker.getSemaphore(), timeOutMS);
}

//----------------------------------------------------------------------------------------------------------------------------------
/*!
    \detail The copyVal function is used to wait until an exposure is finished. Than it gives a deep copy of the inner dataObject within the grabber to the dObj-argument.
            Before the copyVal()-function can be used an acquire() is neccessary.
            If the content of dObj do not need to be deepcopied to another object and will not be overwritten after the next acquire() - getVal() combination.

    \param [in|out] dObj    IN: an dataObject | OUT: an dataObject containing an shallow copy of the last captured image
    \param [in] timeOutMS   TimeOut for the semaphore-wait

    \return retOk or retError
    \sa threadActuator, threadCamera, threadCamera::stopDevice, threadCamera::acquire, threadCamera::getVal, threadCamera::startDevice
*/
ito::RetVal DataIOThreadCtrl::copyVal(ito::DataObject &dObj, int timeOutMS)  /*! < Get a deep-copy of the dataObject */
{
    if (!m_pPlugin)
    {
        return ito::RetVal(ito::retError, 0, "no camera available");
    }

    ItomSharedSemaphoreLocker locker(new ItomSharedSemaphore());
    if (!QMetaObject::invokeMethod(m_pPlugin, "copyVal", Q_ARG(void*, (void *)&dObj), Q_ARG(ItomSharedSemaphore*, locker.getSemaphore())))
    {
        return ito::RetVal(ito::retError, 0, "error invoking copyVal");
    }

    return waitForSemaphore(locker.getSemaphore(), timeOutMS);
}



//----------------------------------------------------------------------------------------------------------------------------------
/*!
    \detail Get the most important parameter of the camera.

    \param [out] bpp        Number of Bits this camera grabs
    \param [out] xsize      Size of the camera in x (cols)
    \param [out] ysize      Size of the camera in y (rows)

    \return retOk or retError
    \sa threadActuator, threadCamera
*/

ito::RetVal DataIOThreadCtrl::getImageParams(int &bpp, int &sizex, int &sizey, int timeOutMS)
{
    if (!m_pPlugin)
    {
        return ito::RetVal(ito::retError, 0, "no camera available");
    }

    
    ItomSharedSemaphoreLocker locker(new ItomSharedSemaphore());
    QSharedPointer<ito::Param> param1(new ito::Param("bpp", ito::ParamBase::Int));
    QSharedPointer<ito::Param> param2(new ito::Param("sizex", ito::ParamBase::Int));
    QSharedPointer<ito::Param> param3(new ito::Param("sizey", ito::ParamBase::Int));
    QVector<QSharedPointer<ito::Param> > params;
    params << param1 << param2 << param3;
    if (!QMetaObject::invokeMethod(m_pPlugin, "getParamVector", Q_ARG(QVector<QSharedPointer<ito::Param> >, params), Q_ARG(ItomSharedSemaphore*, locker.getSemaphore())))
    {
        return ito::RetVal(ito::retError, 0, "error invoking copyVal");
    }

    ito::RetVal retval = waitForSemaphore(locker.getSemaphore(), timeOutMS);

    if (!retval.containsError())
    {
        bpp = params[0]->getVal<int>();
        sizex = params[1]->getVal<int>();
        sizey = params[2]->getVal<int>();
    }

    return retval;
}



//----------------------------------------------------------------------------------------------------------------------------------
ActuatorThreadCtrl::ActuatorThreadCtrl() :
    PluginThreadCtrl(),
    m_numAxes(-1)
{
}

//----------------------------------------------------------------------------------------------------------------------------------
ActuatorThreadCtrl::ActuatorThreadCtrl(const ito::ParamBase &pluginParameter, ito::RetVal *retval /*= NULL*/) :
    PluginThreadCtrl(pluginParameter, retval),
    m_numAxes(-1)
{
}

//----------------------------------------------------------------------------------------------------------------------------------
ActuatorThreadCtrl::ActuatorThreadCtrl(ito::AddInActuator *plugin, ito::RetVal *retval /*= NULL*/) :
    PluginThreadCtrl(plugin, retval),
    m_numAxes(-1)
{
}

//----------------------------------------------------------------------------------------------------------------------------------
ActuatorThreadCtrl::ActuatorThreadCtrl(const ActuatorThreadCtrl &other) : 
    PluginThreadCtrl(other)
{
    m_numAxes = other.m_numAxes;
}

//----------------------------------------------------------------------------------------------------------------------------------
ActuatorThreadCtrl::~ActuatorThreadCtrl()
{
}


//----------------------------------------------------------------------------------------------------------------------------------
/*!
    \detail Move the axis in axisVec with a distance defined in stepSizeVec relative to current position.
            The axisVec and stepSizeVec must be same size. After the invoke-command this thread must wait / synchronize with the actuator-thread.
            Therefore the semaphore->wait is called via the function threadActuator::waitForSemaphore(timeOutMS)
            To enable the algorithm to process data during movement, the waitForSemaphore(timeOutMS) can be skipped by callWait = false.
            The threadActuator::waitForSemaphore(timeOutMS)-function must than be called by the algorithms afterwards / before the next command is send to the actuator.

    \param [in] axisVec         Vector with the axis to move
    \param [in] stepSizeVec     Vector with the distances for every axis
    \param [in] timeOutMS       TimeOut for the semaphore-wait, if (0) the waitForSemaphore is not called and must be called seperate by the algorithm

    \return retOk or retError
    \sa threadActuator
*/
ito::RetVal ActuatorThreadCtrl::setPosRel(const QVector<int> &axes, const QVector<double> &relPositions, int timeOutMS /*= PLUGINWAIT*/)
{
    if (!m_pPlugin)
    {
        return ito::RetVal(ito::retError, 0, "no actuator available");
    }

    if (relPositions.size() != axes.size())
    {
        return ito::RetVal(ito::retError, 0, QObject::tr("Error during setPosRel: Vectors differ in size").toLatin1().data());
    }

    ItomSharedSemaphoreLocker locker(new ItomSharedSemaphore());
    if(!QMetaObject::invokeMethod(m_pPlugin, "setPosRel", Q_ARG(QVector<int>, axes), Q_ARG(QVector<double>, relPositions), Q_ARG(ItomSharedSemaphore*, locker.getSemaphore())))
    {
        return ito::RetVal(ito::retError, 0, "error invoking setPosRel");
    }
    
    return waitForSemaphore(locker.getSemaphore(), timeOutMS);
}

//----------------------------------------------------------------------------------------------------------------------------------
/*!
    \detail Move the axis in axisVec to the positions given in posVec.
            The axisVec and posVec must be same size. After the invoke-command this thread must wait / synchronize with the actuator-thread.
            Therefore the semaphore->wait is called via the function threadActuator::waitForSemaphore(timeOutMS)
            To enable the algorithm to process data during movement, the waitForSemaphore(timeOutMS) can be skipped by callWait = false.
            The threadActuator::waitForSemaphore(timeOutMS)-function must than be called by the algorithms afterwards / before the next command is send to the actuator.

    \param [in] axisVec         Vector with the axis to move
    \param [in] posVec          Vector with the new absolute positions
    \param [in] timeOutMS       TimeOut for the semaphore-wait, if (0) the waitForSemaphore is not called and must be called seperate by the algorithm

    \return retOk or retError
    \sa threadActuator
*/
ito::RetVal ActuatorThreadCtrl::setPosAbs(const QVector<int> &axes, const QVector<double> &absPositions, int timeOutMS /*= PLUGINWAIT*/)
{
    if (!m_pPlugin)
    {
        return ito::RetVal(ito::retError, 0, "no actuator available");
    }

    if (absPositions.size() != axes.size())
    {
        return ito::RetVal(ito::retError, 0, QObject::tr("Error during setPosAbs: Vectors differ in size").toLatin1().data());
    }

    ItomSharedSemaphoreLocker locker(new ItomSharedSemaphore());

    if(!QMetaObject::invokeMethod(m_pPlugin, "setPosAbs", Q_ARG(QVector<int>, axes), Q_ARG(QVector<double>, absPositions), Q_ARG(ItomSharedSemaphore*, locker.getSemaphore())))
    {
        return ito::RetVal(ito::retError, 0, "error invoking setPosAbs");
    }

    return waitForSemaphore(locker.getSemaphore(), timeOutMS);
}

//----------------------------------------------------------------------------------------------------------------------------------
/*!
    \detail Move a single axis specified by axis  with a distance defined in stepSize relative to current position. After the invoke-command this thread must wait / synchronize with the actuator-thread.
            Therefore the semaphore->wait is called via the function threadActuator::waitForSemaphore(timeOutMS)
            To enable the algorithm to process data during movement, the waitForSemaphore(timeOutMS) can be skipped by callWait = false.
            The threadActuator::waitForSemaphore(timeOutMS)-function must than be called by the algorithms afterwards / before the next command is send to the actuator.

    \param [in] axis         Number of the axis
    \param [in] stepSize     Distances from current position
    \param [in] timeOutMS       TimeOut for the semaphore-wait, if (0) the waitForSemaphore is not called and must be called seperate by the algorithm

    \return retOk or retError
    \sa threadActuator
*/
ito::RetVal ActuatorThreadCtrl::setPosRel(int axis, double relPosition, int timeOutMS)
{
    if (!m_pPlugin)
    {
        return ito::RetVal(ito::retError, 0, "no actuator available");
    }

    ItomSharedSemaphoreLocker locker(new ItomSharedSemaphore());

    if (!QMetaObject::invokeMethod(m_pPlugin, "setPosRel", Q_ARG(int, axis), Q_ARG(double, relPosition), Q_ARG(ItomSharedSemaphore*, locker.getSemaphore())))
    {
        return ito::RetVal(ito::retError, 0, "error invoking setPosRel");
    }

    return waitForSemaphore(locker.getSemaphore(), timeOutMS);
}

//----------------------------------------------------------------------------------------------------------------------------------
/*!
    \detail Move a single axis specified by axis to the position pos. After the invoke-command this thread must wait / synchronize with the actuator-thread.
            Therefore the semaphore->wait is called via the function threadActuator::waitForSemaphore(timeOutMS)
            To enable the algorithm to process data during movement, the waitForSemaphore(timeOutMS) can be skipped by callWait = false.
            The threadActuator::waitForSemaphore(timeOutMS)-function must than be called by the algorithms afterwards / before the next command is send to the actuator.

    \param [in] axis         Number of the axis
    \param [in] pos          New position of the axis
    \param [in] timeOutMS       TimeOut for the semaphore-wait, if (0) the waitForSemaphore is not called and must be called seperate by the algorithm

    \return retOk or retError
    \sa threadActuator
*/
ito::RetVal ActuatorThreadCtrl::setPosAbs(int axis, double absPosition, int timeOutMS)
{
    if (!m_pPlugin)
    {
        return ito::RetVal(ito::retError, 0, "no actuator available");
    }

    ItomSharedSemaphoreLocker locker(new ItomSharedSemaphore());
    if (!QMetaObject::invokeMethod(m_pPlugin, "setPosAbs", Q_ARG(int, axis), Q_ARG(double, absPosition), Q_ARG(ItomSharedSemaphore*, locker.getSemaphore())))
    {
        return ito::RetVal(ito::retError, 0, "error invoking setPosAbs");
    }

    return waitForSemaphore(locker.getSemaphore(), timeOutMS);
}

//----------------------------------------------------------------------------------------------------------------------------------
/*!
    \detail Get the position of a single axis specified by axis.

    \param [in] axis         Number of the axis
    \param [out] pos          position of the axis
    \param [in] timeOutMS    TimeOut for the semaphore-wait

    \return retOk or retError
    \sa threadActuator
*/
ito::RetVal ActuatorThreadCtrl::getPos(int axis, double &position, int timeOutMS)
{
    if (!m_pPlugin)
    {
        return ito::RetVal(ito::retError, 0, "no actuator available");
    }

    QSharedPointer<double> posSP(new double);
    *posSP = 0.0;

    ItomSharedSemaphoreLocker locker(new ItomSharedSemaphore());

    if (!QMetaObject::invokeMethod(m_pPlugin, "getPos", Q_ARG(int, (const int) axis), Q_ARG(QSharedPointer<double>, posSP), Q_ARG(ItomSharedSemaphore*, locker.getSemaphore())))
    {
        return ito::RetVal(ito::retError, 0, "error invoking getPos");
    }

    ito::RetVal retval = waitForSemaphore(locker.getSemaphore(), timeOutMS);
    if (!retval.containsError())
    {
        position = *posSP;
    }

    return retval;
}

//----------------------------------------------------------------------------------------------------------------------------------
/*!
    \detail Get the position of a number of axis specified by axisVec.

    \param [in] axisVec         Number of the axis
    \param [out] posVec         Vecotr with position of the axis
    \param [in] timeOutMS    TimeOut for the semaphore-wait

    \return retOk or retError
    \sa threadActuator
*/
ito::RetVal ActuatorThreadCtrl::getPos(QVector<int> axes, QVector<double> &positions, int timeOutMS)
{
    if (!m_pPlugin)
    {
        return ito::RetVal(ito::retError, 0, "no actuator available");
    }

    positions.resize(axes.size());
    QSharedPointer<QVector<double> > posVecSP(new QVector<double>());
    posVecSP->fill(0.0, axes.size());

    ItomSharedSemaphoreLocker locker(new ItomSharedSemaphore());

    if (!QMetaObject::invokeMethod(m_pPlugin, "getPos", Q_ARG(QVector<int>, axes), Q_ARG(QSharedPointer<QVector<double> >, posVecSP), Q_ARG(ItomSharedSemaphore*, locker.getSemaphore())))
    {
        return ito::RetVal(ito::retError, 0, "error invoking getPos");
    }

    ito::RetVal retval = waitForSemaphore(locker.getSemaphore(), timeOutMS);

    if (!retval.containsError())
    {
        positions = *posVecSP;
    }

    return retval;
}

//----------------------------------------------------------------------------------------------------------------------------------
/*!
    \detail Check if a specific axis is within the axisSpace of this actuator

    \param [in] axisNum    Axisnumber

    \return retOk or retError
    \sa threadActuator
*/
ito::RetVal ActuatorThreadCtrl::checkAxis(int axisNum)
{
    if (m_numAxes == -1 && m_pPlugin)
    {
        ito::Param p("numaxes");
        ito::RetVal retval2 = getParam(p);
        if (!retval2.containsError())
        {
            m_numAxes = p.getVal<int>();
        }
        else
        {
            return ito::RetVal(ito::retError, 0, "failed to ask for number of axes of actuator");
        }
    }

    if (m_numAxes == -1)
    {
        return ito::RetVal(ito::retError, 0, "failed to ask for number of axes of actuator");
    }

    if (axisNum < 0 || axisNum >= m_numAxes)
    {
        return ito::retError;
    }
    else
    {
        return ito::retOk;
    }
}


} //end namespace ito

