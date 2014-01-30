/* ********************************************************************
    itom software
    URL: http://www.uni-stuttgart.de/ito
    Copyright (C) 2013, Institut f�r Technische Optik (ITO),
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

#include "helperGrabber.h"

//#include <qdebug.h>

namespace ito
{
    //----------------------------------------------------------------------------------------------------------------------------------
    /*!
        \detail The constructor checks if parameterVector[paramNumber] is a valid camera.
                If yes and the camera is accessable (try bpp) the camera handle is stored in pMyCamera. The Semaphore for the invoke-method is also allocated here.
                If the camera is invalid keeps beeing NULL.

        \param [in] parameterVector     is the ParameterVector (optional or mandatory) of the filter / algorithm
        \param [in] paramNumber         is the zerobased number of the camera in the parameterlist
        \return (void)
        \sa threadCamera
    */
    threadCamera::threadCamera(QVector<ito::ParamBase> *parameterVector, int paramNumber)
    {
        ito::RetVal retval(ito::retOk);

        errorBuffer = ito::retOk;
        pMyCamera = NULL;

        if (parameterVector->isEmpty())
        {
            return;
        }

        if (parameterVector->size() - 1 < paramNumber)
        {
            return;
        }

        if (reinterpret_cast<ito::AddInBase *>((*parameterVector)[paramNumber].getVal<void *>())->getBasePlugin()->getType() & (ito::typeDataIO | ito::typeGrabber))
        {
            pMyCamera = (ito::AddInGrabber *)(*parameterVector)[paramNumber].getVal<void *>();
        }

        if (pMyCamera == NULL)
        {
            return;
        }

        pMySemaphoreLocker = new ItomSharedSemaphore();
        QSharedPointer<ito::Param> qsParamBPP(new ito::Param("bpp", ito::ParamBase::Int));
        QMetaObject::invokeMethod(pMyCamera, "getParam", Q_ARG(QSharedPointer<ito::Param>, qsParamBPP), Q_ARG(ItomSharedSemaphore *, pMySemaphoreLocker.getSemaphore()));

        while (!pMySemaphoreLocker.getSemaphore()->wait(PLUGINWAIT))
        {
            retval += ito::RetVal(ito::retError, 0, QObject::tr("timeout while getting numaxis parameter").toAscii().data());
            return;
        }

        retval += pMySemaphoreLocker.getSemaphore()->returnValue;

        if (retval.containsWarningOrError())
        {
            pMyCamera = NULL;
            return;
        }

        return;
    }

    //----------------------------------------------------------------------------------------------------------------------------------
    /*!
        \detail The destructor. Deletes the semaphore after waiting a last time.
        \return (void)
        \sa threadActuator
    */
    threadCamera::~threadCamera()
    {
        return;
    }

    //----------------------------------------------------------------------------------------------------------------------------------
    /*!
        \detail This function is called by every subroutine of the threadActuator. It checks if the camera-handle and the semaphore handle is zero and if the semaphore has waited after last command.
                If the semaphore droppes or dropped to time-out it returns retError.

        \return retOk or retError
        \sa threadActuator
    */
    inline ito::RetVal threadCamera::securityChecks()
    {
        ito::RetVal retval(ito::retOk);

        if (!pMyCamera)
        {
            return ito::RetVal(ito::retError, 0, QObject::tr("Camera not correctly initialized").toAscii().data());
        }

        return retval;
    }

    //----------------------------------------------------------------------------------------------------------------------------------
    /*!
        \detail After the invoke-command this thread must wait / be synchronize with the camera-thread.
                Therefore the wait-Function of pMySemaphore is called. If the camera do not answer within timeOutMS and the pMyCamera is not alive anymore, the function returns a timeout.

        \param [in] timeOutMS    TimeOut for the semaphore-wait

        \return retOk or retError
        \sa threadActuator, threadCamera
    */
    ito::RetVal threadCamera::waitForSemaphore(int timeOutMS)
    {
        ito::RetVal retval(ito::retOk);

        if (!pMySemaphoreLocker.getSemaphore())
        {
            return ito::RetVal(ito::retError, 0, QObject::tr("Semaphore not correctly initialized").toAscii().data());
        }

        while (!pMySemaphoreLocker.getSemaphore()->wait(timeOutMS))
        {
            return ito::RetVal(ito::retError, 0, QObject::tr("Timeout while Waiting for Semaphore").toAscii().data());
        }

        return pMySemaphoreLocker.getSemaphore()->returnValue;
    }

    //----------------------------------------------------------------------------------------------------------------------------------
    /*!
        \detail Every capture procedure starts with the startDevice() to set the camera active and is ended with stopDevice().

        \param [in] timeOutMS    TimeOut for the semaphore-wait

        \return retOk or retError
        \sa threadActuator, threadCamera, threadCamera::stopDevice, threadCamera::acquire, threadCamera::getVal, threadCamera::copyVal
    */
    ito::RetVal threadCamera::startDevice(int timeOutMS)
    {
        ito::RetVal retval = securityChecks();
        if (retval.containsError())
        {
            return retval;
        }

        pMySemaphoreLocker = new ItomSharedSemaphore();
        QMetaObject::invokeMethod(pMyCamera, "startDevice", Q_ARG(ItomSharedSemaphore *, pMySemaphoreLocker.getSemaphore()));

        return waitForSemaphore(timeOutMS);
    }

    //----------------------------------------------------------------------------------------------------------------------------------
    /*!
        \detail Every capture procedure starts with the startDevice() to set the camera active and is ended with stopDevice().

        \param [in] timeOutMS    TimeOut for the semaphore-wait

        \return retOk or retError
        \sa threadActuator, threadCamera, threadCamera::startDevice, threadCamera::acquire, threadCamera::getVal, threadCamera::copyVal
    */
    ito::RetVal threadCamera::stopDevice(int timeOutMS)
    {
        ito::RetVal retval = securityChecks();
        if (retval.containsError())
        {
            return retval;
        }

        pMySemaphoreLocker = new ItomSharedSemaphore();
        QMetaObject::invokeMethod(pMyCamera, "stopDevice", Q_ARG(ItomSharedSemaphore *, pMySemaphoreLocker.getSemaphore()));

        retval += waitForSemaphore(timeOutMS);
        return retval;
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
    ito::RetVal threadCamera::acquire(const int trigger, int timeOutMS)
    {
        //double starttime = (double)(cv::getTickCount())/cv::getTickFrequency();

        ito::RetVal retval = securityChecks();
        if (retval.containsError())
        {
            return retval;
        }

        //double middletime1 = (double)(cv::getTickCount())/cv::getTickFrequency();
        
        pMySemaphoreLocker = new ItomSharedSemaphore();
        QMetaObject::invokeMethod(pMyCamera, "acquire", Q_ARG(const int, trigger), Q_ARG(ItomSharedSemaphore *, pMySemaphoreLocker.getSemaphore()));

        //double middletime2 = (double)(cv::getTickCount())/cv::getTickFrequency();

        retval += waitForSemaphore(timeOutMS);
        
        //double endtime = (double)(cv::getTickCount())/cv::getTickFrequency();
        //std::cout << "\nAcquire: " << endtime-starttime << "  Checking: " << middletime1 - starttime << "  Invoke: " << middletime2-middletime1 << "  Wait: "<< endtime-middletime2 << "\n";

        return retval;
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
    ito::RetVal threadCamera::getVal(ito::DataObject &dObj, int timeOutMS)   /*! < Get a shallow-copy of the dataObject */
    {
        ito::RetVal retval = securityChecks();
        if (retval.containsError())
        {
            return retval;
        }

        pMySemaphoreLocker = new ItomSharedSemaphore();
        QMetaObject::invokeMethod(pMyCamera, "getVal", Q_ARG(void*, (void *)&dObj), Q_ARG(ItomSharedSemaphore *, pMySemaphoreLocker.getSemaphore()));

        retval += waitForSemaphore(timeOutMS);
        return retval;
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
    ito::RetVal threadCamera::copyVal(ito::DataObject &dObj, int timeOutMS)  /*! < Get a deep-copy of the dataObject */
    {
        //double starttime = (double)(cv::getTickCount())/cv::getTickFrequency();

        ito::RetVal retval = securityChecks();
        if (retval.containsError())
        {
            return retval;
        }

        //double middletime1 = (double)(cv::getTickCount())/cv::getTickFrequency();

        pMySemaphoreLocker = new ItomSharedSemaphore();

        QMetaObject::invokeMethod(pMyCamera, "copyVal", Q_ARG(void*, (void *)&dObj), Q_ARG(ItomSharedSemaphore *, pMySemaphoreLocker.getSemaphore()));

        //double middletime2 = (double)(cv::getTickCount())/cv::getTickFrequency();

        retval += waitForSemaphore(timeOutMS);

        //double endtime = (double)(cv::getTickCount())/cv::getTickFrequency();
        //std::cout << "\nCopyVal: " << endtime-starttime << "  Checking: " << middletime1 - starttime << "  Invoke: " << middletime2-middletime1 << "  Wait: "<< endtime-middletime2 << "\n";

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
    ito::RetVal threadCamera::getParam(ito::Param &val, int timeOutMS)
    {
        ito::RetVal retval = securityChecks();
        if (retval.containsError())
        {
            return retval;
        }

        pMySemaphoreLocker = new ItomSharedSemaphore();

        QSharedPointer<ito::Param> qsParam(new ito::Param(val));
        QMetaObject::invokeMethod(pMyCamera, "getParam", Q_ARG(QSharedPointer<ito::Param>, qsParam), Q_ARG(ItomSharedSemaphore *, pMySemaphoreLocker.getSemaphore()));

        retval += waitForSemaphore(timeOutMS);

        val = *qsParam;
        return retval;
    }

    //----------------------------------------------------------------------------------------------------------------------------------
    /*!
        \detail Get the parameter of the camera defined by val.name to the value of val.

        \param [in] val         Initialised tParam (correct name | value)
        \param [in] timeOutMS   TimeOut for the semaphore-wait

        \return retOk or retError
        \sa threadActuator, threadCamera
    */
    ito::RetVal threadCamera::setParam(ito::ParamBase val, int timeOutMS)
    {
        ito::RetVal retval = securityChecks();
        if (retval.containsError())
        {
            return retval;
        }
        QSharedPointer<ito::ParamBase> qsParam(new ito::ParamBase(val));

        pMySemaphoreLocker = new ItomSharedSemaphore();
        QMetaObject::invokeMethod(pMyCamera, "setParam", Q_ARG(QSharedPointer<ito::ParamBase>, qsParam), Q_ARG(ItomSharedSemaphore *, pMySemaphoreLocker.getSemaphore()));
        retval += waitForSemaphore(timeOutMS);

        return retval;
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

    ito::RetVal threadCamera::getImageParams(int &bpp, int &xsize, int &ysize, int timeOutMS)
    {
        ito::RetVal retval = securityChecks();
        QSharedPointer<ito::Param> qsParamBPP(new ito::Param("bpp", ito::ParamBase::Int));

        pMySemaphoreLocker = new ItomSharedSemaphore();
        QMetaObject::invokeMethod(pMyCamera, "getParam", Q_ARG(QSharedPointer<ito::Param>, qsParamBPP), Q_ARG(ItomSharedSemaphore *, pMySemaphoreLocker.getSemaphore()));

        while (!pMySemaphoreLocker.getSemaphore()->wait(timeOutMS))
        {
            retval += ito::RetVal(ito::retError, 0, QObject::tr("timeout while getting numaxis parameter").toAscii().data());
            return retval;
        }

        retval += pMySemaphoreLocker.getSemaphore()->returnValue;

        if (retval.containsWarningOrError())
        {
            return retval;
        }

        bpp = (*qsParamBPP).getVal<int>();

        pMySemaphoreLocker = new ItomSharedSemaphore();

        QSharedPointer<ito::Param> qsParamSizeX(new ito::Param("sizex", ito::ParamBase::Int));
        QMetaObject::invokeMethod(pMyCamera, "getParam", Q_ARG(QSharedPointer<ito::Param>, qsParamSizeX), Q_ARG(ItomSharedSemaphore *, pMySemaphoreLocker.getSemaphore()));

        while (!pMySemaphoreLocker.getSemaphore()->wait(timeOutMS))
        {
            retval += ito::RetVal(ito::retError, 0, QObject::tr("timeout while getting numaxis parameter").toAscii().data());
            return retval;
        }

        retval += pMySemaphoreLocker.getSemaphore()->returnValue;

        if (retval.containsWarningOrError())
        {
            return retval;
        }

        xsize = (*qsParamSizeX).getVal<int>();

        pMySemaphoreLocker = new ItomSharedSemaphore();

        QSharedPointer<ito::Param> qsParamSizeY(new ito::Param("sizey", ito::ParamBase::Int));
        QMetaObject::invokeMethod(pMyCamera, "getParam", Q_ARG(QSharedPointer<ito::Param>, qsParamSizeY), Q_ARG(ItomSharedSemaphore *, pMySemaphoreLocker.getSemaphore()));

        while (!pMySemaphoreLocker.getSemaphore()->wait(timeOutMS))
        {
            retval += ito::RetVal(ito::retError, 0, QObject::tr("timeout while getting numaxis parameter").toAscii().data());
            break;
        }

        retval += pMySemaphoreLocker.getSemaphore()->returnValue;

        if (retval.containsWarningOrError())
        {
            return retval;
        }

        ysize = (*qsParamSizeY).getVal<int>();

        return retval;
    }
}
