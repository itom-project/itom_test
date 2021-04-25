/* ********************************************************************
    itom software
    URL: http://www.uni-stuttgart.de/ito
    Copyright (C) 2020, Institut fuer Technische Optik (ITO),
    Universitaet Stuttgart, Germany

    This file is part of itom and its software development toolkit (SDK).

    itom is free software; you can redistribute it and/or modify it
    under the terms of the GNU Library General Public Licence as published by
    the Free Software Foundation; either version 2 of the Licence, or (at
    your option) any later version.

    In addition, as a special exception, the Institut fuer Technische
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

#include "../param.h"
#include "../sharedStructures.h"

#include "../numeric.h"
#include <assert.h>

namespace ito {

//-------------------------------------------------------------------------------------
/* converts a flags value, e.g. ParamBase::Readonly | ParamBase::Out to the
   internal 16bit representation
*/
constexpr uint16 toFlagsInternal(uint32 flags)
{
    return (uint16)((flags & ito::paramFlagMask) >> 16);
}

//-------------------------------------------------------------------------------------
/* converts a m_flags value to its representation using the ParamBase::Flags enum flag
 */
constexpr uint32 toFlagsExternal(uint16 flags)
{
    return ((uint32)flags) << 16;
}

//-------------------------------------------------------------------------------------
ParamBase::ParamBase() :
    m_type(0), m_flags(0), m_name(nullptr), m_dVal(0.0, 0.0), m_iVal(0), m_cVal(nullptr)
{
    setDefaultAutosave();
}

//-------------------------------------------------------------------------------------
/** constructor with name only
 *   @param [in] name  name of new ParamBase
 *   @return     new   ParamBase name "name"
 *
 *   creates a new ParamBase with name "name", string is copied
 */
ParamBase::ParamBase(const ByteArray& name) :
    m_type(0), m_flags(0), m_name(name), m_dVal(0.0, 0.0), m_iVal(0), m_cVal(nullptr)
{
    inOutCheck();
    setDefaultAutosave();
}

//-------------------------------------------------------------------------------------
/** constructor with name and type
 *   @param [in] name  name of new ParamBase
 *   @param [in] type  type of new ParamBase for possible types see \ref Type
 *   @return     new Param with name and type
 *
 *   creates a new Param with name and type, string is copied
 */
ParamBase::ParamBase(const ByteArray& name, const uint32 typeAndFlags) :
    m_type(typeAndFlags & ito::paramTypeMask), m_flags(toFlagsInternal(typeAndFlags)), m_name(name),
    m_dVal(0.0, 0.0), m_iVal(0), m_cVal(nullptr)
{
    inOutCheck();
    setDefaultAutosave();
}

//-------------------------------------------------------------------------------------
/** constructor with name and type, char val and optional info
 *   @param [in] name  name of new ParamBase
 *   @param [in] type  type of new ParamBase for possible types see \ref Type
 *   @param [in] val   character pointer to string pointer
 *   @param [in] info  character pointer to string pointer holding information about this ParamBase
 *   @return     new ParamBase with name, type, string value
 *
 *   creates a new ParamBase with name, type, string value. Strings are copied
 */
ParamBase::ParamBase(const ByteArray& name, const uint32 typeAndFlags, const char* val) :
    m_type(typeAndFlags & ito::paramTypeMask), m_flags(toFlagsInternal(typeAndFlags)), m_name(name),
    m_dVal(0.0, 0.0), m_iVal(-1), m_cVal(nullptr)
{
    if (val)
    {
        if (m_type == String)
        {
            m_cVal = new char[strlen(val) + 1];
            memcpy(m_cVal, val, strlen(val) + 1);
            m_iVal = static_cast<int>(strlen((char*)m_cVal));
        }
        else
        {
            m_cVal = const_cast<char*>(val);
            m_iVal = -1;
        }
    }

    inOutCheck();
    setDefaultAutosave();
}

//-------------------------------------------------------------------------------------
/** constructor with name and type, float64 val
 *   @param [in] name   name of new ParamBase
 *   @param [in] type   type of new ParamBase for possible types see \ref Type
 *   @param [in] val    actual value
 *   @return     new ParamBase with name, type and val
 *
 *   creates a new ParamBase with name, type and val. Strings are copied.
 */
ParamBase::ParamBase(const ByteArray& name, const uint32 typeAndFlags, const float64 val) :
    m_type(typeAndFlags & ito::paramTypeMask), m_flags(toFlagsInternal(typeAndFlags)), m_name(name),
    m_dVal(0.0, 0.0), m_iVal(0), m_cVal(nullptr)
{
    switch (m_type)
    {
    case Char:
        m_iVal = (char)val;
        break;
    case Int:
        m_iVal = (int)val;
        break;
    case Complex:
    case Double:
        m_dVal.real = val;
        break;
    default:
        throw std::logic_error(
            "constructor with float64 val is only callable for types Int, Complex and Double");
        break;
    }

    inOutCheck();
    setDefaultAutosave();
}

//-------------------------------------------------------------------------------------
/** constructor with name and type and int val
 *   @param [in] name   name of new ParamBase
 *   @param [in] type   type of new ParamBase for possible types see \ref Type
 *   @param [in] val    actual value
 *   @return     new ParamBase with name, type andval.
 *
 *   creates a new ParamBase with name, type and val
 */
ParamBase::ParamBase(const ByteArray& name, const uint32 typeAndFlags, const int32 val) :
    m_type(typeAndFlags & ito::paramTypeMask), m_flags(toFlagsInternal(typeAndFlags)), m_name(name),
    m_dVal(0.0, 0.0), m_iVal(0), m_cVal(nullptr)
{
    inOutCheck();
    setDefaultAutosave();

    switch (m_type)
    {
    case Char:
        m_iVal = (char)val;
        break;
    case Int:
        m_iVal = val;
        break;
    case Complex:
    case Double:
        m_dVal.real = (float64)val;
        break;
    case String:
        if (val == 0)
        {
            m_iVal = -1;
            m_cVal = nullptr;
        }
        else
        {
            throw std::runtime_error(
                "constructor with int val and String type is not callable for val != nullptr");
        }
        break;
    case HWRef:
        if (val == 0)
        {
            m_iVal = -1;
            m_cVal = nullptr;
        }
        else
        {
            throw std::runtime_error(
                "constructor with int val and Hardware type is not callable for val != nullptr");
        }
        break;
    default:
        throw std::runtime_error(
            "constructor with int32 val is only callable for types Int, Complex, Double, String "
            "(for val==0 only) and Hardware (for val==0 only)");
        break;
    }
}

//-------------------------------------------------------------------------------------
/** constructor with name and type, complex128 val
 *   @param [in] name   name of new ParamBase
 *   @param [in] type   type of new ParamBase for possible types see \ref Type
 *   @param [in] val    actual value
 *   @return     new ParamBase with name, type and val
 *
 *   creates a new ParamBase with name, type and val. Strings are copied.
 */
ParamBase::ParamBase(const ByteArray& name, const uint32 typeAndFlags, const complex128 val) :
    m_type(typeAndFlags & ito::paramTypeMask), m_flags(toFlagsInternal(typeAndFlags)), m_name(name),
    m_dVal(0.0, 0.0), m_iVal(0), m_cVal(nullptr)
{
    switch (m_type)
    {
    case Complex:
        m_dVal.real = val.real();
        m_dVal.imag = val.imag();
        break;
    default:
        throw std::logic_error("constructor with complex128 val is only callable for type Complex");
        break;
    }

    inOutCheck();
    setDefaultAutosave();
}

//-------------------------------------------------------------------------------------
/** constructor with name and type, and a list of strings
 *   @param [in] name   name of new ParamBase
 *   @param [in] type   type of new ParamBase for possible types see \ref Type
 *   @param [in] val    actual value
 *   @return     new ParamBase with name, type and val
 *
 *   creates a new ParamBase with name, type and val. Strings are copied.
 */
ParamBase::ParamBase(
    const ByteArray& name, const uint32 typeAndFlags, const uint32 size, const ByteArray* values) :
    m_type(typeAndFlags & ito::paramTypeMask),
    m_flags(toFlagsInternal(typeAndFlags)), m_name(name), m_dVal(0.0, 0.0), m_iVal(0),
    m_cVal(nullptr)
{
    switch (m_type)
    {
    case StringList: {
        m_iVal = size;
        ByteArray* buf = new ByteArray[size];

        for (uint32 i = 0; i < size; ++i)
        {
            buf[i] = ByteArray(values[i]);
        }

        m_cVal = buf;
        break;
    }
    default:
        throw std::logic_error(
            "constructor with ByteArray values is only callable for type StringList");
        break;
    }

    inOutCheck();
    setDefaultAutosave();
}

//-------------------------------------------------------------------------------------
/** array constructor with name and type, size and array
 *   @param [in] name   name of new ParamBase
 *   @param [in] type   type of new ParamBase for possible types see \ref Type
 *   @param [in] size   array size
 *   @param [in] val    values
 *   @return     new ParamBase (array) with name, type, size and values.
 *
 *   creates a new ParamBase (array) with name, type, size and values.
 */
ParamBase::ParamBase(
    const ByteArray& name, const uint32 typeAndFlags, const unsigned int size, const char* values) :
    m_type(typeAndFlags & ito::paramTypeMask),
    m_flags(toFlagsInternal(typeAndFlags)), m_name(name), m_dVal(0.0, 0.0), m_iVal(size),
    m_cVal(nullptr)
{
    inOutCheck();
    setDefaultAutosave();

    if (values == nullptr)
    {
        m_iVal = -1;
        m_cVal = nullptr;
    }
    else
    {
        switch (m_type)
        {
        case String:
            m_cVal = new char[strlen(values) + 1];
            memcpy(m_cVal, values, strlen(values) + 1);
            m_iVal = static_cast<int>(strlen((char*)m_cVal));
            break;

        case CharArray:
            m_iVal = size;
            if (m_iVal > 0)
            {
                m_cVal = new char[size];
                memcpy(m_cVal, values, size * sizeof(char));
            }
            else
            {
                m_cVal = 0;
            }
            break;

        case IntArray:
            m_iVal = size;
            if (m_iVal > 0)
            {
                m_cVal = new int32[size];
                memcpy(m_cVal, values, size * sizeof(int32));
            }
            else
            {
                m_cVal = 0;
            }
            break;

        case DoubleArray:
            m_iVal = size;
            if (m_iVal > 0)
            {
                m_cVal = new float64[size];
                memcpy(m_cVal, values, size * sizeof(float64));
            }
            else
            {
                m_cVal = 0;
            }
            break;

        case ComplexArray:
            m_iVal = size;
            if (m_iVal > 0)
            {
                m_cVal = new complex128[size];
                memcpy(m_cVal, values, size * sizeof(complex128));
            }
            else
            {
                m_cVal = 0;
            }
            break;

        case HWRef:
        case DObjPtr:
        case Pointer:
        case PointCloudPtr:
        case PointPtr:
        case PolygonMeshPtr:
            m_cVal = const_cast<char*>(values);
            m_iVal = -1;
            break;

        default:
            m_cVal = nullptr;
            m_iVal = -1;
            break;
        }
    }
}

//-------------------------------------------------------------------------------------
/** array constructor with name and type, size and array
 *   @param [in] name   name of new ParamBase
 *   @param [in] type   type of new ParamBase for possible types see \ref Type
 *   @param [in] size   array size
 *   @param [in] val    values
 *   @return     new ParamBase (array) with name, type, size and values.
 *
 *   creates a new ParamBase (array) with name, type, size and values.
 */
ParamBase::ParamBase(
    const ByteArray& name,
    const uint32 typeAndFlags,
    const unsigned int size,
    const int32* values) :
    m_type(typeAndFlags & ito::paramTypeMask),
    m_flags(toFlagsInternal(typeAndFlags)), m_name(name), m_dVal(0.0, 0.0), m_iVal(size),
    m_cVal(nullptr)
{
    inOutCheck();
    setDefaultAutosave();

    if ((size <= 0) || (values == nullptr))
    {
        m_iVal = -1;
        m_cVal = nullptr;
    }
    else
    {
        switch (m_type)
        {
        case CharArray:
            throw std::invalid_argument("int array cannot be converted to char array");
            m_iVal = -1;
            m_cVal = 0;
            break;
        case IntArray:
            m_iVal = size;
            if (m_iVal > 0)
            {
                m_cVal = new int32[size];
                memcpy(m_cVal, values, size * sizeof(int32));
            }
            else
            {
                m_cVal = 0;
            }
            break;
        case DoubleArray:
            m_iVal = size;
            if (m_iVal > 0)
            {
                m_cVal = new float64[size];
                memcpy(m_cVal, values, size * sizeof(float64));
            }
            else
            {
                m_iVal = 0;
            }
            break;
        case ComplexArray:
            m_iVal = size;
            if (m_iVal > 0)
            {
                m_cVal = new complex128[size];
                memcpy(m_cVal, values, size * sizeof(complex128));
            }
            else
            {
                m_iVal = 0;
            }
            break;
        }
    }
}

//-------------------------------------------------------------------------------------
/** array constructor with name and type, size and array
 *   @param [in] name   name of new ParamBase
 *   @param [in] type   type of new ParamBase for possible types see \ref Type
 *   @param [in] size   array size
 *   @param [in] val    values
 *   @return     new ParamBase (array) with name, type, size and values.
 *
 *   creates a new ParamBase (array) with name, type, size and values.
 */
ParamBase::ParamBase(
    const ByteArray& name,
    const uint32 typeAndFlags,
    const unsigned int size,
    const float64* values) :
    m_type(typeAndFlags & ito::paramTypeMask),
    m_flags(toFlagsInternal(typeAndFlags)), m_name(name), m_dVal(0.0, 0.0), m_iVal(size),
    m_cVal(nullptr)
{
    inOutCheck();
    setDefaultAutosave();

    if ((size <= 0) || (values == nullptr))
    {
        m_iVal = -1;
        m_cVal = nullptr;
    }
    else
    {
        switch (m_type)
        {
        case CharArray:
            throw std::invalid_argument("int32 array cannot be converted to char array");
            m_iVal = -1;
            m_cVal = 0;
            break;
        case DoubleArray:
            m_iVal = size;
            if (m_iVal > 0)
            {
                m_cVal = new float64[size];
                memcpy(m_cVal, values, size * sizeof(float64));
            }
            else
            {
                m_cVal = 0;
            }
            break;
        case ComplexArray:
            m_iVal = size;
            if (m_iVal > 0)
            {
                m_cVal = new complex128[size];
                for (unsigned int n = 0; n < size; n++)
                    reinterpret_cast<complex128*>(m_cVal)[n] = static_cast<complex128>(values[n]);
            }
            else
            {
                m_iVal = 0;
            }
            break;
        case IntArray:
            throw std::invalid_argument("double array cannot be converted to char array");
            m_iVal = -1;
            m_cVal = 0;
            break;
        }
    }
}

//-------------------------------------------------------------------------------------
/** array constructor with name and type, size and array
 *   @param [in] name   name of new ParamBase
 *   @param [in] type   type of new ParamBase for possible types see \ref Type
 *   @param [in] size   array size
 *   @param [in] val    values
 *   @return     new ParamBase (array) with name, type, size and values.
 *
 *   creates a new ParamBase (array) with name, type, size and values.
 */
ParamBase::ParamBase(
    const ByteArray& name,
    const uint32 typeAndFlags,
    const unsigned int size,
    const complex128* values) :
    m_type(typeAndFlags & ito::paramTypeMask),
    m_flags(toFlagsInternal(typeAndFlags)), m_name(name), m_dVal(0.0, 0.0), m_iVal(size),
    m_cVal(nullptr)
{
    inOutCheck();
    setDefaultAutosave();

    if ((size <= 0) || (values == nullptr))
    {
        m_iVal = -1;
        m_cVal = nullptr;
    }
    else
    {
        switch (m_type)
        {
        case CharArray:
            m_iVal = -1;
            m_cVal = 0;
            throw std::invalid_argument("complex128 array cannot be converted to char array");
            break;
        case DoubleArray:
            m_iVal = -1;
            m_cVal = 0;
            throw std::invalid_argument("complex128 array cannot be converted to float64 array");
            break;
        case ComplexArray:
            m_iVal = size;
            if (m_iVal > 0)
            {
                m_cVal = new complex128[size];
                memcpy(m_cVal, values, size * sizeof(complex128));
            }
            else
            {
                m_iVal = 0;
            }
            break;
        case IntArray:
            m_iVal = -1;
            m_cVal = 0;
            throw std::invalid_argument("complex128 array cannot be converted to int32 array");
            break;
        }
    }
}

//-------------------------------------------------------------------------------------
/** destructor
 *
 *   clear (frees) the name and in case a string value.
 */
ParamBase::~ParamBase()
{
    // 01.12.16 crash when trying to free a fixed pointer assigned to a paramBase copied to another
    // via = operator, using broken assignment. Guess we must not free pointers, when ival == -1
    // int useLim[10] = {1, 0 ,0, 0, 0, 0};
    // (*param) = ito::ParamBase("useLimits", ito::ParamBase::IntArray, (char*)useLim);
    // but if we do not free we might provocate memory leaks ... needs review
    freeMemory();
}

//-------------------------------------------------------------------------------------
//!< depending on the type, set the default value for the autosave flag.
void ParamBase::setDefaultAutosave()
{
    switch (m_type)
    {
    case ito::ParamBase::DObjPtr:
    case ito::ParamBase::PointCloudPtr:
    case ito::ParamBase::PolygonMeshPtr:
    case ito::ParamBase::PointPtr:
    case ito::ParamBase::HWRef:
        m_flags |= toFlagsInternal(ito::ParamBase::NoAutosave);
        break;
    default:
        m_flags &= ~toFlagsInternal(ito::ParamBase::NoAutosave);
        break;
    }
}

//-------------------------------------------------------------------------------------
void ParamBase::freeMemory()
{
    if (m_cVal)
    {
        switch (m_type)
        {
        case String:
        case CharArray:
            delete[]((char*)m_cVal);
            break;
        case DoubleArray:
            delete[]((ito::float64*)m_cVal);
            break;
        case IntArray:
            delete[]((ito::int32*)m_cVal);
            break;
        case ComplexArray:
            delete[]((ito::complex128*)m_cVal);
            break;
        case StringList:
            delete[]((ito::ByteArray*)m_cVal);
            break;
        }

        m_cVal = nullptr;
        m_iVal = 0;
    }
}

//-------------------------------------------------------------------------------------
/** copy constructor
 *   @param [in] copyConstr ParamBase to copy from
 *   @return     new ParamBase with copied values
 *
 *   creates ParamBase according to passed Param, strings are copied
 */
ParamBase::ParamBase(const ParamBase& copyConstr) :
    m_type(copyConstr.m_type), m_flags(copyConstr.m_flags), m_name(copyConstr.m_name),
    m_dVal(copyConstr.m_dVal), m_iVal(copyConstr.m_iVal), m_cVal(nullptr)
{
    switch (copyConstr.m_type)
    {
    case String:
        if (copyConstr.m_cVal)
        {
            m_cVal = new char[strlen((char*)copyConstr.m_cVal) + 1];
            memcpy(m_cVal, copyConstr.m_cVal, strlen((char*)copyConstr.m_cVal) + 1);
        }
        break;

    case CharArray:
        if (m_iVal > 0)
        {
            m_cVal = new char[m_iVal];
            memcpy(m_cVal, copyConstr.m_cVal, m_iVal * sizeof(char));
        }
        break;

    case IntArray:
        if (m_iVal > 0)
        {
            m_cVal = new int32[m_iVal];
            memcpy(m_cVal, copyConstr.m_cVal, m_iVal * sizeof(int32));
        }
        break;

    case DoubleArray:
        if (m_iVal > 0)
        {
            m_cVal = new float64[m_iVal];
            memcpy(m_cVal, copyConstr.m_cVal, m_iVal * sizeof(float64));
        }
        break;

    case ComplexArray:
        if (m_iVal > 0)
        {
            m_cVal = new complex128[m_iVal];
            memcpy(m_cVal, copyConstr.m_cVal, m_iVal * sizeof(complex128));
        }
        break;

    case StringList:
        if (m_iVal > 0)
        {
            m_cVal = new ByteArray[m_iVal];
            for (int i = 0; i < m_iVal; ++i)
            {
                ((ByteArray*)(m_cVal))[i] = ((ByteArray*)(copyConstr.m_cVal))[i];
            }
        }
        break;

    case HWRef:
    case DObjPtr:
    case Pointer:
    case PointCloudPtr:
    case PointPtr:
    case PolygonMeshPtr:
        m_cVal = copyConstr.m_cVal;
        break;

    default:
        break;
    }
}

//-------------------------------------------------------------------------------------
bool ParamBase::operator==(const ParamBase& rhs) const
{
    if ((m_type) == (rhs.m_type))
    {
        switch (m_type)
        {
        case 0:
            return true; // both are invalid ParamBase objects.
        case Int:
        case Char:
            return (m_iVal == rhs.m_iVal);

        case Double:
        case Complex:
            return ito::areEqual(m_dVal.real, rhs.m_dVal.real) &&
                ito::areEqual(m_dVal.imag, rhs.m_dVal.imag);

        case String:
            if (m_cVal && rhs.m_cVal)
            {
                return (strcmp((char*)m_cVal, (char*)rhs.m_cVal) == 0);
            }
            else
            {
                return m_cVal == rhs.m_cVal;
            }

        case CharArray:
            if (m_iVal > 0 && rhs.m_iVal > 0)
            {
                return (memcmp(m_cVal, rhs.m_cVal, m_iVal * sizeof(char)) == 0);
            }
            else
            {
                return (m_iVal <= 0) && (rhs.m_iVal <= 0);
            }

        case IntArray:
            if (m_iVal > 0 && (m_iVal == rhs.m_iVal))
            {
                return (memcmp(m_cVal, rhs.m_cVal, m_iVal * sizeof(int32)) == 0);
            }
            else
            {
                return (m_iVal == rhs.m_iVal);
            }

        case DoubleArray:
            if (m_iVal > 0 && (m_iVal == rhs.m_iVal))
            {
                for (int i = 0; i < m_iVal; ++i)
                {
                    if (!ito::areEqual(((float64*)m_cVal)[i], ((float64*)rhs.m_cVal)[i]))
                    {
                        return false;
                    }
                }
                return true;
            }
            else
            {
                return (m_iVal == rhs.m_iVal);
            }

        case ComplexArray:
            if (m_iVal > 0 && (m_iVal == rhs.m_iVal))
            {
                for (int i = 0; i < m_iVal; ++i)
                {
                    if (!ito::areEqual(((complex128*)m_cVal)[i], ((complex128*)rhs.m_cVal)[i]))
                    {
                        return false;
                    }
                }
                return true;
            }
            else
            {
                return (m_iVal == rhs.m_iVal);
            }

        case StringList:
            if (m_iVal > 0 && rhs.m_iVal > 0)
            {
                const ByteArray* list1 = (ByteArray*)m_cVal;
                const ByteArray* list2 = (ByteArray*)rhs.m_cVal;

                for (int i = 0; i < m_iVal; ++i)
                {
                    if (list1[i] != list2[i])
                    {
                        return false;
                    }
                }
                return true;
            }
            else
            {
                return (m_iVal <= 0) && (rhs.m_iVal <= 0);
            }

        case HWRef:
        case DObjPtr:
        case Pointer:
        case PointCloudPtr:
        case PointPtr:
        case PolygonMeshPtr:
            return (m_cVal == rhs.m_cVal);

        default:
            return false;
        }
    }
    else
    {
        return false; // type is not equal
    }
}

//-------------------------------------------------------------------------------------
//!< Verifies and possibly corrects the proper set of the direction flags depending on the type.
/*
    Every parameter also has some "direction" flags as part of its type value.
    If the parameter is used to transport a generic value from a caller to a called
    method or returns a new parameter back, the direction indicates, if the called method

    * is consuming this parameter (In flag must be set),
    * will consume the parameter and will change its value
      (only the value, not the type) (In | Out flag must both be set),
    * or will create a new value and set it to a previously empty parameter
      (only if the parameter is a return value of a method) (Out flag only must be set).
      f
    If no in/out flag is set, the in-flag as default is automatically added to m_type.

    \seealso ito::ParamBase::Type
*/
void ParamBase::inOutCheck()
{
    const uint16 in_internal = toFlagsInternal(ParamBase::In);
    const uint16 out_internal = toFlagsInternal(ParamBase::Out);

    if ((m_flags & (in_internal | out_internal)) == 0)
    {
        // if no direction is set, set at least In...
        m_flags |= in_internal;
    }

    // verify that Out-only parameters as part of out-vectors of methods
    // in algorithm plugins must not contain any pointer types (like
    // dataObject, pointCloud, polygonMesh, HWRef, point), since
    // the destruction of the created value inside of the algorithm
    // will usually be earlier than the consumer will read these values.
    if ((m_flags & out_internal) && !(m_flags & in_internal))
    {
        // These types are not allowed to be output-only.
        //        DObjPtr         = 0x000010 | Pointer | NoAutosave,
        //        HWRef           = 0x000040 | Pointer | NoAutosave,
        //        PointCloudPtr   = 0x000080 | Pointer | NoAutosave,
        //        PointPtr        = 0x000100 | Pointer | NoAutosave,
        //        PolygonMeshPtr  = 0x000200 | Pointer | NoAutosave
        // since NoAutosave is not in the type part of m_type it needs to be appended to the
        // comparison mask.
        switch (m_type)
        {
        case DObjPtr:
        case PointCloudPtr:
        case PointPtr:
        case PolygonMeshPtr:
        case HWRef: {
            // throw exception only in debug mode. You don't want
            // Exceptions of this type in a production system.
            // You cannot check where it comes from then.
            // To check the origin of this exception you would need
            // a debugger attached or a call stack at hand.
            assert(
                m_flags & in_internal &&
                "An out-only param must not be a Ptr-type"); // will always be false!

            // do not force the type to be In, too, here, since the
            // parameter is likely to be defined in an out-vector of
            // an algorithm plugin and then it is strictly forbidden
            // to have pointer-like parameters there (beside string).

            // throw std::logic_error("It is not allowed to declare a parameter as OUT"
            //    "only for types DObjPtr, PointCloudPtr, PolygonMeshPtr or HWRef");

            break;
        }

        default:
            // well nothing to be done here
            break;
        }
    }
}

//-------------------------------------------------------------------------------------
int ParamBase::getLen() const
{
    switch (m_type)
    {
    case DoubleArray:
    case IntArray:
    case CharArray:
    case ComplexArray:
    case StringList:
        if (m_cVal)
        {
            return m_iVal;
        }
        else
        {
            return 0; // changed in itom 5.0 (was -1 before)
        }

    case String:
        if (m_cVal)
        {
            return static_cast<int>(strlen((char*)m_cVal));
        }
        else
        {
            return 0;
        }
    case Char:
    case Double:
    case Int:
    case Complex:
        return 1;

    default:
        return -1;
    }
}

//-------------------------------------------------------------------------------------
ito::ByteArray ParamBase::getNameWithIndexSuffix(int index) const
{
    char suffix[16];
    sprintf_s(suffix, 16, "[%i]", index);
    ByteArray newName = m_name;
    newName.append(suffix);
    return newName;
}

//-------------------------------------------------------------------------------------
// SET/GET FURTHER PROPERTIES
//-------------------------------------------------------------------------------------

//-------------------------------------------------------------------------------------
bool ParamBase::isNumeric(void) const
{
    static const int numericTypeMask =
        ito::ParamBase::Char | ParamBase::Int | ParamBase::Double | ParamBase::Complex;
    return ((m_type & numericTypeMask) > 0) && !(m_type & ito::ParamBase::Pointer);
}

//-------------------------------------------------------------------------------------
bool ParamBase::isNumericArray(void) const
{
    static const int numericTypeMask =
        ito::ParamBase::Char | ParamBase::Int | ParamBase::Double | ParamBase::Complex;
    return (m_type & numericTypeMask) && (m_type & ito::ParamBase::Pointer);
}

//-------------------------------------------------------------------------------------
bool ParamBase::isValid(void) const
{
    return m_type != 0;
}

//-------------------------------------------------------------------------------------
uint16 ParamBase::getType() const
{
    return m_type;
}


//-------------------------------------------------------------------------------------
uint32 ParamBase::getFlags() const
{
    return toFlagsExternal(m_flags);
}

//-------------------------------------------------------------------------------------
void ParamBase::setFlags(const uint32 flags)
{
    m_flags = toFlagsInternal(flags);
}

//-------------------------------------------------------------------------------------
const char* ParamBase::getName(void) const
{
    return m_name.data();
}

//-------------------------------------------------------------------------------------
bool ParamBase::getAutosave(void) const
{
    return (getFlags() & NoAutosave) > 0;
}

//-------------------------------------------------------------------------------------
void ParamBase::setAutosave(const bool autosave)
{
    int32 f = getFlags();
    int32 fnew = autosave ? (f | NoAutosave) : (f & ~NoAutosave);

    if (f != fnew)
    {
        setFlags(fnew);
    }
}


//--------------------------------------------------------------------------------------------
//  ASSIGNMENT AND OPERATORS
//--------------------------------------------------------------------------------------------

//-------------------------------------------------------------------------------------
/** braces operator
 *   @param [in] num array index for which the value should be returned
 *   @return     new tParam with values of ParamBase[num] in the array
 *
 *   returns the value of the index num from the array
 */
const ParamBase ParamBase::operator[](const int index) const
{
    auto type = getType();

    if ((type == CharArray) || (type == IntArray) ||
        (type == DoubleArray) || (type == ComplexArray) ||
        (type == StringList))
    {
        if (index >= m_iVal || index < 0)
        {
            return ParamBase();
        }
        else
        {
            int len = 0;
            ito::ByteArray newName = getNameWithIndexSuffix(index);

            if (type == StringList)
            {
                const ByteArray* ba = getVal<const ByteArray*>(len);
                return ParamBase(newName, String, ba[index].data());
            }

            uint32 flags = getFlags();
            flags &= ~toFlagsInternal(NoAutosave); // remove "no autosave"

            switch (type & ~Pointer)
            {
            case Char:
                return ParamBase(newName, Char | flags, (getVal<const char*>(len))[index]);
                break;

            case Int:
                return ParamBase(newName, Int | flags, (getVal<const int32*>(len))[index]);
                break;

            case Double:
                return ParamBase(newName, Double | flags, (getVal<const float64*>(len))[index]);
                break;

            case Complex:
                return ParamBase(newName, Complex | flags, (getVal<const complex128*>(len))[index]);
                break;

            default:
                return ParamBase();
                break;
            }
        }
    }
    else
    {
        return ParamBase();
    }
}

//-------------------------------------------------------------------------------------
/** assignment operator
 *   @param [in] rhs ParamBase to copy from
 *   @return     new ParamBase with copied values
 *
 *   sets values of lhs to values of rhs ParamBase, strings are copied
 */
ParamBase& ParamBase::operator=(const ParamBase& rhs)
{
    // first clear old ParamBase:
    freeMemory();

    // now set new parameters:
    m_type = rhs.m_type;
    m_flags = rhs.m_flags;
    m_name = rhs.m_name;
    m_dVal = rhs.m_dVal;
    m_iVal = rhs.m_iVal;

    switch (rhs.m_type)
    {
    case String:
        if (rhs.m_cVal)
        {
            m_cVal = new char[strlen((char*)rhs.m_cVal) + 1];
            memcpy(m_cVal, rhs.m_cVal, strlen((char*)rhs.m_cVal) + 1);
        }
        else
        {
            m_cVal = nullptr;
        }
        break;

    case CharArray:
        m_iVal = rhs.m_iVal;
        if (m_iVal > 0)
        {
            m_cVal = new char[m_iVal];
            memcpy(m_cVal, rhs.m_cVal, m_iVal * sizeof(char));
        }
        else
        {
            m_cVal = 0;
        }
        break;

    case IntArray:
        m_iVal = rhs.m_iVal;
        if (m_iVal > 0)
        {
            m_cVal = new int32[m_iVal];
            memcpy(m_cVal, rhs.m_cVal, m_iVal * sizeof(int32));
        }
        else
        {
            m_cVal = 0;
        }
        break;

    case DoubleArray:
        m_iVal = rhs.m_iVal;
        if (m_iVal > 0)
        {
            m_cVal = new float64[m_iVal];
            memcpy(m_cVal, rhs.m_cVal, m_iVal * sizeof(float64));
        }
        else
        {
            m_cVal = 0;
        }
        break;

    case ComplexArray:
        m_iVal = rhs.m_iVal;
        if (m_iVal > 0)
        {
            m_cVal = new complex128[m_iVal];
            memcpy(m_cVal, rhs.m_cVal, m_iVal * sizeof(complex128));
        }
        else
        {
            m_cVal = 0;
        }
        break;

    case StringList:
        m_iVal = rhs.m_iVal;
        if (m_iVal > 0)
        {
            m_cVal = new ByteArray[m_iVal];

            for (int i = 0; i < m_iVal; ++i)
            {
                ((ByteArray*)m_cVal)[i] = ((const ByteArray*)rhs.m_cVal)[i];
            }
        }
        else
        {
            m_cVal = nullptr;
        }
        break;

    case HWRef:
    case DObjPtr:
    case Pointer:
    case PointCloudPtr:
    case PointPtr:
    case PolygonMeshPtr:
        m_cVal = rhs.m_cVal;
        break;

    default:
        m_cVal = rhs.m_cVal;
        break;
    }

    return *this;
}

//-------------------------------------------------------------------------------------
ito::RetVal ParamBase::copyValueFrom(const ParamBase* rhs)
{
    if (getType() != rhs->getType())
    {
        return ito::RetVal(ito::retError, 0, "tParam types are not equal");
    }

    switch (m_type)
    {
    case Char:
    case Int:
        m_iVal = rhs->m_iVal;
        break;

    case Double:
    case Complex:
        m_dVal = rhs->m_dVal;
        break;

    case String:
        if (m_cVal)
        {
            DELETE_AND_SET_NULL_ARRAY(m_cVal); // must have been a string, too (since no
                                               // type-change)
        }

        if (rhs->m_cVal)
        {
            m_cVal = new char[strlen((char*)(rhs->m_cVal)) + 1];
            memcpy(m_cVal, rhs->m_cVal, strlen((char*)(rhs->m_cVal)) + 1);
        }
        else
        {
            m_cVal = 0;
        }
        break;

    case CharArray:
        if (m_cVal)
        {
            delete[]((char*)m_cVal); // must have been an int-array, too
            m_cVal = nullptr;
        }

        m_iVal = rhs->m_iVal;

        if (m_iVal > 0)
        {
            m_cVal = new char[m_iVal];
            memcpy(m_cVal, rhs->m_cVal, m_iVal * sizeof(char));
        }
        else
        {
            m_cVal = 0;
        }
        break;

    case IntArray:
        if (m_cVal)
        {
            delete[]((int32*)m_cVal); // must have been an int-array, too
            m_cVal = nullptr;
        }

        m_iVal = rhs->m_iVal;

        if (m_iVal > 0)
        {
            m_cVal = new int32[m_iVal];
            memcpy(m_cVal, rhs->m_cVal, m_iVal * sizeof(int32));
        }
        else
        {
            m_cVal = 0;
        }
        break;

    case DoubleArray:
        if (m_cVal)
        {
            delete[]((float64*)m_cVal); // must have been a double-array, too
            m_cVal = nullptr;
        }

        m_iVal = rhs->m_iVal;

        if (m_iVal > 0)
        {
            m_cVal = new float64[m_iVal];
            memcpy(m_cVal, rhs->m_cVal, m_iVal * sizeof(float64));
        }
        else
        {
            m_iVal = 0;
        }
        break;

    case ComplexArray:
        if (m_cVal)
        {
            delete[]((ito::complex128*)m_cVal); // must have been a double-array, too
            m_cVal = nullptr;
        }

        m_iVal = rhs->m_iVal;

        if (m_iVal > 0)
        {
            m_cVal = new complex128[m_iVal];
            memcpy(m_cVal, rhs->m_cVal, m_iVal * sizeof(complex128));
        }
        else
        {
            m_iVal = 0;
        }
        break;

    case StringList:
        if (m_cVal)
        {
            delete[]((ito::ByteArray*)m_cVal); // must have been a string list, too
            m_cVal = nullptr;
        }

        m_iVal = rhs->m_iVal;

        if (m_iVal > 0)
        {
            m_cVal = new ByteArray[m_iVal];

            for (int i = 0; i < m_iVal; ++i)
            {
                ((ito::ByteArray*)(m_cVal))[i] = ((ito::ByteArray*)(rhs->m_cVal))[i];
            }
        }
        else
        {
            m_iVal = 0;
        }
        break;

    case HWRef:
    case DObjPtr:
    case Pointer:
    case PointCloudPtr:
    case PointPtr:
    case PolygonMeshPtr:
        m_cVal = rhs->m_cVal;
        break;

    default:
        return ito::RetVal(ito::retError, 0, "unknown parameter type (ParamBase)");
        break;
    }

    return ito::RetVal(ito::retOk);
}

//-------------------------------------------------------------------------------------
Param::Param(const ByteArray& name, const uint32 typeAndFlags, const char* val, const char* info) :
    ParamBase(name, typeAndFlags, val), m_pMeta(nullptr), m_info(info)
{
}

//-------------------------------------------------------------------------------------
Param::Param(
    const ByteArray& name,
    const uint32 typeAndFlags,
    const char minVal,
    const char maxVal,
    const char val,
    const char* info) :
    ParamBase(name, typeAndFlags, val),
    m_info(info)
{
    assert((typeAndFlags & ParamBase::Char)); // use this constructor only for type character
    m_pMeta = new CharMeta(minVal, maxVal);
}

//-------------------------------------------------------------------------------------
Param::Param(
    const ByteArray& name,
    const uint32 typeAndFlags,
    const int32 minVal,
    const int32 maxVal,
    const int32 val,
    const char* info) :
    ParamBase(name, typeAndFlags, val),
    m_info(info)
{
    assert((typeAndFlags & ParamBase::Int)); // use this constructor only for type integer
    m_pMeta = new IntMeta(minVal, maxVal);
}

//-------------------------------------------------------------------------------------
Param::Param(
    const ByteArray& name,
    const uint32 typeAndFlags,
    const float64 minVal,
    const float64 maxVal,
    const float64 val,
    const char* info) :
    ParamBase(name, typeAndFlags, val),
    m_info(info)
{
    assert((typeAndFlags & ParamBase::Double)); // use this constructor only for type double
    m_pMeta = new DoubleMeta(minVal, maxVal);
}


//-------------------------------------------------------------------------------------
Param::Param(
    const ByteArray& name,
    const uint32 typeAndFlags,
    const unsigned int size,
    const char* values,
    const char* info) :
    ParamBase(name, typeAndFlags, size, values),
    m_pMeta(nullptr), m_info(info)
{
}

//-------------------------------------------------------------------------------------
Param::Param(
    const ByteArray& name,
    const uint32 typeAndFlags,
    const unsigned int size,
    const int32* values,
    const char* info) :
    ParamBase(name, typeAndFlags, size, values),
    m_pMeta(nullptr), m_info(info)
{
}

//-------------------------------------------------------------------------------------
Param::Param(
    const ByteArray& name,
    const uint32 typeAndFlags,
    const unsigned int size,
    const float64* values,
    const char* info) :
    ParamBase(name, typeAndFlags, size, values),
    m_pMeta(nullptr), m_info(info)
{
}

//-------------------------------------------------------------------------------------
Param::Param(
    const ByteArray& name,
    const uint32 typeAndFlags,
    const unsigned int size,
    const complex128* values,
    const char* info) :
    ParamBase(name, typeAndFlags, size, values),
    m_pMeta(nullptr), m_info(info)
{
}

//-------------------------------------------------------------------------------------
Param::Param(
    const ByteArray& name,
    const uint32 typeAndFlags,
    const unsigned int size,
    const ByteArray* values,
    const char* info) :
    ParamBase(name, typeAndFlags, size, values),
    m_pMeta(nullptr), m_info(info)
{
}

//-------------------------------------------------------------------------------------
Param::Param(
    const ByteArray& name,
    const uint32 typeAndFlags,
    const char val,
    ParamMeta* meta,
    const char* info) :
    ParamBase(name, typeAndFlags, val),
    m_pMeta(nullptr), m_info(info)
{
    setMeta(meta, true); // throws exception if meta does not fit to type
}

//-------------------------------------------------------------------------------------
Param::Param(
    const ByteArray& name,
    const uint32 typeAndFlags,
    const int32 val,
    ParamMeta* meta,
    const char* info) :
    ParamBase(name, typeAndFlags, val),
    m_pMeta(nullptr), m_info(info)
{
    setMeta(meta, true); // throws exception if meta does not fit to type
}

//-------------------------------------------------------------------------------------
Param::Param(
    const ByteArray& name,
    const uint32 typeAndFlags,
    const float64 val,
    ParamMeta* meta,
    const char* info) :
    ParamBase(name, typeAndFlags, val),
    m_pMeta(nullptr), m_info(info)
{
    setMeta(meta, true); // throws exception if meta does not fit to type
}

//-------------------------------------------------------------------------------------
Param::Param(
    const ByteArray& name,
    const uint32 typeAndFlags,
    const complex128 val,
    ParamMeta* meta,
    const char* info) :
    ParamBase(name, typeAndFlags, val),
    m_pMeta(nullptr), m_info(info)
{
    setMeta(meta, true); // throws exception if meta does not fit to type
}

//-------------------------------------------------------------------------------------
Param::Param(
    const ByteArray& name,
    const uint32 typeAndFlags,
    const unsigned int size,
    const char* values,
    ParamMeta* meta,
    const char* info) :
    ParamBase(name, typeAndFlags, size, values),
    m_pMeta(nullptr), m_info(info)
{
    setMeta(meta, true); // throws exception if meta does not fit to type
}

//-------------------------------------------------------------------------------------
Param::Param(
    const ByteArray& name,
    const uint32 typeAndFlags,
    const unsigned int size,
    const int32* values,
    ParamMeta* meta,
    const char* info) :
    ParamBase(name, typeAndFlags, size, values),
    m_pMeta(nullptr), m_info(info)
{
    setMeta(meta, true); // throws exception if meta does not fit to type
}

//-------------------------------------------------------------------------------------
Param::Param(
    const ByteArray& name,
    const uint32 typeAndFlags,
    const unsigned int size,
    const float64* values,
    ParamMeta* meta,
    const char* info) :
    ParamBase(name, typeAndFlags, size, values),
    m_pMeta(nullptr), m_info(info)
{
    setMeta(meta, true); // throws exception if meta does not fit to type
}

//-------------------------------------------------------------------------------------
Param::Param(
    const ByteArray& name,
    const uint32 typeAndFlags,
    const unsigned int size,
    const complex128* values,
    ParamMeta* meta,
    const char* info) :
    ParamBase(name, typeAndFlags, size, values),
    m_pMeta(nullptr), m_info(info)
{
    setMeta(meta, true); // throws exception if meta does not fit to type
}

//-------------------------------------------------------------------------------------
Param::Param(
    const ByteArray& name,
    const uint32 typeAndFlags,
    const unsigned int size,
    const ByteArray* values,
    ParamMeta* meta,
    const char* info) :
    ParamBase(name, typeAndFlags, size, values),
    m_pMeta(nullptr), m_info(info)
{
    setMeta(meta, true); // throws exception if meta does not fit to type
}


//-------------------------------------------------------------------------------------
Param::~Param()
{
    DELETE_AND_SET_NULL(m_pMeta);
}

//-------------------------------------------------------------------------------------
Param::Param(const Param& copyConstr) :
    ParamBase(copyConstr), m_pMeta(nullptr), m_info(copyConstr.m_info)
{
    setMeta(copyConstr.m_pMeta);
}


//--------------------------------------------------------------------------------------------
//  ASSIGNMENT AND OPERATORS
//--------------------------------------------------------------------------------------------

//-------------------------------------------------------------------------------------
const Param Param::operator[](const int index) const
{
    auto type = getType();

    if ((type == CharArray) || (type == IntArray) || (type == DoubleArray) ||
        (type == ComplexArray) || (type == StringList))
    {
        if (index >= getLen() || index < 0)
        {
            return Param();
        }
        else
        {
            ito::ByteArray newName = getNameWithIndexSuffix(index);
            int len;
            uint32 newSingleType = (uint32)type;
            newSingleType &= ~Pointer; // remove pointer
            uint32 flags = getFlags();
            flags &= ~toFlagsInternal(NoAutosave); // remove "no autosave"
            newSingleType |= flags;

            switch ((type & ~Pointer))
            {
            case Char: {
                CharMeta* cMeta = nullptr;

                if (m_pMeta && m_pMeta->getType() == ParamMeta::rttiCharArrayMeta)
                {
                    const CharArrayMeta* caMeta = static_cast<const CharArrayMeta*>(m_pMeta);
                    cMeta = new CharMeta(caMeta->getMin(), caMeta->getMax(), caMeta->getStepSize());
                }
                return Param(
                    newName.data(),
                    newSingleType,
                    (getVal<const char*>(len))[index],
                    cMeta,
                    m_info.data());
            }
            break;

            case Int: {
                IntMeta* iMeta = nullptr;

                if (m_pMeta &&
                    (m_pMeta->getType() == ParamMeta::rttiIntArrayMeta ||
                     m_pMeta->getType() == ParamMeta::rttiIntervalMeta ||
                     m_pMeta->getType() == ParamMeta::rttiRangeMeta))
                {
                    const IntMeta* iaMeta = static_cast<const IntMeta*>(m_pMeta);
                    iMeta = new IntMeta(*iaMeta);
                }

                // no conversion from RectMeta to single valued met
                return Param(
                    newName, newSingleType, (getVal<const int32*>(len))[index], iMeta, m_info.data());
            }
            break;

            case Double: {
                DoubleMeta* dMeta = nullptr;

                if (m_pMeta &&
                    (m_pMeta->getType() == ParamMeta::rttiDoubleIntervalMeta ||
                     m_pMeta->getType() == ParamMeta::rttiDoubleArrayMeta))
                {
                    const DoubleMeta* daMeta = static_cast<const DoubleMeta*>(m_pMeta);
                    dMeta = new DoubleMeta(*daMeta);
                }

                return Param(
                    newName, newSingleType, (getVal<const float64*>(len))[index], dMeta, m_info.data());
            }
            break;

            case Complex: {
                // complex has no meta, since no min or max comparison is defined for complex values
                return Param(
                    newName,
                    newSingleType,
                    (getVal<complex128*>(len))[index],
                    nullptr,
                    m_info.data());
            }
            break;

            case StringList & ~Pointer: {
                // no meta up to now
                return Param(
                    newName,
                    String | flags,
                    (getVal<const ByteArray*>(len))[index].data(),
                    m_info.data());
            }

            default:
                return Param();
                break;
            }
        }
    }
    else
    {
        return Param();
    }
}

//-------------------------------------------------------------------------------------
Param& Param::operator=(const Param& rhs)
{
    ParamBase::operator=(rhs);
    m_info = rhs.m_info;
    setMeta(const_cast<ito::ParamMeta*>(rhs.getMeta()));
    return *this;
}

//-------------------------------------------------------------------------------------
ito::RetVal Param::copyValueFrom(const ParamBase* rhs)
{
    return ParamBase::copyValueFrom(rhs);
}

//-------------------------------------------------------------------------------------
void Param::setMeta(ParamMeta* meta, bool takeOwnership)
{
    ParamMeta* oldMeta = m_pMeta;

    if (meta)
    {
        ito::ParamMeta::MetaRtti metaType = meta->getType();

#if _DEBUG
        bool valid = false;
        ito::uint16 ptype = getType();

        switch (metaType)
        {
        case ParamMeta::rttiCharMeta:
            if (ptype == ito::ParamBase::Char)
                valid = true;
            break;
        case ParamMeta::rttiIntMeta:
            if (ptype == ito::ParamBase::Int)
                valid = true;
            break;
        case ParamMeta::rttiDoubleMeta:
            if (ptype == ito::ParamBase::Double)
                valid = true;
            break;
        case ParamMeta::rttiStringMeta:
            if (ptype == (ito::ParamBase::String))
                valid = true;
            break;
        case ParamMeta::rttiDObjMeta:
            if (ptype == (ito::ParamBase::DObjPtr))
                valid = true;
            break;
        case ParamMeta::rttiHWMeta:
            if (ptype == (ito::ParamBase::HWRef))
                valid = true;
            break;
        case ParamMeta::rttiCharArrayMeta:
            if (ptype == (ito::ParamBase::CharArray))
                valid = true;
            break;
        case ParamMeta::rttiIntArrayMeta:
        case ParamMeta::rttiIntervalMeta:
        case ParamMeta::rttiRangeMeta:
        case ParamMeta::rttiRectMeta:
            if (ptype == (ito::ParamBase::IntArray))
                valid = true;
            break;
        case ParamMeta::rttiDoubleArrayMeta:
        case ParamMeta::rttiDoubleIntervalMeta:
            if (ptype == (ito::ParamBase::DoubleArray))
                valid = true;
            break;
        case ParamMeta::rttiStringListMeta:
            if (ptype == (ito::ParamBase::StringList))
                valid = true;
            break;
        default:
            valid = false;
        }

        if (!valid)
        {
            throw std::logic_error("type of meta information does not fit to given parameter type");
        }
#endif

        if (takeOwnership)
        {
            m_pMeta = meta; // Param takes ownership of meta
        }
        else
        {
            switch (metaType)
            {
            case ParamMeta::rttiCharMeta:
                m_pMeta = new CharMeta(*(CharMeta*)(meta));
                break;
            case ParamMeta::rttiIntMeta:
                m_pMeta = new IntMeta(*(IntMeta*)(meta));
                break;
            case ParamMeta::rttiDoubleMeta:
                m_pMeta = new DoubleMeta(*(DoubleMeta*)(meta));
                break;
            case ParamMeta::rttiStringMeta:
                m_pMeta = new StringMeta(*(StringMeta*)(meta));
                break;
            case ParamMeta::rttiDObjMeta:
                m_pMeta = new DObjMeta(*(DObjMeta*)(meta));
                break;
            case ParamMeta::rttiHWMeta:
                m_pMeta = new HWMeta(*(HWMeta*)(meta));
                break;
            case ParamMeta::rttiCharArrayMeta:
                m_pMeta = new CharArrayMeta(*(CharArrayMeta*)(meta));
                break;
            case ParamMeta::rttiIntArrayMeta:
                m_pMeta = new IntArrayMeta(*(IntArrayMeta*)(meta));
                break;
            case ParamMeta::rttiDoubleArrayMeta:
                m_pMeta = new DoubleArrayMeta(*(DoubleArrayMeta*)(meta));
                break;
            case ParamMeta::rttiIntervalMeta:
                m_pMeta = new IntervalMeta(*(IntervalMeta*)(meta));
                break;
            case ParamMeta::rttiRangeMeta:
                m_pMeta = new RangeMeta(*(RangeMeta*)(meta));
                break;
            case ParamMeta::rttiDoubleIntervalMeta:
                m_pMeta = new DoubleIntervalMeta(*(DoubleIntervalMeta*)(meta));
                break;
            case ParamMeta::rttiRectMeta:
                m_pMeta = new RectMeta(*(RectMeta*)(meta));
                break;
            case ParamMeta::rttiStringListMeta:
                m_pMeta = new StringListMeta(*(StringListMeta*)(meta));
                break;
            default:
                throw std::logic_error(
                    "Type of meta [ParamMeta] is unknown and cannot not be copied or assigned.");
            }
        }
    }
    else
    {
        m_pMeta = nullptr;
    }

    if (oldMeta)
    {
        delete oldMeta;
        oldMeta = nullptr;
    }
}

//-------------------------------------------------------------------------------------
/** returns minimum value of parameter if this is available and exists.
 *
 *   This method is a wrapper method for ((ito::IntMeta*)getMeta())->getMax()...
 *   and returns the minimum value of the underlying meta information. It only
 *   returns a valid value for meta structures of type char, charArray, int, intArray, interval,
 * range, double, doubleMeta.
 *
 *   @return     minimum value of -Inf maximum does not exist
 */
float64 Param::getMin() const
{
    if (m_pMeta)
    {
        switch (m_pMeta->getType())
        {
        case ParamMeta::rttiCharMeta:
        case ParamMeta::rttiCharArrayMeta:
            return static_cast<const CharMeta*>(m_pMeta)->getMin();
        case ParamMeta::rttiIntMeta:
        case ParamMeta::rttiIntArrayMeta:
        case ParamMeta::rttiIntervalMeta:
        case ParamMeta::rttiRangeMeta:
            return static_cast<const IntMeta*>(m_pMeta)->getMin();
        case ParamMeta::rttiDoubleMeta:
        case ParamMeta::rttiDoubleArrayMeta:
            return static_cast<const DoubleMeta*>(m_pMeta)->getMin();
        }
    }
    return -std::numeric_limits<float64>::infinity();
}

//-------------------------------------------------------------------------------------
/** returns maximum value of parameter if this is available and exists.
 *
 *   This method is a wrapper method for ((ito::IntMeta*)getMeta())->getMax()...
 *   and returns the maximum value of the underlying meta information. It only
 *   returns a valid value for meta structures of type char, charArray, int, intArray, range,
 *   double, doubleMeta.
 *
 *   @return     maximum value of Inf maximum does not exist
 */
float64 Param::getMax() const
{
    if (m_pMeta)
    {
        switch (m_pMeta->getType())
        {
        case ParamMeta::rttiCharMeta:
        case ParamMeta::rttiCharArrayMeta:
            return static_cast<const CharMeta*>(m_pMeta)->getMax();
        case ParamMeta::rttiIntMeta:
        case ParamMeta::rttiIntArrayMeta:
        case ParamMeta::rttiIntervalMeta:
        case ParamMeta::rttiRangeMeta:
            return static_cast<const IntMeta*>(m_pMeta)->getMax();
        case ParamMeta::rttiDoubleMeta:
        case ParamMeta::rttiDoubleArrayMeta:
            return static_cast<const DoubleMeta*>(m_pMeta)->getMax();
        }
    }
    return std::numeric_limits<float64>::infinity();
}

}; // end namespace ito
