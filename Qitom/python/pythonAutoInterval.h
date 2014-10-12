/* ********************************************************************
    itom software
    URL: http://www.uni-stuttgart.de/ito
    Copyright (C) 2013, Institut fuer Technische Optik (ITO),
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

#ifndef PYTHONAUTOINTERVAL
#define PYTHONAUTOINTERVAL

/* includes */
#ifndef Q_MOC_RUN
    #define PY_ARRAY_UNIQUE_SYMBOL itom_ARRAY_API //see numpy help ::array api :: Miscellaneous :: Importing the api (this line must bebefore include global.h)
    #define NO_IMPORT_ARRAY
    //python
    // see http://vtk.org/gitweb?p=VTK.git;a=commitdiff;h=7f3f750596a105d48ea84ebfe1b1c4ca03e0bab3
    #if (defined _DEBUG) && (!defined linux)
        #undef _DEBUG
        #include "Python.h" 
        #define _DEBUG
    #else
        #include "Python.h"   
    #endif
#endif

#include "../../common/typeDefs.h"
#include "../../common/interval.h"
#include "structmember.h"
#include <qobject.h>

namespace ito 
{
class PythonAutoInterval
    {

    public:

        //-------------------------------------------------------------------------------------------------
        // typedefs
        //------------------------------------------------------------------------------------------------- 
        typedef struct
        {
            PyObject_HEAD
            ito::AutoInterval interval;
        }
        PyAutoInterval;


        #define PyAutoInterval_Check(op) PyObject_TypeCheck(op, &ito::PythonAutoInterval::PyAutoIntervalType)

        
        //-------------------------------------------------------------------------------------------------
        // constructor, deconstructor, alloc, dellaoc
        //------------------------------------------------------------------------------------------------- 

        static void PyAutoInterval_dealloc(PyAutoInterval *self);
        static PyObject *PyAutoInterval_new(PyTypeObject *type, PyObject *args, PyObject *kwds);
        static int PyAutoInterval_init(PyAutoInterval *self, PyObject *args, PyObject *kwds);

        static PyAutoInterval* createEmptyPyAutoInterval();


        //-------------------------------------------------------------------------------------------------
        // general members
        //------------------------------------------------------------------------------------------------- 
        static PyObject *PyAutoInterval_name(PyAutoInterval *self);

        static PyObject* PyAutoInterval_repr(PyAutoInterval *self);

        static PyObject* PyAutoInterval_RichCompare(PyAutoInterval *self, PyObject *other, int cmp_op);

        static PyGetSetDef PyAutoInterval_getseters[];

        static PyObject* PyAutoInterval_getValue(PyAutoInterval *self, void *closure);
        static int PyAutoInterval_setValue(PyAutoInterval *self, PyObject *value, void *closure);

        static PyObject* PyAutoInterval_Reduce(PyAutoInterval *self, PyObject *args);
        static PyObject* PyAutoInterval_SetState(PyAutoInterval *self, PyObject *args);

        
        //-------------------------------------------------------------------------------------------------
        // type structures
        //------------------------------------------------------------------------------------------------- 
        static PyMemberDef PyAutoInterval_members[];
        static PyMethodDef PyAutoInterval_methods[];
        static PyTypeObject PyAutoIntervalType;
        static PyModuleDef PyAutoIntervalModule;

        //-------------------------------------------------------------------------------------------------
        // helper methods
        //-------------------------------------------------------------------------------------------------    

        //-------------------------------------------------------------------------------------------------
        // static type methods
        //-------------------------------------------------------------------------------------------------


};

} //end namespace ito

#endif
