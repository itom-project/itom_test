#include <iostream>

#include "../../Common/sharedStructures.h"

//opencv
#pragma warning( disable : 4996 ) //C:\OpenCV2.3\build\include\opencv2/flann/logger.h(70): warning C4996: 'fopen': This function or variable may be unsafe. Consider using fopen_s instead.
#pragma once
#include "opencv/cv.h"
#include "../../DataObject/dataobj.h"
#include "gtest/gtest.h"
//#include "test_global.h"
#include "commonChannel.h"


/*! \class 
	\brief 
*/
template <typename _Tp> class rgbaOperatorTests : public ::testing::Test 
	{ 
public:
	
	virtual void SetUp(void)
	{
        valWhite            = _Tp(255);
        valBlackTransparent = _Tp::ZEROS();
        valRED              = ito::rgba32::RED();
        valBLUE             = ito::rgba32::BLUE();
        valGREEN            = ito::rgba32::GREEN();
        valBLACK            = _Tp::BLACK();
        valOneGray          = _Tp(1);
        valDarkGray         = _Tp(64);
        valGray             = _Tp(128);
	};
 
	virtual void TearDown(void) {};
	typedef _Tp valueType;	

    _Tp valWhite;
    _Tp valBlackTransparent;
    ito::rgba32 valRED;
    ito::rgba32 valBLUE;
    ito::rgba32 valGREEN;
    _Tp valBLACK;
    _Tp valOneGray;
    _Tp valDarkGray;
    _Tp valGray;

	};
	
TYPED_TEST_CASE(rgbaOperatorTests, ItomColorAllTypes);

/*!
	
*/
TYPED_TEST(rgbaOperatorTests, testMultiplication)
{
	valueType tempVal = valDarkGray * valBlackTransparent; // Multiplication off zero with white must be zeros;
    EXPECT_EQ(tempVal, valBlackTransparent);

    // Multiplication off white with zero must be zeros;
	tempVal = valBlackTransparent * valDarkGray;
    EXPECT_EQ(tempVal, valBlackTransparent);
	tempVal = valBlackTransparent; 
    tempVal *= valDarkGray; 
    EXPECT_EQ(tempVal, valBlackTransparent);
   
    // Multiplication off ones with white must be ones;
	tempVal = valDarkGray * valWhite;
    EXPECT_EQ(tempVal, valDarkGray);
	tempVal = valDarkGray; 
    tempVal *= valWhite;
    EXPECT_EQ(tempVal, valDarkGray);

    // Multiplication off gray128 with white must be gray128;
	tempVal = valGray * valWhite;
    EXPECT_EQ(tempVal, valGray);
	tempVal = valGray; 
    tempVal *= valWhite;
    EXPECT_EQ(tempVal, valGray);

    // Multiplication off white with white must be white;
	tempVal = valWhite * valWhite;
    EXPECT_EQ(tempVal, valWhite);
	tempVal = valWhite; 
    tempVal *= valWhite;
    EXPECT_EQ(tempVal, valWhite);

    // Multiplication off white with white must be white;
	tempVal = valGray * valGray;
    EXPECT_EQ(tempVal, valDarkGray);
	tempVal = valGray; 
    tempVal *= valGray;
    EXPECT_EQ(tempVal, valDarkGray);
}

TYPED_TEST(rgbaOperatorTests, testAddition)
{

	valueType tempVal = valDarkGray + valBlackTransparent; // Addition off zeros to value must be value;
    EXPECT_EQ(tempVal, valDarkGray);
    tempVal = valDarkGray;
    tempVal += valBlackTransparent; // Addition off zeros to value must be value;
    EXPECT_EQ(tempVal, valDarkGray);

	tempVal = valBlackTransparent + valDarkGray; // Addition off value to zeros must be value;
    EXPECT_EQ(tempVal, valDarkGray);
	tempVal = valBlackTransparent;
    tempVal += valDarkGray; // Addition off value to zeros must be value;
    EXPECT_EQ(tempVal, valDarkGray);

	tempVal = valWhite + valDarkGray; // Addition off any value to white must be white;
    EXPECT_EQ(tempVal, valWhite);
	tempVal = valWhite;
    tempVal += valDarkGray; // Addition off any value to white must be white;
    EXPECT_EQ(tempVal, valWhite);

	tempVal = valDarkGray + valWhite; // Addition off white to any value must be white;
    EXPECT_EQ(tempVal, valWhite);
	tempVal = valDarkGray;
    tempVal +=  valWhite; // Addition off white to any value must be white;
    EXPECT_EQ(tempVal, valWhite);

	tempVal = valDarkGray + valDarkGray; // Addition off white to any value must be white;
    EXPECT_EQ(tempVal, valGray);
	tempVal = valDarkGray;
    tempVal += valDarkGray; // Addition off white to any value must be white;
    EXPECT_EQ(tempVal, valGray);

	tempVal = valGray + valGray; // Addition off white to any value must be white;
    EXPECT_EQ(tempVal, valWhite);
    EXPECT_NE(tempVal, valBlackTransparent);
}

TYPED_TEST(rgbaOperatorTests, testSubstraction)
{
    
    // Addition off zeros to value must be value;
	valueType tempVal = valDarkGray - valBlackTransparent; 
    EXPECT_EQ(tempVal, valDarkGray);
	tempVal = valDarkGray;
    tempVal -= valBlackTransparent;
    EXPECT_EQ(tempVal, valDarkGray);


    // Addition off value to zeros must be value;
	tempVal = valBlackTransparent - valDarkGray; 
    EXPECT_EQ(tempVal, valBlackTransparent);
	tempVal = valBlackTransparent;
    tempVal -= valDarkGray;
    EXPECT_EQ(tempVal, valBlackTransparent);

    // Addition off white to any value must be white;
    tempVal = valDarkGray - valWhite;
    EXPECT_EQ(tempVal, valBlackTransparent);
    tempVal = valDarkGray;
    tempVal -= valWhite; 
    EXPECT_EQ(tempVal, valBlackTransparent);

    // Addition off white to any value must be white;
	tempVal = valGray - valDarkGray;
    if(typeid(valueType) == typeid(ito::rgba32))
    {
        (*(reinterpret_cast<ito::rgba32* >(&tempVal))).alpha() = 255;
    }
    EXPECT_EQ(tempVal, valDarkGray);

	tempVal = valGray;
    tempVal -= valDarkGray;
    if(typeid(valueType) == typeid(ito::rgba32))
    {
        (*(reinterpret_cast<ito::rgba32* >(&tempVal))).alpha() = 255;
    }
    EXPECT_EQ(tempVal, valDarkGray);
}

TYPED_TEST(rgbaOperatorTests, testDividation)
{
    EXPECT_ANY_THROW(valDarkGray / valBlackTransparent);


	valueType tempVal = valBlackTransparent /  valWhite; // Multiplication off zero with white must be zeros;
    EXPECT_EQ(tempVal, valBlackTransparent);

	tempVal = valBlackTransparent;
    tempVal /=  valWhite; // Multiplication off zero with white must be zeros;
    EXPECT_EQ(tempVal, valBlackTransparent);


	tempVal = valWhite / valWhite; // Multiplication off white with zero must be zeros;
    EXPECT_EQ(tempVal, valWhite);

	tempVal = valWhite;
    tempVal /= valWhite; // Multiplication off white with zero must be zeros;
    EXPECT_EQ(tempVal, valWhite);

	tempVal = valWhite / valDarkGray; // Multiplication off white with zero must be zeros;
    EXPECT_EQ(tempVal, valWhite);
	tempVal = valWhite;
    tempVal /= valDarkGray; // Multiplication off white with zero must be zeros;
    EXPECT_EQ(tempVal, valWhite);
   
	tempVal = valDarkGray / valWhite; // Multiplication off white with zero must be zeros;
    EXPECT_EQ(tempVal, valDarkGray);
	tempVal = valDarkGray;
    tempVal /= valWhite; // Multiplication off white with zero must be zeros;
    EXPECT_EQ(tempVal, valDarkGray);

	tempVal = valDarkGray / valGray; // Multiplication off white with zero must be zeros;

    if(typeid(valueType) == typeid(ito::rgba32))
    {
        EXPECT_EQ((*(reinterpret_cast<ito::rgba32* >(&tempVal))).alpha(), 255);
        EXPECT_EQ((*(reinterpret_cast<ito::rgba32* >(&tempVal))).red(), 127);
        EXPECT_EQ((*(reinterpret_cast<ito::rgba32* >(&tempVal))).blue(), 127);
        EXPECT_EQ((*(reinterpret_cast<ito::rgba32* >(&tempVal))).green(), 127);
    }
    else if(typeid(valueType) == typeid(ito::alphaChannel))
    {
        EXPECT_EQ(tempVal, valueType(127));
    }

	tempVal = valDarkGray;
    tempVal /= valGray; // Multiplication off white with zero must be zeros;

    if(typeid(valueType) == typeid(ito::rgba32))
    {
        EXPECT_EQ((*(reinterpret_cast<ito::rgba32* >(&tempVal))).alpha(), 255);
        EXPECT_EQ((*(reinterpret_cast<ito::rgba32* >(&tempVal))).red(), 127);
        EXPECT_EQ((*(reinterpret_cast<ito::rgba32* >(&tempVal))).blue(), 127);
        EXPECT_EQ((*(reinterpret_cast<ito::rgba32* >(&tempVal))).green(), 127);
    }
    else
    {
        EXPECT_EQ(tempVal, valueType(127));
    }
    
}

TYPED_TEST(rgbaOperatorTests, testElementSize)
{
    EXPECT_EQ(sizeof(valueType), 4);
    EXPECT_EQ(sizeof(valueType), sizeof(ito::uint32));
}

TYPED_TEST(rgbaOperatorTests, testConstructors)
{
    if(typeid(valueType) == typeid(ito::rgba32))
    {
        EXPECT_EQ(ito::rgba32(255, 255, 255,255), ito::rgba32(255));
        EXPECT_EQ(valueType(255), valueType(255));
        EXPECT_EQ(ito::rgba32(0, 0, 0,0), ito::rgba32::ZEROS());
        EXPECT_NE(valueType(255), valueType::ZEROS());

        EXPECT_EQ(ito::rgba32(255, 255, 0,0), ito::rgba32::RED());
        EXPECT_NE(ito::rgba32(255, 0, 255,0), ito::rgba32::RED());

        EXPECT_EQ((*static_cast<ito::RGBChannel_t<ito::rgba32::RGBA_R>*>(&ito::rgba32::RED())).value(), ito::rgba32::RED().red());

        EXPECT_EQ(ito::rgba32(255, 0, 255,0), ito::rgba32::GREEN());
        EXPECT_NE(ito::rgba32(255, 12, 255,0), ito::rgba32::GREEN());
        EXPECT_EQ((*static_cast<ito::RGBChannel_t<ito::rgba32::RGBA_G>*>(&ito::rgba32::GREEN())).value(), ito::rgba32::GREEN().green());

        EXPECT_EQ(ito::rgba32(255, 0, 0,255), ito::rgba32::BLUE());
        EXPECT_NE(ito::rgba32(255, 12, 255,0), ito::rgba32::BLUE());
        EXPECT_EQ((*static_cast<ito::RGBChannel_t<ito::rgba32::RGBA_B>*>(&ito::rgba32::BLUE())).value(), ito::rgba32::BLUE().blue());

        EXPECT_EQ(ito::rgba32(255, 0, 0,0), ito::rgba32::BLACK());
        EXPECT_NE(ito::rgba32(255, 0, 1,0), ito::rgba32::BLACK());
        EXPECT_EQ((*static_cast<ito::RGBChannel_t<ito::rgba32::RGBA_A>*>(&ito::rgba32::BLACK())).value(), 255);
        EXPECT_EQ(255, ito::rgba32::BLACK().alpha());

        ito::rgba32 tempVal(128, 3, 4, 5);

        EXPECT_EQ(tempVal.alpha(), 128);
        EXPECT_EQ(tempVal.red(), 3);
        EXPECT_EQ(tempVal.green(), 4);
        EXPECT_EQ(tempVal.blue(), 5);
    }
    else if(typeid(valueType) == typeid(ito::alphaChannel))
    {
        EXPECT_EQ(valueType(255), valueType(255));
        EXPECT_EQ(valueType(0), valueType::ZEROS());
        EXPECT_EQ(valueType::ZEROS(), valueType::ZEROS());

        EXPECT_EQ(valueType(255), *(reinterpret_cast<valueType*>(&ito::rgba32(255))));
        EXPECT_NE(valueType(128), *(reinterpret_cast<valueType*>(&ito::rgba32(128))));

        EXPECT_EQ(valueType(255), *(reinterpret_cast<valueType*>(&ito::rgba32(255, 0,0,0))));
        EXPECT_EQ(valueType(128), *(reinterpret_cast<valueType*>(&ito::rgba32(128, 0,0,0))));    
    }
    else
    {
        EXPECT_EQ(valueType(255), valueType(255));
        EXPECT_EQ(valueType(0), valueType::ZEROS());
        EXPECT_NE((*reinterpret_cast<ito::rgba32*>(&valueType(0))).alpha(), ito::rgba32::ZEROS().alpha());
        EXPECT_EQ((*reinterpret_cast<ito::rgba32*>(&valueType(0))).alpha(), 255);
        EXPECT_EQ(valueType::ZEROS(), valueType::ZEROS());

        EXPECT_EQ(valueType(255), *(reinterpret_cast<valueType*>(&ito::rgba32(255))));
        EXPECT_EQ(valueType(128), *(reinterpret_cast<valueType*>(&ito::rgba32(128))));
    }

    valueType t1 = *(reinterpret_cast<valueType*>(&ito::rgba32(255, 11 , 125, 15)));
    valueType t2 = t1;
    valueType t3 = t2;

    EXPECT_EQ(t1, t3);

}

TYPED_TEST(rgbaOperatorTests, testDefines)
{
    if(typeid(valueType) == typeid(ito::rgba32))
    {
        EXPECT_EQ(ito::rgba32::RGBA_B, 0);
        EXPECT_EQ(ito::rgba32::RGBA_G, 1);
        EXPECT_EQ(ito::rgba32::RGBA_R, 2);
        EXPECT_EQ(ito::rgba32::RGBA_A, 3);
        EXPECT_EQ(ito::rgba32::RGBA_Y, 4);
        EXPECT_EQ(ito::rgba32::RGBA_RGB, 5);
    }
}

TYPED_TEST(rgbaOperatorTests, testGrayConversion)
{
    ito::rgba32 myVal(255);

    EXPECT_TRUE( ito::isZeroValue<ito::float32>(valWhite.gray()-255.0, std::numeric_limits<ito::float32>::epsilon()));
    EXPECT_TRUE( ito::isZeroValue<ito::float32>(valBlackTransparent.gray(), std::numeric_limits<ito::float32>::epsilon()));
}

TYPED_TEST(rgbaOperatorTests, testAssignment)
{
    ito::uint32 longWhite = 0xFFFFFFFF;
    ito::uint32 longTrans = 0x00000000;
    ito::uint32 longBlack = 0xFF000000;

    ito::uint32 longRED   = 0xFFFF0000;
    ito::uint32 longGREEN = 0xFF00FF00;
    ito::uint32 longBLUE  = 0xFF0000FF;

    valueType tmpVal;
    tmpVal = longWhite;
    EXPECT_EQ( tmpVal, valWhite);

    tmpVal = longTrans;
    EXPECT_EQ( tmpVal, valBlackTransparent);

    tmpVal = longBlack;
    EXPECT_EQ( tmpVal, valBLACK);

    tmpVal = longRED;
    EXPECT_EQ( tmpVal, (*reinterpret_cast<valueType*>(&valRED)));

    tmpVal = longGREEN;
    EXPECT_EQ( tmpVal, (*reinterpret_cast<valueType*>(&valGREEN)));

    tmpVal = longBLUE;
    EXPECT_EQ( tmpVal, (*reinterpret_cast<valueType*>(&valBLUE)));

    valueType tempVal = valWhite; // Multiplication off zero with white must be zeros;
    tempVal = valBlackTransparent;
    tempVal = valBLACK;
    valueType tempVal2(0);
    valueType tempVal3(0);
    tempVal2 = tempVal;
    tempVal3 = tempVal2;
    tempVal = tempVal3;

    EXPECT_EQ(tempVal, valBLACK);

    tempVal = valWhite;
    *(reinterpret_cast<ito::rgba32*>(&tempVal2)) = *(reinterpret_cast<ito::rgba32*>(&tempVal));
    EXPECT_EQ( tempVal2, valWhite);

    *(reinterpret_cast<ito::redChannel*>(&tempVal2)) = *(reinterpret_cast<ito::redChannel*>(&valWhite));
    EXPECT_EQ( tempVal2, valWhite);

    *(reinterpret_cast<ito::blueChannel*>(&tempVal2)) = *(reinterpret_cast<ito::blueChannel*>(&valWhite));
    EXPECT_EQ( tempVal2, valWhite);

    *(reinterpret_cast<ito::greenChannel*>(&tempVal2)) = *(reinterpret_cast<ito::greenChannel*>(&valWhite));
    EXPECT_EQ( tempVal2, valWhite);

    *(reinterpret_cast<ito::alphaChannel*>(&tempVal2)) = *(reinterpret_cast<ito::alphaChannel*>(&valWhite));
    EXPECT_EQ( tempVal2, valWhite);
}

TYPED_TEST(rgbaOperatorTests, testBoolEqual)
{
    valueType val1 = *(static_cast<valueType*>(&ito::rgba32(0,0,0,0)));
    valueType val2 = *(static_cast<valueType*>(&ito::rgba32(1,0,0,0)));
    if(typeid(valueType) == typeid(ito::rgba32) || typeid(valueType) == typeid(ito::alphaChannel))
    {
        EXPECT_TRUE ( val1 != val2 );
        EXPECT_FALSE( val1 == val2);
    }
    else
    {
        EXPECT_TRUE ( val1 == val2 );
        EXPECT_FALSE( val1 != val2 );    
    }

    val2 = *(static_cast<valueType*>(&ito::rgba32(0,1,0,0)));
    if(typeid(valueType) == typeid(ito::rgba32) || typeid(valueType) == typeid(ito::redChannel))
    {
        EXPECT_TRUE ( val1 != val2 );
        EXPECT_FALSE( val1 == val2 );
    }
    else
    {
        EXPECT_TRUE ( val1 == val2);
        EXPECT_FALSE( val1 != val2);    
    }

    val2 = *(static_cast<valueType*>(&ito::rgba32(0,0,1,0)));
    if(typeid(valueType) == typeid(ito::rgba32) || typeid(valueType) == typeid(ito::greenChannel))
    {
        EXPECT_TRUE ( val1 != val2 );
        EXPECT_FALSE( val1 == val2 );
    }
    else
    {
        EXPECT_TRUE ( val1 == val2);
        EXPECT_FALSE( val1 != val2);    
    }

    val2 = *(static_cast<valueType*>(&ito::rgba32(0,0,0,1)));
    if(typeid(valueType) == typeid(ito::rgba32) || typeid(valueType) == typeid(ito::blueChannel))
    {
        EXPECT_TRUE ( val1 != val2 );
        EXPECT_FALSE( val1 == val2 );
    }
    else
    {
        EXPECT_TRUE ( val1 == val2);
        EXPECT_FALSE( val1 != val2);    
    }

    EXPECT_TRUE( valWhite == valueType(255) );
    EXPECT_FALSE( valWhite != valueType(255) );

    if(typeid(valueType) == typeid(ito::rgba32) || typeid(valueType) == typeid(ito::alphaChannel))
    {
        EXPECT_FALSE( valBlackTransparent == valBLACK );
        EXPECT_TRUE( valBlackTransparent != valBLACK );
    }
    else
    {
        EXPECT_FALSE( valBlackTransparent != valBLACK );
        EXPECT_TRUE( valBlackTransparent == valBLACK );    
    }
}

/*! \class 
	\brief 
*/
template <typename _Tp> class rgbaChannalTests : public ::testing::Test 
	{ 
public:
	
	virtual void SetUp(void)
	{

	};
 
	virtual void TearDown(void) {};
	typedef _Tp valueType;	

	};

TYPED_TEST_CASE(rgbaChannalTests, ItomAllChannelTypes);
/*!
	
*/
TYPED_TEST(rgbaChannalTests, testChannelSpecificOperators)
{


}