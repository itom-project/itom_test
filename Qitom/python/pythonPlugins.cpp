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

#include "pythonPlugins.h"
#include "pythonDataObject.h"
#include "pythonQtConversion.h"
#include "pythonCommon.h"

#if (defined linux) | (defined CMAKE)
    #include "structmember.h"
#else
    #include "structmember.h"   //python structmember
#endif
#include "../organizer/addInManager.h"
#include "../AppManagement.h"
#include <qlist.h>
#include <qmap.h>
#include <qobject.h>

#include <qsharedpointer.h>
#include "../helper/sharedPointerHelper.h"
#include "../helper/paramHelper.h"
#include "../../common/helperCommon.h"

#include "pythontParamConversion.h"
#include "pythonSharedPointerGuard.h"
#include <qdockwidget.h>
#include <qaction.h>

using namespace ito;

//#include "./memoryCheck/setDebugNew.h"
//#include "./memoryCheck/reportingHook.h"

namespace ito
{

//----------------------------------------------------------------------------------------------------------------------------------
bool SetLoadPluginReturnValueMessage(ito::RetVal &retval, QString &pluginName)
{
    if (retval.containsError())
    {
        if (retval.errorMessage())
        {
            PyErr_Format(PyExc_RuntimeError, "Could not load plugin: %s with error message: \n%s", pluginName.toAscii().data(), retval.errorMessage());
        }
        else
        {
            PyErr_Format(PyExc_RuntimeError, "Could not load plugin: %s with unspecified error.", pluginName.toAscii().data());
        }
        return false;
    }

    if (retval.containsWarning())
    {
        std::cerr << "Warning while loading plugin: " << pluginName.toAscii().data() << "\n" << std::endl;

        if (retval.errorMessage() != NULL)
        {
            std::cerr << " Message: " << retval.errorMessage() << "\n" << std::endl;
        }
        else
        {
            std::cerr << " Message: No warning message indicated.\n" << std::endl;
        }
    }
    return true;
}

//----------------------------------------------------------------------------------------------------------------------------------
bool SetLoadPluginReturnValueMessage(ito::RetVal &retval, const char *pluginName)
{
    QString pName(pluginName);
    return SetLoadPluginReturnValueMessage(retval, pName);
}

//----------------------------------------------------------------------------------------------------------------------------------
bool SetReturnValueMessage(ito::RetVal &retval, QString &functionName)
{
    if (retval.containsError())
    {
        QByteArray name = functionName.toAscii();
        char* msg = retval.errorMessage();
        if (msg)
        {
            PyErr_Format(PyExc_RuntimeError, "Error invoking function %s with error message: \n%s", name.data(), msg);
        }
        else
        {
            PyErr_Format(PyExc_RuntimeError, "Error invoking function %s.", name.data());
        }
        return false;
    }

    if (retval.containsWarning())
    {
        std::cerr << "Warning invoking " << functionName.toAscii().data() << "\n" << std::endl;
        if (retval.errorMessage() != NULL)
        {
            std::cerr << " Message: " << retval.errorMessage() << "\n" << std::endl;
        }
        else
        {
            std::cerr << " Message: No warning message indicated.\n" << std::endl;
        }
    }
    return true;
}

//----------------------------------------------------------------------------------------------------------------------------------
bool SetReturnValueMessage(ito::RetVal &retval, const char *functionName)
{
    QString fName(functionName);
    return SetReturnValueMessage(retval, fName);
}

//----------------------------------------------------------------------------------------------------------------------------------
/** Helper function that accepts a python parameter list and returns pointers to the parameters' values and a list with their types
*   @param [in]   args      list with python parameters
*   @param [in]   length    number of parameters passed
*   @param [out]  cargs     pointers to the parsed parameters
*   @param [out]  cargt     list with the parameter's types
*   @return                 return 0 if all parameters passed could be parsed to a known type, -1 otherwise
*
*   The function accepts a list of python parameters and tries to parse them to make them available to c/c++ functions. The found
*   parameters pointers are given back in the cargs array ant the parameters' types in the cargt array. To free the generated lists
*   use the \ref freeParams function.
*/
int parseParams(PyObject *args, int length, char **&cargs, char *&cargt)
{
   PyObject *tempPyObj = NULL;
   cargs = (char **)calloc(length, sizeof(char*));
   cargt = (char *)calloc(length, sizeof(char));

   tempPyObj = PyTuple_GetItem(args, 0);

   for (int n = 0; n < length; n++)
   {
      tempPyObj = PyTuple_GetItem(args, n);
      if (PyLong_CheckExact(tempPyObj))
      {
         cargs[n] = (char*)malloc(sizeof(long));
         (*(long*)cargs[n]) = PyLong_AsLong(tempPyObj);
         cargt[n] = 'l';
      }
      else if (PyFloat_CheckExact(tempPyObj))
      {
         cargs[n] = (char*)malloc(sizeof(double));
         (*(double*)cargs[n]) = PyFloat_AsDouble(tempPyObj);
         cargt[n] = 'f';
      }
      else if (PyComplex_CheckExact(tempPyObj))
      {
         cargs[n] = (char*)malloc(sizeof(Py_complex));
         (*(Py_complex*)cargs[n]) = PyComplex_AsCComplex(tempPyObj);
         cargt[n] = 'c';
      }
      else if (PyUnicode_Check(tempPyObj))
      {
         bool ok = false;
         QByteArray ba = PythonQtConversion::PyObjGetBytes(tempPyObj,false,ok);
         cargs[n] = ba.data();
         cargt[n] = 's';
      }
      else if (Py_TYPE(tempPyObj) == &PythonPlugins::PyDataIOPluginType)
      {
         cargt[n] = 'o';
         cargs[n] = (char*)(((PythonPlugins::PyDataIOPlugin *)tempPyObj)->dataIOObj);
      }
      else if (Py_TYPE(tempPyObj) == &PythonPlugins::PyActuatorPluginType)
      {
         cargt[n] = 'o';
         cargs[n] = (char*)(((PythonPlugins::PyActuatorPlugin *)tempPyObj)->actuatorObj);
      }
#if 0 //algo plugins do not exist as instances, they only contain static methods, callable by itom.filter
      else if (Py_TYPE(tempPyObj) == &PythonPlugins::PyAlgoPluginType)
      {
         cargt[n] = 'o';
         cargs[n] = (char*)(((PythonPlugins::PyAlgoPlugin *)tempPyObj)->algoObj);
      }
#endif
      else
      {
          PyErr_Format(PyExc_TypeError, "type of parameter %i cannot be parsed: %s", n+1, tempPyObj->ob_type->tp_name);
          return -1;
      }
   }

   return 0;
}

//----------------------------------------------------------------------------------------------------------------------------------
/** frees parameter and parameter type arrays generated by the \ref parseParams function
*   @param [in] length  number of parameters
*   @param [in] cargt   array with parameter types
*   @param [in] cargs   array with the parameter pointers / values
*   @return             0
*
*   The function frees the arrays generated by the \ref parseParams function, i.e. the array with the parsed parameter values and the
*   array with their types.
*/
int freeParams(int length, char *&cargt, char **&cargs)
{
   for (int n = 0; n < length; n++)
   {
      if ((cargt[n] == 'l') || (cargt[n] == 'f') || (cargt[n] == 'c'))
      {
         free(cargs[n]);
      }
   }

   free(cargs);
   free(cargt);

   return 0;
}

//----------------------------------------------------------------------------------------------------------------------------------
/** returns the names of the parameters available in a plugin
*   @param [in] aib the plugin for which the parameter names are requested
*   @return     python object with a string list with the parameters' names
*/
PyObject * getParamList(ito::AddInBase *aib)
{
   PyObject *result = NULL;
   QMap<QString, ito::Param> *paramList = NULL;

   aib->getParamList(&paramList);

   if (paramList)
   {
      result = PyList_New(0);
      QMap<QString, ito::Param>::const_iterator paramIt;

      for (paramIt = paramList->constBegin(); paramIt != paramList->constEnd(); paramIt++)
      {
          PyList_Append(result, PyUnicode_FromString(paramIt.value().getName()));
      }
   }

   return result;
}


//----------------------------------------------------------------------------------------------------------------------------------
/** returns the all informations of the parameters available in a plugin
*   @param [in] aib     the plugin for which the parameter names are requested
*   @param [in] args    1 Item-Vector with bool request for additional dictionary return
*   @return     python list of python tuple with the parameters' names, min, max, current value, (infostring)
*/
PyObject * getParamListInfo(ito::AddInBase *aib, PyObject *args)
{
   PyObject *result = NULL;
   int length = PyTuple_Size(args);
   int output = 0;

   QMap<QString, ito::Param> *paramList = NULL;

   aib->getParamList(&paramList);

    if (length == 1)
    {
        if (!PyArg_ParseTuple(args, "i", &output))
        {
            PyErr_Format(PyExc_ValueError, "wrong input parameter");
            return NULL;
        }
    }
    else if (length > 1)
    {
        PyErr_Format(PyExc_ValueError, "wrong number of input arguments");
        return NULL;
    }

   if (paramList)
   {
      std::cout << "Plugin parameters are:\n";

      QVector<ito::Param> parameter = paramList->values().toVector();
      result = PrntOutParams(&parameter, false, true, -1);
   }
   else
   {
       result = PyDict_New();
       std::cout << " \nPlugin does not accept parameters! \n";
   }

   //std::cout << "\n";

    if ((length == 0) || (output==0))
    {
        Py_DECREF(result);
        Py_RETURN_NONE;
    }
    else
        return result;

}

//----------------------------------------------------------------------------------------------------------------------------------
PyObject* plugin_showConfiguration(ito::AddInBase *aib)
{
    if (aib)
    {
        if (aib->hasConfDialog())
        {
            QMetaObject::invokeMethod(ito::AddInManager::getInstance(), "showConfigDialog", Q_ARG(ito::AddInBase *, aib));
        }
        else
        {
            return PyErr_Format(PyExc_RuntimeError, "actuator has no configuration dialog");
        }
    }

    Py_RETURN_NONE;
}

//----------------------------------------------------------------------------------------------------------------------------------


/** returns the list of available parameters
*   @param [in] self    the actuator object (python)
*
*   This method simply open the widget
*/
PyObject* plugin_showToolbox(ito::AddInBase *aib)
{
    ItomSharedSemaphoreLocker locker(new ItomSharedSemaphore());
    ito::RetVal retval;

    if (aib)
    {
        QMetaObject::invokeMethod(ito::AddInManager::getInstance(), "showDockWidget", Q_ARG(ito::AddInBase *, aib), Q_ARG(int,1), Q_ARG(ItomSharedSemaphore*,locker.getSemaphore()));

        if (!locker.getSemaphore()->wait(5000))
        {
            retval += ito::RetVal(ito::retError,0,"timeout while showing toolbox");
        }
        else
        {
            retval += locker.getSemaphore()->returnValue;
        }
    }

    if (!SetReturnValueMessage(retval, "showToolbox"))
    {
        return NULL;
    }

    Py_RETURN_NONE;
}

//----------------------------------------------------------------------------------------------------------------------------------
/** returns the list of available parameters
*   @param [in] self    the actuator object (python)
*
*   This method simply close the widget
*/
PyObject* plugin_hideToolbox(ito::AddInBase *aib)
{
    ItomSharedSemaphoreLocker locker(new ItomSharedSemaphore());
    ito::RetVal retval;

    if (aib)
    {
        QMetaObject::invokeMethod(ito::AddInManager::getInstance(), "showDockWidget", Q_ARG(ito::AddInBase *, aib), Q_ARG(int,0), Q_ARG(ItomSharedSemaphore*,locker.getSemaphore()));

        if (!locker.getSemaphore()->wait(5000))
        {
            retval += ito::RetVal(ito::retError, 0, "timeout while showing toolbox");
        }
        else
        {
            retval += locker.getSemaphore()->returnValue;
        }
    }

    if (!SetReturnValueMessage(retval, "hideToolbox"))
    {
        return NULL;
    }

    Py_RETURN_NONE;
}


//----------------------------------------------------------------------------------------------------------------------------------
/** returns a list of execFunction available in a plugin similar to filterHelp 
*   @param [in] aib     the plugin for which the execFuncs names are requested
*   @param [in] args    2 Item-Vector with integer request for additional dictionary return
*   @return             python dictionary with list of functions or specific dictionary for one execFunc with the parameters' names, min, max, current value, (infostring)
*/
PyObject * getExecFuncsInfo(ito::AddInBase *aib, PyObject *args, PyObject *kwds)
{
    const char *kwlist[] = {"funcName", "detailLevel", NULL};
    char* funcName = NULL;
    int detailLevel = 0;

    PyObject *result = NULL;
    QString funcNameString("");

    if (!PyArg_ParseTupleAndKeywords(args, kwds, "|si", const_cast<char**>(kwlist), &funcName, &detailLevel))
    {
        return NULL;
    }

    QMap<QString, ExecFuncParams> *funcList = NULL;

    aib->getExecFuncList(&funcList);
    result = PyDict_New();

    if (funcList && funcList->size() > 0)
    {
        if (funcName != NULL)
        {
            funcNameString = QString(funcName);
        }

        QStringList execFuncs = funcList->keys();
        PyObject *execFuncslist = NULL;

        if (execFuncs.size() > 0)
        {
            if (!funcNameString.isEmpty() && execFuncs.contains(funcNameString))    // got an exect match
            {
                (*funcList)[funcNameString].infoString;
                (*funcList)[funcNameString].paramsMand;
                (*funcList)[funcNameString].paramsOpt;

                std::cout << "\nParameters for the execFunction '"<< funcNameString.toAscii().data() <<"' are :\n";
                QVector<ito::Param> parameter = (*funcList)[funcNameString].paramsMand;
                if (parameter.size())
                {
                    std::cout << "\nMandatory parameters:\n";
                    execFuncslist = PrntOutParams(&parameter, false, true, -1);
                    PyDict_SetItemString(result, "Mandatory Parameters", execFuncslist);
                    Py_DECREF(execFuncslist);
                }
                else
                {
                    std::cout << "\nMandatory parameters: Filter function has no mandatory parameters. \n";
                }

                parameter = (*funcList)[funcNameString].paramsOpt;
                if (parameter.size())
                {
                    std::cout << "\nOptional parameters:\n";
                    execFuncslist = PrntOutParams(&parameter, false, true, -1);
                    PyDict_SetItemString(result, "Optional Parameters", execFuncslist);
                    Py_DECREF(execFuncslist);
                }
                else
                {
                    std::cout << "\nOptional parameters: Filter function has no optional parameters. \n";
                }
                std::cout << "\n";

                parameter = (*funcList)[funcNameString].paramsOut;
                if (parameter.size())
                {
                    std::cout << "\nOutput values:\n";
                    execFuncslist = PrntOutParams(&parameter, false, true, -1);
                    PyDict_SetItemString(result, "Output Parameters", execFuncslist);
                    Py_DECREF(execFuncslist);
                }
                else
                {
                    std::cout << "\nOutput values: Filter has no output-parameters defined. \n";
                }
                std::cout << "\n";
            
            }
            else
            {
                execFuncs.sort();
                
                std::cout << "\nPlugin execFunctions are:\n\n";

                QList<QPair<QString, QString> > outPut;
                outPut.clear();
                int longname = 0;

                for (int funcs = 0; funcs < execFuncs.size(); funcs++)
                {  
                    if (longname < execFuncs.value(funcs).length())
                        longname = execFuncs.value(funcs).length();

                    outPut.append(QPair<QString, QString>(execFuncs.value(funcs), (*funcList)[execFuncs.value(funcs)].infoString));

                    PyObject *text = PythonQtConversion::QByteArrayToPyUnicodeSecure((*funcList)[execFuncs.value(funcs)].infoString.toAscii());
                    PyDict_SetItemString(result, execFuncs.value(funcs).toAscii().data() , text);
                    Py_DECREF(text);
                    text = NULL;
                }
                longname+= 3;
                std::cout << "No " << QString("Name").leftJustified(longname, ' ', false).toAscii().data() << "   \tInfostring\n"; 
                for (int funcs = 0; funcs < outPut.size(); funcs++)
                {
                    std::cout << funcs << "  " << outPut.value(funcs).first.leftJustified(longname, ' ', false).toAscii().data() << "  \t'" << outPut.value(funcs).second.toAscii().data() << "'\n"; 
                }    
            }
        }
        else
        {
            std::cout << " \nPlugin has no execFunctions! \n";        
        }

    }
    else
    {
        std::cout << " \nPlugin has no execFunctions! \n";
    }

    std::cout << "\n";

    if ((funcName == NULL) || (detailLevel < 1))
    {
        Py_DECREF(result);
        Py_RETURN_NONE;
    }
    else
        return result;

}

//----------------------------------------------------------------------------------------------------------------------------------
/** returns the name of a python plugin
*   @param [in] addInObj    the plugin whoes name should be returned
*   @return     the plugin name
*/
template<typename _Tp> PyObject* getName(_Tp *addInObj)
{
    ito::RetVal ret = ito::retOk;

    ItomSharedSemaphoreLocker locker(new ItomSharedSemaphore());
    QSharedPointer<ito::Param> qsParam(new ito::Param("name", ito::ParamBase::String));
    QMetaObject::invokeMethod(addInObj, "getParam", Q_ARG(QSharedPointer<ito::Param>, qsParam), Q_ARG(ItomSharedSemaphore *, locker.getSemaphore()));

    while (!locker.getSemaphore()->wait(AppManagement::timeouts.pluginGeneral))
    {
        if (!addInObj->isAlive())
        {
            ret += ito::RetVal(ito::retError, 0, QObject::tr("timeout while getting name parameter").toAscii().data());
            break;
        }
    }

    ret += locker.getSemaphore()->returnValue;

    if (!SetReturnValueMessage(ret, "getName"))
    {
        return NULL;
    }

    return PyUnicode_FromString((*qsParam).getVal<char*>());
}

//----------------------------------------------------------------------------------------------------------------------------------

PyObject* execFunc(ito::AddInBase *aib, PyObject *args, PyObject *kwds)
{
    ito::RetVal ret = ito::retOk;
    QMap<QString, ExecFuncParams> *funcList;
    QSharedPointer<QVector<ito::ParamBase> > paramsMand(new QVector<ito::ParamBase>());
    QSharedPointer<QVector<ito::ParamBase> > paramsOpt(new QVector<ito::ParamBase>());
    QSharedPointer<QVector<ito::ParamBase> > paramsOut(new QVector<ito::ParamBase>());
    QString name;
    int argsLength = PyTuple_Size(args);
    PyObject *pyObj = NULL;
    bool ok;

    if (argsLength < 1)
    {
        ret += ito::RetVal(ito::retError,0,QObject::tr("you must provide at least one parameter with the name of the function").toAscii().data());
    }
    else
    {
        pyObj = PyTuple_GET_ITEM(args,0); //borrowed
        name = PythonQtConversion::PyObjGetString(pyObj,true,ok);
        if (!ok)
        {
            ret += ito::RetVal(ito::retError,0,QObject::tr("the first function name parameter can not be interpreted as string").toAscii().data());
        }
    }
    
    if (!ret.containsError())
    {
        ret += aib->getExecFuncList(&funcList);
        QMap<QString, ExecFuncParams>::const_iterator it = funcList->constFind(name);

        if (it == funcList->constEnd())
        {
            ret += ito::RetVal::format(ito::retError,0,QObject::tr("plugin does not provide an execution of function '%s'").toAscii().data(),name.toAscii().data());
        }
        else
        {
            //split first argument from args
            pyObj = PyTuple_GetSlice(args, 1, argsLength); //new ref

            //parses python-parameters with respect to the default values given py (*it).paramsMand and (*it).paramsOpt and returns default-initialized ParamBase-Vectors paramsMand and paramsOpt.
            ret += parseInitParams(&(*it).paramsMand, &(*it).paramsOpt, pyObj, kwds, *paramsMand, *paramsOpt);

            //makes deep copy from default-output parameters (*it).paramsOut and returns it in paramsOut (ParamBase-Vector)
            ret += copyParamVector(&(*it).paramsOut, *paramsOut);

            Py_XDECREF(pyObj);

            if (!ret.containsError())
            {
                ItomSharedSemaphoreLocker locker(new ItomSharedSemaphore());
                QMetaObject::invokeMethod(aib, "execFunc", Q_ARG(QString, name), Q_ARG(QSharedPointer<QVector<ito::ParamBase> >, paramsMand), Q_ARG(QSharedPointer<QVector<ito::ParamBase> >, paramsOpt), Q_ARG(QSharedPointer<QVector<ito::ParamBase> >, paramsOut), Q_ARG(ItomSharedSemaphore *, locker.getSemaphore()));

                while (!locker.getSemaphore()->wait(AppManagement::timeouts.pluginGeneral))
                {
                    if (!aib->isAlive())
                    {
                        ret += ito::RetVal(ito::retError, 0, QObject::tr("timeout while calling specific function in plugin.").toAscii().data());
                        break;
                    }
                }

                ret += locker.getSemaphore()->returnValue;
            }

        }
    }

    if (!SetReturnValueMessage(ret, "exec"))
    {
        return NULL;
    }
    else
    {
        if (paramsOut->size() == 0)
        {
            Py_RETURN_NONE;
        }
        else if (paramsOut->size() == 1)
        {
            PyObject* out = PythonParamConversion::ParamBaseToPyObject((*paramsOut)[0]); //new ref
            if (!SetReturnValueMessage(ret, "exec"))
            {
                return NULL;
            }
            else
            {
                return out;
            }
        }
        else
        {
            //parse output vector to PyObject-Tuple
            PyObject* out = PyTuple_New(paramsOut->size());
            PyObject* temp;
            Py_ssize_t i = 0;

            foreach(const ito::ParamBase &p, *paramsOut)
            {
                temp = PythonParamConversion::ParamBaseToPyObject(p); //new ref
                if (temp)
                {
                    PyTuple_SetItem(out,i,temp); //steals ref
                    i++;
                }
                else
                {
                    break;
                }
            }

            if (!SetReturnValueMessage(ret, "exec"))
            {
                Py_DECREF(out);
                return NULL;
            }
            else
            {
                return out;
            }
        }
    }

    
}

//----------------------------------------------------------------------------------------------------------------------------------
/** return a parameter value
*   @param [in] addInObj    the addIn whoes parameter is requested
*   @param [in] args        the parameter name
*   @return     python object with the parameter value on success (parameter exists), NULL otherwise
*
*   The function tries to retrieve the value of the parameter with the name given in args. If the parameter does not exist
*   NULL is returned. To actually retrieve the value the getParam function of the plugin is invoked.
*/
template<typename _Tp> PyObject* getParam(_Tp *addInObj, PyObject *args)
{
    PyObject *result = NULL;
    const char *paramName = NULL;
    //bool paramNameCheck;

    ItomSharedSemaphoreLocker locker(new ItomSharedSemaphore());

    ito::RetVal ret = ito::retOk;

    if (!PyArg_ParseTuple(args, "s", &paramName))
    {
        PyErr_Format(PyExc_ValueError, "no parameter name specified");
        return NULL;
    }

    //check parameter name and split it into its components
    bool hasIndex;
    QString nameOnly;
    int index;
    QString additionalTag;
    if(ito::ParamHelper::parseParamName(paramName, nameOnly, hasIndex, index, additionalTag).containsError())
    {
        PyErr_Format(PyExc_TypeError, "parameter name is invalid. It must have the following format: paramName['['index']'][:additionalTag]");
        return NULL;
    }

    //now get pointer to the parameter-map from plugin and check whether paramName is available
    QMap<QString, Param> *params;
    QMap<QString, Param>::iterator it;
    ((ito::AddInBase*)addInObj)->getParamList(&params); //always returns ok

    //find parameter in params
    it = params->find(nameOnly);
    if (it == params->end())
    {
        PyErr_Format(PyExc_ValueError, "Parameter '%s' not contained in plugin.", nameOnly.toAscii().data());
        return NULL;
    }

    QSharedPointer<ito::Param> qsParam(new ito::Param(paramName)); //here it is sufficient to provide an empty param container with name only, the content will be filled by the plugin (including type)
    
    QMetaObject::invokeMethod(addInObj, "getParam", Q_ARG(QSharedPointer<ito::Param>, qsParam), Q_ARG(ItomSharedSemaphore *, locker.getSemaphore()));
    while (!locker.getSemaphore()->wait(AppManagement::timeouts.pluginGeneral))
    {
        if (!addInObj->isAlive())
        {
            ret += ito::RetVal(ito::retError, 0, QObject::tr("timeout while getting parameter").toAscii().data());
            break;
        }
    }

    ret += locker.getSemaphore()->returnValue;

    result = ito::PythonParamConversion::ParamBaseToPyObject(*qsParam);

    if (!SetReturnValueMessage(ret, "getParam"))
    {
        return NULL;
    }

    return result;
}


//general docstrings BEGIN
PyDoc_STRVAR(pyPluginName_doc, "name() -> returns the plugin name\n\
\n\
Returns \n\
-------- \n\
name : {str} \n\
    name of the plugin, which corresponds to `getParam('name')` \n\
\n\
See Also \n\
--------- \n\
getParam");

PyDoc_STRVAR(pyPluginGetParamList_doc, "getParamList() -> returns a list of the names of the internal parameters of the plugin\n\
\n\
Each plugin defines a set of parameters, where each parameter has got a name and maps to any value. The value is represented by \n\
the C++ class ito::ParamBase and can have one of the following types: \n\
\n\
* String \n\
* Char \n\
* Integer \n\
* Double \n\
* CharArray \n\
* IntegerArray \n\
* DoubleArray \n\
* DataObject \n\
* PolygonMesh \n\
* PointCloud \n\
* Another plugin instance \n\
\n\
Using one of the parameter names, its current value can be obtained by `getParam('name')` and \n\
is writable by `setParam('name', newValue)` (if not read-only) \n\
\n\
Returns \n\
------- \n\
out : {list} \n\
    list of parameter names \n\
\n\
See Also \n\
--------- \n\
getParam, setParam, getParamListInfo");

PyDoc_STRVAR(pyPluginGetParamListInfo_doc, "getParamListInfo([detailLevel]) -> prints detailed information about all plugin parameters. \n\
\n\
Each plugin defines a set of parameters, where each parameter has got a name and maps to any value. The value is represented by \n\
the C++ class ito::ParamBase and can have one of the following types: \n\
\n\
* String \n\
* Char \n\
* Integer \n\
* Double \n\
* CharArray \n\
* IntegerArray \n\
* DoubleArray \n\
* DataObject \n\
* PolygonMesh \n\
* PointCloud \n\
* Another plugin instance \n\
\n\
Using one of the parameter names, its current value can be obtained by `getParam('name')` and \n\
is writable by `setParam('name', newValue)` (if not read-only) \n\
\n\
This method prints a detailed table with the name, current value, description string and further meta information of every plugin parameter. \n\
\n\
Parameters \n\
----------- \n\
detailLevel : {dict}, optional \n\
    if `detailLevel == 1`, function returns a dictionary with parameters, else None is returned [default]\n\
\n\
Returns \n\
------- \n\
out : {None, dict} \n\
    If `detailLevel == 1`, a dictionary containing all printed information is returned \n\
\n\
See Also \n\
---------- \n\
getParam, setParam, getParamList");

PyDoc_STRVAR(pyPluginGetParam_doc, "getParam(name) -> current value of the plugin parameter 'name'.\n\
\n\
Returns the current value of the internal plugin parameter with 'name'. The type of the returned value depends on the \n\
real type of the internal plugin, which may be: \n\
\n\
* String -> str \n\
* Char, Integer -> int \n\
* Double -> float \n\
* CharArray, IntegerArray -> tuple of int \n\
* DoubleArray -> tuple of float \n\
* DataObject -> dataObject \n\
* PolygonMesh -> polygonMesh \n\
* PointCloud -> pointCloud \n\
* Another plugin instance -> dataIO or actuator \n\
\n\
The name of the parameter must have the following form: \n\
\n\
* name \n\
* name:additionalTag (additionalTag can be a special feature of some plugins) \n\
* name[index] (only possible if parameter is an array type and you only want to get one single value, specified by the integer index [0,nrOfArrayItems-1]) \n\
* name[index]:additionalTag \n\
\n\
Parameters \n\
----------- \n\
name : {str}\n\
    name of the requested parameter\n\
\n\
Returns \n\
------- \n\
out : {variant}\n\
    value of the parameter \n\
\n\
Raises \n\
------- \n\
ValueError \n\
    if parameter does not exist \n\
\n\
See Also \n\
--------- \n\
setParam, getParamList, getParamListInfo");

PyDoc_STRVAR(pyPluginSetParam_doc, "setParam(name, value) -> sets parameter 'name' to the given value.\n\
\n\
Sets the internal plugin parameter with 'name' to a new value. The plugin itsself can decide whether the given value is \n\
accepted as new value. This may depend on the type of the given value, but also on the allowed value range indicated by \n\
further meta information of the internal parameter. Parameters that have the read-only flag set can not be reset. \n\
\n\
The name of the parameter must have the following form: \n\
\n\
* name \n\
* name:additionalTag (additionalTag can be a special feature of some plugins) \n\
* name[index] (only possible if parameter is an array type and you only want to get one single value, specified by the integer index [0,nrOfArrayItems-1]) \n\
* name[index]:additionalTag \n\
\n\
Parameters \n\
----------- \n\
name : {str}\n\
    name of the parameter\n\
value : {str, int, double, ...}\n\
    value that will be set. Only the name and existance of the parameter is checked before passing the request to the plugin. \n\
    The plugin itsself is responsible for further validations (including read-only attribute). \n\
\n\
See Also \n\
--------- \n\
getParam, getParamList, getParamListInfo");

PyDoc_STRVAR(pyPluginShowToolbox_doc, "showToolbox() -> open toolbox of the plugin \n\
\n\
Raises \n\
------ \n\
RuntimeError \n\
    if plugin does not provide a toolbox \n\
\n\
See Also \n\
--------- \n\
hideToolbox");

PyDoc_STRVAR(pyPluginHideToolbox_doc, "hideToolbox() -> hides toolbox of the plugin \n\
\n\
Raises \n\
------ \n\
RuntimeError \n\
    if plugin does not provide a toolbox \n\
\n\
See Also \n\
--------- \n\
showToolbox");

PyDoc_STRVAR(pyPluginShowConfiguration_doc, "showConfiguration() -> show configuration dialog of the plugin\n\
\n\
Raises \n\
------- \n\
RuntimeError \n\
    if plugin does not provide a configuration dialog");

PyDoc_STRVAR(pyPlugInGetExecFuncsInfo_doc, "getExecFuncsInfo([funcName [, detailLevel]]) -> plots a list of available execFuncs or a detailed description of the specified execFunc. \n\
\n\
Every plugin can define further functions, that are called by plugin.exec('funcName' [,param1, param2...]). This can for  \n\
instance be used in order to call specific calibration routines of cameras or actuators. This method allows printing \n\
information about available functions of this type. \n\
\n\
Parameters \n\
----------- \n\
funcName : {str}, optional \n\
    is the fullname or a part of any execFunc-name which should be displayed. \n\
    If funcName is none or no execFunc matches funcName casesensitiv a list with all suitable execFuncs is given. \n\
detailLevel : {dict}, optional \n\
    if `detailLevel == 1`, function returns a dictionary with parameters [default: 0]. \n\
\n\
Returns \n\
------- \n\
out : {None or dict}\n\
    depending on the value of *detailLevel*. \n\
\n\
See Also \n\
-------- \n\
exec");

PyDoc_STRVAR(PyPlugin_execFunc_doc, "exec(funcName [, param1, ...]) -> invoke the function 'funcName' registered as execFunc within the plugin.\n\
\n\
Every plugin can define further functions that can for instance be used in order to call specific calibration routines \n\
of cameras or actuators. This general method is used to call one of these specific functions registered under `funcName`. \n\
\n\
Parameters \n\
----------- \n\
funcName : {str} \n\
    The name of the function\n\
param1 : {variant}, optional \n\
    Further parameters depending on the requirements of the specific function.\n\
\n\
Returns \n\
------- \n\
out : {variant, list of variants}.\n\
    The return values depend on the function itself.\n\
\n\
See Also \n\
-------- \n\
execFuncsInfo");
//general docstrings END



//----------------------------------------------------------------------------------------------------------------------------------
/** set a parameter value
*   @param [in] addInObj    the addIn whoes parameter is requested
*   @param [in] args        the parameter name and value in a python object
*   @return     Py_Return_None on success, NULL otherwise
*
*   The function tries to set the value of the parameter with the name given in args. If the parameter does not exist
*   or is incompatible with the value passed, NULL is returned. To actually set the value the setParam function of the plugin is invoked.
*/
template<typename _Tp> PyObject* setParam(_Tp *addInObj, PyObject *args)
{
    const char *key = NULL;
    ItomSharedSemaphore *waitCond = NULL;
    ito::RetVal ret = ito::retOk;
    //ito::Param param;
    ito::AddInBase* aib = (ito::AddInBase*)addInObj;
    PyObject *value = NULL;

    QSharedPointer<ito::ParamBase> qsParam;

    if(!PyArg_ParseTuple(args, "sO", &key, &value))
    {
        PyErr_Format(PyExc_ValueError, "Parameter name and its value required.");
        return NULL;
    }

    //check parameter name and split it into its components
    bool hasIndex;
    QString paramName;
    int index;
    QString additionalTag;
    if(ito::ParamHelper::parseParamName(key, paramName, hasIndex, index, additionalTag).containsError())
    {
        PyErr_Format(PyExc_TypeError, "parameter name is invalid. It must have the following format: paramName['['index']'][:additionalTag]");
        return NULL;
    }

    //now get pointer to the parameter-map from plugin and check whether paramName is available
    QMap<QString, Param> *params;
    QMap<QString, Param>::iterator it;
    aib->getParamList(&params); //always returns ok

    //find parameter in params
    it = params->find(paramName);
    if (it == params->end())
    {
        PyErr_Format(PyExc_ValueError, "Parameter '%s' not contained in plugin.", paramName.toAscii().data());
        return NULL;
    }

    if(hasIndex)
    {
        switch(it->getType())
        {
        case ito::ParamBase::CharArray:
            qsParam = PythonParamConversion::PyObjectToParamBase(value, key, ret, ito::ParamBase::Char, false);
            break;
        case ito::ParamBase::IntArray:
            qsParam = PythonParamConversion::PyObjectToParamBase(value, key, ret, ito::ParamBase::Int, false);
            break;
        case ito::ParamBase::DoubleArray:
            qsParam = PythonParamConversion::PyObjectToParamBase(value, key, ret, ito::ParamBase::Double, false);
            break;
        default:
            PyErr_Format(PyExc_ValueError, "Parameter '%s' of plugin is no array.", paramName.toAscii().data());
            return NULL;
        }
    }
    else
    {
        qsParam = PythonParamConversion::PyObjectToParamBase(value, key, ret, it->getType(), false);
    }

    if(ret.containsError())
    {
        PyErr_Format(PyExc_ValueError, "The given value could not be transformed to type of parameter.", paramName.toAscii().data());
        return NULL;
    }
    else
    {
        bool timeout = false;
        waitCond = new ItomSharedSemaphore();
        QMetaObject::invokeMethod(addInObj, "setParam", Q_ARG(QSharedPointer<ito::ParamBase>, qsParam), Q_ARG(ItomSharedSemaphore *, waitCond));
        while (!waitCond->wait(AppManagement::timeouts.pluginGeneral))
        {
            if (!addInObj->isAlive())
            {
                ret += ito::RetVal(ito::retError, 0, QObject::tr("timeout.").toAscii().data());
                timeout = true;
                break;
            }
        }

        if (!timeout)
        {
            ret += waitCond->returnValue;
        }

         waitCond->deleteSemaphore();
         waitCond = NULL;
    }

    if (!SetReturnValueMessage(ret, "setParam"))
    {
        return NULL;
    }
    
    Py_RETURN_NONE;
}


//----------------------------------------------------------------------------------------------------------------------------------
/** desctructor for actuator object in python
*   @param [in] self
*
*   Destructs an actuator object (plugin), i.e. deletes the according python variable and invokes
*   the closeAddIn function. The object itself is only deleted if the object's reference
*   counter is zero.
*/
void PythonPlugins::PyActuatorPlugin_dealloc(PyActuatorPlugin* self)
{
    if (self->actuatorObj)
    {
        ito::AddInInterfaceBase *aib = self->actuatorObj->getBasePlugin();
        if (!aib)
        {
            std::cerr << "error closing plugin" << std::endl;
            //PyErr_Format(PyExc_RuntimeError, "error closing plugin");
        }
        else
        {
            ito::RetVal retval(ito::retOk);
            ito::AddInManager *aim = ito::AddInManager::getInstance();

            ItomSharedSemaphore *waitCond = new ItomSharedSemaphore();
            QMetaObject::invokeMethod(aim, "closeAddIn", Q_ARG(ito::AddInBase**, (ito::AddInBase**)&self->actuatorObj), Q_ARG(ItomSharedSemaphore*, waitCond));
//            retval = aim->closeAddIn((ito::AddInBase**)&self->actuatorObj);
            waitCond->wait(-1);
            retval += waitCond->returnValue;
             waitCond->deleteSemaphore();
             waitCond = NULL;
            
            PythonCommon::transformRetValToPyException(retval);
        }
    }

    Py_TYPE(self)->tp_free((PyObject*)self);
}

//----------------------------------------------------------------------------------------------------------------------------------
/** constructor for actuator object in python
*   @param [in] type
*   @return     new python actuator object
*
*   Creates a new pythonActuator object. The actual actuator object (itom) is only created later.
*/
PyObject* PythonPlugins::PyActuatorPlugin_new(PyTypeObject *type, PyObject* /*args*/, PyObject* /*kwds*/)
{
   PyActuatorPlugin *self = NULL;

   self = (PyActuatorPlugin *)type->tp_alloc(type, 0);
   if (self != NULL)
   {
      self->actuatorObj = NULL;
      self->base = NULL;
   }

   return (PyObject *)self;
}

//----------------------------------------------------------------------------------------------------------------------------------
PyDoc_STRVAR(pyActuatorInit_doc, "actuator(name[, mandparams, optparams]) -> creates new instance of actuator plugin 'name' \n\
\n\
This is the constructor for an `actuator` plugin. It initializes an new instance of the plugin specified by 'name'. \n\
The initialisation parameters are parsed and unnamed parameters are used in their incoming order to fill first \n\
mandatory parameters and afterwards optional parameters. Parameters may be passed with name as well but after \n\
the first named parameter no more unnamed parameters are allowed.\n\
\n\
See pluginHelp(name) for detail information about the specific initialisation parameters.\n\
\n\
Parameters \n\
----------- \n\
name : {str} \n\
    is the fullname (case sensitive) of an 'actuator'-plugin as specified in the plugin-window. \n\
mandparams : {variant(s)} \n\
    arguments corresponding the mandatory initialization parameters. The number of arguments and their order must fit the the required mandatory parameters \n\
optparams : {variant(s)}, optional \n\
    argument corresponding to the optional initialization parameters. If unnamed arguments are used, their order must correspond to the order of the optional parameters, keyword-based parameters are allowed as well. \n\
\n\
Returns \n\
-------- \n\
inst : {actuator} \n\
    new instance of the actuator-plugin");

/** constructor for actuator object (plugin) accessible from python
*   @param [in] self    the according pythonActuator object
*   @param [in] args    unnamed arguments passed to the constructor in python
*   @param [in] kwds    keyword parameters passed to the constructor in pyhton
*   @return             -1 in case an error occured, else 0
*
*   At first the list of available plugins is searched whether the plugin can be found (by name). If it was found
*   the plugin's manadtory ans optional initialization parameters are retrieved and a parameter check is done. In
*   case everything went right a new instance of a plugin is created with the parameters passed to the constructor.
*/
int PythonPlugins::PyActuatorPlugin_init(PyActuatorPlugin *self, PyObject *args, PyObject *kwds)
{
    self->actuatorObj = NULL;

    if (args == NULL) //args is only NULL, instance of actuator is created by a c-code fragment. Then the content of the type-struct has to be filled by the c-code, too.
    {
        return 0;
    }

    int length = PyTuple_Size(args);

    if (length == 0)
    {
        PyErr_Format(PyExc_ValueError, "no plugin specified");
        return -1;
    }
    else if (length == 1) //!< copy constructor or name only
    {
        PyActuatorPlugin* copyPlugin = NULL;

        if (PyArg_ParseTuple(args, "O!", &PyActuatorPluginType, &copyPlugin))
        {
            //try to increment reference of copyPlugin->actuatorObj
            if (copyPlugin->actuatorObj)
            {
                copyPlugin->actuatorObj->getBasePlugin()->incRef(copyPlugin->actuatorObj);
            }

            self->actuatorObj = copyPlugin->actuatorObj;
            self->base = copyPlugin->base;
            return 0;
        }
    }

    PyErr_Clear();

    QVector<ito::Param> *paramsMand = NULL;
    QVector<ito::Param> *paramsOpt = NULL;
    ito::RetVal retval = 0;
    int pluginNum = -1;
    PyObject *pnameObj = NULL;
    PyObject *params = NULL;
    QString pluginName = NULL;

    QVector<ito::ParamBase> paramsMandCpy;
    QVector<ito::ParamBase> paramsOptCpy;

    ito::AddInManager *AIM = ito::AddInManager::getInstance();
    if (!AIM)
    {
        PyErr_Format(PyExc_RuntimeError, "no addin-manager found");
        return -1;
    }

    pnameObj = PyTuple_GetItem(args, 0);
    if (PyUnicode_Check(pnameObj))
    {
        bool ok = false;
        pluginName = PythonQtConversion::PyObjGetString(pnameObj,false,ok);
    }
    else
    {
        PyErr_Format(PyExc_TypeError, "invalid parameters");
        return -1;
    }


    retval = AIM->getInitParams(pluginName, ito::typeActuator, &pluginNum, paramsMand, paramsOpt);
    if (retval.containsWarningOrError())
    {
        if (retval.errorMessage())
        {
            PyErr_Format(PyExc_RuntimeError, "Could not load plugin: %s with error message: \n%s\n", pluginName.toAscii().data(), retval.errorMessage());
        }
        else
        {
            PyErr_Format(PyExc_RuntimeError, "Could not load plugin: %s\n", pluginName.toAscii().data());
        }
        return -1;
    }


    bool enableAutoLoadParams = false;
    retval = findAndDeleteReservedInitKeyWords(kwds, &enableAutoLoadParams);
    if (retval.containsWarningOrError())
    {
        if (retval.errorMessage())
        {
            PyErr_Format(PyExc_RuntimeError, "Could not load plugin: %s with error message: \n%s\n", pluginName.toAscii().data(), retval.errorMessage());
        }
        else
        {
            PyErr_Format(PyExc_RuntimeError, "Could not load plugin: %s\n", pluginName.toAscii().data());
        }
        return -1;
    }

    params = PyTuple_GetSlice(args, 1, PyTuple_Size(args));

    //retval += copyParamVector(paramsMand, paramsMandCpy);
    //retval += copyParamVector(paramsOpt, paramsOptCpy);

    if (!retval.containsError())
    {

        if (parseInitParams(paramsMand, paramsOpt, params, kwds, paramsMandCpy, paramsOptCpy) != ito::retOk)
        {
            PyErr_Format(PyExc_RuntimeError, "error while parsing parameters.");
            return -1;
        }
        Py_DECREF(params);

        ItomSharedSemaphore *waitCond = new ItomSharedSemaphore();
        QMetaObject::invokeMethod(AIM, "initAddIn", Q_ARG(const int, pluginNum), Q_ARG(const QString&, pluginName), Q_ARG(ito::AddInActuator**, &self->actuatorObj), Q_ARG(QVector<ito::ParamBase>*, &paramsMandCpy), Q_ARG(QVector<ito::ParamBase>*, &paramsOptCpy), Q_ARG(bool, enableAutoLoadParams), Q_ARG(ItomSharedSemaphore*, waitCond));
        waitCond->wait(-1);
        retval += waitCond->returnValue;
         waitCond->deleteSemaphore();
         waitCond = NULL;

        paramsMandCpy.clear();
        paramsOptCpy.clear();
    }

    if (!SetLoadPluginReturnValueMessage(retval, pluginName))
    {
        return -1;
    }

    return 0;
}

//----------------------------------------------------------------------------------------------------------------------------------
PyObject* PythonPlugins::PyActuatorPlugin_repr(PyActuatorPlugin *self)
{
    PyObject *result;
    if (self->actuatorObj == NULL)
    {
        result = PyUnicode_FromFormat("empty actuator plugin");
    }
    else
    {
        PyObject *tempObj = NULL;
        if ((tempObj = getName(self->actuatorObj)) != NULL)
        {   
            QString ident = self->actuatorObj->getIdentifier();
            if(ident != "")
            {
                result = PyUnicode_FromFormat("Actuator-Plugin(%U, %s, ID: %i)", tempObj, ident.toAscii().data(), self->actuatorObj->getID());
            }
            else
            {
                result = PyUnicode_FromFormat("Actuator-Plugin(%U, ID: %i)", tempObj, self->actuatorObj->getID());
            }
            Py_DECREF(tempObj);    
        }
        else
            result = NULL;
    }
    return result;
}

//----------------------------------------------------------------------------------------------------------------------------------
PyMemberDef PythonPlugins::PyActuatorPlugin_members[] = {
    {NULL}  /* Sentinel */
};

//----------------------------------------------------------------------------------------------------------------------------------
//PyDoc_STRVAR(pyActuatorName_doc, -> see pyPluginName_doc);
/** Returns the plugin's name
*   @param [in] self    the plugin object
*   @return             the name of the plugin
*
*                       Queries the name of a plugin by invoking a getParam on the plugin for the name parameter
*/
PyObject* PythonPlugins::PyActuatorPlugin_name(PyActuatorPlugin* self)
{
    return getName(self->actuatorObj);
}

//----------------------------------------------------------------------------------------------------------------------------------
/** returns the list of available parameters
*   @param [in] self    the actuator object (python)
*   @return             a string with all available parameters for this actuator
*
*   All parameters of the plugin are shown. This can be useful as there are only few standard parameters
*   for an actuator. The majority is depending on the actual hardware and accordingly is different for each
*   plugin.
*/
PyObject* PythonPlugins::PyActuatorPlugin_getParamList(PyActuatorPlugin* self)
{
    ito::AddInBase *aib = self->actuatorObj;
    return getParamList(aib);
}

//----------------------------------------------------------------------------------------------------------------------------------
/** returns the list of available parameters and additional information about the plugin
*   @param [in] self    the actuator object (python)
*   @return             a string with all available parameters for this actuator
*
*   All parameters of the plugin are shown with additional information as min, max and infostring.
*   This can be useful as there are only few standard parameters for an actuator. The majority is
*   depending on the actual hardware and accordingly is different for each plugin.
*/
PyObject* PythonPlugins::PyActuatorPlugin_getParamListInfo(PyActuatorPlugin* self, PyObject *args)
{
    ito::AddInBase *aib = self->actuatorObj;
    return getParamListInfo(aib, args);
}

/** returns the list of available parameters and additional information about the plugin ExecFunctions
*   @param [in] self    the actuator object (python)
*   @return             a dictionary with all available parameters for this actuator
*
*   All ExecFunctions of the plugin are shown or one specific ExecFunctions with additional information as min, max and infostring is shown.
*   This can be useful as there are only few standard parameters for an actuator. The majority is
*   depending on the actual hardware and accordingly is different for each plugin.
*/
PyObject* PythonPlugins::PyActuatorPlugin_getExecFuncsInfo(PyActuatorPlugin* self, PyObject *args, PyObject *kwds)
{
    ito::AddInBase *aib = self->actuatorObj;
    return getExecFuncsInfo(aib, args, kwds);
}

//----------------------------------------------------------------------------------------------------------------------------------


/** gets a parameter value
*   @param [in] self    the actuator object (python)
*   @param [in] args    the parameter name
*   @return             the parameter value
*
*   The getParam method of the plugin is invoked and the actual parameter value is returned. If the parameter
*   doesn't exist an error is returned.
*/
PyObject* PythonPlugins::PyActuatorPlugin_getParam(PyActuatorPlugin* self, PyObject * args)
{
    return getParam(self->actuatorObj, args);
}

//----------------------------------------------------------------------------------------------------------------------------------
/** set a parameter to a new value
*   @param [in] self    the actuator object (python)
*   @param [in] args    the parameter name and new value
*   @return             an error if the parameter wasn't found or the passed value is out of the limits
*
*   The setParam method of the plugin is invoked and the parameter is set to the new value in case the passed value
*   is within the limits.
*/
PyObject* PythonPlugins::PyActuatorPlugin_setParam(PyActuatorPlugin* self, PyObject * args)
{
    return setParam(self->actuatorObj, args);
}

//----------------------------------------------------------------------------------------------------------------------------------
PyDoc_STRVAR(pyActuatorCalib_doc, "calib(axis[, axis1, ...]) -> starts calibration or homing of given axes (0-based).\n\
\n\
Most actuators have the possibility to calibrate or home certain axes. Use this command to start the calibration. \n\
\n\
Parameters \n\
----------- \n\
axis : {int}\n\
    index of the first axis to calibrate (e.g. 0 for first axis) \n\
axis1 : {int}\n\
    add the indices of further axes as optional arguments if they should be calibrated as well\n\
\n\
Raises \n\
---------- \n\
NotImplemented \n\
    if calibration not available");
/** calibrate actuator axi(e)s
*   @param [in] self    the actuator object (python)
*   @param [in] args    the axi(e)s numbers
*   @return             status of calibration
*
*   Invokes the calibrate method on an actuator object with the numbers of the axis passed. The status is
*   of the calibration is returned or an error.
*/
PyObject* PythonPlugins::PyActuatorPlugin_calib(PyActuatorPlugin* self, PyObject * args)
{
    ito::RetVal ret = ito::retOk;
    char **cargs = NULL;
    char *cargt = NULL;
    int length = PyTuple_Size(args);
    QVector<int> axisVec;

    if (length == 0)
    {
        PyErr_Format(PyExc_ValueError, "no axis specified");
        return NULL;
    }
    else
    {
        if (parseParams(args, length, cargs, cargt) < 0)
        {
            //PyErr_Format(PyExc_TypeError, "invalid parameters"); //message already set
            freeParams(length, cargt, cargs);
            return NULL;
        }
    }

    if (length == 1)
    {
        if (cargt[0] != 'l')
        {
            PyErr_Format(PyExc_TypeError, "invalid parameter type");
            freeParams(length, cargt, cargs);
            return NULL;
        }
    }
    else
    {
        for (int n = 0; n < length; n++)
        {
            if (cargt[n] != 'l')
            {
                PyErr_Format(PyExc_TypeError, "invalid parameter type");
                axisVec.clear();
                freeParams(length, cargt, cargs);
                return NULL;
            }
            axisVec.append(*cargs[n]);
        }
    }

    ItomSharedSemaphoreLocker locker(new ItomSharedSemaphore());
    if (length == 1)
    {
        QMetaObject::invokeMethod(self->actuatorObj, "calib", Q_ARG(const int, (const int) *cargs[0]), Q_ARG(ItomSharedSemaphore *, locker.getSemaphore()));
    }
    else
    {
        QMetaObject::invokeMethod(self->actuatorObj, "calib", Q_ARG(QVector<int>, axisVec), Q_ARG(ItomSharedSemaphore *, locker.getSemaphore()));
    }
    while (!locker.getSemaphore()->wait(AppManagement::timeouts.pluginGeneral))
    {
        if (!self->actuatorObj->isAlive())
        {
            ret += ito::RetVal(ito::retError, 0, QObject::tr("timeout while calibration").toAscii().data());
            break;
        }
    }

    ret += locker.getSemaphore()->returnValue;

    freeParams(length, cargt, cargs);
    axisVec.clear();

    if (!SetReturnValueMessage(ret, "calib"))
    {
        return NULL;
    }
    /*
    if (ret != ito::retOk)
    {
        PyErr_Format(PyExc_RuntimeError, QObject::tr("error invoking calib with error message: \n%s\n").toAscii(), QObject::tr(ret.errorMessage()).toAscii().data());
        return NULL;
    }
    */
    Py_RETURN_NONE;
}

//----------------------------------------------------------------------------------------------------------------------------------
PyDoc_STRVAR(pyActuatorSetOrigin_doc, "setOrigin(axis[, axis1, ...]) -> defines the actual position of the given axes to value 0. \n\
\n\
The current positions of all indicated axes (axis, axis1,...) are considered to be 0 such that following positioning commands \n\
are relative with respect to the current position. \n\
\n\
Parameters \n\
----------- \n\
axis : {int}\n\
    index of the first axis (e.g. 0 for first axis) \n\
axis1 : {int}\n\
    add the indices of further axes as optional arguments\n\
\n\
Raises \n\
---------- \n\
NotImplemented \n\
    if actuator does not support this feature");

/** set the origin of axi(e)s
*   @param [in] self    the actuator object (python)
*   @param [in] args    the axi(e)s
*   @return             status of setOrigin
*
*   The axi(e)s current position is set as new origin of the axi(e)s.
*/
PyObject* PythonPlugins::PyActuatorPlugin_setOrigin(PyActuatorPlugin* self, PyObject * args)
{
    ito::RetVal ret = ito::retOk;
    int length = PyTuple_Size(args);
    char **cargs = NULL;
    char *cargt = NULL;
    QVector<int> axisVec;

    if (length == 0)
    {
        PyErr_Format(PyExc_ValueError, "no axis specified");
        return NULL;
    }
    else
    {
        if (parseParams(args, length, cargs, cargt) < 0)
        {
            //PyErr_Format(PyExc_TypeError, "invalid parameters"); //message already set
            freeParams(length, cargt, cargs);
            return NULL;
        }
    }

    if (length == 1)
    {
        if (cargt[0] != 'l')
        {
            PyErr_Format(PyExc_TypeError, "invalid parameter type");
            freeParams(length, cargt, cargs);
            return NULL;
        }
    }
    else
    {
        for (int n = 0; n < length; n++)
        {
            if (cargt[n] != 'l')
            {
                PyErr_Format(PyExc_TypeError, "invalid parameter type");
                freeParams(length, cargt, cargs);
                axisVec.clear();
                return NULL;
            }
            axisVec.append(*cargs[n]);
        }
    }

    ItomSharedSemaphoreLocker locker(new ItomSharedSemaphore());
    if (length == 1)
    {
        QMetaObject::invokeMethod(self->actuatorObj, "setOrigin", Q_ARG(const int, (const int) *cargs[0]), Q_ARG(ItomSharedSemaphore *, locker.getSemaphore()));
    }
    else
    {
        QMetaObject::invokeMethod(self->actuatorObj, "setOrigin", Q_ARG(QVector<int>, axisVec), Q_ARG(ItomSharedSemaphore *, locker.getSemaphore()));
    }
    while (!locker.getSemaphore()->wait(AppManagement::timeouts.pluginGeneral))
    {
        if (!self->actuatorObj->isAlive())
        {
            ret += ito::RetVal(ito::retError, 0, QObject::tr("timeout while setting origin").toAscii().data());
            break;
        }
    }

    ret += locker.getSemaphore()->returnValue;

    freeParams(length, cargt, cargs);

    if (!SetReturnValueMessage(ret, "setOrigin"))
    {
        return NULL;
    }
    /*
    if (ret != ito::retOk)
    {
        PyErr_Format(PyExc_RuntimeError, QObject::tr("error invoking setOrigin with error message: \n%s\n").toAscii(), QObject::tr(ret.errorMessage()).toAscii().data());
        return NULL;
    }
    */
    Py_RETURN_NONE;
}

//----------------------------------------------------------------------------------------------------------------------------------
PyDoc_STRVAR(pyActuatorGetStatus_doc, "getStatus() -> returns a list of status values for each axis\n\
\n\
Each axis of an actuator plugin has got a status value that is used for informing about the current status of the axis. \n\
\n\
The status value is an or-combination of the following possible values: \n\
\n\
Moving flags: \n\
\n\
* actuatorUnknown     = 0x0001 : unknown current moving status \n\
* actuatorInterrupted = 0x0002 : movement has been interrupted by the user or another error during the movement occurred \n\
* actuatorMoving      = 0x0004 : axis is currently moving \n\
* actuatorAtTarget    = 0x0008 : axis reached the target position \n\
* actuatorTimeout     = 0x0010 : timout during movement. Unknown status of the movement \n\
\n\
Switches flags: \n\
\n\
* actuatorEndSwitch      = 0x0100 : axis reached any end switch (e.g. if only one end switch is available) \n\
* actuatorLeftEndSwitch  = 0x0200 : axis reached the left end switch \n\
* actuatorRightEndSwitch = 0x0400 : axis reached the right end switch \n\
* actuatorRefSwitch      = 0x0800 : axis reached any reference switch (e.g. for calibration...) \n\
* actuatorLeftRefSwitch  = 0x1000 : axis reached left reference switch \n\
* actuatorRightRefSwitch = 0x2000 : axis reached right reference switch \n\
\n\
Status flags: \n\
\n\
* actuatorAvailable = 0x4000 : the axis is available \n\
* actuatorEnabled   = 0x8000 : the axis is currently enabled and can be moved \n\
\n\
Returns \n\
------- \n\
status : {list of integers} \n\
    list of integers (size corresponds to number of axes) with the current status of each axis");
/** get the status of an actuator
*   @param [in] self    the actuator object (python)
*   @param [in] args    the axi(e)s numbers
*   @return             an error if the parameter wasn't found or the passed value is out of the limits
*
*   Returns the status of the axi(e)s passed as parameter.
*/
PyObject* PythonPlugins::PyActuatorPlugin_getStatus(PyActuatorPlugin* self, PyObject * args)
{
    ItomSharedSemaphoreLocker locker(new ItomSharedSemaphore());

    ito::RetVal ret = ito::retOk;
    int length = PyTuple_Size(args);

    QSharedPointer<QVector<int> > status(new QVector<int>());

    PyObject *result = NULL;

    if (length != 0)
    {
        PyErr_Format(PyExc_ValueError, "too many parameters");
        return NULL;
    }

    QMetaObject::invokeMethod(self->actuatorObj, "getStatus", Q_ARG(QSharedPointer<QVector<int> >, status), Q_ARG(ItomSharedSemaphore *, locker.getSemaphore()));
    while (!locker.getSemaphore()->wait(AppManagement::timeouts.pluginGeneral))
    {
        if (!self->actuatorObj->isAlive())
        {
            ret += ito::RetVal(ito::retError, 0, QObject::tr("timeout while getting Status").toAscii().data());
            break;
        }
    }

    ret += locker.getSemaphore()->returnValue;

    if (ret != ito::retOk)
    {
        PyErr_Format(PyExc_RuntimeError, "error invoking getStatus with error message: \n%s\n", ret.errorMessage());
        return NULL;
    }

    int size = status->size();
    result = PyList_New(size); //new ref
    for (int i = 0; i < size; ++i)
    {
        PyList_SetItem(result,i, PyLong_FromLong((*status)[i]));
    }

    return result;
}

//----------------------------------------------------------------------------------------------------------------------------------
PyDoc_STRVAR(pyActuatorGetPos_doc, "getPos(axis[, axis1, ...]) -> returns the actual positions of the given axes (in mm or degree).\n\
\n\
This method requests the current position(s) of the given axes and returns it. \n\
\n\
Parameters \n\
----------- \n\
axis : {int}\n\
    index of the first axis (e.g. 0 for first axis) \n\
axis1 : {int}\n\
    add the indices of further axes as optional arguments\n\
\n\
Returns \n\
------- \n\
positions : {float or tuple of float} \n\
    Current position as float value if only one axis is given or tuple of floats for multiple axes. The unit is mm or degree. \n\
\n\
See Also \n\
--------- \n\
setPosRel, setPosAbs");                         
                               
/** get the current position of axi(e)S
*   @param [in] self    the actuator object (python)
*   @param [in] args    the axi(e)s numbers
*   @return             the axi(e)s position(s)
*
*   Reads the position of the axi(e)s passed as parameter.
*/
PyObject* PythonPlugins::PyActuatorPlugin_getPos(PyActuatorPlugin* self, PyObject * args)
{
    ito::RetVal ret = ito::retOk;
    int length = PyTuple_Size(args);
    char **cargs = NULL;
    char *cargt = NULL;
    PyObject *result = NULL;
    QVector<int> axisVec;

    QSharedPointer<double> pos(new double);
    *pos = 0.0;
    QSharedPointer<QVector<double> > posVec(new QVector<double>());

    if (length < 1)
    {
        PyErr_Format(PyExc_ValueError, "no axis specified");
        return NULL;
    }
    else
    {
        if (parseParams(args, length, cargs, cargt) < 0)
        {
            //PyErr_Format(PyExc_TypeError, "invalid parameters"); //message already set
            freeParams(length, cargt, cargs);
            return NULL;
        }
    }

    if (length == 1)
    {
        if (cargt[0] != 'l')
        {
            PyErr_Format(PyExc_TypeError, "invalid parameter type");
            freeParams(length, cargt, cargs);
            return NULL;
        }
    }
    else
    {
        for (int n = 0; n < length; n++)
        {
            if (cargt[n] != 'l')
            {
                PyErr_Format(PyExc_TypeError, "invalid parameter type");
                freeParams(length, cargt, cargs);
                axisVec.clear();
                posVec.clear();
                return NULL;
            }
            axisVec.append(static_cast<int>(*(long *)cargs[n]));
            posVec->append(0);
        }
    }

    ItomSharedSemaphoreLocker locker(new ItomSharedSemaphore());

    if (length == 1)
    {
        long axis = *(long *)cargs[0];
        QMetaObject::invokeMethod(self->actuatorObj, "getPos", Q_ARG(const int, (const int) axis), Q_ARG(QSharedPointer<double>, pos), Q_ARG(ItomSharedSemaphore *, locker.getSemaphore()));
    }
    else
    {
        QMetaObject::invokeMethod(self->actuatorObj, "getPos", Q_ARG(QVector<int>, axisVec), Q_ARG(QSharedPointer<QVector<double> >, posVec), Q_ARG(ItomSharedSemaphore *, locker.getSemaphore()));
    }
    while (!locker.getSemaphore()->wait(AppManagement::timeouts.pluginGeneral))
    {
        if (!self->actuatorObj->isAlive())
        {
            ret += ito::RetVal(ito::retError, 0, QObject::tr("timeout while getting position values").toAscii().data());
            break;
        }
    }

    ret += locker.getSemaphore()->returnValue;

    if (length > 1)
    {
        result = PyTuple_New(length);
        for (int n = 0; n < length; n++)
        {
            PyTuple_SetItem(result, n, PyFloat_FromDouble((*posVec)[n]));
        }
    }
    else
    {
        result = PyFloat_FromDouble(*pos);
    }

    freeParams(length, cargt, cargs);
    axisVec.clear();

    if (!SetReturnValueMessage(ret, "getPos"))
    {
        return NULL;
    }
    /*
    if (ret != ito::retOk)
    {
        PyErr_Format(PyExc_RuntimeError, QObject::tr("error invoking getPos with error message: \n%s\n").toAscii(), QObject::tr(ret.errorMessage()).toAscii().data());
        return NULL;
    }
    */
    return result;
}
//----------------------------------------------------------------------------------------------------------------------------------
PyDoc_STRVAR(PyActuatorPlugin_getType_doc, "getType() -> returns actuator type");
/** returns the type of the actuator object
*   @param [in] self    the actuator object (python)
*   @return             a string with the type
*
*   This method simply returns the type of the actuator object
*/
PyObject* PythonPlugins::PyActuatorPlugin_getType(PyActuatorPlugin *self)
{
    PyObject *result = NULL;
    if (self == NULL || self->actuatorObj == NULL)
    {
        PyErr_SetString(PyExc_RuntimeError,"empty actuator plugin");
        return NULL;
    }
    else
    {
        ito::AddInInterfaceBase *aib = self->actuatorObj->getBasePlugin();
        if (aib)
        {
            result = PyLong_FromLong(aib->getType());
        }
        else
        {
            PyErr_SetString(PyExc_RuntimeError,"interface of plugin is NULL");
            return NULL;
        }
    }
    
    return result;
}

//----------------------------------------------------------------------------------------------------------------------------------
PyObject* PythonPlugins::PyActuatorPlugin_execFunc(PyActuatorPlugin *self, PyObject *args, PyObject *kwds)
{
    return execFunc(self->actuatorObj, args, kwds);
}

//----------------------------------------------------------------------------------------------------------------------------------
/** open configuration dialog
*   @param [in] self    the actuator object (python)
*
*   This method simply open the configuration dialog
*/
PyObject* PythonPlugins::PyActuatorPlugin_showConfiguration(PyActuatorPlugin* self)
{
    ito::AddInBase *aib = self->actuatorObj;
    return plugin_showConfiguration(aib);
}

//----------------------------------------------------------------------------------------------------------------------------------


/** returns the list of available parameters
*   @param [in] self    the actuator object (python)
*
*   This method simply open the widget
*/
PyObject* PythonPlugins::PyActuatorPlugin_showToolbox(PyActuatorPlugin* self)
{
    ito::AddInBase *aib = self->actuatorObj;
    return plugin_showToolbox(aib);
}

//----------------------------------------------------------------------------------------------------------------------------------
/** returns the list of available parameters
*   @param [in] self    the actuator object (python)
*
*   This method simply close the widget
*/
PyObject* PythonPlugins::PyActuatorPlugin_hideToolbox(PyActuatorPlugin* self)
{
    ito::AddInBase *aib = self->actuatorObj;
    return plugin_hideToolbox(aib);
}

//----------------------------------------------------------------------------------------------------------------------------------
/** helper function to parse the positioning parameters for an actuator
*   @param [in]  args       arguments passed to the function (in python)
*   @param [in/out] cargs   parsed argument values
*   @param [in/out] cargt   parsed argument types
*   @param [out]    axisVec Vector with axes numbers
*   @param [out]    posVec  Vector with position values
*   @return                 retOk of parameters could be parsed, retError otherwise
*
*   Parses the parameters passed to a setPos command in python. For each axis that should be positioned
*   an axis number and a position value are expected.
*/
ito::RetVal parsePosParams(PyObject *args, char **&cargs, char *&cargt, QVector<int> &axisVec, QVector<double> &posVec)
{
    ito::RetVal ret = ito::retOk;
    int length = PyTuple_Size(args);

    if (length < 2)
    {
        PyErr_Format(PyExc_ValueError, "no axis specified of position");
        return ito::retError;
    }
    else if ((length % 2) != 0)
    {
        PyErr_Format(PyExc_ValueError, "number of axis and position values are not equal");
        return ito::retError;
    }
    else
    {
        if (parseParams(args, length, cargs, cargt) < 0)
        {
            //PyErr_Format(PyExc_TypeError, "invalid parameters"); //message already set
            freeParams(length, cargt, cargs);
            return ito::retError;
        }
    }

    if (length == 2)
    {
        if ((cargt[0] != 'l') || ((cargt[1] != 'f') && (cargt[1] != 'l')))
        {
            PyErr_Format(PyExc_TypeError, "invalid parameter type");
            freeParams(length, cargt, cargs);
            return ito::retError;
        }
        if (cargt[1] == 'l')
        {
            double tdouble = *((long*)cargs[1]);
            free(cargs[1]);
            cargs[1] = (char*)malloc(sizeof(double));
            cargt[1] = 'f';
            *(double*)cargs[1] = tdouble;
        }
    }
    else
    {
        for (int n = 0; n < length / 2; n++)
        {
            if (cargt[n * 2] != 'l')
            {
                PyErr_Format(PyExc_TypeError, "invalid parameter type");
                freeParams(length, cargt, cargs);
                axisVec.clear();
                posVec.clear();
                return ito::retError;
            }
            axisVec.append(static_cast<int>(*(long *)cargs[n * 2]));

            if ((cargt[n * 2 + 1] != 'f') && (cargt[n * 2 + 1] != 'l'))
            {
                PyErr_Format(PyExc_TypeError, "invalid parameter type");
                freeParams(length, cargt, cargs);
                axisVec.clear();
                posVec.clear();
                return ito::retError;
            }
            if (cargt[n * 2 + 1] == 'f')
            {
                posVec.append(*(double *)cargs[n * 2 + 1]);
            }
            else
            {
                posVec.append(*(long *)cargs[n * 2 + 1]);
            }
        }
    }

    return ito::retOk;
}

//----------------------------------------------------------------------------------------------------------------------------------
PyDoc_STRVAR(pyActuatorSetPosAbs_doc,"setPosAbs(axis0, pos0 [, axis1, pos1, ...]) -> moves given axes to given absolute values (in mm or degree).\n\
\n\
All arguments are a pair of axis index and the new target position of this axis. This method starts the absolute positioning of all given axes. \n\
If the 'async' parameter of the plugin is 0 (usually default), a synchronous positioning is started, hence, this method returns after that all \n\
axes reached their target position or a timeout occurred. Else this method immediately returns and the actuator goes on moving. \n\
\n\
Parameters \n\
----------- \n\
axisM : {int} \n\
    index of the axis to position \n\
posM : {float} \n\
    absolute target position of the `axisM` (in mm or degree) \n\
\n\
See Also \n\
--------- \n\
getPos, setPosRel");                             

/** set actuator axi(e)s to new absolute position(s)
*   @param [in] self    the actuator object (python)
*   @param [in] args    the axi(e)s numbers
*   @return             status of positioning command
*
*   The passed parameters are parsed using the helper function \ref parsePosParams and in case a meaningful
*   number of axi(e)s and position(s) are found the setPosAbs method of the actuator object is invoked.
*/
PyObject* PythonPlugins::PyActuatorPlugin_setPosAbs(PyActuatorPlugin* self, PyObject * args)
{
    ito::RetVal ret = ito::retOk;
    int length = PyTuple_Size(args);
    char **cargs = NULL;
    char *cargt = NULL;
    QVector<int> axisVec;
    QVector<double> posVec;

    if ((ret = parsePosParams(args, cargs, cargt, axisVec, posVec)) != ito::retOk)
    {
        return NULL;
    }

    ItomSharedSemaphoreLocker locker(new ItomSharedSemaphore());
    if (length == 2)
    {
        long axis = *(long *)cargs[0];
        QMetaObject::invokeMethod(self->actuatorObj, "setPosAbs", Q_ARG(const int, (const int) axis), Q_ARG(const double, (const double)(*((double*)(cargs[1])))), Q_ARG(ItomSharedSemaphore *, locker.getSemaphore()));
    }
    else
    {
        QMetaObject::invokeMethod(self->actuatorObj, "setPosAbs", Q_ARG(QVector<int>, axisVec), Q_ARG(QVector<double>, posVec), Q_ARG(ItomSharedSemaphore *, locker.getSemaphore()));
    }
    while (!locker.getSemaphore()->wait(AppManagement::timeouts.pluginGeneral))
    {
        if (!self->actuatorObj->isAlive())
        {
            ret += ito::RetVal(ito::retError, 0, QObject::tr("timeout while setting absolute position").toAscii().data());
            break;
        }
    }

    ret += locker.getSemaphore()->returnValue;

    freeParams(length, cargt, cargs);
    axisVec.clear();
    posVec.clear();

    if (!SetReturnValueMessage(ret, "setPosAbs"))
    {
        return NULL;
    }

    Py_RETURN_NONE;
}

//----------------------------------------------------------------------------------------------------------------------------------
PyDoc_STRVAR(pyActuatorSetPosRel_doc,"setPosRel(axis0, pos0[, axis1, pos1, ...]) -> relatively moves given axes by the given distances [in mm or degree].\n\
\n\
All arguments are a pair of axis index and the relative moving-distance of this axis. This method starts the relative positioning of all given axes. \n\
If the 'async' parameter of the plugin is 0 (usually default), a synchronous positioning is started, hence, this method returns after that all \n\
axes reached their target position or a timeout occurred. Else this method immediately returns and the actuator goes on moving. \n\
\n\
Parameters \n\
----------- \n\
axisM : {int} \n\
    index of the axis to position \n\
posM : {float} \n\
    relative target position of the `axisM` (in mm or degree) \n\
\n\
See Also \n\
--------- \n\
getPos, setPosAbs"); 
/** set actuator axi(e)s to new relative position(s)
*   @param [in] self    the actuator object (python)
*   @param [in] args    the axi(e)s numbers
*   @return             status of positioning command
*
*   The passed parameters are parsed using the helper function \ref parsePosParams and in case a meaningful
*   number of axi(e)s and position(s) are found the setPosRel method of the actuator object is invoked.
*/
PyObject* PythonPlugins::PyActuatorPlugin_setPosRel(PyActuatorPlugin* self, PyObject * args)
{
    ito::RetVal ret = ito::retOk;
    int length = PyTuple_Size(args);
    char **cargs = NULL;
    char *cargt = NULL;
    QVector<int> axisVec;
    QVector<double> posVec;

    if ((ret = parsePosParams(args, cargs, cargt, axisVec, posVec)) != ito::retOk)
    {
        return NULL;
    }

    ItomSharedSemaphoreLocker locker(new ItomSharedSemaphore());
    if (length == 2)
    {
        long axis = *(long *)cargs[0];
        QMetaObject::invokeMethod(self->actuatorObj, "setPosRel", Q_ARG(const int, (const int) axis), Q_ARG(const double, (const double)(*((double*)(cargs[1])))), Q_ARG(ItomSharedSemaphore *, locker.getSemaphore()));
    }
    else
    {
        QMetaObject::invokeMethod(self->actuatorObj, "setPosRel", Q_ARG(QVector<int>, axisVec), Q_ARG(QVector<double>, posVec), Q_ARG(ItomSharedSemaphore *, locker.getSemaphore()));
    }
    while (!locker.getSemaphore()->wait(AppManagement::timeouts.pluginGeneral))
    {
        if (!self->actuatorObj->isAlive())
        {
            ret += ito::RetVal(ito::retError, 0, QObject::tr("timeout while setting relative position").toAscii().data());
            break;
        }
    }

    ret += locker.getSemaphore()->returnValue;

    freeParams(length, cargt, cargs);
    axisVec.clear();
    posVec.clear();

    if (!SetReturnValueMessage(ret, "setPosRel"))
    {
        return NULL;
    }
    /*
    if (ret != ito::retOk)
    {
        PyErr_Format(PyExc_RuntimeError, QObject::tr("error invoking setPos with error message: \n%s\n").toAscii(), QObject::tr(ret.errorMessage()).toAscii().data());
        return NULL;
    }
    */
    Py_RETURN_NONE;
}

//----------------------------------------------------------------------------------------------------------------------------------
PyMethodDef PythonPlugins::PyActuatorPlugin_methods[] = {
   {"getParamList", (PyCFunction)PythonPlugins::PyActuatorPlugin_getParamList, METH_NOARGS, pyPluginGetParamList_doc},
   {"getParamListInfo", (PyCFunction)PythonPlugins::PyActuatorPlugin_getParamListInfo, METH_VARARGS, pyPluginGetParamListInfo_doc},
   {"getExecFuncsInfo", (PyCFunction)PythonPlugins::PyActuatorPlugin_getExecFuncsInfo, METH_VARARGS | METH_KEYWORDS, pyPlugInGetExecFuncsInfo_doc},
   {"name", (PyCFunction)PythonPlugins::PyActuatorPlugin_name, METH_NOARGS, pyPluginName_doc},
   {"getParam", (PyCFunction)PythonPlugins::PyActuatorPlugin_getParam, METH_VARARGS, pyPluginGetParam_doc},
   {"setParam", (PyCFunction)PythonPlugins::PyActuatorPlugin_setParam, METH_VARARGS, pyPluginSetParam_doc},
   {"calib", (PyCFunction)PythonPlugins::PyActuatorPlugin_calib, METH_VARARGS, pyActuatorCalib_doc},
   {"setOrigin", (PyCFunction)PythonPlugins::PyActuatorPlugin_setOrigin, METH_VARARGS, pyActuatorSetOrigin_doc},
   {"getStatus", (PyCFunction)PythonPlugins::PyActuatorPlugin_getStatus, METH_VARARGS, pyActuatorGetStatus_doc},
   {"getPos", (PyCFunction)PythonPlugins::PyActuatorPlugin_getPos, METH_VARARGS, pyActuatorGetPos_doc},
   {"setPosAbs", (PyCFunction)PythonPlugins::PyActuatorPlugin_setPosAbs, METH_VARARGS, pyActuatorSetPosAbs_doc},
   {"setPosRel", (PyCFunction)PythonPlugins::PyActuatorPlugin_setPosRel, METH_VARARGS, pyActuatorSetPosRel_doc},
   {"getType", (PyCFunction)PythonPlugins::PyActuatorPlugin_getType, METH_NOARGS, PyActuatorPlugin_getType_doc},
   {"exec", (PyCFunction)PythonPlugins::PyActuatorPlugin_execFunc, METH_KEYWORDS | METH_VARARGS, PyPlugin_execFunc_doc},
   {"showConfiguration", (PyCFunction)PythonPlugins::PyActuatorPlugin_showConfiguration, METH_NOARGS, pyPluginShowConfiguration_doc},
   {"showToolbox", (PyCFunction)PythonPlugins::PyActuatorPlugin_showToolbox, METH_NOARGS, pyPluginShowToolbox_doc},
   {"hideToolbox", (PyCFunction)PythonPlugins::PyActuatorPlugin_hideToolbox, METH_NOARGS, pyPluginHideToolbox_doc},
   {NULL}  /* Sentinel */
};

//----------------------------------------------------------------------------------------------------------------------------------
PyModuleDef PythonPlugins::PyActuatorPluginModule = {
   PyModuleDef_HEAD_INIT,
   "actuatorPlugin",
   QObject::tr("Itom ActuatorPlugin type in python").toAscii().data(),
   -1,
   NULL, NULL, NULL, NULL, NULL
};

//----------------------------------------------------------------------------------------------------------------------------------
PyTypeObject PythonPlugins::PyActuatorPluginType = {
   PyVarObject_HEAD_INIT(NULL, 0)
   "itom.actuator",                         /* tp_name */
   sizeof(PyActuatorPlugin),                /* tp_basicsize */
   0,                                       /* tp_itemsize */
   (destructor)PyActuatorPlugin_dealloc,    /* tp_dealloc */
   0,                                       /* tp_print */
   0,                                       /* tp_getattr */
   0,                                       /* tp_setattr */
   0,                                       /* tp_reserved */
   (reprfunc)PyActuatorPlugin_repr,         /* tp_repr */
   0,                                       /* tp_as_number */
   0,                                       /* tp_as_sequence */
   0,                                       /* tp_as_mapping */
   0,                                       /* tp_hash  */
   0,                                       /* tp_call */
   0,                                       /* tp_str */
   0,                                       /* tp_getattro */
   0,                                       /* tp_setattro */
   0,                                       /* tp_as_buffer */
   Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE,   /* tp_flags */
   pyActuatorInit_doc,                      /* tp_doc */
   0,                                        /* tp_traverse */
   0,                                        /* tp_clear */
   0,                                        /* tp_richcompare */
   0,                                        /* tp_weaklistoffset */
   0,                                        /* tp_iter */
   0,                                        /* tp_iternext */
   PyActuatorPlugin_methods,                /* tp_methods */
   PyActuatorPlugin_members,                /* tp_members */
   0,                                       /* tp_getset */
   0,                                       /* tp_base */
   0,                                       /* tp_dict */
   0,                                       /* tp_descr_get */
   0,                                       /* tp_descr_set */
   0,                                       /* tp_dictoffset */
   (initproc)PythonPlugins::PyActuatorPlugin_init,      /* tp_init */
   0,                                       /* tp_alloc */
   PyActuatorPlugin_new                     /*PyType_GenericNew*/ /*PythonStream_new,*/                 /* tp_new */
};






//----------------------------------------------------------------------------------------------------------------------------------
/** desctructor for dataIO object in python
*   @param [in] self
*
*   Destructs an instance of a dataIO object (plugin), i.e. deletes the according python variable and invokes
*   the closeAddIn function. The object itself is only deleted if the object's <<<erence
*   counter is zero.
*/
void PythonPlugins::PyDataIOPlugin_dealloc(PyDataIOPlugin* self)
{
    if (self->dataIOObj)
    {
        ito::AddInInterfaceBase *aib = self->dataIOObj->getBasePlugin();
        if (!aib)
        {
            std::cerr << "error closing plugin" << std::endl;
            //PyErr_Format(PyExc_RuntimeError, "error closing plugin");
        }
        else
        {
            ito::AddInManager *aim = ito::AddInManager::getInstance();
            ito::RetVal retval(ito::retOk);

            ItomSharedSemaphore *waitCond = new ItomSharedSemaphore();
            QMetaObject::invokeMethod(aim, "closeAddIn", Q_ARG(ito::AddInBase**, (ito::AddInBase**)&self->dataIOObj), Q_ARG(ItomSharedSemaphore*, waitCond));
//            ito::RetVal retval = aim->closeAddIn((ito::AddInBase**)&self->dataIOObj);
            waitCond->wait(-1);
            retval += waitCond->returnValue;
            waitCond->deleteSemaphore();
            waitCond = NULL;

            PythonCommon::transformRetValToPyException(retval);
            /*if (retval != ito::retOk)
            {
                PyErr_Format(PyExc_RuntimeError, "error closing plugin");
            }*/
        }
    }

    Py_TYPE(self)->tp_free((PyObject*)self);
}

//----------------------------------------------------------------------------------------------------------------------------------
/** constructor for dataIO object in python
*   @param [in] type
*   @return     new python dataIO object
*
*   Creates a new pythonDataIO object. The actual dataIO object (itom) is only created later.
*/
PyObject* PythonPlugins::PyDataIOPlugin_new(PyTypeObject *type, PyObject * /*args*/, PyObject * /*kwds*/)
{
    PyDataIOPlugin *self = NULL;

    self = (PyDataIOPlugin *)type->tp_alloc(type, 0);
    if (self != NULL)
    {
        self->dataIOObj = NULL;
        self->base = NULL;
    }

    return (PyObject *)self;
}

//----------------------------------------------------------------------------------------------------------------------------------
PyDoc_STRVAR(pyDataIOInit_doc, "dataIO(name[, mandparams, optparams]) -> creates new instance of dataIO plugin 'name' \n\
\n\
This is the constructor for a `dataIO` plugin. It initializes an new instance of the plugin specified by 'name'. \n\
The initialisation parameters are parsed and unnamed parameters are used in their incoming order to fill first \n\
mandatory parameters and afterwards optional parameters. Parameters may be passed with name as well but after \n\
the first named parameter no more unnamed parameters are allowed.\n\
\n\
See pluginHelp(name) for detail information about the specific initialisation parameters.\n\
\n\
Parameters \n\
----------- \n\
name : {str} \n\
    is the fullname (case sensitive) of an 'actuator'-plugin as specified in the plugin-window. \n\
mandparams : {variant(s)} \n\
    arguments corresponding the mandatory initialization parameters. The number of arguments and their order must fit the the required mandatory parameters \n\
optparams : {variant(s)}, optional \n\
    argument corresponding to the optional initialization parameters. If unnamed arguments are used, their order must correspond to the order of the optional parameters, keyword-based parameters are allowed as well. \n\
\n\
Returns \n\
-------- \n\
inst : {dataIO} \n\
    new instance of the dataIO-plugin");

/** constructor for dataIO object
*   @param [in] self    the according dataIO object
*   @param [in] args    unnamed arguments passed to the constructor in python
*   @return             -1 in case an error occured, else 0
*
*   The dataIO passed must be a valid dataIO object. In case the autoloading of parameters is activated for this
*   plugin the default parameters are loaded.
*/
int PythonPlugins::PyDataIOPlugin_init(PyDataIOPlugin *self, PyObject *args, PyObject *kwds)
{

    if (args == NULL) //args is only NULL, instance of dataIO is created by a c-code fragment. Then the content of the type-struct has to be filled by the c-code, too.
    {
        return 0;
    }

    self->dataIOObj = NULL;

    int length = PyTuple_Size(args);

    if (length == 0)
    {
        PyErr_Format(PyExc_ValueError, "no plugin specified");
        return -1;
    }
    else if (length == 1) //!< copy constructor or name only
    {
        PyDataIOPlugin* copyPlugin = NULL;

        if (PyArg_ParseTuple(args, "O!", &PyDataIOPluginType, &copyPlugin))
        {
            //try to increment reference of copyPlugin->dataIOObj
            if (copyPlugin->dataIOObj)
            {
                copyPlugin->dataIOObj->getBasePlugin()->incRef(copyPlugin->dataIOObj);
            }
            
            self->dataIOObj = copyPlugin->dataIOObj;
            self->base = copyPlugin->base;

            return 0;
        }
    }

    PyErr_Clear();
    QVector<ito::Param> *paramsMand = NULL;
    QVector<ito::Param> *paramsOpt = NULL;
    ito::RetVal retval = 0;
    int pluginNum = -1;
    PyObject *pnameObj = NULL;
    PyObject *params = NULL;
    QString pluginName;

    QVector<ito::ParamBase> paramsMandCpy;
    QVector<ito::ParamBase> paramsOptCpy;

    ito::AddInManager *AIM = ito::AddInManager::getInstance();
    if (!AIM)
    {
        PyErr_Format(PyExc_RuntimeError, "no addin-manager found");
        return -1;
    }

    pnameObj = PyTuple_GetItem(args, 0);
    if (PyUnicode_Check(pnameObj))
    {
        bool ok = false;
        pluginName = PythonQtConversion::PyObjGetString(pnameObj,false,ok);
    }
    else
    {
        PyErr_Format(PyExc_TypeError, "invalid parameters");
        return -1;
    }

    retval = AIM->getInitParams(pluginName, ito::typeDataIO, &pluginNum, paramsMand, paramsOpt);
    if (retval.containsWarningOrError())
    {
        if (retval.errorMessage())
        {
            PyErr_Format(PyExc_RuntimeError, "Could not load plugin: %s with error message: \n%s\n", pluginName.toAscii().data(), retval.errorMessage());
        }
        else
        {
            PyErr_Format(PyExc_RuntimeError, "Could not load plugin: %s\n", pluginName.toAscii().data());
        }
        return -1;
    }

    bool enableAutoLoadParams = false;
    retval = findAndDeleteReservedInitKeyWords(kwds, &enableAutoLoadParams);
    if (retval.containsWarningOrError())
    {
        if (retval.errorMessage())
        {
            PyErr_Format(PyExc_RuntimeError, "Could not load plugin: %s with error message: \n%s\n", pluginName.toAscii().data(), retval.errorMessage());
        }
        else
        {
            PyErr_Format(PyExc_RuntimeError, "Could not load plugin: %s\n", pluginName.toAscii().data());
        }
        return -1;
    }

    params = PyTuple_GetSlice(args, 1, PyTuple_Size(args));

    //retval += copyParamVector(paramsMand, paramsMandCpy);
    //retval += copyParamVector(paramsOpt, paramsOptCpy);

    if (!retval.containsError())
    {
        if (parseInitParams(paramsMand, paramsOpt, params, kwds, paramsMandCpy, paramsOptCpy) != ito::retOk)
        {
            PyErr_Format(PyExc_ValueError, "error while parsing parameters.");
            return -1;
        }
        Py_DECREF(params);

        ItomSharedSemaphore *waitCond = new ItomSharedSemaphore();
        QMetaObject::invokeMethod(AIM, "initAddIn", Q_ARG(const int, pluginNum), Q_ARG(const QString&, pluginName), Q_ARG(ito::AddInDataIO**, &self->dataIOObj), Q_ARG(QVector<ito::ParamBase>*, &paramsMandCpy), Q_ARG(QVector<ito::ParamBase>*, &paramsOptCpy), Q_ARG(bool, enableAutoLoadParams), Q_ARG(ItomSharedSemaphore*, waitCond));
    //    retval = AIM->initAddIn(pluginNum, pluginName, &self->dataIOObj, paramsMand, paramsOpt, enableAutoLoadParams);
        waitCond->wait(-1);
        retval += waitCond->returnValue;
        waitCond->deleteSemaphore();
        waitCond = NULL;

        paramsMandCpy.clear();
        paramsOptCpy.clear();
    }

    if (!SetLoadPluginReturnValueMessage(retval, pluginName))
    {
        return -1;
    }

    return 0;
}



//----------------------------------------------------------------------------------------------------------------------------------   
//PyDoc_STRVAR(PyDataIOPlugin_name_doc, -> see pyPluginName_doc);

/** Returns the plugin's name
*   @param [in] self    the plugin object
*   @return             the name of the plugin
*
*                       Queries the name of a plugin by invoking a getParam on the plugin for the name parameter
*/
PyObject* PythonPlugins::PyDataIOPlugin_name(PyDataIOPlugin* self)
{
    return getName(self->dataIOObj);
}

//----------------------------------------------------------------------------------------------------------------------------------
PyObject* PythonPlugins::PyDataIOPlugin_repr(PyDataIOPlugin *self)
{
    PyObject *result;
    if (self->dataIOObj == NULL)
    {
        result = PyUnicode_FromFormat("empty dataIO plugin");
    }
    else
    {
        PyObject *name = getName(self->dataIOObj);
        if (name)
        {
            QString ident = self->dataIOObj->getIdentifier();
            if(ident != "")
            {
                result = PyUnicode_FromFormat("DataIO-Plugin(%U, %s, ID: %i)", name, ident.toAscii().data(), self->dataIOObj->getID());
            }
            else
            {
                result = PyUnicode_FromFormat("DataIO-Plugin(%U, ID: %i)", name, self->dataIOObj->getID());
            }
            Py_DECREF(name);
        }
        else
            result = PyUnicode_FromFormat("dataIO-Plugin, time out reading name");
    }
    return result;
}

//----------------------------------------------------------------------------------------------------------------------------------
/** returns the list of available parameters
*   @param [in] self    the dataIO object (python)
*   @return             a string with all available parameters for this dataIO
*
*   All parameters of the plugin are shown. This can be useful as there are only few standard parameters
*   for an dataIO. The majority is depending on the actual hardware and accordingly is different for each
*   plugin.
*/
PyObject* PythonPlugins::PyDataIOPlugin_getParamList(PyDataIOPlugin* self)
{
    ito::AddInBase *aib = self->dataIOObj;
    return getParamList(aib);
}
//----------------------------------------------------------------------------------------------------------------------------------
/** returns the list of available parameters and additional information about the plugin
*   @param [in] self    the dataIO object (python)
*   @return             a string with all available parameters for this dataIO
*
*   All parameters of the plugin are shown with additional information as min, max and infostring.
*   This can be useful as there are only few standard parameters for an dataIO. The majority is
*   depending on the actual hardware and accordingly is different for each plugin.
*/
PyObject* PythonPlugins::PyDataIOPlugin_getParamListInfo(PyDataIOPlugin* self, PyObject *args)
{
    ito::AddInBase *aib = self->dataIOObj;
    return getParamListInfo(aib, args);
}
/** returns the list of available parameters and additional information about the plugin ExecFunctions
*   @param [in] self    the actuator object (python)
*   @return             a dictionary with all available parameters for this actuator
*
*   All ExecFunctions of the plugin are shown or one specific ExecFunctions with additional information as min, max and infostring is shown.
*   This can be useful as there are only few standard parameters for an dataIO. The majority is
*   depending on the actual hardware and accordingly is different for each plugin.
*/
PyObject* PythonPlugins::PyDataIOPlugin_getExecFuncsInfo(PyDataIOPlugin* self, PyObject *args, PyObject *kwds)
{
    ito::AddInBase *aib = self->dataIOObj;
    return getExecFuncsInfo(aib, args, kwds);
}

//----------------------------------------------------------------------------------------------------------------------------------
/** return a parameter value
*   @param [in] self        the addIn whoes parameter is requested
*   @param [in] args        the parameter name
*   @return     python object with the parameter value on success (parameter exists), NULL otherwise
*
*   The function tries to retrieve the value of the parameter with the name given in args. If the parameter does not exist
*   NULL is returned. To actually retrieve the value the getParam function of the plugin is invoked.
*/
PyObject* PythonPlugins::PyDataIOPlugin_getParam(PyDataIOPlugin *self, PyObject *args)
{
    return getParam(self->dataIOObj, args);
}

//----------------------------------------------------------------------------------------------------------------------------------
/** set a parameter to a new value
*   @param [in] self    the actuator object (python)
*   @param [in] args    the parameter name and new value
*   @return             an error if the parameter wasn't found or the passed value is out of the limits
*
*   The setParam method of the plugin is invoked and the parameter is set to the new value in case the passed value
*   is within the limits.
*/
PyObject* PythonPlugins::PyDataIOPlugin_setParam(PyDataIOPlugin *self, PyObject *args)
{
    return setParam(self->dataIOObj, args);
}

//----------------------------------------------------------------------------------------------------------------------------------
PyDoc_STRVAR(PyDataIOPlugin_startDevice_doc,"startDevice([count=1]) -> starts the given dataIO-plugin. \n\
\n\
This command starts the dataIO plugin such that it is ready for data acquisition. Call this method before you start \n\
using commands like acquire, `getVal` or `copyVal`. If the device already is started, an internal start-counter is incremented \n\
by the parameter 'count'. The corresponding `stopDevice` method then decrements this counter and finally stops the device once \n\
the counter drops to zero again. \n\
\n\
The counter is necessary, since every connected live image needs to start the device without knownledge about any \n\
previous start. No acquisition is possible, if the device has not been started, hence the counter is 0. \n\
\n\
Parameters \n\
----------- \n\
count : {int}, optional \n\
    Number of increments to the internal start-counter [default:1] \n\
\n\
See Also \n\
--------- \n\
stopDevice()");
/** start a dataIO device, i.e. prepare it for recording data
*   @param [in] self    the dataIO object (python)
*   @param [in] args    should be empty
*   @return             an error if the device could not be started
*
*   Start a dataIO device, i.e. prepare it for the acquisition of data. Usually the device is first initialized, then
*   then it is started using this function afterwards a number of acquisitions is done (using \ref PyDataIOPlugin_acquire and
*   \ref PyDataIOPlugin_getVal) and only after the last data set is recorded it is stopped again.
*/
PyObject* PythonPlugins::PyDataIOPlugin_startDevice(PyDataIOPlugin *self, PyObject *args)
{
    int count = 1;

    if (!PyArg_ParseTuple(args, "|i", &count))
    {
        return NULL;
    }

    if (count < 0)
    {
        return PyErr_Format(PyExc_ValueError, "argument 'count' must be >= 0");
    }
    ito::RetVal ret = ito::retOk;
    ItomSharedSemaphore *waitCond = NULL;

    for (int i = 0 ; i < count ; i++)
    {
        waitCond = new ItomSharedSemaphore();
        QMetaObject::invokeMethod(self->dataIOObj, "startDevice", Q_ARG(ItomSharedSemaphore *, waitCond));

        while (!waitCond->wait(AppManagement::timeouts.pluginGeneral))
        {
            if (!self->dataIOObj->isAlive())
            {
                break;
            }
        }

        ret += waitCond->returnValue;
        waitCond->deleteSemaphore();
        waitCond = NULL;

        if (!SetReturnValueMessage(ret, "startDevice"))
        {
            return NULL;
        }
    }
    
    Py_RETURN_NONE;
}

//----------------------------------------------------------------------------------------------------------------------------------
PyDoc_STRVAR(PyDataIOPlugin_stopDevice_doc,"stopDevice([count=1]) -> stops the given dataIO-plugin. \n\
\n\
If this method is called as many times as the corresponding `startDevice` (or if the counts are equal), the \n\
dataIO device is stopped (not deleted) and it is not possible to acquire further data. \n\
\n\
Once a live image is connected to a camera, `startDevice` is automatically called at start of the live acquisition \n\
and `stopDevice` at shutdown. \n\
\n\
Parameters \n\
----------- \n\
count : {int}, optional\n\
    default = 1\n\
    if count > 1, `stopDevice` is executed 'count' times, in order to decrement the grabber internal start counter. \n\
    You can also use -1 as count argument, then `stopDevice` is repeated until the internal start counter is 0. The number of effective counts is then returned \n\
\n\
Returns \n\
-------- \n\
counts : {None or int} \n\
    If `count == -1` the number of required counts to finally stop the device is returned. Else: None \n\
\n\
See Also \n\
--------- \n\
startDevice()");

/** stop a dataIO device
*   @param [in] self    the dataIO object (python)QString
*   @param [in] args    should be empty
*   @return             an error if the device could not be stopped
*
*   Stop a dataIO device, i.e. it is no longer possible to acquire data with it. See also \ref PyDataIOPlugin_startDevice
*/
PyObject* PythonPlugins::PyDataIOPlugin_stopDevice(PyDataIOPlugin *self, PyObject *args)
{
    int count = 1;

    if (!PyArg_ParseTuple(args, "|i", &count))
    {
        return NULL;
    }

    ito::RetVal ret = ito::retOk;
    ItomSharedSemaphore *waitCond = NULL;

    if (count >= 0)
    {
        for (int i = 0 ; i < count ; i++)
        {
            waitCond = new ItomSharedSemaphore();
            QMetaObject::invokeMethod(self->dataIOObj, "stopDevice", Q_ARG(ItomSharedSemaphore *, waitCond));

            while (!waitCond->wait(AppManagement::timeouts.pluginGeneral))
            {
                if (!self->dataIOObj->isAlive())
                {
                    ret += ito::RetVal(ito::retError,0,"timeout while stopping device");
                    break;
                }
            }

            ret += waitCond->returnValue;
            waitCond->deleteSemaphore();
            waitCond = NULL;

            if (!SetReturnValueMessage(ret, "stopDevice"))
            {
                return NULL;
            }
        }
    
        Py_RETURN_NONE;
    }
    else if (count == -1)
    {
        count = -1;
        bool timeout = false;
        while(!ret.containsWarningOrError())
        {
            count++;
            waitCond = new ItomSharedSemaphore();
            QMetaObject::invokeMethod(self->dataIOObj, "stopDevice", Q_ARG(ItomSharedSemaphore *, waitCond));

            while (!waitCond->wait(AppManagement::timeouts.pluginGeneral))
            {
                if (!self->dataIOObj->isAlive())
                {
                    ret += ito::RetVal(ito::retError,0,"timeout while stopping device");
                    timeout = true;
                    break;
                }
            }

            ret += waitCond->returnValue;
            waitCond->deleteSemaphore();
            waitCond = NULL;
        }

        if (timeout)
        {
            if (!SetReturnValueMessage(ret, "stopDevice"))
            {
                return NULL;
            }
            return NULL;
        }
        else
        {
            return Py_BuildValue("i",count);
        }

    }
    else
    {
        return PyErr_Format(PyExc_ValueError, "argument 'count' must be >= 0 or -1");
    }
}

//----------------------------------------------------------------------------------------------------------------------------------
PyDoc_STRVAR(PyDataIOPlugin_acquire_doc,"acquire(trigger=dataIO.TRIGGER_SOFTWARE) -> triggers a new the camera acquisition \n\
\n\
This method triggers a new data acquisition. This method immediately returns even if the acquisition is not finished yet. \n\
Use `getVal` or `copyVal` to get the acquired data. Both methods block until the data is available. \n\
\n\
Parameters \n\
----------- \n\
trigger : {int}, optional\n\
    Type of the trigger: \n\
    \n\
    * `dataIO.TRIGGER_SOFTWARE = 0` : a software trigger is started, hence, the acquisition is immediately started when calling this method\n\
    * others : depending on your camera, this parameter can be used to set other triggers, like hardware trigger with raising or falling edges...");

/** acquire data with a dataIO device
*   @param [in] self    the dataIO object (python)
*   @param [in] args    number of acquisitions
*   @return             an error if no data could be acquired
*
*   After the device has been initialized and started this method can be used to trigger the acquisition of a
*   data set. The data is then recorded depending on the actually set parameters of the device and can
*   be afterwards retrieved from the device using the \ref getVal method.
*/
PyObject* PythonPlugins::PyDataIOPlugin_acquire(PyDataIOPlugin *self, PyObject *args)
{
    int trigger = 0;
    ito::RetVal ret = ito::retOk;

    if (PyArg_ParseTuple(args, "|i", &trigger) == false)
    {
        return NULL;
    }

    ItomSharedSemaphore *waitCond = new ItomSharedSemaphore();
    QMetaObject::invokeMethod(self->dataIOObj, "acquire", Q_ARG(const int, trigger), Q_ARG(ItomSharedSemaphore *, waitCond));

    while (!waitCond->wait(AppManagement::timeouts.pluginGeneral))
    {
        if (!self->dataIOObj->isAlive())
        {
            break;
        }
    }

    ret += waitCond->returnValue;

    waitCond->deleteSemaphore();
    waitCond = NULL;

    if (!SetReturnValueMessage(ret, "acquire"))
    {
        return NULL;
    }

    Py_RETURN_NONE;
}

//----------------------------------------------------------------------------------------------------------------------------------
PyDoc_STRVAR(PyDataIOPlugin_getVal_doc,"getVal(buffer=`dataObject`|`bytearray`|`bytes` [,length=maxlength]) -> returns shallow copy of internal camera image if `dataObject`-buffer is provided. Else values from plugin are copied to given byte or byte-array buffer. \n\
\n\
Returns a reference (shallow copy) of the recently acquired image (located in the internal memory if the plugin) if the plugin is a grabber or camera and the buffer is a `dataObject`. \n\
Please consider that the values of the `dataObject` might change if a new image is acquired since it is only a reference. Therefore consider copying the `dataObject` or directly use \n\
`copyVal`. \n\
\n\
If no acquisition has been triggered, this method raises a RuntimeError. If the acquisition is not finished yet, this method \n\
blocks and waits until the end of the acquisition. \n\
\n\
If the plugin is another type than a grabber or camera (e.g. serialIO), this method requires any buffer-object that is preallocated with a reasonable size. Then, the currently available \n\
data is copied into this buffer object and the size of the copied data is returned. If the buffer is too small, only the data that fits into the buffer is copied. Another call to \n\
`getVal` will copy the rest. \n\
\n\
Parameters \n\
----------- \n\
buffer : {`dataObject`, `bytearray`, `bytes` or `str`} \n\
    this parameter depends on the type of dataIO-instance: \n\
    \n\
    * cameras, grabber: the buffer must be a `dataObject` (no length parameter): A reference (shallow copy) to the internal memory of the camera plugin is set to the given data object. \
    Therefore its content may change if a new image is being acquired by the camera. Consider taking a deep copy if the image (`dataObject.copy`) or use the method `copyVal`. \n\
    * other IO-devices (AD-converters): The buffer must be an object of type bytearray, bytes or unicode string. The length parameter is then set to the size of the buffers. The effective \
    size of the used memory in buffer is returned. \n\
length : {int}, optional \n\
    size of the given buffer. This value is usually automatically determined and must not be given. \n\
\n\
Returns \n\
-------- \n\
out : {None or int} \n\
    None or size of used buffer if buffer is no `dataObject` \n\
\n\
See Also \n\
--------- \n\
copyVal()");

/** get values from a dataIO device
*   @param [in] self    the dataIO object (python)
*   @param [in] args    input buffer
*   @return             an error if no data could be retrieved
*
*   After a device has been started and an acquisition was triggered the result can be retrieved from the device with the
*   getVal method. As argument the input buffer is passed.
*/
PyObject* PythonPlugins::PyDataIOPlugin_getVal(PyDataIOPlugin *self, PyObject *args)
{
    ito::RetVal ret = ito::retOk;
    PyObject* bufferObj = NULL;
    PythonDataObject::PyDataObject* bufferDataObj = NULL;
    ito::DataObject *dObj = NULL;
    Py_ssize_t length = -1;
    ItomSharedSemaphoreLocker locker;
    unsigned int invokeMethod = -1;
    QSharedPointer<int> maxLength(new int);
    *maxLength = 0;
    QSharedPointer<char> sharedBuffer;
    char* tempBuf = NULL;

    //check whether object is a data object
    if (PyArg_ParseTuple(args, "O!", &PythonDataObject::PyDataObjectType, &bufferDataObj))
    {
        dObj = ((PythonDataObject::PyDataObject *)bufferDataObj)->dataObject;

        if (dObj == NULL)
        {
            PyErr_Format(PyExc_RuntimeError, "given data object is empty (internal dataObject-pointer is NULL)");
            return NULL;
        }

        locker = (new ItomSharedSemaphore());
        QMetaObject::invokeMethod(self->dataIOObj, "getVal", Q_ARG(void *, (void*)dObj), Q_ARG(ItomSharedSemaphore *, locker.getSemaphore()));
        invokeMethod = 1;
    }
    else if (PyErr_Clear(), PyArg_ParseTuple(args, "O|i", &bufferObj, &length))
    {
        if (PyByteArray_Check(bufferObj))
        {
            tempBuf  = (char *)PyByteArray_AsString(bufferObj);
            sharedBuffer = PythonSharedPointerGuard::createPythonSharedPointer<char>(tempBuf, bufferObj);
            *maxLength = static_cast<int>(length < 0 ? PyByteArray_Size(bufferObj) : qMin(PyByteArray_Size(bufferObj),length));
        }
        else if (PyBytes_Check(bufferObj))
        {
            tempBuf  = (char *)PyBytes_AsString(bufferObj);
            sharedBuffer = PythonSharedPointerGuard::createPythonSharedPointer<char>(tempBuf, bufferObj);
            *maxLength = static_cast<int>(length < 0 ? PyBytes_Size(bufferObj) : qMin(PyBytes_Size(bufferObj),length));
        }
        else if (sizeof(Py_UNICODE) == sizeof(char) && PyUnicode_Check(bufferObj))
        {
            tempBuf = (char*)PyUnicode_AS_DATA(bufferObj);
            sharedBuffer = PythonSharedPointerGuard::createPythonSharedPointer<char>(tempBuf, bufferObj);
            *maxLength = static_cast<int>(length < 0 ? (Py_ssize_t)strlen(tempBuf) : qMin((Py_ssize_t)strlen(tempBuf), length));
        }
        else
        {
            PyErr_Format(PyExc_RuntimeError, "arguments of method must be a byte array, byte object or unicode object (only if unicode corresponds to a 8bit char) - in the case that a length value is provided");
            return NULL;
        }

        if (*maxLength <= 0)
        {
            PyErr_Format(PyExc_RuntimeError, "length of given buffer is zero.");
            return NULL;
        }

        locker = (new ItomSharedSemaphore());
        QMetaObject::invokeMethod(self->dataIOObj, "getVal", Q_ARG(QSharedPointer<char>, sharedBuffer), Q_ARG(QSharedPointer<int>, maxLength), Q_ARG(ItomSharedSemaphore *, locker.getSemaphore()));
        invokeMethod = 2;
    }
    else
    {
        PyErr_Clear();
        PyErr_Format(PyExc_RuntimeError, "arguments of method must be either one data object or a byte array, byte object or unicode object (only if unicode corresponds to a 8bit char) followed by an optional maximum length.");
        return NULL;
    }

    while (!locker.getSemaphore()->wait(AppManagement::timeouts.pluginGeneral))
    {
        if (!self->dataIOObj->isAlive())
        {
            break;
        }
    }

    ret += locker.getSemaphore()->returnValue;
    
    if (!SetReturnValueMessage(ret, "getVal"))
    {
        return NULL;
    }

    if (invokeMethod == 1)
    {
        Py_RETURN_NONE; //in case of data-object
    }
    else
    {
        return PyLong_FromLong(*maxLength);
    }
}

//----------------------------------------------------------------------------------------------------------------------------------
PyDoc_STRVAR(PyDataIOPlugin_copyVal_doc,"copyVal(dataObject) -> gets deep copy of data of this plugin, stored in the given data object. \n\
\n\
Returns a deep copy of the recently acquired data (for grabber and ADDA only) of the camera or AD-converter device. \n\
The deep copy sometimes requires one copy operation more than the similar command `getVal`. However, `getVal` only returns \n\
a reference to the plugin internal data structure whose values might be changed if another data acquisition is started. \n\
\n\
If no acquisition has been triggered, this method raises a RuntimeError. If the acquisition is not finished yet, this method \n\
blocks and waits until the end of the acquisition. \n\
\n\
Parameters \n\
----------- \n\
dataObject : {`dataObject`}\n\
    `dataObject` where the plugin data is copied to. Either provide an empty `dataObject` or a `dataObject` whose size (or region of interest) \n\
    exactly has the same size than the available data of the plugin. Therefore you can allocate a 3D data object, set a region of interest \n\
    to one plane such that the data from the plugin is copied into this plane. \n\
\n\
Raises \n\
------- \n\
RuntimeError \n\
    if the dataIO plugin is anything else than ADDA or grabber\n\
    or if no acquisition has been triggered \n\
\n\
See Also \n\
--------- \n\
getVal()");
/** copy values from a dataIO device to an existing dataObject
*   @param [in] self    the dataIO object (python)
*   @param [in] args    input buffer
*   @return             an error if no data could be retrieved
*
*   After a device has been started and an acquisition was triggered the result can be retrieved from the device with the
*   getVal method. As argument the input buffer is passed.
*/
PyObject* PythonPlugins::PyDataIOPlugin_copyVal(PyDataIOPlugin *self, PyObject *args)
{
    int length = PyTuple_Size(args);
    PyObject *tempObj = NULL;
    ito::RetVal ret = ito::retOk;

    if (self->dataIOObj->getBasePlugin()->getType() & ito::typeGrabber)
    {
        if (length != 1)
        {
            PyErr_Format(PyExc_ValueError, "too many parameters");
            return NULL;
        }

        tempObj = PyTuple_GetItem(args, 0);
        ito::DataObject *dObj = NULL;

        if ((Py_TYPE(tempObj) == &PythonDataObject::PyDataObjectType))
        {
            dObj = ((PythonDataObject::PyDataObject *)tempObj)->dataObject;
        }
        else
        {
            PyErr_Format(PyExc_TypeError, "invalid parameters");
            return NULL;
        }

        if (dObj == NULL)
        {
            PyErr_Format(PyExc_ValueError, "invalid dataObject");
            return NULL;
        }

        ItomSharedSemaphoreLocker locker(new ItomSharedSemaphore());

        QMetaObject::invokeMethod(self->dataIOObj, "copyVal", Q_ARG(void*, (void *)dObj), Q_ARG(ItomSharedSemaphore *, locker.getSemaphore()));

        while (!locker.getSemaphore()->wait(AppManagement::timeouts.pluginGeneral))
        {
            if (!self->dataIOObj->isAlive())
            {
                break;
            }
        }

        ret += locker.getSemaphore()->returnValue;

    }
    else if (self->dataIOObj->getBasePlugin()->getType() & ito::typeADDA)
    {
        if (length != 1)
        {
            PyErr_Format(PyExc_ValueError, "too many parameters");
            return NULL;
        }

        ito::DataObject *dObj = NULL;
        tempObj = PyTuple_GetItem(args, 0);

        if ((Py_TYPE(tempObj) == &PythonDataObject::PyDataObjectType))
        {
            dObj = ((PythonDataObject::PyDataObject *)tempObj)->dataObject;
        }

        ItomSharedSemaphoreLocker locker(new ItomSharedSemaphore());
        QMetaObject::invokeMethod(self->dataIOObj, "copyVal", Q_ARG(void *, (void *)dObj), Q_ARG(ItomSharedSemaphore *, locker.getSemaphore()));

        while (!locker.getSemaphore()->wait(AppManagement::timeouts.pluginGeneral))
        {
            if (!self->dataIOObj->isAlive())
            {
                break;
            }
        }

        ret += locker.getSemaphore()->returnValue;
    }
    else
    {
        ret += ito::RetVal(ito::retError, 0, QObject::tr("copyVal function only implemented for typeADDA and typeGrabber").toAscii().data());
    }

    if (!SetReturnValueMessage(ret, "copyVal"))
    {
        return NULL;
    }

    Py_RETURN_NONE;
}

//----------------------------------------------------------------------------------------------------------------------------------
PyDoc_STRVAR(PyDataIOPlugin_setVal_doc,"setVal(dataObjectOrBuffer [, length=1]) -> transfer given `dataObject` to ADDA plugin or further buffer to other dataIO plugin.\n\
\n\
If the dataIO plugin has the subtype ADDA, this method is used to send data to one or more analog outputs of the device. \n\
In this case a `dataObject` must be given as first argument and the second argument `length` must be 1. \n\
\n\
For other dataIO plugins, the first argument must be any buffer object, like `bytearray`, `bytes` or `unicode string`. The length is then extracted \n\
from this value. However it is also possible to define a user-defined size using the 'length' argument. \n\
\n\
Parameters \n\
----------- \n\
dataObjectOrBuffer : {`dataObject`, `bytearray`, `bytes`, `str`}\n\
    value to send to plugin. For an ADDA plugin, a `dataObject` is required whose content is sent to the analogous outputs of the device. For other dataIO \n\
    plugins buffer values like `bytearray`, `bytes` or `unicode string` is required. \n\
length : {int}, optional \n\
    usually this value is not required, since the length of the buffer is automatically extracted from the given objects and 1 for a `dataObject`");

/** write values to a dataIO device
*   @param [in] self    the dataIO object (python)
*   @param [in] args    output buffer
*   @return             an error if no data could be retrieved
*
*   Analog to the \ref getVal method this method writes data to a dataIO device (e.g. a DA converter or serial port).The
*   data passed in the output buffer is written to the device according to its current parameters.
*/
PyObject* PythonPlugins::PyDataIOPlugin_setVal(PyDataIOPlugin *self, PyObject *args)
{
    int length = PyTuple_Size(args);
    PyObject *tempObj = NULL;
    PyObject *tempObj1 = NULL;
    ito::RetVal ret = ito::retOk;

    if (length == 0 || length > 2)
    {
        PyErr_Format(PyExc_ValueError, "invalid number of parameters (1 or 2 arguments requested)");
        return NULL;
    }

    if (self->dataIOObj->getBasePlugin()->getType() & ito::typeADDA)
    {
        ito::DataObject *dObj = NULL;
        int datalen = 0;
        tempObj = PyTuple_GetItem(args, 0);
        if (length > 1)
        {
            tempObj1 = PyTuple_GetItem(args, 1);
            datalen = PyLong_AsLong(tempObj1);
            if (datalen != 1)
            {
                PyErr_Format(PyExc_ValueError, "only one dataobject can be passed");
                return NULL;
            }
        }

        if ((Py_TYPE(tempObj) == &PythonDataObject::PyDataObjectType))
        {
            dObj = ((PythonDataObject::PyDataObject *)tempObj)->dataObject;
        }

        ItomSharedSemaphore *waitCond = new ItomSharedSemaphore();
        QMetaObject::invokeMethod(self->dataIOObj, "setVal", Q_ARG(const void *, (const void *)dObj), Q_ARG(const int, 1), Q_ARG(ItomSharedSemaphore *, waitCond));

        while (!waitCond->wait(AppManagement::timeouts.pluginGeneral))
        {
            if (!self->dataIOObj->isAlive())
            {
                break;
            }
        }

        ret += waitCond->returnValue;

        waitCond->deleteSemaphore();
        waitCond = NULL;
    }
    else
    {
        const char *buf = NULL;
        tempObj = PyTuple_GetItem(args, 0);

        if (length >= 2)
        {
            tempObj1 = PyTuple_GetItem(args, 1);
        }

        QString tempString;
        QByteArray ba;
        int datalen = 0;

        if (PyByteArray_Check(tempObj))
        {
            buf = PyByteArray_AsString(tempObj);
            datalen = PyByteArray_Size(tempObj);
        }
        else if (PyBytes_Check(tempObj))
        {
            buf = PyBytes_AsString(tempObj);
            datalen = PyBytes_Size(tempObj);
        }
        else if (PyUnicode_Check(tempObj))
        {
            //Py_ssize_t stringLengthByte = PyUnicode_GET_DATA_SIZE(tempObj);
            if (sizeof(Py_UNICODE) == sizeof(wchar_t))
            {
                tempString = QString::fromWCharArray((wchar_t*)PyUnicode_AS_DATA(tempObj));
                ba = tempString.toAscii();
                buf = ba.data();
                datalen = ba.length();
            }
            else if (sizeof(Py_UNICODE) == 1)
            {
                buf = PyUnicode_AS_DATA(tempObj);
                datalen = (int)strlen(buf);
            }
            else if (sizeof(Py_UNICODE) == 2)
            {
                tempString = QString::fromUtf16((ushort*)PyUnicode_AS_DATA(tempObj));
                ba = tempString.toAscii();
                buf = ba.data();
                datalen = ba.length();
            }
            else if (sizeof(Py_UNICODE) == 4)
            {
                tempString = QString::fromUcs4((uint*)PyUnicode_AS_DATA(tempObj));
                ba = tempString.toAscii();
                buf = ba.data();
                datalen = ba.length();
            }
            else
            {
                PyErr_Format(PyExc_TypeError, "given unicode must have an element size of 1,2 or 4 bytes. Given is %i.", sizeof(Py_UNICODE));
                return NULL;
            }
        }
        else
        {
            PyErr_Format(PyExc_TypeError, "wrong parameter type (char buffer | byte array)");
            return NULL;
        }

        if (length == 2)
        {
            if (PyLong_Check(tempObj1))
            {
                datalen = PyLong_AsLong(tempObj1);
            }
            else
            {
                PyErr_Format(PyExc_RuntimeError, "given length parameter must be a fixed-point number");
                return NULL;
            }
        }

        ItomSharedSemaphore *waitCond = new ItomSharedSemaphore();
        QMetaObject::invokeMethod(self->dataIOObj, "setVal", Q_ARG(const void *, (const void *)buf), Q_ARG(const int, datalen), Q_ARG(ItomSharedSemaphore *, waitCond));

        while (!waitCond->wait(AppManagement::timeouts.pluginGeneral))
        {
            if (!self->dataIOObj->isAlive())
            {
                break;
            }
        }

        ret += waitCond->returnValue;

        waitCond->deleteSemaphore();
        waitCond = NULL;
    }

    if (!SetReturnValueMessage(ret, "setVal"))
    {
        return NULL;
    }

    Py_RETURN_NONE;
}

//----------------------------------------------------------------------------------------------------------------------------------
PyDoc_STRVAR(PyDataIOPlugin_enableAutoGrabbing_doc,"enableAutoGrabbing() -> enable auto grabbing for the grabber (camera...), \n\
\n\
If the auto grabbing flag is set, the camera acquisition is automatically and continuously triggered if at least one live image is \n\
connected. This is an undesired behaviour if a measurement is started where the acquisition should be controlled by a specific \n\
script or something similar. Then disable the auto grabbing property. All connected live images will then get new images when \n\
`getVal` or `copyVal` is called by the script. The live image timer is disabled. \n\
\n\
This method enables the auto grabbing flag. \n\
\n\
See Also \n\
--------- \n\
setAutoGrabbing(on), disableAutoGrabbing(), getAutoGrabbing()");
               
/** enable timer triggered autograbbing of a dataIO device
*   @param [in] self    the dataIO object (python)
*   @param [in] args    empty
*   @return             an error if autograbbing is not possible
*
*   For live viewing of data incoming from a dataIO device the autograbbing must be enabled. It starts a timer which triggers
*   periodically the data acquisition and pushes the new buffer to the output graph(s).
*/
PyObject *PythonPlugins::PyDataIOPlugin_enableAutoGrabbing(PyDataIOPlugin *self, PyObject * /*args*/)
{
    ito::RetVal ret = ito::retOk;
    ItomSharedSemaphore *waitCond = new ItomSharedSemaphore();
    QMetaObject::invokeMethod(self->dataIOObj, "enableAutoGrabbing", Q_ARG(ItomSharedSemaphore *, waitCond));

    while (!waitCond->wait(AppManagement::timeouts.pluginGeneral))
    {
        if (!self->dataIOObj->isAlive())
        {
            break;
        }
    }

    ret += waitCond->returnValue;

    waitCond->deleteSemaphore();
    waitCond = NULL;

    if (!SetReturnValueMessage(ret, "setVal"))
    {
        return NULL;
    }
    /*
    if (ret != ito::retOk)
    {
        PyErr_Format(PyExc_RuntimeError, QObject::tr("error while enabling the auto grabbing functionality: \n%s\n").toAscii(), QObject::tr(ret.errorMessage()).toAscii().data());
        return NULL;
    }
    */
    Py_RETURN_NONE;
}

//----------------------------------------------------------------------------------------------------------------------------------
PyDoc_STRVAR(PyDataIOPlugin_disableAutoGrabbing_doc,"disableAutoGrabbing() -> Disable auto grabbing for the grabber (camera...), \n\
\n\
If the auto grabbing flag is set, the camera acquisition is automatically and continuously triggered if at least one live image is \n\
connected. This is an undesired behaviour if a measurement is started where the acquisition should be controlled by a specific \n\
script or something similar. Then disable the auto grabbing property. All connected live images will then get new images when \n\
`getVal` or `copyVal` is called by the script. The live image timer is disabled. \n\
\n\
This method disables the auto grabbing flag. \n\
\n\
See Also \n\
--------- \n\
setAutoGrabbing(on), disableAutoGrabbing(), getAutoGrabbing()");                                                  
/** disable timer triggered autograbbing of a dataIO device
*   @param [in] self    the dataIO object (python)
*   @param [in] args    empty
*   @return             an error if autograbbing could not be stopped
*
*   When the autograbbing is stopped the device will no longer periodically acquire data triggered by an internal
*   timer event. Anyway if the connected live view is still present it will receive data recorded by manual
*   acquisitions. This may be usful when a sort of live view is desired within a measurement loop where a
*   timer based autograbbing would disturbe the measurement process.
*/
PyObject *PythonPlugins::PyDataIOPlugin_disableAutoGrabbing(PyDataIOPlugin *self, PyObject * /*args*/)
{
    ito::RetVal ret = ito::retOk;
    ItomSharedSemaphore *waitCond = new ItomSharedSemaphore();
    QMetaObject::invokeMethod(self->dataIOObj, "disableAutoGrabbing", Q_ARG(ItomSharedSemaphore *, waitCond));

    while (!waitCond->wait(AppManagement::timeouts.pluginGeneral))
    {
        if (!self->dataIOObj->isAlive())
        {
            break;
        }
    }

    ret += waitCond->returnValue;

    waitCond->deleteSemaphore();
    waitCond = NULL;

    if (!SetReturnValueMessage(ret, "disableAutoGrabbing"))
    {
        return NULL;
    }

    Py_RETURN_NONE;
}

//----------------------------------------------------------------------------------------------------------------------------------
PyDoc_STRVAR(PyDataIOPlugin_setAutoGrabbing_doc,"setAutoGrabbing(on) -> Set auto grabbing of the grabber device to on or off\n\
\n\
If the auto grabbing flag is set, the camera acquisition is automatically and continuously triggered if at least one live image is \n\
connected. This is an undesired behaviour if a measurement is started where the acquisition should be controlled by a specific \n\
script or something similar. Then disable the auto grabbing property. All connected live images will then get new images when \n\
`getVal` or `copyVal` is called by the script. The live image timer is disabled. \n\
\n\
This method allows setting this flag. \n\
\n\
Parameters \n\
----------- \n\
on : {bool}\n\
    * TRUE = on\n\
    * FALSE = off\n\
\n\
See Also \n\
--------- \n\
enableAutoGrabbing(), disableAutoGrabbing(), getAutoGrabbing()");
PyObject *PythonPlugins::PyDataIOPlugin_setAutoGrabbing(PyDataIOPlugin *self, PyObject * args)
{
    ito::RetVal ret = ito::retOk;
    bool val;

    if (!PyArg_ParseTuple(args, "b", &val))
    {
        return NULL;
    }

    ItomSharedSemaphore *waitCond = new ItomSharedSemaphore();
    if (val)
    {
        QMetaObject::invokeMethod(self->dataIOObj, "enableAutoGrabbing", Q_ARG(ItomSharedSemaphore *, waitCond));
    }
    else
    {
        QMetaObject::invokeMethod(self->dataIOObj, "disableAutoGrabbing", Q_ARG(ItomSharedSemaphore *, waitCond));
    }

    while (!waitCond->wait(AppManagement::timeouts.pluginGeneral))
    {
        if (!self->dataIOObj->isAlive())
        {
            break;
        }
    }

    ret += waitCond->returnValue;

    waitCond->deleteSemaphore();
    waitCond = NULL;

    if (!SetReturnValueMessage(ret, "setAutoGrabbing"))
    {
        return NULL;
    }

    Py_RETURN_NONE;
}

//----------------------------------------------------------------------------------------------------------------------------------
PyDoc_STRVAR(PyDataIOPlugin_getAutoGrabbing_doc,"getAutoGrabbing() -> return the status of the auto grabbing flag. \n\
\n\
If the auto grabbing flag is set, the camera acquisition is automatically and continuously triggered if at least one live image is \n\
connected. This is an undesired behaviour if a measurement is started where the acquisition should be controlled by a specific \n\
script or something similar. Then disable the auto grabbing property. All connected live images will then get new images when \n\
`getVal` or `copyVal` is called by the script. The live image timer is disabled. \n\
\n\
Returns \n\
------- \n\
auto grabbing flag : {bool}\n\
    * False = auto grabbing off \n\
    * True = auto grabbing on. \n\
\n\
See Also \n\
--------- \n\
enableAutoGrabbing(), disableAutoGrabbing(), setAutoGrabbing(bool)");

/** return the status of the autograbbing
*   @param [in] self    the dataIO object (python)
*   @param [in] args    empty
*   @return             the status of the autograbbing
*
*   This method simply returns the status of the autograbbing.
*/
PyObject *PythonPlugins::PyDataIOPlugin_getAutoGrabbing(PyDataIOPlugin *self, PyObject * /*args*/)
{
    if (self->dataIOObj->getAutoGrabbing())
    {
        Py_RETURN_TRUE;
    }
    Py_RETURN_FALSE;
}
//----------------------------------------------------------------------------------------------------------------------------------
PyDoc_STRVAR(PyDataIOPlugin_getType_doc, "getType() -> returns dataIO type");
/** returns the type of the dataIO object
*   @param [in] self    the dataIO object (python)
*   @return             a string with the type
*
*   This method simply returns the type of the dataIO object
*/
PyObject* PythonPlugins::PyDataIOPlugin_getType(PyDataIOPlugin *self)
{
    PyObject *result = NULL;
    if (self == NULL || self->dataIOObj == NULL)
    {
        PyErr_SetString(PyExc_RuntimeError,"empty dataIO plugin");
        return NULL;
    }
    else
    {
        ito::AddInInterfaceBase *aib = self->dataIOObj->getBasePlugin();
        if (aib)
        {
            result = PyLong_FromLong(aib->getType());
        }
        else
        {
            PyErr_SetString(PyExc_RuntimeError,"interface of plugin is NULL");
            return NULL;
        }
    }
    
    return result; 
}

//----------------------------------------------------------------------------------------------------------------------------------
PyObject* PythonPlugins::PyDataIOPlugin_execFunc(PyDataIOPlugin *self, PyObject *args, PyObject *kwds)
{
    return execFunc(self->dataIOObj, args, kwds);
}

//----------------------------------------------------------------------------------------------------------------------------------
/** open configuration dialog
*   @param [in] self    the actuator object (python)
*
*   This method simply open the configuration dialog
*/
PyObject* PythonPlugins::PyDataIOPlugin_showConfiguration(PyDataIOPlugin* self)
{
    ito::AddInBase *aib = self->dataIOObj;
    return plugin_showConfiguration(aib);
}

//----------------------------------------------------------------------------------------------------------------------------------
/** returns the list of available parameters
*   @param [in] self    the actuator object (python)
*
*   This method simply open the widget
*/
PyObject* PythonPlugins::PyDataIOPlugin_showToolbox(PyDataIOPlugin* self)
{
    ito::AddInBase *aib = self->dataIOObj;
    return plugin_showToolbox(aib);
}

//----------------------------------------------------------------------------------------------------------------------------------
/** returns the list of available parameters
*   @param [in] self    the actuator object (python)
*
*   This method simply close the widget
*/
PyObject* PythonPlugins::PyDataIOPlugin_hideToolbox(PyDataIOPlugin* self)
{
    ito::AddInBase *aib = self->dataIOObj;
    return plugin_hideToolbox(aib);
}

//----------------------------------------------------------------------------------------------------------------------------------
PyMemberDef PythonPlugins::PyDataIOPlugin_members[] = {
    {NULL}  /* Sentinel */
};

//----------------------------------------------------------------------------------------------------------------------------------
PyMethodDef PythonPlugins::PyDataIOPlugin_methods[] = {
   {"getParamList", (PyCFunction)PythonPlugins::PyDataIOPlugin_getParamList, METH_NOARGS, pyPluginGetParamList_doc},
   {"getParamListInfo", (PyCFunction)PythonPlugins::PyDataIOPlugin_getParamListInfo, METH_VARARGS, pyPluginGetParamListInfo_doc},
   {"getExecFuncsInfo", (PyCFunction)PythonPlugins::PyDataIOPlugin_getExecFuncsInfo, METH_VARARGS | METH_KEYWORDS, pyPlugInGetExecFuncsInfo_doc},
   {"name", (PyCFunction)PythonPlugins::PyDataIOPlugin_name, METH_NOARGS, pyPluginName_doc},
   {"getParam", (PyCFunction)PythonPlugins::PyDataIOPlugin_getParam, METH_VARARGS, pyPluginGetParam_doc},
   {"setParam", (PyCFunction)PythonPlugins::PyDataIOPlugin_setParam, METH_VARARGS, pyPluginSetParam_doc},
   {"startDevice", (PyCFunction)PythonPlugins::PyDataIOPlugin_startDevice, METH_VARARGS, PyDataIOPlugin_startDevice_doc},
   {"stopDevice", (PyCFunction)PythonPlugins::PyDataIOPlugin_stopDevice, METH_VARARGS, PyDataIOPlugin_stopDevice_doc},
   {"acquire", (PyCFunction)PythonPlugins::PyDataIOPlugin_acquire, METH_VARARGS, PyDataIOPlugin_acquire_doc},
   {"getVal", (PyCFunction)PythonPlugins::PyDataIOPlugin_getVal, METH_VARARGS, PyDataIOPlugin_getVal_doc},
   {"copyVal", (PyCFunction)PythonPlugins::PyDataIOPlugin_copyVal, METH_VARARGS, PyDataIOPlugin_copyVal_doc},
   {"setVal", (PyCFunction)PythonPlugins::PyDataIOPlugin_setVal, METH_VARARGS, PyDataIOPlugin_setVal_doc},
   {"enableAutoGrabbing", (PyCFunction)PythonPlugins::PyDataIOPlugin_enableAutoGrabbing, METH_NOARGS, PyDataIOPlugin_enableAutoGrabbing_doc},
   {"disableAutoGrabbing", (PyCFunction)PythonPlugins::PyDataIOPlugin_disableAutoGrabbing, METH_NOARGS, PyDataIOPlugin_disableAutoGrabbing_doc},
   {"setAutoGrabbing", (PyCFunction)PythonPlugins::PyDataIOPlugin_setAutoGrabbing, METH_VARARGS, PyDataIOPlugin_setAutoGrabbing_doc},
   {"getAutoGrabbing", (PyCFunction)PythonPlugins::PyDataIOPlugin_getAutoGrabbing, METH_NOARGS, PyDataIOPlugin_getAutoGrabbing_doc},
   {"getType", (PyCFunction)PythonPlugins::PyDataIOPlugin_getType, METH_NOARGS, PyDataIOPlugin_getType_doc},
   {"exec", (PyCFunction)PythonPlugins::PyDataIOPlugin_execFunc, METH_KEYWORDS | METH_VARARGS, PyPlugin_execFunc_doc},
   {"showConfiguration", (PyCFunction)PythonPlugins::PyDataIOPlugin_showConfiguration, METH_NOARGS, pyPluginShowConfiguration_doc},
   {"showToolbox", (PyCFunction)PythonPlugins::PyDataIOPlugin_showToolbox, METH_NOARGS, pyPluginShowToolbox_doc},
   {"hideToolbox", (PyCFunction)PythonPlugins::PyDataIOPlugin_hideToolbox, METH_NOARGS, pyPluginHideToolbox_doc},
   {NULL}  /* Sentinel */
};

//----------------------------------------------------------------------------------------------------------------------------------
PyModuleDef PythonPlugins::PyDataIOPluginModule = {
   PyModuleDef_HEAD_INIT,
   "dataIOPlugin",
   QObject::tr("Itom DataIOPlugin type in python").toAscii().data(),
   -1,
   NULL, NULL, NULL, NULL, NULL
};

//----------------------------------------------------------------------------------------------------------------------------------
PyTypeObject PythonPlugins::PyDataIOPluginType = {
   PyVarObject_HEAD_INIT(NULL, 0)
   "itom.dataIO",                       /* tp_name */
   sizeof(PyDataIOPlugin),              /* tp_basicsize */
   0,                                   /* tp_itemsize */
   (destructor)PyDataIOPlugin_dealloc,  /* tp_dealloc */
   0,                                   /* tp_print */
   0,                                   /* tp_getattr */
   0,                                   /* tp_setattr */
   0,                                   /* tp_reserved */
   (reprfunc)PyDataIOPlugin_repr,       /* tp_repr */
   0,                                   /* tp_as_number */
   0,                                   /* tp_as_sequence */
   0,                                   /* tp_as_mapping */
   0,                                   /* tp_hash  */
   0,                                   /* tp_call */
   0,                                   /* tp_str */
   0,                                   /* tp_getattro */
   0,                                   /* tp_setattro */
   0,                                   /* tp_as_buffer */
   Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE,   /* tp_flags */
   pyDataIOInit_doc,                    /* tp_doc */
   0,                                    /* tp_traverse */
   0,                                    /* tp_clear */
   0,                                    /* tp_richcompare */
   0,                                    /* tp_weaklistoffset */
   0,                                    /* tp_iter */
   0,                                    /* tp_iternext */
   PyDataIOPlugin_methods,              /* tp_methods */
   PyDataIOPlugin_members,              /* tp_members */
   0,                                   /* tp_getset */
   0,                                   /* tp_base */
   0,                                   /* tp_dict */
   0,                                   /* tp_descr_get */
   0,                                   /* tp_descr_set */
   0,                                   /* tp_dictoffset */
   (initproc)PythonPlugins::PyDataIOPlugin_init,      /* tp_init */
   0,                                   /* tp_alloc */
   PyDataIOPlugin_new                   /*PyType_GenericNew*/ /*PythonStream_new,*/                 /* tp_new */
};

/*static*/ void PythonPlugins::PyDataIOPlugin_addTpDict(PyObject *tp_dict)
{
    PyObject *value = NULL;

    //add dialog types
    value = Py_BuildValue("i", 0);
    PyDict_SetItemString(tp_dict, "TRIGGER_SOFTWARE", value);
    Py_DECREF(value);
    
}

#if 0 //algo plugins do not exist as instances, they only contain static methods, callable by itom.filter

//----------------------------------------------------------------------------------------------------------------------------------
//----------------------------------------------------------------------------------------------------------------------------------
/** desctructor for algo object in python
*   @param [in] self
*
*   Destructs an algo object (plugin), i.e. deletes the according python variable and invokes
*   the closeAddIn function. The object itself is only deleted if the object's reference
*   counter is zero.
*/
void PythonPlugins::PyAlgoPlugin_dealloc(PyAlgoPlugin* self)
{
    ito::RetVal retval = 0;

    if (self->algoObj)
    {
        ito::AddInInterfaceBase *aib = self->algoObj->getBasePlugin();
        if (!aib)
        {
            PyErr_Format(PyExc_RuntimeError, "error closing plugin");
        }
        else
        {
            ito::AddInManager *aim = ito::AddInManager::getInstance();
            ito::RetVal retval(ito::retOk);

            ItomSharedSemaphore *waitCond = new ItomSharedSemaphore();
            QMetaObject::invokeMethod(aim, "closeAddIn", Q_ARG(ito::AddInBase**, (ito::AddInBase**)&self->algoObj), Q_ARG(ItomSharedSemaphore*, waitCond));
            waitCond->wait(-1);
            retval += waitCond->returnValue;
            waitCond->deleteSemaphore();
            waitCond = NULL;

//            retval = aim->closeAddIn((ito::AddInBase**)&self->algoObj);
            
            PythonCommon::transformRetValToPyException(retval);
        }
    }

    Py_TYPE(self)->tp_free((PyObject*)self);
}

//----------------------------------------------------------------------------------------------------------------------------------
/** constructor for algo object in python
*   @param [in] type
*   @return     new python algo object
*
*   Creates a new pythonAlgo object. The actual algo object (itom) is only created later.
*/
PyObject* PythonPlugins::PyAlgoPlugin_new(PyTypeObject *type, PyObject * /*args*/, PyObject * /*kwds*/)
{
    PyAlgoPlugin *self = NULL;

    self = (PyAlgoPlugin *)type->tp_alloc(type, 0);
    if (self != NULL)
    {
        self->algoObj = NULL;
        self->base = NULL;
    }

    return (PyObject *)self;
}

//----------------------------------------------------------------------------------------------------------------------------------
/** constructor for algo object
*   @param [in] self    the according algo object
*   @param [in] args    unnamed arguments passed to the constructor in python
*   @return             -1 in case an error occured, else 0
*
*   The algo passed must be a valid algo object.
*/
int PythonPlugins::PyAlgoPlugin_init(PyAlgoPlugin *self, PyObject *args, PyObject *kwds)
{
    self->algoObj = NULL;

    int length = PyTuple_Size(args);

    if (length == 0)
    {
        PyErr_Format(PyExc_ValueError, "no plugin specified");
        return -1;
    }
    else if (length == 1) //!< copy constructor or name only
    {
        PyAlgoPlugin* copyPlugin = NULL;

        if (PyArg_ParseTuple(args, "O!", &PyAlgoPluginType, &copyPlugin))
        {
            //try to increment reference of copyPlugin->algoObj
            if (copyPlugin->algoObj)
            {
                copyPlugin->algoObj->getBasePlugin()->incRef(copyPlugin->algoObj);
            }

            self->algoObj = copyPlugin->algoObj;
            self->base = copyPlugin->base;

            return 0;
        }
    }

    PyErr_Clear();
    QVector<ito::Param> *paramsMand = NULL;
    QVector<ito::Param> *paramsOpt = NULL;
    ito::RetVal retval = 0;
    int pluginNum = -1;
    PyObject *pnameObj = NULL;
    PyObject *params = NULL;
    QString pluginName = NULL;
    QVector<ito::ParamBase> paramsMandCpy;
    QVector<ito::ParamBase> paramsOptCpy;

    ito::AddInManager *AIM = ito::AddInManager::getInstance();
    if (!AIM)
    {
        PyErr_Format(PyExc_RuntimeError, "no addin-manager found");
        return -1;
    }

    pnameObj = PyTuple_GetItem(args, 0);
    if (PyUnicode_Check(pnameObj))
    {
        bool ok = false;
        pluginName = PythonQtConversion::PyObjGetString(pnameObj,false,ok);
    }
    else
    {
        PyErr_Format(PyExc_TypeError, "invalid parameters");
        return -1;
    }

    retval = AIM->getInitParams(pluginName, ito::typeAlgo, &pluginNum, paramsMand, paramsOpt);
    if (retval.containsWarningOrError())
    {
        if (retval.errorMessage())
        {
            PyErr_Format(PyExc_RuntimeError, "Could not load plugin: %s with error message: \n%s\n", pluginName.toAscii().data(), retval.errorMessage());
        }
        else
        {
            PyErr_Format(PyExc_RuntimeError, "Could not load plugin: %s\n", pluginName.toAscii().data());
        }
        return -1;
    }


    bool enableAutoLoadParams = false;
    retval = findAndDeleteReservedInitKeyWords(kwds, &enableAutoLoadParams);
    if (retval.containsWarningOrError())
    {
        if (retval.errorMessage())
        {
            PyErr_Format(PyExc_RuntimeError, "Could not load plugin: %s with error message: \n%s\n", pluginName.toAscii().data(), retval.errorMessage());
        }
        else
        {
            PyErr_Format(PyExc_RuntimeError, "Could not load plugin: %s\n", pluginName.toAscii().data());
        }
        return -1;
    }

    params = PyTuple_GetSlice(args, 1, PyTuple_Size(args));

    //retval += copyParamVector(paramsMand, paramsMandCpy);
    //retval += copyParamVector(paramsOpt, paramsOptCpy);

    if (!retval.containsError())
    {

        if (parseInitParams(paramsMand, paramsOpt, params, kwds, paramsMandCpy, paramsOptCpy) != ito::retOk)
        {
            PyErr_Format(PyExc_RuntimeError, "error while parsing parameters.");
            return -1;
        }
        Py_DECREF(params);

        ItomSharedSemaphore *waitCond = new ItomSharedSemaphore();
        QMetaObject::invokeMethod(AIM, "initAddIn", Q_ARG(const int, pluginNum), Q_ARG(const QString&, pluginName), Q_ARG(ito::AddInAlgo**, &self->algoObj), Q_ARG(QVector<ito::ParamBase>*, &paramsMandCpy), Q_ARG(QVector<ito::ParamBase>*, &paramsOptCpy), Q_ARG(bool, enableAutoLoadParams), Q_ARG(ItomSharedSemaphore*, waitCond));
        waitCond->wait(-1);
        retval += waitCond->returnValue;
        waitCond->deleteSemaphore();
        waitCond = NULL;

        paramsMandCpy.clear();
        paramsOptCpy.clear();
    }

    if (!SetLoadPluginReturnValueMessage(retval, pluginName))
    {
        return -1;
    }

    return 0;
}

//----------------------------------------------------------------------------------------------------------------------------------
PyMemberDef PythonPlugins::PyAlgoPlugin_members[] = {
    {NULL}  /* Sentinel */
};

//----------------------------------------------------------------------------------------------------------------------------------
/** Returns the plugin's name
*   @param [in] self    the plugin object
*   @return             the name of the plugin
*
*                       Queries the name of a plugin by invoking a getParam on the plugin for the name parameter
*/
PyObject* PythonPlugins::PyAlgoPlugin_name(PyAlgoPlugin* self)
{
    return getName(self->algoObj);
}

//----------------------------------------------------------------------------------------------------------------------------------
PyDoc_STRVAR(PyAlgoPlugin_getParamList_doc, "getParamList() -> returns list of available parameters.");
/** returns the list of available parameters
*   @param [in] self    the algo object (python)
*   @return             a string with all available parameters for this algorithm
*
*   All parameters of the plugin are shown. This can be useful as there are only few standard parameters
*   for an algorithm. The majority is depending on the actual hardware and accordingly is different for each
*   plugin.
*/
PyObject* PythonPlugins::PyAlgoPlugin_getParamList(PyAlgoPlugin* self)
{
    ito::AddInBase *aib = self->algoObj;
    return getParamList(aib);
}
//----------------------------------------------------------------------------------------------------------------------------------
PyDoc_STRVAR(PyAlgoPlugin_getParamListInfo_doc, "getParamListInfo([detailLevel]) -> plots informations about plugin parameters. \n\
\n\
Parameters \n\
----------- \n\
detailLevel : {dict}, optional \n\
    if dictionary == 1, function returns an Py_Dictionary with parameters \n\
    Default value is 0.\n\
\n\
Returns \n\
------- \n\
Returns none or a PyDictionary depending on the value of detailLevel.\n\
\n\
Notes \n\
----- \n\
\n\
Generates an online help for available parameters and additional informations of the plugin.");  

/** returns the list of available parameters and additional information about the plugin
*   @param [in] self    the algo object (python)
*   @return             a string with all available parameters for this algo
*
*   All parameters of the plugin are shown with additional information as min, max and infostring.
*   This can be useful as there are only few standard parameters for an actuator. The majority is
*   depending on the actual hardware and accordingly is different for each plugin.
*/
PyObject* PythonPlugins::PyAlgoPlugin_getParamListInfo(PyAlgoPlugin* self, PyObject *args)
{
    ito::AddInBase *aib = self->algoObj;

    return getParamListInfo(aib, args);
}

/** returns the list of available parameters and additional information about the plugin ExecFunctions
*   @param [in] self    the algorithm object (python)
*   @return             a dictionary with all available parameters for this actuator
*
*   All ExecFunctions of the plugin are shown or one specific ExecFunctions with additional information as min, max and infostring is shown.
*   This can be useful as there are only few standard parameters for an dataIO. The majority is
*   depending on the actual hardware and accordingly is different for each plugin.
*
*   This function os obsolet because currently no algo has an living instance in python
*/
PyObject* PythonPlugins::PyAlgoPlugin_getExecFuncsInfo(PyAlgoPlugin* self, PyObject *args, PyObject *kwds)
{
    ito::AddInBase *aib = self->algoObj;
    return getExecFuncsInfo(aib, args, kwds);
}

//----------------------------------------------------------------------------------------------------------------------------------
/** return a parameter value
*   @param [in] self        the addIn whoes parameter is requested
*   @param [in] args        the parameter name
*   @return     python object with the parameter value on success (parameter exists), NULL otherwise
*
*   The function tries to retrieve the value of the parameter with the name given in args. If the parameter does not exist
*   NULL is returned. To actually retrieve the value the getParam function of the plugin is invoked.
*/
PyObject* PythonPlugins::PyAlgoPlugin_getParam(PyAlgoPlugin *self, PyObject *args)
{
    return getParam(self->algoObj, args);
}

//----------------------------------------------------------------------------------------------------------------------------------
/** set a parameter to a new value
*   @param [in] self    the algo object (python)
*   @param [in] args    the parameter name and new value
*   @return             an error if the parameter wasn't found or the passed value is out of the limits
*
*   The setParam method of the plugin is invoked and the parameter is set to the new value in case the passed value
*   is within the limits. This method effects only parameters of a "whole" algo plugin - not single parameters of a
*   destinct filter.
*/
PyObject* PythonPlugins::PyAlgoPlugin_setParam(PyAlgoPlugin *self, PyObject *args)
{
    return setParam(self->algoObj, args);
}

//----------------------------------------------------------------------------------------------------------------------------------
PyDoc_STRVAR(PyAlgoPlugin_getType_doc, "getType() -> returns AlgoPlugin type");
/** returns the type of the Algo object
*   @param [in] self    the Algo object (python)
*   @return             a string with the type
*
*   This method simply returns the type of the Algo object
*/
PyObject* PythonPlugins::PyAlgoPlugin_getType(PyAlgoPlugin *self)
{
    PyObject *result = NULL;
    if (self == NULL || self->algoObj == NULL)
    {
        PyErr_SetString(PyExc_RuntimeError,"empty Algo plugin");
        return NULL;
    }
    else
    {
        ito::AddInInterfaceBase *aib = self->algoObj->getBasePlugin();
        if (aib)
        {
            result = PyLong_FromLong(aib->getType());
        }
        else
        {
            PyErr_SetString(PyExc_RuntimeError,"interface of plugin is NULL");
            return NULL;
        }
    }
    
    return result; 
}

//----------------------------------------------------------------------------------------------------------------------------------
PyMethodDef PythonPlugins::PyAlgoPlugin_methods[] = {
   {"getParamList", (PyCFunction)PythonPlugins::PyAlgoPlugin_getParamList, METH_NOARGS, pyPluginGetParamList_doc},
   {"getParamListInfo", (PyCFunction)PythonPlugins::PyAlgoPlugin_getParamListInfo, METH_VARARGS, PyAlgoPlugin_getParamListInfo_doc},
   {"getExecFuncsInfo", (PyCFunction)PythonPlugins::PyAlgoPlugin_getExecFuncsInfo, METH_VARARGS | METH_KEYWORDS, pyPlugInGetExecFuncsInfo_doc},
   {"name", (PyCFunction)PythonPlugins::PyAlgoPlugin_name, METH_NOARGS, "name() -> returns name of algorithm plugin"},
   {"getParam", (PyCFunction)PythonPlugins::PyAlgoPlugin_getParam, METH_VARARGS, "getParam(name) -> returns value of given parameter"},
   {"setParam", (PyCFunction)PythonPlugins::PyAlgoPlugin_setParam, METH_VARARGS, "setParam(name,value) -> sets value of given parameter"},
   {"getType", (PyCFunction)PythonPlugins::PyAlgoPlugin_getType, METH_NOARGS, PyAlgoPlugin_getType_doc},
   {NULL}  /* Sentinel */
};

//----------------------------------------------------------------------------------------------------------------------------------
PyModuleDef PythonPlugins::PyAlgoPluginModule = {
   PyModuleDef_HEAD_INIT,
   "algorithmPlugin",
   QObject::tr("Itom AlgorithmPlugin type in python").toAscii().data(),
   -1,
   NULL, NULL, NULL, NULL, NULL
};

//----------------------------------------------------------------------------------------------------------------------------------
PyTypeObject PythonPlugins::PyAlgoPluginType = {
   PyVarObject_HEAD_INIT(NULL, 0)
   "itom.algo",             /* tp_name */
   sizeof(PyAlgoPlugin),             /* tp_basicsize */
   0,                         /* tp_itemsize */
   (destructor)PyAlgoPlugin_dealloc, /* tp_dealloc */
   0,                         /* tp_print */
   0,                         /* tp_getattr */
   0,                         /* tp_setattr */
   0,                         /* tp_reserved */
   0,                         /* tp_repr */
   0,                         /* tp_as_number */
   0,                         /* tp_as_sequence */
   0,                         /* tp_as_mapping */
   0,                         /* tp_hash  */
   0,                         /* tp_call */
   0,                         /* tp_str */
   0,                         /* tp_getattro */
   0,                         /* tp_setattro */
   0,                         /* tp_as_buffer */
   Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE,   /* tp_flags */
   "algo plugins",           /* tp_doc */
   0,                       /* tp_traverse */
   0,                       /* tp_clear */
   0,                       /* tp_richcompare */
   0,                       /* tp_weaklistoffset */
   0,                       /* tp_iter */
   0,                       /* tp_iternext */
   PyAlgoPlugin_methods,             /* tp_methods */
   PyAlgoPlugin_members,             /* tp_members */
   0,                         /* tp_getset */
   0,                         /* tp_base */
   0,                         /* tp_dict */
   0,                         /* tp_descr_get */
   0,                         /* tp_descr_set */
   0,                         /* tp_dictoffset */
   (initproc)PythonPlugins::PyAlgoPlugin_init,      /* tp_init */
   0,                         /* tp_alloc */
   PyAlgoPlugin_new /*PyType_GenericNew*/ /*PythonStream_new,*/                 /* tp_new */
};

#endif 

} //end namespace ito

//----------------------------------------------------------------------------------------------------------------------------------
