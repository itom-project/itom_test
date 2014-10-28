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

    This file is a port and modified version of the 
    CTK Common Toolkit (http://www.commontk.org)
*********************************************************************** */

// Qt includes
#include <QDebug>
#include <QMouseEvent>
#include <QKeyEvent>
#include <QStyleOptionSlider>
#include <QApplication>
#include <QStylePainter>
#include <QStyle>
#include <QToolTip>

//  includes
#include "rangeSlider.h"

class RangeSliderPrivate
{
  Q_DECLARE_PUBLIC(RangeSlider);
protected:
  RangeSlider* const q_ptr;
public:
  /// Boolean indicates the selected handle
  ///   True for the minimum range handle, false for the maximum range handle
  enum Handle {
    NoHandle = 0x0,
    MinimumHandle = 0x1,
    MaximumHandle = 0x2
  };
  Q_DECLARE_FLAGS(Handles, Handle);

  RangeSliderPrivate(RangeSlider& object);
  void init();

  /// Return the handle at the given pos, or none if no handle is at the pos.
  /// If a handle is selected, handleRect is set to the handle rect.
  /// otherwise return NoHandle and handleRect is set to the combined rect of
  /// the min and max handles
  Handle handleAtPos(const QPoint& pos, QRect& handleRect)const;

  int bound(int min, int max, int step, int value, bool snapToBoundaries = true) const;
  void rangeBound(int valLimitMin, int valLimitMax, Handle handleChangePriority, int &valMin, int &valMax);

  /// Copied verbatim from QSliderPrivate class (see QSlider.cpp)
  int pixelPosToRangeValue(int pos) const;
  int pixelPosFromRangeValue(int val) const;

  /// Draw the bottom and top sliders.
  void drawMinimumSlider( QStylePainter* painter ) const;
  void drawMaximumSlider( QStylePainter* painter ) const;
    
  /// End points of the range on the Model
  int m_MaximumValue;
  int m_MinimumValue;

  /// End points of the range on the GUI. This is synced with the model.
  int m_MaximumPosition;
  int m_MinimumPosition;

  uint m_PositionStepSize;
  uint m_MinimumRange;
  uint m_MaximumRange;
  uint m_StepSizeRange;
  bool m_RangeIncludesLimits;

  /// Controls selected ?
  QStyle::SubControl m_MinimumSliderSelected;
  QStyle::SubControl m_MaximumSliderSelected;

  /// See QSliderPrivate::clickOffset. 
  /// Overrides this ivar
  int m_SubclassClickOffset;
    
  /// See QSliderPrivate::position
  /// Overrides this ivar.
  int m_SubclassPosition;
  
  /// Original width between the 2 bounds before any moves
  float m_SubclassWidth;

  RangeSliderPrivate::Handles m_SelectedHandles;

  /// When symmetricMoves is true, moving a handle will move the other handle
  /// symmetrically, otherwise the handles are independent.
  bool m_SymmetricMoves;

  QString m_HandleToolTip;

private:
  Q_DISABLE_COPY(RangeSliderPrivate);
};

// --------------------------------------------------------------------------
RangeSliderPrivate::RangeSliderPrivate(RangeSlider& object)
  :q_ptr(&object)
{
  this->m_MinimumValue = 0;
  this->m_MaximumValue = 100;
  this->m_MinimumPosition = 0;
  this->m_MaximumPosition = 100;
  this->m_MinimumSliderSelected = QStyle::SC_None;
  this->m_MaximumSliderSelected = QStyle::SC_None;
  this->m_SubclassClickOffset = 0;
  this->m_SubclassPosition = 0;
  this->m_SubclassWidth = 0.0;
  this->m_SelectedHandles = 0;
  this->m_SymmetricMoves = false;

  this->m_PositionStepSize = 1;
  this->m_MinimumRange = 0;
  this->m_RangeIncludesLimits = false;
  this->m_MaximumRange = (this->m_RangeIncludesLimits ? 1 : 0) + this->m_MaximumValue - this->m_MinimumValue;
  this->m_StepSizeRange = 1;
  
}

// --------------------------------------------------------------------------
void RangeSliderPrivate::init()
{
  Q_Q(RangeSlider);
  this->m_MinimumValue = q->minimum();
  this->m_MaximumValue = q->maximum();
  this->m_MinimumPosition = q->minimum();
  this->m_MaximumPosition = q->maximum();

  this->m_MinimumRange = 0;
  this->m_MaximumRange = (this->m_RangeIncludesLimits ? 1 : 0) + this->m_MaximumValue - this->m_MinimumValue;

  q->connect(q, SIGNAL(rangeChanged(int,int)), q, SLOT(onRangeChanged(int,int)));
}

// --------------------------------------------------------------------------
RangeSliderPrivate::Handle RangeSliderPrivate::handleAtPos(const QPoint& pos, QRect& handleRect)const
{
  Q_Q(const RangeSlider);

  QStyleOptionSlider option;
  q->initStyleOption( &option );

  // The functinos hitTestComplexControl only know about 1 handle. As we have
  // 2, we change the position of the handle and test if the pos correspond to
  // any of the 2 positions.
  
  // Test the MinimumHandle
  option.sliderPosition = this->m_MinimumPosition;
  option.sliderValue    = this->m_MinimumValue;

  QStyle::SubControl minimumControl = q->style()->hitTestComplexControl(
    QStyle::CC_Slider, &option, pos, q);
  QRect minimumHandleRect = q->style()->subControlRect(
      QStyle::CC_Slider, &option, QStyle::SC_SliderHandle, q);

  // Test if the pos is under the Maximum handle 
  option.sliderPosition = this->m_MaximumPosition;
  option.sliderValue    = this->m_MaximumValue;

  QStyle::SubControl maximumControl = q->style()->hitTestComplexControl(
    QStyle::CC_Slider, &option, pos, q);
  QRect maximumHandleRect = q->style()->subControlRect(
      QStyle::CC_Slider, &option, QStyle::SC_SliderHandle, q);

  // The pos is above both handles, select the closest handle
  if (minimumControl == QStyle::SC_SliderHandle &&
      maximumControl == QStyle::SC_SliderHandle)
    {
    int minDist = 0;
    int maxDist = 0;
    if (q->orientation() == Qt::Horizontal)
      {
      minDist = pos.x() - minimumHandleRect.left();
      maxDist = maximumHandleRect.right() - pos.x(); 
      }
    else //if (q->orientation() == Qt::Vertical)
      {
      minDist = minimumHandleRect.bottom() - pos.y();
      maxDist = pos.y() - maximumHandleRect.top(); 
      }
    Q_ASSERT( minDist >= 0 && maxDist >= 0);
    minimumControl = minDist < maxDist ? minimumControl : QStyle::SC_None;
    }

  if (minimumControl == QStyle::SC_SliderHandle)
    {
    handleRect = minimumHandleRect;
    return MinimumHandle;
    }
  else if (maximumControl == QStyle::SC_SliderHandle)
    {
    handleRect = maximumHandleRect;
    return MaximumHandle;
    }
  handleRect = minimumHandleRect.united(maximumHandleRect);
  return NoHandle;
}

// --------------------------------------------------------------------------
int RangeSliderPrivate::bound(int min, int max, int step, int value, bool snapToBoundaries /*= true*/) const
{
    if (step == 1)
    {
        return qBound(min, value, max);
    }
    else
    {
        value = qBound(min, value, max);

        //try to round to nearest value following the step size
        int remainder = (value - min) % step;
        if (snapToBoundaries && ((value - remainder) == min))
        {
            value = min;
        }
        else if (snapToBoundaries && ((value + (step - remainder)) == max))
        {
            value = max;
        }
        else if (remainder > (step/2)) //we want to round up
        {
            //check upper limit
            if (value + remainder <= max)
            {
                //not exceeded, then go to the next upper step value
                value += (step - remainder);
            }
            else
            {
                //decrementing is always allowed
                value -= remainder;
            }
        }
        else
        {
            //decrementing is always allowed
            value -= remainder;
        }
        return value;
    }
}

// --------------------------------------------------------------------------
void RangeSliderPrivate::rangeBound(int valLimitMin, int valLimitMax, Handle handleChangePriority, int &valMin, int &valMax)
{
    int offset = (this->m_RangeIncludesLimits ? 1 : 0);
    int range = offset + valMax - valMin;
    bool rangeOk = (range == bound(m_MinimumRange, m_MaximumRange, m_StepSizeRange, range, false));

    if (rangeOk)
    {
        //try to fix left boundary and move right one
        if (handleChangePriority == MaximumHandle || (handleChangePriority == NoHandle && (qAbs(valLimitMin - valMin) < qAbs(valLimitMax - valMax))))
        {
            valMin = bound(valLimitMin, valLimitMax, m_PositionStepSize, valMin);
            valMax = bound(valMin + m_MinimumRange, qMin(valLimitMax, valMin + (int)m_MaximumRange), m_StepSizeRange, valMin + range, false) - offset;
        }
        else //try to fix right boundary and move left one
        {
            valMax = bound(valLimitMin, valLimitMax, m_PositionStepSize, valMax);
            valMin = bound(qMax(valLimitMin, valMax - (int)m_MaximumRange), valMax - m_MinimumRange, m_StepSizeRange, valMax - range, false) + offset;
        }
    }
    else
    {
        //try to fix left boundary and move right one
        if (handleChangePriority == MaximumHandle || (handleChangePriority == NoHandle && (qAbs(valLimitMin - valMin) < qAbs(valLimitMax - valMax))))
        {
            valMin = bound(valLimitMin, valLimitMax, m_PositionStepSize, valMin);
            range = bound(m_MinimumRange, m_MaximumRange, m_StepSizeRange, valMax - valMin + offset, false);
            valMax = bound(valMin + m_MinimumRange, qMin(valLimitMax, valMin + (int)m_MaximumRange), m_StepSizeRange, valMin + range, false) - offset;
        }
        else //try to fix right boundary and move left one
        {
            valMax = bound(valLimitMin, valLimitMax, m_PositionStepSize, valMax);
            range = bound(m_MinimumRange, m_MaximumRange, m_StepSizeRange, valMax - valMin + offset, false);
            valMin = bound(qMax(valLimitMin, valMax - (int)m_MaximumRange), valMax - m_MinimumRange, m_StepSizeRange, valMax - range, false) + offset;
        }
    }
}

// --------------------------------------------------------------------------
// Copied verbatim from QSliderPrivate::pixelPosToRangeValue. See QSlider.cpp
//
int RangeSliderPrivate::pixelPosToRangeValue( int pos ) const
{
  Q_Q(const RangeSlider);
  QStyleOptionSlider option;
  q->initStyleOption( &option );

  QRect gr = q->style()->subControlRect( QStyle::CC_Slider, 
                                            &option, 
                                            QStyle::SC_SliderGroove, 
                                            q );
  QRect sr = q->style()->subControlRect( QStyle::CC_Slider, 
                                            &option, 
                                            QStyle::SC_SliderHandle, 
                                            q );
  int sliderMin, sliderMax, sliderLength;
  if (option.orientation == Qt::Horizontal) 
    {
    sliderLength = sr.width();
    sliderMin = gr.x();
    sliderMax = gr.right() - sliderLength + 1;
    }
  else
    {
    sliderLength = sr.height();
    sliderMin = gr.y();
    sliderMax = gr.bottom() - sliderLength + 1;
    }

  return QStyle::sliderValueFromPosition( q->minimum(), 
                                          q->maximum(), 
                                          pos - sliderMin,
                                          sliderMax - sliderMin, 
                                          option.upsideDown );
}

//---------------------------------------------------------------------------
int RangeSliderPrivate::pixelPosFromRangeValue( int val ) const
{
  Q_Q(const RangeSlider);
  QStyleOptionSlider option;
  q->initStyleOption( &option );

  QRect gr = q->style()->subControlRect( QStyle::CC_Slider, 
                                            &option, 
                                            QStyle::SC_SliderGroove, 
                                            q );
  QRect sr = q->style()->subControlRect( QStyle::CC_Slider, 
                                            &option, 
                                            QStyle::SC_SliderHandle, 
                                            q );
  int sliderMin, sliderMax, sliderLength;
  if (option.orientation == Qt::Horizontal) 
    {
    sliderLength = sr.width();
    sliderMin = gr.x();
    sliderMax = gr.right() - sliderLength + 1;
    }
  else
    {
    sliderLength = sr.height();
    sliderMin = gr.y();
    sliderMax = gr.bottom() - sliderLength + 1;
    }

  return QStyle::sliderPositionFromValue( q->minimum(), 
                                          q->maximum(), 
                                          val,
                                          sliderMax - sliderMin, 
                                          option.upsideDown ) + sliderMin;
}

//---------------------------------------------------------------------------
// Draw slider at the bottom end of the range
void RangeSliderPrivate::drawMinimumSlider( QStylePainter* painter ) const
{
  Q_Q(const RangeSlider);
  QStyleOptionSlider option;
  q->initMinimumSliderStyleOption( &option );

  option.subControls = QStyle::SC_SliderHandle;
  option.sliderValue = m_MinimumValue;
  option.sliderPosition = m_MinimumPosition;
  if (q->isMinimumSliderDown())
    {
    option.activeSubControls = QStyle::SC_SliderHandle;
    option.state |= QStyle::State_Sunken;
    }
#ifdef Q_OS_MAC
  // On mac style, drawing just the handle actually draws also the groove.
  QRect clip = q->style()->subControlRect(QStyle::CC_Slider, &option,
                                          QStyle::SC_SliderHandle, q);
  painter->setClipRect(clip);
#endif
  painter->drawComplexControl(QStyle::CC_Slider, option);
}

//---------------------------------------------------------------------------
// Draw slider at the top end of the range
void RangeSliderPrivate::drawMaximumSlider( QStylePainter* painter ) const
{
  Q_Q(const RangeSlider);
  QStyleOptionSlider option;
  q->initMaximumSliderStyleOption( &option );

  option.subControls = QStyle::SC_SliderHandle;
  option.sliderValue = m_MaximumValue;
  option.sliderPosition = m_MaximumPosition;
  if (q->isMaximumSliderDown())
    {
    option.activeSubControls = QStyle::SC_SliderHandle;
    option.state |= QStyle::State_Sunken;
    }
#ifdef Q_OS_MAC
  // On mac style, drawing just the handle actually draws also the groove.
  QRect clip = q->style()->subControlRect(QStyle::CC_Slider, &option,
                                          QStyle::SC_SliderHandle, q);
  painter->setClipRect(clip);
#endif
  painter->drawComplexControl(QStyle::CC_Slider, option);
}

// --------------------------------------------------------------------------
RangeSlider::RangeSlider(QWidget* _parent)
  : QSlider(_parent)
  , d_ptr(new RangeSliderPrivate(*this))
{
  Q_D(RangeSlider);
  d->init();
}

// --------------------------------------------------------------------------
RangeSlider::RangeSlider( Qt::Orientation o,
                                  QWidget* parentObject )
  :QSlider(o, parentObject)
  , d_ptr(new RangeSliderPrivate(*this))
{
  Q_D(RangeSlider);
  d->init();
}

// --------------------------------------------------------------------------
RangeSlider::RangeSlider(RangeSliderPrivate* impl, QWidget* _parent)
  : QSlider(_parent)
  , d_ptr(impl)
{
  Q_D(RangeSlider);
  d->init();
}

// --------------------------------------------------------------------------
RangeSlider::RangeSlider( RangeSliderPrivate* impl, Qt::Orientation o,
                                QWidget* parentObject )
  :QSlider(o, parentObject)
  , d_ptr(impl)
{
  Q_D(RangeSlider);
  d->init();
}

// --------------------------------------------------------------------------
RangeSlider::~RangeSlider()
{
}

// --------------------------------------------------------------------------
int RangeSlider::minimumValue() const
{
  Q_D(const RangeSlider);
  return d->m_MinimumValue;
}

// --------------------------------------------------------------------------
void RangeSlider::setMinimumValue( int min )
{
  Q_D(RangeSlider);
  this->setValues( min, qMax(d->m_MaximumValue,min) );
}

// --------------------------------------------------------------------------
int RangeSlider::maximumValue() const
{
  Q_D(const RangeSlider);
  return d->m_MaximumValue;
}

// --------------------------------------------------------------------------
void RangeSlider::setMaximumValue( int max )
{
  Q_D(RangeSlider);
  this->setValues( qMin(d->m_MinimumValue, max), max );
}

// --------------------------------------------------------------------------
uint RangeSlider::stepSizePosition() const
{
  Q_D(const RangeSlider);
  return d->m_PositionStepSize;
}

// --------------------------------------------------------------------------
void RangeSlider::setStepSizePosition(uint stepSize)
{
  Q_D(RangeSlider);
  d->m_PositionStepSize = stepSize;
  d->m_StepSizeRange = d->bound(stepSize, std::numeric_limits<int>::max(), stepSize, d->m_StepSizeRange);
  this->setValues( d->m_MinimumValue, d->m_MaximumValue );
}
  
// --------------------------------------------------------------------------
uint RangeSlider::minimumRange() const
{
  Q_D(const RangeSlider);
  return d->m_MinimumRange;
}

// --------------------------------------------------------------------------
void RangeSlider::setMinimumRange(uint min)
{
  Q_D(RangeSlider);
  d->m_MinimumRange = d->bound(min - d->m_StepSizeRange, d->m_MaximumRange, d->m_StepSizeRange, min);
  this->setValues( d->m_MinimumValue, d->m_MaximumValue );
}
  
// --------------------------------------------------------------------------
uint RangeSlider::maximumRange() const
{
  Q_D(const RangeSlider);
  return d->m_MaximumRange;
}

// --------------------------------------------------------------------------
void RangeSlider::setMaximumRange(uint max)
{
  Q_D(RangeSlider);
  d->m_MaximumRange = d->bound(d->m_MinimumRange, max + d->m_StepSizeRange, d->m_StepSizeRange, max);
  this->setValues( d->m_MinimumValue, d->m_MaximumValue );
}
  
// --------------------------------------------------------------------------
uint RangeSlider::stepSizeRange() const
{
  Q_D(const RangeSlider);
  return d->m_StepSizeRange;
}

// --------------------------------------------------------------------------
void RangeSlider::setStepSizeRange(uint stepSize)
{
  Q_D(RangeSlider);
  d->m_StepSizeRange = d->bound(d->m_PositionStepSize, std::numeric_limits<int>::max(), d->m_PositionStepSize, stepSize);
  d->m_MaximumRange = d->bound(d->m_MinimumRange, d->m_MaximumRange + stepSize, stepSize, d->m_MaximumRange);
  this->setValues( d->m_MinimumValue, d->m_MaximumValue );
}
  
// --------------------------------------------------------------------------
bool RangeSlider::rangeIncludeLimits() const
{
  Q_D(const RangeSlider);
  return d->m_RangeIncludesLimits;
}

// --------------------------------------------------------------------------
void RangeSlider::setRangeIncludeLimits(bool include)
{
  Q_D(RangeSlider);
  d->m_RangeIncludesLimits = include;
  this->setValues( d->m_MinimumValue, d->m_MaximumValue );
}

// --------------------------------------------------------------------------
void RangeSlider::setValues(int l, int u)
{
  Q_D(RangeSlider);
  int minValue = qMin(l,u);
  int maxValue = qMax(l,u);

  RangeSliderPrivate::Handle handleChangePriority = RangeSliderPrivate::NoHandle; //both handles per default
  if ((minValue != d->m_MinimumValue) && maxValue == d->m_MaximumValue)
  {
      handleChangePriority = RangeSliderPrivate::MinimumHandle;
  }
  else if ((minValue == d->m_MinimumValue) && maxValue != d->m_MaximumValue)
  {
      handleChangePriority = RangeSliderPrivate::MaximumHandle;
  }

  d->rangeBound(this->minimum(), this->maximum(), handleChangePriority, minValue, maxValue);

  bool emitMinValChanged = (minValue != d->m_MinimumValue);
  bool emitMaxValChanged = (maxValue != d->m_MaximumValue);
  
  d->m_MinimumValue = minValue;
  d->m_MaximumValue = maxValue;
  
  bool emitMinPosChanged = 
    (minValue != d->m_MinimumPosition);
  bool emitMaxPosChanged = 
    (maxValue != d->m_MaximumPosition);
  d->m_MinimumPosition = minValue;
  d->m_MaximumPosition = maxValue;
  
  if (isSliderDown())
    {
    if (emitMinPosChanged || emitMaxPosChanged)
      {
      emit positionsChanged(d->m_MinimumPosition, d->m_MaximumPosition);
      }
    if (emitMinPosChanged)
      {
      emit minimumPositionChanged(d->m_MinimumPosition);
      }
    if (emitMaxPosChanged)
      {
      emit maximumPositionChanged(d->m_MaximumPosition);
      }
    }
  if (emitMinValChanged || emitMaxValChanged)
    {
    emit valuesChanged(d->m_MinimumValue, 
                       d->m_MaximumValue);
    }
  if (emitMinValChanged)
    {
    emit minimumValueChanged(d->m_MinimumValue);
    }
  if (emitMaxValChanged)
    {
    emit maximumValueChanged(d->m_MaximumValue);
    }
  if (emitMinPosChanged || emitMaxPosChanged || 
      emitMinValChanged || emitMaxValChanged)
    {
    this->update();
    }
}

// --------------------------------------------------------------------------
int RangeSlider::minimumPosition() const
{
  Q_D(const RangeSlider);
  return d->m_MinimumPosition;
}

// --------------------------------------------------------------------------
int RangeSlider::maximumPosition() const
{
  Q_D(const RangeSlider);
  return d->m_MaximumPosition;
}

// --------------------------------------------------------------------------
void RangeSlider::setMinimumPosition(int l)
{
  Q_D(const RangeSlider);
  this->setPositions(l, qMax(l, d->m_MaximumPosition));
}

// --------------------------------------------------------------------------
void RangeSlider::setMaximumPosition(int u)
{
  Q_D(const RangeSlider);
  this->setPositions(qMin(d->m_MinimumPosition, u), u);
}

// --------------------------------------------------------------------------
void RangeSlider::setPositions(int min, int max)
{
  Q_D(RangeSlider);
  int minPosition = qMin(min,max);
  int maxPosition = qMax(min,max);

  RangeSliderPrivate::Handle handleChangePriority = RangeSliderPrivate::NoHandle; //both handles per default
  if ((minPosition != d->m_MinimumPosition) && maxPosition == d->m_MaximumPosition)
  {
      handleChangePriority = RangeSliderPrivate::MinimumHandle;
  }
  else if ((minPosition == d->m_MinimumPosition) && maxPosition != d->m_MaximumPosition)
  {
      handleChangePriority = RangeSliderPrivate::MaximumHandle;
  }

  d->rangeBound(this->minimum(), this->maximum(), handleChangePriority, minPosition, maxPosition);

  bool emitMinPosChanged = (minPosition != d->m_MinimumPosition);
  bool emitMaxPosChanged = (maxPosition != d->m_MaximumPosition);
  
  if (!emitMinPosChanged && !emitMaxPosChanged)
    {
    return;
    }

  d->m_MinimumPosition = minPosition;
  d->m_MaximumPosition = maxPosition;

  if (!this->hasTracking())
    {
    this->update();
    }
  if (isSliderDown())
    {
    if (emitMinPosChanged)
      {
      emit minimumPositionChanged(d->m_MinimumPosition);
      }
    if (emitMaxPosChanged)
      {
      emit maximumPositionChanged(d->m_MaximumPosition);
      }
    if (emitMinPosChanged || emitMaxPosChanged)
      {
      emit positionsChanged(d->m_MinimumPosition, d->m_MaximumPosition);
      }
    }
  if (this->hasTracking())
    {
    this->triggerAction(SliderMove);
    this->setValues(d->m_MinimumPosition, d->m_MaximumPosition);
    }
}

// --------------------------------------------------------------------------
void RangeSlider::setSymmetricMoves(bool symmetry)
{
  Q_D(RangeSlider);
  d->m_SymmetricMoves = symmetry;
}

// --------------------------------------------------------------------------
bool RangeSlider::symmetricMoves()const
{
  Q_D(const RangeSlider);
  return d->m_SymmetricMoves;
}

// --------------------------------------------------------------------------
void RangeSlider::onRangeChanged(int _minimum, int _maximum)
{
  Q_UNUSED(_minimum);
  Q_UNUSED(_maximum);
  Q_D(RangeSlider);
  this->setValues(d->m_MinimumValue, d->m_MaximumValue);
}

// --------------------------------------------------------------------------
// Render
void RangeSlider::paintEvent( QPaintEvent* )
{
  Q_D(RangeSlider);
  QStyleOptionSlider option;
  this->initStyleOption(&option);

  QStylePainter painter(this);
  option.subControls = QStyle::SC_SliderGroove;
  // Move to minimum to not highlight the SliderGroove.
  // On mac style, drawing just the slider groove also draws the handles,
  // therefore we give a negative (outside of view) position.
  option.sliderValue = this->minimum() - this->maximum();
  option.sliderPosition = this->minimum() - this->maximum();
  painter.drawComplexControl(QStyle::CC_Slider, option);

  option.sliderPosition = d->m_MinimumPosition;
  const QRect lr = style()->subControlRect( QStyle::CC_Slider, 
                                            &option, 
                                            QStyle::SC_SliderHandle, 
                                            this);
  option.sliderPosition = d->m_MaximumPosition;

  const QRect ur = style()->subControlRect( QStyle::CC_Slider, 
                                            &option, 
                                            QStyle::SC_SliderHandle, 
                                            this);

  QRect sr = style()->subControlRect( QStyle::CC_Slider, 
                                      &option, 
                                      QStyle::SC_SliderGroove, 
                                      this);
  QRect rangeBox;
  if (option.orientation == Qt::Horizontal)
    {
    rangeBox = QRect(
      QPoint(qMin( lr.center().x(), ur.center().x() ), sr.center().y() - 2),
      QPoint(qMax( lr.center().x(), ur.center().x() ), sr.center().y() + 1));
    }
  else
    {
    rangeBox = QRect(
      QPoint(sr.center().x() - 2, qMin( lr.center().y(), ur.center().y() )),
      QPoint(sr.center().x() + 1, qMax( lr.center().y(), ur.center().y() )));
    }

  // -----------------------------
  // Render the range
  //
  QRect groove = this->style()->subControlRect( QStyle::CC_Slider, 
                                                &option, 
                                                QStyle::SC_SliderGroove, 
                                                this );
  groove.adjust(0, 0, -1, 0);

  // Create default colors based on the transfer function.
  //
  QColor highlight = this->palette().color(QPalette::Normal, QPalette::Highlight);
  QLinearGradient gradient;
  if (option.orientation == Qt::Horizontal)
    {
    gradient = QLinearGradient( groove.center().x(), groove.top(),
                                groove.center().x(), groove.bottom());
    }
  else
    {
    gradient = QLinearGradient( groove.left(), groove.center().y(),
                                groove.right(), groove.center().y());
    }

  // TODO: Set this based on the supplied transfer function
  //QColor l = Qt::darkGray;
  //QColor u = Qt::black;

  gradient.setColorAt(0, highlight.darker(120));
  gradient.setColorAt(1, highlight.lighter(160));

  painter.setPen(QPen(highlight.darker(150), 0));
  painter.setBrush(gradient);
  painter.drawRect( rangeBox.intersected(groove) );

  //  -----------------------------------
  // Render the sliders
  //
  if (this->isMinimumSliderDown())
    {
    d->drawMaximumSlider( &painter );
    d->drawMinimumSlider( &painter );
    }
  else
    {
    d->drawMinimumSlider( &painter );
    d->drawMaximumSlider( &painter );
    }
}

// --------------------------------------------------------------------------
// Standard Qt UI events
void RangeSlider::mousePressEvent(QMouseEvent* mouseEvent)
{
  Q_D(RangeSlider);
  if (minimum() == maximum() || (mouseEvent->buttons() ^ mouseEvent->button()))
    {
    mouseEvent->ignore();
    return;
    }
  int mepos = this->orientation() == Qt::Horizontal ?
    mouseEvent->pos().x() : mouseEvent->pos().y();

  QStyleOptionSlider option;
  this->initStyleOption( &option );

  QRect handleRect;
  RangeSliderPrivate::Handle handle_ = d->handleAtPos(mouseEvent->pos(), handleRect);

  if (handle_ != RangeSliderPrivate::NoHandle)
    {
    d->m_SubclassPosition = (handle_ == RangeSliderPrivate::MinimumHandle)?
      d->m_MinimumPosition : d->m_MaximumPosition;

    // save the position of the mouse inside the handle for later
    d->m_SubclassClickOffset = mepos - (this->orientation() == Qt::Horizontal ?
      handleRect.left() : handleRect.top());

    this->setSliderDown(true);

    if (d->m_SelectedHandles != handle_)
      {
      d->m_SelectedHandles = handle_;
      this->update(handleRect);
      }
    // Accept the mouseEvent
    mouseEvent->accept();
    return;
    }

  // if we are here, no handles have been pressed
  // Check if we pressed on the groove between the 2 handles
  
  QStyle::SubControl control = this->style()->hitTestComplexControl(
    QStyle::CC_Slider, &option, mouseEvent->pos(), this);
  QRect sr = style()->subControlRect(
    QStyle::CC_Slider, &option, QStyle::SC_SliderGroove, this);
  int minCenter = (this->orientation() == Qt::Horizontal ?
    handleRect.left() : handleRect.top());
  int maxCenter = (this->orientation() == Qt::Horizontal ?
    handleRect.right() : handleRect.bottom());
  if (control == QStyle::SC_SliderGroove &&
      mepos > minCenter && mepos < maxCenter)
    {
    // warning lost of precision it might be fatal
    d->m_SubclassPosition = (d->m_MinimumPosition + d->m_MaximumPosition) / 2.;
    d->m_SubclassClickOffset = mepos - d->pixelPosFromRangeValue(d->m_SubclassPosition);
    d->m_SubclassWidth = (d->m_MaximumPosition - d->m_MinimumPosition) / 2.;
    qMax(d->m_SubclassPosition - d->m_MinimumPosition, d->m_MaximumPosition - d->m_SubclassPosition);
    this->setSliderDown(true);
    if (!this->isMinimumSliderDown() || !this->isMaximumSliderDown())
      {
      d->m_SelectedHandles = 
        QFlags<RangeSliderPrivate::Handle>(RangeSliderPrivate::MinimumHandle) | 
        QFlags<RangeSliderPrivate::Handle>(RangeSliderPrivate::MaximumHandle);
      this->update(handleRect.united(sr));
      }
    mouseEvent->accept();
    return;
    }
  mouseEvent->ignore();
}

// --------------------------------------------------------------------------
// Standard Qt UI events
void RangeSlider::mouseMoveEvent(QMouseEvent* mouseEvent)
{
  Q_D(RangeSlider);
  if (!d->m_SelectedHandles)
    {
    mouseEvent->ignore();
    return;
    }
  int mepos = this->orientation() == Qt::Horizontal ?
    mouseEvent->pos().x() : mouseEvent->pos().y();

  QStyleOptionSlider option;
  this->initStyleOption(&option);

  const int m = style()->pixelMetric( QStyle::PM_MaximumDragDistance, &option, this );

  int newPosition = d->pixelPosToRangeValue(mepos - d->m_SubclassClickOffset);

  if (m >= 0)
    {
    const QRect r = rect().adjusted(-m, -m, m, m);
    if (!r.contains(mouseEvent->pos()))
      {
      newPosition = d->m_SubclassPosition;
      }
    }

  // Only the lower/left slider is down
  if (this->isMinimumSliderDown() && !this->isMaximumSliderDown())
    {
    double newMinPos = qMin(newPosition,d->m_MaximumPosition);
    this->setPositions(newMinPos, d->m_MaximumPosition +
      (d->m_SymmetricMoves ? d->m_MinimumPosition - newMinPos : 0));
    }
  // Only the upper/right slider is down
  else if (this->isMaximumSliderDown() && !this->isMinimumSliderDown())
    {
    double newMaxPos = qMax(d->m_MinimumPosition, newPosition);
    this->setPositions(d->m_MinimumPosition -
      (d->m_SymmetricMoves ? newMaxPos - d->m_MaximumPosition: 0),
      newMaxPos);
    }
  // Both handles are down (the user clicked in between the handles)
  else if (this->isMinimumSliderDown() && this->isMaximumSliderDown())
    {
    this->setPositions(newPosition - static_cast<int>(d->m_SubclassWidth),
                       newPosition + static_cast<int>(d->m_SubclassWidth + .5));
    }
  mouseEvent->accept();
}

// --------------------------------------------------------------------------
// Standard Qt UI mouseEvents
void RangeSlider::mouseReleaseEvent(QMouseEvent* mouseEvent)
{
  Q_D(RangeSlider);
  this->QSlider::mouseReleaseEvent(mouseEvent);

  setSliderDown(false);
  d->m_SelectedHandles = 0;

  this->update();
}

// --------------------------------------------------------------------------
bool RangeSlider::isMinimumSliderDown()const
{
  Q_D(const RangeSlider);
  return d->m_SelectedHandles & RangeSliderPrivate::MinimumHandle;
}

// --------------------------------------------------------------------------
bool RangeSlider::isMaximumSliderDown()const
{
  Q_D(const RangeSlider);
  return d->m_SelectedHandles & RangeSliderPrivate::MaximumHandle;
}

// --------------------------------------------------------------------------
void RangeSlider::initMinimumSliderStyleOption(QStyleOptionSlider* option) const
{
  this->initStyleOption(option);
}

// --------------------------------------------------------------------------
void RangeSlider::initMaximumSliderStyleOption(QStyleOptionSlider* option) const
{
  this->initStyleOption(option);
}

// --------------------------------------------------------------------------
QString RangeSlider::handleToolTip()const
{
  Q_D(const RangeSlider);
  return d->m_HandleToolTip;
}

// --------------------------------------------------------------------------
void RangeSlider::setHandleToolTip(const QString& _toolTip)
{
  Q_D(RangeSlider);
  d->m_HandleToolTip = _toolTip;
}

// --------------------------------------------------------------------------
bool RangeSlider::event(QEvent* _event)
{
  Q_D(RangeSlider);
  switch(_event->type())
    {
    case QEvent::ToolTip:
      {
      QHelpEvent* helpEvent = static_cast<QHelpEvent*>(_event);
      QStyleOptionSlider opt;
      // Test the MinimumHandle
      opt.sliderPosition = d->m_MinimumPosition;
      opt.sliderValue = d->m_MinimumValue;
      this->initStyleOption(&opt);
      QStyle::SubControl hoveredControl =
        this->style()->hitTestComplexControl(
          QStyle::CC_Slider, &opt, helpEvent->pos(), this);
      if (!d->m_HandleToolTip.isEmpty() &&
          hoveredControl == QStyle::SC_SliderHandle)
        {
        QToolTip::showText(helpEvent->globalPos(), d->m_HandleToolTip.arg(this->minimumValue()));
        _event->accept();
        return true;
        }
      // Test the MaximumHandle
      opt.sliderPosition = d->m_MaximumPosition;
      opt.sliderValue = d->m_MaximumValue;
      this->initStyleOption(&opt);
      hoveredControl = this->style()->hitTestComplexControl(
        QStyle::CC_Slider, &opt, helpEvent->pos(), this);
      if (!d->m_HandleToolTip.isEmpty() &&
          hoveredControl == QStyle::SC_SliderHandle)
        {
        QToolTip::showText(helpEvent->globalPos(), d->m_HandleToolTip.arg(this->maximumValue()));
        _event->accept();
        return true;
        }
      }
    default:
      break;
    }
  return this->Superclass::event(_event);
}
