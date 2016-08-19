/*********************************************************************
This is a library for our Monochrome OLEDs based on SSD1306 drivers

  Pick one up today in the adafruit shop!
  ------> http://www.adafruit.com/category/63_98

These displays use SPI to communicate, 4 or 5 pins are required to  
interface

Adafruit invests time and resources providing this open source code, 
please support Adafruit and open-source hardware by purchasing 
products from Adafruit!

Written by Limor Fried/Ladyada  for Adafruit Industries.  
BSD license, check license.txt for more information
All text above, and the splash screen must be included in any redistribution
*********************************************************************/

/*
 *  Modified by Neal Horman 7/14/2012 for use in mbed
 */

#ifndef _ADAFRUIT_SSD1306_H_
#define _ADAFRUIT_SSD1306_H_
#include "im4oled_default_config.h"
#include "mx_gfx.h"

#if (OLED_USE_VECTOR==1)
#include <vector>
#endif
#include <algorithm>

#define OLED_HAS_RESET      0

// A DigitalOut sub-class that provides a constructed default state
class DigitalOut2 : public DigitalOut
{
public:
	DigitalOut2(PinName pin, bool active = false) : DigitalOut(pin) { write(active); };
	DigitalOut2& operator= (int value) { write(value); return *this; };
	DigitalOut2& operator= (DigitalOut2& rhs) { write(rhs.read()); return *this; };
	operator int() { return read(); };
};

#define SSD1306_EXTERNALVCC 0x1
#define SSD1306_SWITCHCAPVCC 0x2

/** The pure base class for the SSD1306 display driver.
 *
 * You should derive from this for a new transport interface type,
 * such as the SPI and I2C drivers.
 */
class MxSSD1306 : public MxGfx
{
public:
#if (OLED_HAS_RESET==1)
	MxSSD1306(PinName RST, uint8_t rawHeight = 32, uint8_t rawWidth = 128)
		: MxGfx(rawWidth,rawHeight)
        , colBlock(0)
        , rowBlock(0)
		, rst(RST,false)
#else
    MxSSD1306(uint8_t rawHeight = 32, uint8_t rawWidth = 128)
        : MxGfx(rawWidth,rawHeight)
        , colBlock(0)
        , rowBlock(0)
#endif
	{
	    //Initialize as all dirty
	    memset(&dirty[0], 0xff, sizeof(dirty));

#if (OLED_USE_VECTOR==0)
	    memset(&buffer[0], 0, sizeof(buffer));
#else
	    buffer.resize(rawHeight * rawWidth / 8);
#endif
	};

    /** Initialize
     * @return 0 if success, else I2C or SPI error code
     */
	uint8_t begin(uint8_t switchvcc = SSD1306_SWITCHCAPVCC);
	
	// These must be implemented in the derived transport driver
	virtual uint8_t command(uint8_t c) = 0;
	virtual uint8_t data(uint8_t c) = 0;
	virtual void drawPixel(int16_t x, int16_t y, uint16_t color);

	/**
	 * Clear the display buffer. Requires a display() call at some point afterwards.
	 * NOTE that this function will make the WHOLE display as dirty! The next display() call will update the
	 * entire display! This can be prevented by just clearing the required part of the display using fillRect()!
	 */
	void clearDisplay(void);

    /** Set display contrast
     * @return 0 if success, else I2C or SPI error code
     */
    virtual uint8_t setContrast(uint8_t contrast);

    /** Turn display on or off
     * @return 0 if success, else I2C or SPI error code
     */
    virtual uint8_t displayOn(bool on);

    /** Invert Display
	 * @return 0 if success, else I2C or SPI error code
	 */
	virtual uint8_t invertDisplay(bool i);

	/** Cause the display to be updated with the buffer content.
	 * @return 0 if success, else I2C or SPI error code
	 */
	uint8_t display();

    // Fill the buffer with the AdaFruit splash screen.
	virtual void splash();
    
protected:
    /** Write contents of display buffer to OLED display.
     * @return 0 if success, else I2C or SPI error code
     */
	virtual uint8_t sendDisplayBuffer() = 0;

    /** Write a block of display data. The 128x64 pixels are divided into:
     * - 8 RowBlocks, each with 8 rows. This is 1 page of the SSD1206
     * - 8 ColBlocks, each with 16 columns.
     *
     * @param rowBlock Value 0-7 indicating what block of 8 rows to write. 0=0-7,
     *        1=8-15, ...., 7=56-63
     * @param colBlock Value 0-7 indicating what block of 16 columns to write. 0=0-15,
     *        1=16-31, ...., 7=112-127
     * @return 0 if success, else I2C or SPI error code
     */
    virtual uint8_t sendDisplayBuffer(uint8_t rowBlock, uint8_t colBlock) = 0;

public:
    // Set whole display as being dirty
    virtual void setAllDirty();

    // Protected Data
protected:
    uint8_t colBlock, rowBlock;

#if (OLED_WIDTH <= 128 )
    uint8_t dirty[OLED_HEIGHT/8];   //Each bit marks block of "8 Rows x 16 Columns". So, a single byte is enough for up to 128col. One byte for each 8 rows.
#elif (OLED_WIDTH <= 256 )
    uint16_t dirty[OLED_HEIGHT/8];  //Each bit marks block of "8 Rows x 16 Columns". One UINT16 = 16x16 = 256 columns.
#endif

#if (OLED_HAS_RESET==1)
	DigitalOut2 rst;
#endif

	// the memory buffer for the LCD
#if (OLED_USE_VECTOR==1)
    std::vector<uint8_t> buffer;
#else
    uint8_t buffer[OLED_HEIGHT * OLED_WIDTH / 8];
#endif
};


/** This is the SPI SSD1306 display driver transport class
 *
 */
class MxSSD1306_SPI : public MxSSD1306
{
public:
	/** Create a SSD1306 SPI transport display driver instance with the specified DC, RST, and CS pins, as well as the display dimentions
	 *
	 * Required parameters
	 * @param spi - a reference to an initialized SPI object
	 * @param DC (Data/Command) pin name
	 * @param RST (Reset) pin name
	 * @param CS (Chip Select) pin name
	 *
	 * Optional parameters
	 * @param rawHeight - the vertical number of pixels for the display, defaults to 32
	 * @param rawWidth - the horizonal number of pixels for the display, defaults to 128
	 */
#if (OLED_HAS_RESET==1)
    MxSSD1306_SPI(SPI &spi, PinName DC, PinName RST, PinName CS, uint8_t rawHieght = 32, uint8_t rawWidth = 128)
	    : MxSSD1306(RST, rawHieght, rawWidth)
#else
    MxSSD1306_SPI(SPI &spi, PinName DC, PinName CS, uint8_t rawHieght = 32, uint8_t rawWidth = 128)
        : MxSSD1306(rawHieght, rawWidth)
#endif
	    , cs(CS,true)
	    , dc(DC,false)
	    , mspi(spi)
	    {
		    begin();
		    splash();
		    display();
	    };

    /** Send command via SPI
     * @param c The command to send
     * @return 0 if success, else SPI error
     */
	virtual uint8_t command(uint8_t c)
	{
	    cs = 1;
	    dc = 0;
	    cs = 0;
	    mspi.write(c);
	    cs = 1;
	    return 0;
	};

    /** Send Data via SPI
     * @param c The data to send
     * @return 0 if success, else SPI error
     */
	virtual uint8_t data(uint8_t c)
	{
	    cs = 1;
	    dc = 1;
	    cs = 0;
	    mspi.write(c);
	    cs = 1;
	    return 0;
	};

protected:
	virtual uint8_t sendDisplayBuffer()
	{
	    uint8_t retVal;
		cs = 1;
		dc = 1;
		cs = 0;

#if (OLED_USE_VECTOR==0)
		for(uint16_t i=0, q=sizeof(buffer); i<q; i++) {
#else
	    for(uint16_t i=0, q=buffer.size(); i<q; i++) {
#endif
			if((retVal=mspi.write(buffer[i])) != 0) {
			    cs = 1;
			    return retVal;
			}
		}

		if(height() == 32)
		{
#if (OLED_USE_VECTOR==0)
		    for(uint16_t i=0, q=sizeof(buffer); i<q; i++) {
#else
	        for(uint16_t i=0, q=buffer.size(); i<q; i++) {
#endif
		        if((retVal=mspi.write(0)) != 0) {
	                cs = 1;
	                return retVal;
	            }
			}
		}

		cs = 1;
		return 0;
	};

	DigitalOut2 cs, dc;
	SPI &mspi;
};

/** This is the I2C SSD1306 display driver transport class
 *
 */
class MxSSD1306_I2C : public MxSSD1306
{
public:
	#define SSD_I2C_ADDRESS     0x78
	/** Create a SSD1306 I2C transport display driver instance with the specified RST pin name, the I2C address, as well as the display dimensions
	 *
	 * Required parameters
	 * @param i2c - A reference to an initialized I2C object
	 * @param RST - The Reset pin name
	 *
	 * Optional parameters
	 * @param i2cAddress - The i2c address of the display
	 * @param rawHeight - The vertical number of pixels for the display, defaults to 32
	 * @param rawWidth - The horizonal number of pixels for the display, defaults to 128
	 */
#if (OLED_HAS_RESET==1)
    Adafruit_SSD1306_I2c(I2C &i2c, PinName RST, uint8_t i2cAddress = SSD_I2C_ADDRESS, uint8_t rawHeight = 32, uint8_t rawWidth = 128)
	    : MxSSD1306(RST, rawHeight, rawWidth)
#else
    MxSSD1306_I2C(I2C &i2c, uint8_t i2cAddress = SSD_I2C_ADDRESS, uint8_t rawHeight = 32, uint8_t rawWidth = 128)
        : MxSSD1306(rawHeight, rawWidth)
#endif
        , mi2c(i2c)
	    , mi2cAddress(i2cAddress)
    {
        begin();
        splash();
        display();
    };


    /**Constructor without doing any initialization requiring I2C object. Use this constructor if the I2C object must still be
     * initialized, or used after startup delay.
     * !!!!! IMPORTANT !!!!!
     * This constructor must be followed by calling the init() function BEFORE using any other functions!
     *
     * Required parameters
     * @param i2cAddress - The i2c address of the display
     * @param i2c - A reference to an initialized I2C object
     *
     * Optional parameters
     * @param rawHeight - The vertical number of pixels for the display, defaults to 32
     * @param rawWidth - The horizonal number of pixels for the display, defaults to 128
     */
    MxSSD1306_I2C(uint8_t i2cAddress, I2C &i2c, uint8_t rawHeight = 32, uint8_t rawWidth = 128)
        : MxSSD1306(rawHeight, rawWidth), mi2c(i2c), mi2cAddress(i2cAddress)
    {
    };


    /**
     * Initialize with given I2C bus.
     * !!!!! IMPORTANT !!!!!
     * This function must be called after the Adafruit_SSD1306_I2c(rawHeight, rawWidth) constructor!
     *
     * @param i2c I2C Bus to use
     * @return 0 if OK, else error code
     */
    uint8_t init()
    {
        uint8_t retVal;
        if((retVal=begin()) != 0) {
            return retVal;  //Return error code
        }

        //splash();
        if((retVal=display()) != 0) {
            return retVal;  //Return error code
        }
        return 0;
    }

    /** Send command via I2C
     * @param c The command to send
     * @return 0 if success, else I2C error
     */
    virtual uint8_t command(uint8_t c)
	{
		char buff[2];
		buff[0] = 0; // Command Mode
		buff[1] = c;
		return mi2c.write(mi2cAddress, buff, sizeof(buff));
	}

    /** Send Data via I2C
     * @param c The data to send
     * @return 0 if success, else I2C error
     */
	virtual uint8_t data(uint8_t c)
	{
		char buff[2];
		buff[0] = 0x40; // Data Mode
		buff[1] = c;
		return mi2c.write(mi2cAddress, buff, sizeof(buff));
	};

protected:
    virtual uint8_t sendDisplayBuffer()
	{
        char buff[17];
        buff[0] = 0x40; // Data Mode

        // send display buffer in 16 byte chunks
#if (OLED_USE_VECTOR==0)
        for(uint16_t i=0, q=sizeof(buffer); i<q; i+=16 ) {
#else
        for(uint16_t i=0, q=buffer.size(); i<q; i+=16 ) {
#endif
            uint8_t retVal;
            uint8_t x;

            for(x=1; x<sizeof(buff); x++) {
                buff[x] = buffer[i+x-1];
            }

            if((retVal=mi2c.write(mi2cAddress, buff, sizeof(buff))) != 0) {
                return retVal;  //Return error code
            }
        }
        return 0;
	};

    /** Write a block of display data. The 128x64 pixels are divided into:
     * - 8 RowBlocks, each with 8 rows. This is 1 page of the SSD1206
     * - 8 ColBlocks, each with 16 columns.
     *
     * @param rowBlock Value 0-7 indicating what block of 8 rows to write. 0=0-7,
     *        1=8-15, ...., 7=56-63
     * @param colBlock Value 0-7 indicating what block of 16 columns to write. 0=0-15,
     *        1=16-31, ...., 7=112-127
     * @return 0 if success, else I2C or SPI error code
     */
    uint8_t sendDisplayBuffer(uint8_t rowBlock, uint8_t colBlock) {
        uint8_t retVal;
        int idxBuffer;
        char buff[17];

        buff[0] = 0x40; // Data Mode

        //Each byte of buffer contains 8pixels for single column, and 8 rows. For example:
        //buffer[0] contains row 0-7 for column 0
        //buffer[1] contains row 0-7 for column 1
        idxBuffer = (rowBlock*128) + (colBlock*16);

        // Copy requested "row block" and "column block"
        for(uint16_t i=0; i<16; i++) {
            buff[i+1] = buffer[idxBuffer+i];
        }

        //Write all display data
        if((retVal=mi2c.write(mi2cAddress, buff, 17)) != 0) {
            return retVal;  //Return error code
        }

        return 0;
    }

	I2C &mi2c;
	uint8_t mi2cAddress;
};

#endif
