/*
 / _____)             _              | |
( (____  _____ ____ _| |_ _____  ____| |__
 \____ \| ___ |    (_   _) ___ |/ ___)  _ \
 _____) ) ____| | | || |_| ____( (___| | | |
(______/|_____)_|_|_| \__)_____)\____)_| |_|
    ( C )2014 Semtech

Description: Actual implementation of a SX1276 radio, inherits Radio

License: Revised BSD License, see LICENSE.TXT file include in the project

Maintainers: Miguel Luis, Gregory Cristian and Nicolas Huguenin
*/
#include "inair.h"

//MODTRONIX BEGIN /////////////////////////////////////////////////////////////
#define DEBUG_ENABLE            0
#if (DEBUG_ENABLE == 1)
    extern Stream* pMxDebug;            //Define Stream for debug output in user code called pMxDebug
    #define MX_DEBUG pMxDebug->printf
#else
    #define MX_DEBUG(format, args...) ((void)0)
#endif
//MODTRONIX END ///////////////////////////////////////////////////////////////

#define INAIR_ENABLE_FSK    0

const FskBandwidth_t InAir::FskBandwidths[] =
{       
    { 2600  , 0x17 },   
    { 3100  , 0x0F },
    { 3900  , 0x07 },
    { 5200  , 0x16 },
    { 6300  , 0x0E },
    { 7800  , 0x06 },
    { 10400 , 0x15 },
    { 12500 , 0x0D },
    { 15600 , 0x05 },
    { 20800 , 0x14 },
    { 25000 , 0x0C },
    { 31300 , 0x04 },
    { 41700 , 0x13 },
    { 50000 , 0x0B },
    { 62500 , 0x03 },
    { 83333 , 0x12 },
    { 100000, 0x0A },
    { 125000, 0x02 },
    { 166700, 0x11 },
    { 200000, 0x09 },
    { 250000, 0x01 },
    { 300000, 0x00 }, // Invalid Badwidth
};


InAir::InAir( void ( *txDone )( ), void ( *txTimeout ) ( ), void ( *rxDone ) ( uint8_t *payload, uint16_t size, int16_t rssi, int8_t snr ),
                void ( *rxTimeout ) ( ), void ( *rxError ) ( ), void ( *fhssChangeChannel ) ( uint8_t channelIndex ), void ( *cadDone ) ( bool channelActivityDetected ),
                PinName mosi, PinName miso, PinName sclk, PinName nss, PinName reset,
                PinName dio0, PinName dio1, PinName dio2, PinName dio3)
            :   Radio( txDone, txTimeout, rxDone, rxTimeout, rxError, fhssChangeChannel, cadDone ),
                spi( mosi, miso, sclk ),
                nss( nss ),
                reset( reset ),
                dio0( dio0 ), dio1( dio1 ), dio2( dio2 ), dio3( dio3 ),
                isRadioActive( false )
{
    wait_ms( 10 );
    this->rxTx = 0;
    this->rxBuffer = new uint8_t[RX_BUFFER_SIZE];
    previousOpMode = RF_OPMODE_STANDBY;
    
    this->settings.State = IDLE;

    // From sx1276-inAir Constructor //////////////////////////////////////////
    Reset( );

    boardConnected = BOARD_UNKNOWN;

    RxChainCalibration( );

    IoInit( );

    SetOpMode( RF_OPMODE_SLEEP );

    IoIrqInit();

    RadioRegistersInit( );

    SetModem( MODEM_LORA );

    this->settings.State = IDLE ;
}

InAir::~InAir( )
{
    delete this->rxBuffer;
}

void InAir::task(void)
{
#if(INAIR_DIO0_IS_INTERRUPT==0)
    static bool dio0Was0 = false;
#endif
#if(INAIR_DIO1_IS_INTERRUPT==0)
    static bool dio1Was0 = false;
#endif
#if(INAIR_DIO2_IS_INTERRUPT==0)
    static bool dio2Was0 = false;
#endif
#if(INAIR_DIO3_IS_INTERRUPT==0)
    static bool dio3Was0 = false;
#endif

#if(INAIR_DIO0_IS_INTERRUPT==0)
    if (dio0.read() == 0) {
        dio0Was0 = true;
    }
    else {
        //Only do once on rising edge of 0-to-1 transition
        if (dio0Was0 == true) {
            dio0Was0 = false;
            OnDio0Irq();
        }
    }
#endif

#if(INAIR_DIO1_IS_INTERRUPT==0)
    if (dio1.read() == 0) {
        dio1Was0 = true;
    }
    else {
        //Only do once on rising edge of 0-to-1 transition
        if (dio1Was0 == true) {
            dio1Was0 = false;
            OnDio1Irq();
        }
    }
#endif

#if(INAIR_DIO2_IS_INTERRUPT==0)
    if (dio2.read() == 0) {
        dio2Was0 = true;
    }
    else {
        //Only do once on rising edge of 0-to-1 transition
        if (dio2Was0 == true) {
            dio2Was0 = false;
            OnDio2Irq();
        }
    }
#endif

#if(INAIR_DIO3_IS_INTERRUPT==0)
    if (dio3.read() == 0) {
        dio3Was0 = true;
    }
    else {
        //Only do once on rising edge of 0-to-1 transition
        if (dio3Was0 == true) {
            dio3Was0 = false;
            OnDio3Irq();
        }
    }
#endif

}


uint8_t InAir::GetBoardType( void )
{
    return boardConnected;
}

void InAir::SetBoardType( uint8_t boardType)
{
    boardConnected = boardType;
}

void InAir::RxChainCalibration( void )
{
    uint8_t regPaConfigInitVal;
    uint32_t initialFreq;

    // Save context
    regPaConfigInitVal = this->Read( REG_PACONFIG );
    initialFreq = ( double )( ( ( uint32_t )this->Read( REG_FRFMSB ) << 16 ) |
                              ( ( uint32_t )this->Read( REG_FRFMID ) << 8 ) |
                              ( ( uint32_t )this->Read( REG_FRFLSB ) ) ) * ( double )FREQ_STEP;

    // Cut the PA just in case, RFO output, power = -1 dBm
    this->Write( REG_PACONFIG, 0x00 );

    // Launch Rx chain calibration for LF band
    Write ( REG_IMAGECAL, ( Read( REG_IMAGECAL ) & RF_IMAGECAL_IMAGECAL_MASK ) | RF_IMAGECAL_IMAGECAL_START );
    while( ( Read( REG_IMAGECAL ) & RF_IMAGECAL_IMAGECAL_RUNNING ) == RF_IMAGECAL_IMAGECAL_RUNNING )
    {
    }

    // Sets a Frequency in HF band
    settings.Channel=  868000000 ;

    // Launch Rx chain calibration for HF band 
    Write ( REG_IMAGECAL, ( Read( REG_IMAGECAL ) & RF_IMAGECAL_IMAGECAL_MASK ) | RF_IMAGECAL_IMAGECAL_START );
    while( ( Read( REG_IMAGECAL ) & RF_IMAGECAL_IMAGECAL_RUNNING ) == RF_IMAGECAL_IMAGECAL_RUNNING )
    {
    }

    // Restore context
    this->Write( REG_PACONFIG, regPaConfigInitVal );
    SetChannel( initialFreq );
}

RadioState InAir::GetStatus( void )
{
    return this->settings.State;
}

void InAir::SetChannel( uint32_t freq )
{
    this->settings.Channel = freq;
    freq = ( uint32_t )( ( double )freq / ( double )FREQ_STEP );
    Write( REG_FRFMSB, ( uint8_t )( ( freq >> 16 ) & 0xFF ) );
    Write( REG_FRFMID, ( uint8_t )( ( freq >> 8 ) & 0xFF ) );
    Write( REG_FRFLSB, ( uint8_t )( freq & 0xFF ) );
}

bool InAir::IsChannelFree( ModemType modem, uint32_t freq, int8_t rssiThresh )
{
    int16_t rssi = 0;
    
    SetModem( modem );

    SetChannel( freq );
    
    SetOpMode( RF_OPMODE_RECEIVER );

    wait_ms( 1 );
    
    rssi = GetRssi( modem );
    
    Sleep( );
    
    if( rssi > ( int16_t )rssiThresh )
    {
        return false;
    }
    return true;
}

uint32_t InAir::Random( void )
{
    uint8_t i;
    uint32_t rnd = 0;

    /*
     * Radio setup for random number generation 
     */
    // Set LoRa modem ON
    SetModem( MODEM_LORA );

    // Disable LoRa modem interrupts
    Write( REG_LR_IRQFLAGSMASK, RFLR_IRQFLAGS_RXTIMEOUT |
                  RFLR_IRQFLAGS_RXDONE |
                  RFLR_IRQFLAGS_PAYLOADCRCERROR |
                  RFLR_IRQFLAGS_VALIDHEADER |
                  RFLR_IRQFLAGS_TXDONE |
                  RFLR_IRQFLAGS_CADDONE |
                  RFLR_IRQFLAGS_FHSSCHANGEDCHANNEL |
                  RFLR_IRQFLAGS_CADDETECTED );

    // Set radio in continuous reception
    SetOpMode( RF_OPMODE_RECEIVER );

    for( i = 0; i < 32; i++ )
    {
        wait_ms( 1 );
        // Unfiltered RSSI value reading. Only takes the LSB value
        rnd |= ( ( uint32_t )Read( REG_LR_RSSIWIDEBAND ) & 0x01 ) << i;
    }

    Sleep( );

    return rnd;
}

/*!
 * Returns the known FSK bandwidth registers value
 *
 * \param [IN] bandwidth Bandwidth value in Hz
 * \retval regValue Bandwidth register value.
 */
uint8_t InAir::GetFskBandwidthRegValue( uint32_t bandwidth )
{
    uint8_t i;

    for( i = 0; i < ( sizeof( FskBandwidths ) / sizeof( FskBandwidth_t ) ) - 1; i++ )
    {
        if( ( bandwidth >= FskBandwidths[i].bandwidth ) && ( bandwidth < FskBandwidths[i + 1].bandwidth ) )
        {
            return FskBandwidths[i].RegValue;
        }
    }
    // ERROR: Value not found
    while( 1 );
}

void InAir::SetRxConfig( ModemType modem, uint32_t bandwidth,
                         uint32_t datarate, uint8_t coderate,
                         uint32_t bandwidthAfc, uint16_t preambleLen,
                         uint16_t symbTimeout, bool fixLen,
                         uint8_t payloadLen,
                         bool crcOn, bool freqHopOn, uint8_t hopPeriod,
                         bool iqInverted, bool rxContinuous )
{
    SetModem( modem );

    switch( modem )
    {
    case MODEM_FSK:
        {
#if (INAIR_ENABLE_FSK==1)
            this->settings.Fsk.Bandwidth = bandwidth;
            this->settings.Fsk.Datarate = datarate;
            this->settings.Fsk.BandwidthAfc = bandwidthAfc;
            this->settings.Fsk.FixLen = fixLen;
            this->settings.Fsk.PayloadLen = payloadLen;
            this->settings.Fsk.CrcOn = crcOn;
            this->settings.Fsk.IqInverted = iqInverted;
            this->settings.Fsk.RxContinuous = rxContinuous;
            this->settings.Fsk.PreambleLen = preambleLen;
            
            datarate = ( uint16_t )( ( double )XTAL_FREQ / ( double )datarate );
            Write( REG_BITRATEMSB, ( uint8_t )( datarate >> 8 ) );
            Write( REG_BITRATELSB, ( uint8_t )( datarate & 0xFF ) );

            Write( REG_RXBW, GetFskBandwidthRegValue( bandwidth ) );
            Write( REG_AFCBW, GetFskBandwidthRegValue( bandwidthAfc ) );

            Write( REG_PREAMBLEMSB, ( uint8_t )( ( preambleLen >> 8 ) & 0xFF ) );
            Write( REG_PREAMBLELSB, ( uint8_t )( preambleLen & 0xFF ) );

            Write( REG_PACKETCONFIG1,
                         ( Read( REG_PACKETCONFIG1 ) & 
                           RF_PACKETCONFIG1_CRC_MASK &
                           RF_PACKETCONFIG1_PACKETFORMAT_MASK ) |
                           ( ( fixLen == 1 ) ? RF_PACKETCONFIG1_PACKETFORMAT_FIXED : RF_PACKETCONFIG1_PACKETFORMAT_VARIABLE ) |
                           ( crcOn << 4 ) );
            if( fixLen == 1 )
            {
                Write( REG_PAYLOADLENGTH, payloadLen );
            }
#endif  //#if (INAIR_ENABLE_FSK==1)
        }
        break;
    case MODEM_LORA:
        {
            if( bandwidth > 9 )
            {
                // Fatal error: Bandwidth must be 0-9 (7.8 - 500khz)
                while( 1 );
            }
            //bandwidth += 7;   //Changed bandwidth from 0-2 to 0-10
            this->settings.LoRa.Bandwidth = bandwidth;
            this->settings.LoRa.Datarate = datarate;
            this->settings.LoRa.Coderate = coderate;
            this->settings.LoRa.FixLen = fixLen;
            this->settings.LoRa.PayloadLen = payloadLen;
            this->settings.LoRa.CrcOn = crcOn;
            this->settings.LoRa.FreqHopOn = freqHopOn;
            this->settings.LoRa.HopPeriod = hopPeriod;
            this->settings.LoRa.IqInverted = iqInverted;
            this->settings.LoRa.RxContinuous = rxContinuous;
            
            if( datarate > 12 )
            {
                datarate = 12;
            }
            else if( datarate < 6 )
            {
                datarate = 6;
            }
        
            //bandwidth 6=62.5, 7=125, 8=250, 9=500, datarate=SF. LowDatarateOptimize is mandatory when symbol length > 16ms
            //LowDatarateOptimize = 0 when (BW=500) or (BW=250 and SF=12),  else it is ON (Tsym > 16ms)
            if( ( ( bandwidth == 8 ) && ( datarate == 12 ) ) ||
                ( ( bandwidth == 7 ) && ( datarate > 10 ) )  ||
                ( ( bandwidth == 6 ) && ( datarate > 9 ) )   ||
                ( ( bandwidth == 5 ) && ( datarate > 9 ) )   ||
                ( ( bandwidth == 4 ) && ( datarate > 8 ) )   ||  ( bandwidth < 4 )
                //The below is actually correct method, but assume BW = 20.8 and lower will always have SF > 8
//                ( ( bandwidth == 3 ) && ( datarate > 8 ) )   ||
//                ( ( bandwidth == 2 ) && ( datarate > 7 ) )   ||
//                ( ( bandwidth == 1 ) && ( datarate > 7 ) )   ||
//                ( ( bandwidth == 0 ) && ( datarate > 6 ) )
                )
            {
                this->settings.LoRa.LowDatarateOptimize = 0x01;
            }
            else
            {
                this->settings.LoRa.LowDatarateOptimize = 0x00;
            }

            Write( REG_LR_MODEMCONFIG1, 
                         ( Read( REG_LR_MODEMCONFIG1 ) &
                           RFLR_MODEMCONFIG1_BW_MASK &
                           RFLR_MODEMCONFIG1_CODINGRATE_MASK &
                           RFLR_MODEMCONFIG1_IMPLICITHEADER_MASK ) |
                           ( bandwidth << 4 ) | ( coderate << 1 ) | 
                           fixLen );
                        
            Write( REG_LR_MODEMCONFIG2,
                         ( Read( REG_LR_MODEMCONFIG2 ) &
                           RFLR_MODEMCONFIG2_SF_MASK &
                           RFLR_MODEMCONFIG2_RXPAYLOADCRC_MASK &
                           RFLR_MODEMCONFIG2_SYMBTIMEOUTMSB_MASK ) |
                           ( datarate << 4 ) | ( crcOn << 2 ) |
                           ( ( symbTimeout >> 8 ) & ~RFLR_MODEMCONFIG2_SYMBTIMEOUTMSB_MASK ) );

            Write( REG_LR_MODEMCONFIG3, 
                         ( Read( REG_LR_MODEMCONFIG3 ) &
                           RFLR_MODEMCONFIG3_LOWDATARATEOPTIMIZE_MASK ) |
                           ( this->settings.LoRa.LowDatarateOptimize << 3 ) );

            Write( REG_LR_SYMBTIMEOUTLSB, ( uint8_t )( symbTimeout & 0xFF ) );
            
            Write( REG_LR_PREAMBLEMSB, ( uint8_t )( ( preambleLen >> 8 ) & 0xFF ) );
            Write( REG_LR_PREAMBLELSB, ( uint8_t )( preambleLen & 0xFF ) );

            if( fixLen == 1 )
            {
                Write( REG_LR_PAYLOADLENGTH, payloadLen );
            }

            if( this->settings.LoRa.FreqHopOn == true )
            {
                Write( REG_LR_PLLHOP, ( Read( REG_LR_PLLHOP ) & RFLR_PLLHOP_FASTHOP_MASK ) | RFLR_PLLHOP_FASTHOP_ON );
                Write( REG_LR_HOPPERIOD, this->settings.LoRa.HopPeriod );
            }

            if( datarate == 6 )
            {
                Write( REG_LR_DETECTOPTIMIZE, 
                             ( Read( REG_LR_DETECTOPTIMIZE ) &
                               RFLR_DETECTIONOPTIMIZE_MASK ) |
                               RFLR_DETECTIONOPTIMIZE_SF6 );
                Write( REG_LR_DETECTIONTHRESHOLD, 
                             RFLR_DETECTIONTHRESH_SF6 );
            }
            else
            {
                Write( REG_LR_DETECTOPTIMIZE,
                             ( Read( REG_LR_DETECTOPTIMIZE ) &
                             RFLR_DETECTIONOPTIMIZE_MASK ) |
                             RFLR_DETECTIONOPTIMIZE_SF7_TO_SF12 );
                Write( REG_LR_DETECTIONTHRESHOLD, 
                             RFLR_DETECTIONTHRESH_SF7_TO_SF12 );
            }
        }
        break;
    }
}

void InAir::SetTxConfig( ModemType modem, int8_t power, uint32_t fdev,
                        uint32_t bandwidth, uint32_t datarate,
                        uint8_t coderate, uint16_t preambleLen,
                        bool fixLen, bool crcOn, bool freqHopOn, 
                        uint8_t hopPeriod, bool iqInverted, uint32_t timeout )
{
    uint8_t paConfig = 0;
    uint8_t paDac = 0;

    SetModem( modem );
    
    paConfig = Read( REG_PACONFIG );
    paDac = Read( REG_PADAC );

    paConfig = ( paConfig & RF_PACONFIG_PASELECT_MASK ) | GetPaSelect( this->settings.Channel );
    paConfig = ( paConfig & RF_PACONFIG_MAX_POWER_MASK ) | 0x70;

    if( ( paConfig & RF_PACONFIG_PASELECT_PABOOST ) == RF_PACONFIG_PASELECT_PABOOST )
    {
        if( power > 17 )
        {
            paDac = ( paDac & RF_PADAC_20DBM_MASK ) | RF_PADAC_20DBM_ON;
        }
        else
        {
            paDac = ( paDac & RF_PADAC_20DBM_MASK ) | RF_PADAC_20DBM_OFF;
        }
        if( ( paDac & RF_PADAC_20DBM_ON ) == RF_PADAC_20DBM_ON )
        {
            if( power < 5 )
            {
                power = 5;
            }
            if( power > 20 )
            {
                power = 20;
            }
            paConfig = ( paConfig & RF_PACONFIG_OUTPUTPOWER_MASK ) | ( uint8_t )( ( uint16_t )( power - 5 ) & 0x0F );
        }
        else
        {
            if( power < 2 )
            {
                power = 2;
            }
            if( power > 17 )
            {
                power = 17;
            }
            paConfig = ( paConfig & RF_PACONFIG_OUTPUTPOWER_MASK ) | ( uint8_t )( ( uint16_t )( power - 2 ) & 0x0F );
        }
    }
    else
    {
        if( power < -1 )
        {
            power = -1;
        }
        if( power > 14 )
        {
            power = 14;
        }
        paConfig = ( paConfig & RF_PACONFIG_OUTPUTPOWER_MASK ) | ( uint8_t )( ( uint16_t )( power + 1 ) & 0x0F );
    }
    Write( REG_PACONFIG, paConfig );
    Write( REG_PADAC, paDac );

    switch( modem )
    {
    case MODEM_FSK:
        {
#if (INAIR_ENABLE_FSK==1)
            this->settings.Fsk.Power = power;
            this->settings.Fsk.Fdev = fdev;
            this->settings.Fsk.Bandwidth = bandwidth;
            this->settings.Fsk.Datarate = datarate;
            this->settings.Fsk.PreambleLen = preambleLen;
            this->settings.Fsk.FixLen = fixLen;
            this->settings.Fsk.CrcOn = crcOn;
            this->settings.Fsk.IqInverted = iqInverted;
            this->settings.Fsk.TxTimeout = timeout;
            
            fdev = ( uint16_t )( ( double )fdev / ( double )FREQ_STEP );
            Write( REG_FDEVMSB, ( uint8_t )( fdev >> 8 ) );
            Write( REG_FDEVLSB, ( uint8_t )( fdev & 0xFF ) );

            datarate = ( uint16_t )( ( double )XTAL_FREQ / ( double )datarate );
            Write( REG_BITRATEMSB, ( uint8_t )( datarate >> 8 ) );
            Write( REG_BITRATELSB, ( uint8_t )( datarate & 0xFF ) );

            Write( REG_PREAMBLEMSB, ( preambleLen >> 8 ) & 0x00FF );
            Write( REG_PREAMBLELSB, preambleLen & 0xFF );

            Write( REG_PACKETCONFIG1,
                         ( Read( REG_PACKETCONFIG1 ) & 
                           RF_PACKETCONFIG1_CRC_MASK &
                           RF_PACKETCONFIG1_PACKETFORMAT_MASK ) |
                           ( ( fixLen == 1 ) ? RF_PACKETCONFIG1_PACKETFORMAT_FIXED : RF_PACKETCONFIG1_PACKETFORMAT_VARIABLE ) |
                           ( crcOn << 4 ) );
#endif  //#if (INAIR_ENABLE_FSK==1)
        }
        break;
    case MODEM_LORA:
        {
            this->settings.LoRa.Power = power;
            if( bandwidth > 9 )
            {
                // Fatal error: Bandwidth must be 0-9 (7.8 - 500khz)
                while( 1 );
            }
            //bandwidth += 7;
            this->settings.LoRa.Bandwidth = bandwidth;
            this->settings.LoRa.Datarate = datarate;
            this->settings.LoRa.Coderate = coderate;
            this->settings.LoRa.PreambleLen = preambleLen;
            this->settings.LoRa.FixLen = fixLen;
            this->settings.LoRa.CrcOn = crcOn;
            this->settings.LoRa.FreqHopOn = freqHopOn;
            this->settings.LoRa.HopPeriod = hopPeriod;
            this->settings.LoRa.IqInverted = iqInverted;
            this->settings.LoRa.TxTimeout = timeout;

            if( datarate > 12 )
            {
                datarate = 12;
            }
            else if( datarate < 6 )
            {
                datarate = 6;
            }
            //bandwidth 6=62.5, 7=125, 8=250, 9=500, datarate=SF. LowDatarateOptimize is mandatory when symbol length > 16ms
            //LowDatarateOptimize = 0 when (BW=500) or (BW=250 and SF=12),  else it is ON (Tsym > 16ms)
            if( ( ( bandwidth == 8 ) && ( datarate == 12 ) ) ||
                ( ( bandwidth == 7 ) && ( datarate > 10 ) )  ||
                ( ( bandwidth == 6 ) && ( datarate > 9 ) )   ||
                ( ( bandwidth == 5 ) && ( datarate > 9 ) )   ||
                ( ( bandwidth == 4 ) && ( datarate > 8 ) )   ||  ( bandwidth < 4 )
                //The below is actually correct method, but assume BW = 20.8 and lower will always have SF > 8
//                ( ( bandwidth == 3 ) && ( datarate > 8 ) )   ||
//                ( ( bandwidth == 2 ) && ( datarate > 7 ) )   ||
//                ( ( bandwidth == 1 ) && ( datarate > 7 ) )   ||
//                ( ( bandwidth == 0 ) && ( datarate > 6 ) )
                )
            {
                this->settings.LoRa.LowDatarateOptimize = 0x01;
            }
            else
            {
                this->settings.LoRa.LowDatarateOptimize = 0x00;
            }
            
            if( this->settings.LoRa.FreqHopOn == true )
            {
                Write( REG_LR_PLLHOP, ( Read( REG_LR_PLLHOP ) & RFLR_PLLHOP_FASTHOP_MASK ) | RFLR_PLLHOP_FASTHOP_ON );
                Write( REG_LR_HOPPERIOD, this->settings.LoRa.HopPeriod );
            }
            
            Write( REG_LR_MODEMCONFIG1, 
                         ( Read( REG_LR_MODEMCONFIG1 ) &
                           RFLR_MODEMCONFIG1_BW_MASK &
                           RFLR_MODEMCONFIG1_CODINGRATE_MASK &
                           RFLR_MODEMCONFIG1_IMPLICITHEADER_MASK ) |
                           ( bandwidth << 4 ) | ( coderate << 1 ) | 
                           fixLen );

            Write( REG_LR_MODEMCONFIG2,
                         ( Read( REG_LR_MODEMCONFIG2 ) &
                           RFLR_MODEMCONFIG2_SF_MASK &
                           RFLR_MODEMCONFIG2_RXPAYLOADCRC_MASK ) |
                           ( datarate << 4 ) | ( crcOn << 2 ) );

            Write( REG_LR_MODEMCONFIG3, 
                         ( Read( REG_LR_MODEMCONFIG3 ) &
                           RFLR_MODEMCONFIG3_LOWDATARATEOPTIMIZE_MASK ) |
                           ( this->settings.LoRa.LowDatarateOptimize << 3 ) );
        
            Write( REG_LR_PREAMBLEMSB, ( preambleLen >> 8 ) & 0x00FF );
            Write( REG_LR_PREAMBLELSB, preambleLen & 0xFF );
            
            if( datarate == 6 )
            {
                Write( REG_LR_DETECTOPTIMIZE, 
                             ( Read( REG_LR_DETECTOPTIMIZE ) &
                               RFLR_DETECTIONOPTIMIZE_MASK ) |
                               RFLR_DETECTIONOPTIMIZE_SF6 );
                Write( REG_LR_DETECTIONTHRESHOLD, 
                             RFLR_DETECTIONTHRESH_SF6 );
            }
            else
            {
                Write( REG_LR_DETECTOPTIMIZE,
                             ( Read( REG_LR_DETECTOPTIMIZE ) &
                             RFLR_DETECTIONOPTIMIZE_MASK ) |
                             RFLR_DETECTIONOPTIMIZE_SF7_TO_SF12 );
                Write( REG_LR_DETECTIONTHRESHOLD, 
                             RFLR_DETECTIONTHRESH_SF7_TO_SF12 );
            }
        }
        break;
    }
}

double InAir::TimeOnAir( ModemType modem, uint8_t pktLen )
{
    double airTime = 0.0;

    switch( modem )
    {
    case MODEM_FSK:
#if (INAIR_ENABLE_FSK==1)
        {
            airTime = ceil( ( 8 * ( this->settings.Fsk.PreambleLen +
                                     ( ( Read( REG_SYNCCONFIG ) & ~RF_SYNCCONFIG_SYNCSIZE_MASK ) + 1 ) +
                                     ( ( this->settings.Fsk.FixLen == 0x01 ) ? 0.0 : 1.0 ) +
                                     ( ( ( Read( REG_PACKETCONFIG1 ) & ~RF_PACKETCONFIG1_ADDRSFILTERING_MASK ) != 0x00 ) ? 1.0 : 0 ) +
                                     pktLen +
                                     ( ( this->settings.Fsk.CrcOn == 0x01 ) ? 2.0 : 0 ) ) /
                                     this->settings.Fsk.Datarate ) * 1e6 );
        }
#endif  //#if (INAIR_ENABLE_FSK==1)
        break;
    case MODEM_LORA:
        {
            double bw = 0.0;
            switch( this->settings.LoRa.Bandwidth )
            {
            case 0: // 7.8 kHz
                bw = 78e2;
                break;
            case 1: // 10.4 kHz
                bw = 104e2;
                break;
            case 2: // 15.6 kHz
                bw = 156e2;
                break;
            case 3: // 20.8 kHz
                bw = 208e2;
                break;
            case 4: // 31.2 kHz
                bw = 312e2;
                break;
            case 5: // 41.4 kHz
                bw = 414e2;
                break;
            case 6: // 62.5 kHz
                bw = 625e2;
                break;
            case 7: // 125 kHz
                bw = 125e3;
                break;
            case 8: // 250 kHz
                bw = 250e3;
                break;
            case 9: // 500 kHz
                bw = 500e3;
                break;
            }

            // Symbol rate : time for one symbol (secs)
            double rs = bw / ( 1 << this->settings.LoRa.Datarate );
            double ts = 1 / rs;
            // time of preamble
            double tPreamble = ( this->settings.LoRa.PreambleLen + 4.25 ) * ts;
            // Symbol length of payload and time
            double tmp = ceil( ( 8 * pktLen - 4 * this->settings.LoRa.Datarate +
                                 28 + 16 * this->settings.LoRa.CrcOn -
                                 ( this->settings.LoRa.FixLen ? 20 : 0 ) ) /
                                 ( double )( 4 * this->settings.LoRa.Datarate -
                                 ( ( this->settings.LoRa.LowDatarateOptimize > 0 ) ? 8 : 0 ) ) ) *
                                 ( this->settings.LoRa.Coderate + 4 );
            double nPayload = 8 + ( ( tmp > 0 ) ? tmp : 0 );
            double tPayload = nPayload * ts;
            // Time on air 
            double tOnAir = tPreamble + tPayload;
            // return us secs
            airTime = floor( tOnAir * 1e6 + 0.999 );
        }
        break;
    }
    return airTime;
}

void InAir::Send( uint8_t *buffer, uint8_t size )
{
    uint32_t txTimeout = 0;

    this->settings.State = IDLE;

    switch( this->settings.Modem )
    {
    case MODEM_FSK:
#if (INAIR_ENABLE_FSK==1)
        {
            this->settings.FskPacketHandler.NbBytes = 0;
            this->settings.FskPacketHandler.Size = size;

            if( this->settings.Fsk.FixLen == false )
            {
                WriteFifo( ( uint8_t* )&size, 1 );
            }
            else
            {
                Write( REG_PAYLOADLENGTH, size );
            }            
            
            if( ( size > 0 ) && ( size <= 64 ) )
            {
                this->settings.FskPacketHandler.ChunkSize = size;
            }
            else
            {
                this->settings.FskPacketHandler.ChunkSize = 32;
            }

            // Write payload buffer
            WriteFifo( buffer, this->settings.FskPacketHandler.ChunkSize );
            this->settings.FskPacketHandler.NbBytes += this->settings.FskPacketHandler.ChunkSize;
            txTimeout = this->settings.Fsk.TxTimeout;
        }
#endif  //#if (INAIR_ENABLE_FSK==1)
        break;
    case MODEM_LORA:
        {
            if( this->settings.LoRa.IqInverted == true )
            {
                Write( REG_LR_INVERTIQ, ( ( Read( REG_LR_INVERTIQ ) & RFLR_INVERTIQ_TX_MASK & RFLR_INVERTIQ_RX_MASK ) | RFLR_INVERTIQ_RX_OFF | RFLR_INVERTIQ_TX_ON ) );
            }
            else
            {
                Write( REG_LR_INVERTIQ, ( ( Read( REG_LR_INVERTIQ ) & RFLR_INVERTIQ_TX_MASK & RFLR_INVERTIQ_RX_MASK ) | RFLR_INVERTIQ_RX_OFF | RFLR_INVERTIQ_TX_OFF ) );
            }      
        
            this->settings.LoRaPacketHandler.Size = size;

            // Initializes the payload size
            Write( REG_LR_PAYLOADLENGTH, size );

            // Full buffer used for Tx            
            Write( REG_LR_FIFOTXBASEADDR, 0 );
            Write( REG_LR_FIFOADDRPTR, 0 );

            // FIFO operations can not take place in Sleep mode
            if( ( Read( REG_OPMODE ) & ~RF_OPMODE_MASK ) == RF_OPMODE_SLEEP )
            {
                Standby( );
                wait_ms( 1 );
            }
            // Write payload buffer
            WriteFifo( buffer, size );
            txTimeout = this->settings.LoRa.TxTimeout;
        }
        break;
    }

    Tx( txTimeout );
}

void InAir::Sleep( void )
{
    // Initialize driver timeout timers
    txTimeoutTimer.detach(  );
    rxTimeoutTimer.detach( );
    SetOpMode( RF_OPMODE_SLEEP );
}

void InAir::Standby( void )
{
    txTimeoutTimer.detach(  );
    rxTimeoutTimer.detach( );
    SetOpMode( RF_OPMODE_STANDBY );
}

void InAir::Rx( uint32_t timeout )
{
    bool rxContinuous = false;

    switch( this->settings.Modem )
    {
    case MODEM_FSK:
#if (INAIR_ENABLE_FSK==1)
        {
            rxContinuous = this->settings.Fsk.RxContinuous;
            
            // DIO0=PayloadReady
            // DIO1=FifoLevel
            // DIO2=SyncAddr
            // DIO3=FifoEmpty
            // DIO4=Preamble
            // DIO5=ModeReady
            Write( REG_DIOMAPPING1, ( Read( REG_DIOMAPPING1 ) & RF_DIOMAPPING1_DIO0_MASK & RF_DIOMAPPING1_DIO1_MASK &
                                                                            RF_DIOMAPPING1_DIO2_MASK ) |
                                                                            RF_DIOMAPPING1_DIO0_00 |
                                                                            RF_DIOMAPPING1_DIO2_11 );
            
            Write( REG_DIOMAPPING2, ( Read( REG_DIOMAPPING2 ) & RF_DIOMAPPING2_DIO4_MASK &
                                                                            RF_DIOMAPPING2_MAP_MASK ) | 
                                                                            RF_DIOMAPPING2_DIO4_11 |
                                                                            RF_DIOMAPPING2_MAP_PREAMBLEDETECT );
            
            this->settings.FskPacketHandler.FifoThresh = Read( REG_FIFOTHRESH ) & 0x3F;
            
            this->settings.FskPacketHandler.PreambleDetected = false;
            this->settings.FskPacketHandler.SyncWordDetected = false;
            this->settings.FskPacketHandler.NbBytes = 0;
            this->settings.FskPacketHandler.Size = 0;
        }
#endif  //#if (INAIR_ENABLE_FSK==1)
        break;
    case MODEM_LORA:
        {
            if( this->settings.LoRa.IqInverted == true )
            {
                Write( REG_LR_INVERTIQ, ( ( Read( REG_LR_INVERTIQ ) & RFLR_INVERTIQ_TX_MASK & RFLR_INVERTIQ_RX_MASK ) | RFLR_INVERTIQ_RX_ON | RFLR_INVERTIQ_TX_OFF ) );
            }
            else
            {
                Write( REG_LR_INVERTIQ, ( ( Read( REG_LR_INVERTIQ ) & RFLR_INVERTIQ_TX_MASK & RFLR_INVERTIQ_RX_MASK ) | RFLR_INVERTIQ_RX_OFF | RFLR_INVERTIQ_TX_OFF ) );
            }         
        
            rxContinuous = this->settings.LoRa.RxContinuous;
            
            if( this->settings.LoRa.FreqHopOn == true )
            {
                Write( REG_LR_IRQFLAGSMASK, //RFLR_IRQFLAGS_RXTIMEOUT |
                                              //RFLR_IRQFLAGS_RXDONE |
                                              //RFLR_IRQFLAGS_PAYLOADCRCERROR |
                                              RFLR_IRQFLAGS_VALIDHEADER |
                                              RFLR_IRQFLAGS_TXDONE |
                                              RFLR_IRQFLAGS_CADDONE |
                                              //RFLR_IRQFLAGS_FHSSCHANGEDCHANNEL |
                                              RFLR_IRQFLAGS_CADDETECTED );
                                              
                // DIO0=RxDone, DIO2=FhssChangeChannel
                Write( REG_DIOMAPPING1, ( Read( REG_DIOMAPPING1 ) & RFLR_DIOMAPPING1_DIO0_MASK & RFLR_DIOMAPPING1_DIO2_MASK  ) | RFLR_DIOMAPPING1_DIO0_00 | RFLR_DIOMAPPING1_DIO2_00 );
            }
            else
            {
                Write( REG_LR_IRQFLAGSMASK, //RFLR_IRQFLAGS_RXTIMEOUT |
                                              //RFLR_IRQFLAGS_RXDONE |
                                              //RFLR_IRQFLAGS_PAYLOADCRCERROR |
                                              RFLR_IRQFLAGS_VALIDHEADER |
                                              RFLR_IRQFLAGS_TXDONE |
                                              RFLR_IRQFLAGS_CADDONE |
                                              RFLR_IRQFLAGS_FHSSCHANGEDCHANNEL |
                                              RFLR_IRQFLAGS_CADDETECTED );
                                              
                // DIO0=RxDone
                Write( REG_DIOMAPPING1, ( Read( REG_DIOMAPPING1 ) & RFLR_DIOMAPPING1_DIO0_MASK ) | RFLR_DIOMAPPING1_DIO0_00 );
            }
            
            Write( REG_LR_FIFORXBASEADDR, 0 );
            Write( REG_LR_FIFOADDRPTR, 0 );
        }
        break;
    }

    memset( rxBuffer, 0, ( size_t )RX_BUFFER_SIZE );

    this->settings.State = RX_DONE;
    if( timeout != 0 )
    {
        rxTimeoutTimer.attach_us( this, &InAir::OnTimeoutIrq, timeout );
    }

    if( this->settings.Modem == MODEM_FSK )
    {
#if (INAIR_ENABLE_FSK==1)
        SetOpMode( RF_OPMODE_RECEIVER );
        
        if( rxContinuous == false )
        {
            rxTimeoutSyncWord.attach_us( this, &InAir::OnTimeoutIrq, ( 8.0 * ( this->settings.Fsk.PreambleLen +
                                                         ( ( Read( REG_SYNCCONFIG ) &
                                                            ~RF_SYNCCONFIG_SYNCSIZE_MASK ) +
                                                         1.0 ) + 1.0 ) /
                                                        ( double )this->settings.Fsk.Datarate ) * 1e6 ) ;
        }
#endif  //#if (INAIR_ENABLE_FSK==1)
    }
    else
    {
        if( rxContinuous == true )
        {
            SetOpMode( RFLR_OPMODE_RECEIVER );
        }
        else
        {
            SetOpMode( RFLR_OPMODE_RECEIVER_SINGLE );
        }
    }
}

void InAir::Tx( uint32_t timeout )
{ 
    switch( this->settings.Modem )
    {
    case MODEM_FSK:
#if (INAIR_ENABLE_FSK==1)
        {
            // DIO0=PacketSent
            // DIO1=FifoLevel
            // DIO2=FifoFull
            // DIO3=FifoEmpty
            // DIO4=LowBat
            // DIO5=ModeReady
            Write( REG_DIOMAPPING1, ( Read( REG_DIOMAPPING1 ) & RF_DIOMAPPING1_DIO0_MASK & RF_DIOMAPPING1_DIO1_MASK &
                                                                            RF_DIOMAPPING1_DIO2_MASK ) );

            Write( REG_DIOMAPPING2, ( Read( REG_DIOMAPPING2 ) & RF_DIOMAPPING2_DIO4_MASK &
                                                                            RF_DIOMAPPING2_MAP_MASK ) );
            this->settings.FskPacketHandler.FifoThresh = Read( REG_FIFOTHRESH ) & 0x3F;
        }
#endif  //#if (INAIR_ENABLE_FSK==1)
        break;
    case MODEM_LORA:
        {
        
            if( this->settings.LoRa.FreqHopOn == true )
            {
                Write( REG_LR_IRQFLAGSMASK, RFLR_IRQFLAGS_RXTIMEOUT |
                                              RFLR_IRQFLAGS_RXDONE |
                                              RFLR_IRQFLAGS_PAYLOADCRCERROR |
                                              RFLR_IRQFLAGS_VALIDHEADER |
                                              //RFLR_IRQFLAGS_TXDONE |
                                              RFLR_IRQFLAGS_CADDONE |
                                              //RFLR_IRQFLAGS_FHSSCHANGEDCHANNEL |
                                              RFLR_IRQFLAGS_CADDETECTED );
                                              
                // DIO0=TxDone
                Write( REG_DIOMAPPING1, ( Read( REG_DIOMAPPING1 ) & RFLR_DIOMAPPING1_DIO0_MASK ) | RFLR_DIOMAPPING1_DIO0_01 );
                // DIO2=FhssChangeChannel
                Write( REG_DIOMAPPING1, ( Read( REG_DIOMAPPING1 ) & RFLR_DIOMAPPING1_DIO2_MASK ) | RFLR_DIOMAPPING1_DIO2_00 );  
            }
            else
            {
                Write( REG_LR_IRQFLAGSMASK, RFLR_IRQFLAGS_RXTIMEOUT |
                                              RFLR_IRQFLAGS_RXDONE |
                                              RFLR_IRQFLAGS_PAYLOADCRCERROR |
                                              RFLR_IRQFLAGS_VALIDHEADER |
                                              //RFLR_IRQFLAGS_TXDONE |
                                              RFLR_IRQFLAGS_CADDONE |
                                              RFLR_IRQFLAGS_FHSSCHANGEDCHANNEL |
                                              RFLR_IRQFLAGS_CADDETECTED );
                                              
                // DIO0=TxDone
                Write( REG_DIOMAPPING1, ( Read( REG_DIOMAPPING1 ) & RFLR_DIOMAPPING1_DIO0_MASK ) | RFLR_DIOMAPPING1_DIO0_01 ); 
            }
        }
        break;
    }

    this->settings.State = TX_DONE;
    txTimeoutTimer.attach_us( this, &InAir::OnTimeoutIrq, timeout );
    SetOpMode( RF_OPMODE_TRANSMITTER );
}

void InAir::StartCad( void )
{
    switch( this->settings.Modem )
    {
    case MODEM_FSK:
        {
           
        }
        break;
    case MODEM_LORA:
        {
            Write( REG_LR_IRQFLAGSMASK, RFLR_IRQFLAGS_RXTIMEOUT |
                                        RFLR_IRQFLAGS_RXDONE |
                                        RFLR_IRQFLAGS_PAYLOADCRCERROR |
                                        RFLR_IRQFLAGS_VALIDHEADER |
                                        RFLR_IRQFLAGS_TXDONE |
                                        //RFLR_IRQFLAGS_CADDONE |
                                        RFLR_IRQFLAGS_FHSSCHANGEDCHANNEL // |
                                        //RFLR_IRQFLAGS_CADDETECTED 
                                        );
                                          
            // DIO3=CADDone
            Write( REG_DIOMAPPING1, ( Read( REG_DIOMAPPING1 ) & RFLR_DIOMAPPING1_DIO0_MASK ) | RFLR_DIOMAPPING1_DIO0_00 );
            
            this->settings.State = CAD;
            SetOpMode( RFLR_OPMODE_CAD );
        }
        break;
    default:
        break;
    }
}

int16_t InAir::GetRssi( ModemType modem )
{
    int16_t rssi = 0;

    switch( modem )
    {
#if (INAIR_ENABLE_FSK==1)
    case MODEM_FSK:
        rssi = -( Read( REG_RSSIVALUE ) >> 1 );
        break;
#endif
    case MODEM_LORA:
        if( this->settings.Channel > RF_MID_BAND_THRESH )
        {
            rssi = RSSI_OFFSET_HF + Read( REG_LR_RSSIVALUE );
        }
        else
        {
            rssi = RSSI_OFFSET_LF + Read( REG_LR_RSSIVALUE );
        }
        break;
    default:
        rssi = -1;
        break;
    }
    return rssi;
}

void InAir::SetOpMode( uint8_t opMode )
{
    if( opMode != previousOpMode )
    {
        previousOpMode = opMode;
        if( opMode == RF_OPMODE_SLEEP )
        {
            //SetAntSwLowPower( true );
        }
        else
        {
            //SetAntSwLowPower( false );
        }
        Write( REG_OPMODE, ( Read( REG_OPMODE ) & RF_OPMODE_MASK ) | opMode );
    }
}

void InAir::SetModem( ModemType modem )
{
    if( this->settings.Modem != modem )
    {
        this->settings.Modem = modem;
        switch( this->settings.Modem )
        {
        default:
        case MODEM_FSK:
            SetOpMode( RF_OPMODE_SLEEP );
            Write( REG_OPMODE, ( Read( REG_OPMODE ) & RFLR_OPMODE_LONGRANGEMODE_MASK ) | RFLR_OPMODE_LONGRANGEMODE_OFF );
        
            Write( REG_DIOMAPPING1, 0x00 );
            Write( REG_DIOMAPPING2, 0x30 ); // DIO5=ModeReady
            break;
        case MODEM_LORA:
            SetOpMode( RF_OPMODE_SLEEP );
            Write( REG_OPMODE, ( Read( REG_OPMODE ) & RFLR_OPMODE_LONGRANGEMODE_MASK ) | RFLR_OPMODE_LONGRANGEMODE_ON );
            Write( 0x30, 0x00 ); //  IF = 0
            Write( REG_LR_DETECTOPTIMIZE, ( Read( REG_LR_DETECTOPTIMIZE ) & 0x7F ) ); // Manual IF
            Write( REG_DIOMAPPING1, 0x00 );
            Write( REG_DIOMAPPING2, 0x00 );
            break;
        }
    }
}

void InAir::OnTimeoutIrq( void )
{
    switch( this->settings.State )
    {
    case RX_DONE:
        if( this->settings.Modem == MODEM_FSK )
        {
#if (INAIR_ENABLE_FSK==1)
            this->settings.FskPacketHandler.PreambleDetected = false;
            this->settings.FskPacketHandler.SyncWordDetected = false;
            this->settings.FskPacketHandler.NbBytes = 0;
            this->settings.FskPacketHandler.Size = 0;

            // Clear Irqs
            Write( REG_IRQFLAGS1, RF_IRQFLAGS1_RSSI | 
                                        RF_IRQFLAGS1_PREAMBLEDETECT |
                                        RF_IRQFLAGS1_SYNCADDRESSMATCH );
            Write( REG_IRQFLAGS2, RF_IRQFLAGS2_FIFOOVERRUN );

            if( this->settings.Fsk.RxContinuous == true )
            {
                // Continuous mode restart Rx chain
                Write( REG_RXCONFIG, Read( REG_RXCONFIG ) | RF_RXCONFIG_RESTARTRXWITHOUTPLLLOCK );
            }
            else
            {
                this->settings.State = IDLE;
                rxTimeoutSyncWord.detach( );
            }
#endif  //#if (INAIR_ENABLE_FSK==1)
        }
        if( ( rxTimeout != NULL ) )
        {
            rxTimeout( );
        }
        break;
    case TX_DONE:
        this->settings.State = IDLE;
        if( ( txTimeout != NULL ) )
        {
            txTimeout( );
        }
        break;
    default:
        break;
    }
}

void InAir::OnDio0Irq( void )
{
    __IO uint8_t irqFlags = 0;
  
    switch( this->settings.State )
    {                
        case RX_DONE:
            //TimerStop( &RxTimeoutTimer );
            // RxDone interrupt
            switch( this->settings.Modem )
            {
#if (INAIR_ENABLE_FSK==1)
            case MODEM_FSK:
                if( this->settings.Fsk.CrcOn == true )
                {
                    irqFlags = Read( REG_IRQFLAGS2 );
                    if( ( irqFlags & RF_IRQFLAGS2_CRCOK ) != RF_IRQFLAGS2_CRCOK )
                    {
                        // Clear Irqs
                        Write( REG_IRQFLAGS1, RF_IRQFLAGS1_RSSI | 
                                                    RF_IRQFLAGS1_PREAMBLEDETECT |
                                                    RF_IRQFLAGS1_SYNCADDRESSMATCH );
                        Write( REG_IRQFLAGS2, RF_IRQFLAGS2_FIFOOVERRUN );
    
                        if( this->settings.Fsk.RxContinuous == false )
                        {
                            this->settings.State = IDLE;
                            rxTimeoutSyncWord.attach_us( this, &InAir::OnTimeoutIrq, (  8.0 * ( this->settings.Fsk.PreambleLen +
                                                             ( ( Read( REG_SYNCCONFIG ) &
                                                                ~RF_SYNCCONFIG_SYNCSIZE_MASK ) +
                                                             1.0 ) + 1.0 ) /
                                                            ( double )this->settings.Fsk.Datarate ) * 1e6 ) ;
                        }
                        else
                        {
                            // Continuous mode restart Rx chain
                            Write( REG_RXCONFIG, Read( REG_RXCONFIG ) | RF_RXCONFIG_RESTARTRXWITHOUTPLLLOCK );
                        }
                        rxTimeoutTimer.detach( );
    
                        if( ( rxError != NULL ) )
                        {
                            rxError( ); 
                        }
                        this->settings.FskPacketHandler.PreambleDetected = false;
                        this->settings.FskPacketHandler.SyncWordDetected = false;
                        this->settings.FskPacketHandler.NbBytes = 0;
                        this->settings.FskPacketHandler.Size = 0;
                        break;
                    }
                }
                
                // Read received packet size
                if( ( this->settings.FskPacketHandler.Size == 0 ) && ( this->settings.FskPacketHandler.NbBytes == 0 ) )
                {
                    if( this->settings.Fsk.FixLen == false )
                    {
                        ReadFifo( ( uint8_t* )&this->settings.FskPacketHandler.Size, 1 );
                    }
                    else
                    {
                        this->settings.FskPacketHandler.Size = Read( REG_PAYLOADLENGTH );
                    }
                    ReadFifo( rxBuffer + this->settings.FskPacketHandler.NbBytes, this->settings.FskPacketHandler.Size - this->settings.FskPacketHandler.NbBytes );
                    this->settings.FskPacketHandler.NbBytes += ( this->settings.FskPacketHandler.Size - this->settings.FskPacketHandler.NbBytes );
                }
                else
                {
                    ReadFifo( rxBuffer + this->settings.FskPacketHandler.NbBytes, this->settings.FskPacketHandler.Size - this->settings.FskPacketHandler.NbBytes );
                    this->settings.FskPacketHandler.NbBytes += ( this->settings.FskPacketHandler.Size - this->settings.FskPacketHandler.NbBytes );
                }
 
                if( this->settings.Fsk.RxContinuous == false )
                {
                    this->settings.State = IDLE;
                    rxTimeoutSyncWord.attach_us( this, &InAir::OnTimeoutIrq, ( 8.0 * ( this->settings.Fsk.PreambleLen +
                                                         ( ( Read( REG_SYNCCONFIG ) &
                                                            ~RF_SYNCCONFIG_SYNCSIZE_MASK ) +
                                                         1.0 ) + 1.0 ) /
                                                        ( double )this->settings.Fsk.Datarate ) * 1e6 ) ;
                }
                else
                {
                    // Continuous mode restart Rx chain
                    Write( REG_RXCONFIG, Read( REG_RXCONFIG ) | RF_RXCONFIG_RESTARTRXWITHOUTPLLLOCK );
                }
                rxTimeoutTimer.detach( );
 
                if( (rxDone != NULL ) )
                {
                    rxDone( rxBuffer, this->settings.FskPacketHandler.Size, this->settings.FskPacketHandler.RssiValue, 0 ); 
                } 
                this->settings.FskPacketHandler.PreambleDetected = false;
                this->settings.FskPacketHandler.SyncWordDetected = false;
                this->settings.FskPacketHandler.NbBytes = 0;
                this->settings.FskPacketHandler.Size = 0;
                break;
#endif  //#if (INAIR_ENABLE_FSK==1)
            case MODEM_LORA:
                {
                    // Clear Irq
                    Write( REG_LR_IRQFLAGS, RFLR_IRQFLAGS_RXDONE );

                    irqFlags = Read( REG_LR_IRQFLAGS );
                    if( ( irqFlags & RFLR_IRQFLAGS_PAYLOADCRCERROR_MASK ) == RFLR_IRQFLAGS_PAYLOADCRCERROR )
                    {
                        // Clear Irq
                        Write( REG_LR_IRQFLAGS, RFLR_IRQFLAGS_PAYLOADCRCERROR );

                        if( this->settings.LoRa.RxContinuous == false )
                        {
                            this->settings.State = IDLE;
                        }
                        // For Continuous reception, we remain in Receive mode. Delete current packet!
                        // - RegFifoAddrPtr = FIFO Pointer used for SPI instructions.
                        // - RegFifoRxByteAddr = Address of last byte written to FIFO + 1 = Start address of next receive packet
                        else {
                            //Get start address of next packet
                            uint8_t startAddNxtPckt = Read(REG_LR_FIFORXBYTEADDR);
                            //Set SPI pointer to address of next packet we will receive = delete current packet!
                            Write(REG_LR_FIFOADDRPTR, startAddNxtPckt);
                        }
                        rxTimeoutTimer.detach( );

                        if( ( rxError != NULL ) )
                        {
                            rxError( ); 
                        }
                        break;
                    }

                    this->settings.LoRaPacketHandler.SnrValue = Read( REG_LR_PKTSNRVALUE );

                    int16_t rssi = Read( REG_LR_PKTRSSIVALUE );
                    if( this->settings.LoRaPacketHandler.SnrValue < 0 )
                    {
                        if( this->settings.Channel > RF_MID_BAND_THRESH )
                        {
                            this->settings.LoRaPacketHandler.RssiValue = RSSI_OFFSET_HF;
                        }
                        else
                        {
                            this->settings.LoRaPacketHandler.RssiValue = RSSI_OFFSET_LF;
                        }
                        this->settings.LoRaPacketHandler.RssiValue += rssi + ( rssi >> 4 )
                                + (this->settings.LoRaPacketHandler.SnrValue / 4);
                    }
                    else
                    {    
                        if( this->settings.Channel > RF_MID_BAND_THRESH )
                        {
                            this->settings.LoRaPacketHandler.RssiValue = RSSI_OFFSET_HF;
                        }
                        else
                        {
                            this->settings.LoRaPacketHandler.RssiValue = RSSI_OFFSET_LF;
                        }
                        this->settings.LoRaPacketHandler.RssiValue += rssi + ( rssi >> 4 );
                    }

                    this->settings.LoRaPacketHandler.Size = Read( REG_LR_RXNBBYTES );

                    //Debug Output:
                    // - RegRxNbBytes = Nb bytes RXed
                    // - RegFifoAddrPtr = FIFO Pointer used for SPI instructions.
                    // - RegFifoRxBaseAddr = Set to 0 in Rx() function
                    // - RegFifoRxCurrentAddr = Start address of last packet received
                    // - RegFifoRxByteAddr = Address of last byte written to FIFO + 1 = Start address of next receive packet
                    //
                    MX_DEBUG("\r\nNB=%d AP=%d RB=%d CA=%d BA=%d", this->settings.LoRaPacketHandler.Size,
                            Read(REG_LR_FIFOADDRPTR),
                            Read(REG_LR_FIFORXBASEADDR),
                            Read(REG_LR_FIFORXCURRENTADDR),
                            Read(REG_LR_FIFORXBYTEADDR));
                    //Ensure we read the last packet received. Without doing this, the bytes read can get out of sync
                    //with the last packet received after errors occur!
                    Write(REG_LR_FIFOADDRPTR, Read(REG_LR_FIFORXCURRENTADDR));
                    ReadFifo( rxBuffer, this->settings.LoRaPacketHandler.Size );
                
                    if( this->settings.LoRa.RxContinuous == false )
                    {
                        this->settings.State = IDLE;
                    }
                    rxTimeoutTimer.detach( );

                    if( ( rxDone != NULL ) )
                    {
                        rxDone( rxBuffer, this->settings.LoRaPacketHandler.Size, this->settings.LoRaPacketHandler.RssiValue, this->settings.LoRaPacketHandler.SnrValue );
                    }
                }
                break;
            default:
                break;
            }
            break;
        case TX_DONE:
            txTimeoutTimer.detach(  );
            // TxDone interrupt
            switch( this->settings.Modem )
            {
            case MODEM_LORA:
                // Clear Irq
                Write( REG_LR_IRQFLAGS, RFLR_IRQFLAGS_TXDONE );
                // Intentional fall through
            case MODEM_FSK:
            default:
                this->settings.State = IDLE;
                if( ( txDone != NULL ) )
                {
                    txDone( ); 
                } 
                break;
            }
            break;
        default:
            break;
    }
}

void InAir::OnDio1Irq( void )
{
    switch( this->settings.State )
    {                
        case RX_DONE:
            switch( this->settings.Modem )
            {
#if (INAIR_ENABLE_FSK==1)
            case MODEM_FSK:
                // FifoLevel interrupt
                // Read received packet size
                if( ( this->settings.FskPacketHandler.Size == 0 ) && ( this->settings.FskPacketHandler.NbBytes == 0 ) )
                {
                    if( this->settings.Fsk.FixLen == false )
                    {
                        ReadFifo( ( uint8_t* )&this->settings.FskPacketHandler.Size, 1 );
                    }
                    else
                    {
                        this->settings.FskPacketHandler.Size = Read( REG_PAYLOADLENGTH );
                    }
                }

                if( ( this->settings.FskPacketHandler.Size - this->settings.FskPacketHandler.NbBytes ) > this->settings.FskPacketHandler.FifoThresh )
                {
                    ReadFifo( ( rxBuffer + this->settings.FskPacketHandler.NbBytes ), this->settings.FskPacketHandler.FifoThresh );
                    this->settings.FskPacketHandler.NbBytes += this->settings.FskPacketHandler.FifoThresh;
                }
                else
                {
                    ReadFifo( ( rxBuffer + this->settings.FskPacketHandler.NbBytes ), this->settings.FskPacketHandler.Size - this->settings.FskPacketHandler.NbBytes );
                    this->settings.FskPacketHandler.NbBytes += ( this->settings.FskPacketHandler.Size - this->settings.FskPacketHandler.NbBytes );
                }
                break;
#endif  //#if (INAIR_ENABLE_FSK==1)
            case MODEM_LORA:
                // Sync time out
                rxTimeoutTimer.detach( );
                this->settings.State = IDLE;
                if( ( rxTimeout != NULL ) )
                {
                    rxTimeout( );
                }
                break;
            default:
                break;
            }
            break;
        case TX_DONE:
            switch( this->settings.Modem )
            {
#if (INAIR_ENABLE_FSK==1)
            case MODEM_FSK:
                // FifoLevel interrupt
                if( ( this->settings.FskPacketHandler.Size - this->settings.FskPacketHandler.NbBytes ) > this->settings.FskPacketHandler.ChunkSize )
                {
                    WriteFifo( ( rxBuffer + this->settings.FskPacketHandler.NbBytes ), this->settings.FskPacketHandler.ChunkSize );
                    this->settings.FskPacketHandler.NbBytes += this->settings.FskPacketHandler.ChunkSize;
                }
                else 
                {
                    // Write the last chunk of data
                    WriteFifo( rxBuffer + this->settings.FskPacketHandler.NbBytes, this->settings.FskPacketHandler.Size - this->settings.FskPacketHandler.NbBytes );
                    this->settings.FskPacketHandler.NbBytes += this->settings.FskPacketHandler.Size - this->settings.FskPacketHandler.NbBytes;
                }
                break;
#endif  //#if (INAIR_ENABLE_FSK==1)
            case MODEM_LORA:
                break;
            default:
                break;
            }
            break;      
        default:
            break;
    }
}

void InAir::OnDio2Irq( void )
{
    switch( this->settings.State )
    {                
        case RX_DONE:
            switch( this->settings.Modem )
            {
#if (INAIR_ENABLE_FSK==1)
            case MODEM_FSK:
                if( ( this->settings.FskPacketHandler.PreambleDetected == true ) && ( this->settings.FskPacketHandler.SyncWordDetected == false ) )
                {
                    rxTimeoutSyncWord.detach( );
                    
                    this->settings.FskPacketHandler.SyncWordDetected = true;
                
                    this->settings.FskPacketHandler.RssiValue = -( Read( REG_RSSIVALUE ) >> 1 );

                    this->settings.FskPacketHandler.AfcValue = ( int32_t )( double )( ( ( uint16_t )Read( REG_AFCMSB ) << 8 ) |
                                                                           ( uint16_t )Read( REG_AFCLSB ) ) *
                                                                           ( double )FREQ_STEP;
                    this->settings.FskPacketHandler.RxGain = ( Read( REG_LNA ) >> 5 ) & 0x07;
                }
                break;
#endif  //#if (INAIR_ENABLE_FSK==1)
            case MODEM_LORA:
                if( this->settings.LoRa.FreqHopOn == true )
                {
                    // Clear Irq
                    Write( REG_LR_IRQFLAGS, RFLR_IRQFLAGS_FHSSCHANGEDCHANNEL );
                    
                    if( ( fhssChangeChannel != NULL ) )
                    {
                        fhssChangeChannel( ( Read( REG_LR_HOPCHANNEL ) & RFLR_HOPCHANNEL_CHANNEL_MASK ) );
                    }
                }    
                break;
            default:
                break;
            }
            break;
        case TX_DONE:
            switch( this->settings.Modem )
            {
            case MODEM_FSK:
                break;
            case MODEM_LORA:
                if( this->settings.LoRa.FreqHopOn == true )
                {
                    // Clear Irq
                    Write( REG_LR_IRQFLAGS, RFLR_IRQFLAGS_FHSSCHANGEDCHANNEL );
                    
                    if( ( fhssChangeChannel != NULL ) )
                    {
                        fhssChangeChannel( ( Read( REG_LR_HOPCHANNEL ) & RFLR_HOPCHANNEL_CHANNEL_MASK ) );
                    }
                }    
                break;
            default:
                break;
            }
            break;      
        default:
            break;
    }
}

void InAir::OnDio3Irq( void )
{
    switch( this->settings.Modem )
    {
    case MODEM_FSK:
        break;
    case MODEM_LORA:
        if( ( Read( REG_LR_IRQFLAGS ) & 0x01 ) == 0x01 )
        {
            // Clear Irq
            Write( REG_LR_IRQFLAGS, RFLR_IRQFLAGS_CADDETECTED_MASK | RFLR_IRQFLAGS_CADDONE);
            if( ( cadDone != NULL ) )
            {
                cadDone( true );
            }
        }
        else
        {        
            // Clear Irq
            Write( REG_LR_IRQFLAGS, RFLR_IRQFLAGS_CADDONE );
            if( ( cadDone != NULL ) )
            {
                cadDone( false );
            }
        }
        break;
    default:
        break;
    }
}

/*
void InAir::OnDio4Irq( void )
{
    switch( this->settings.Modem )
    {
#if (INAIR_ENABLE_FSK==1)
    case MODEM_FSK:
        {
            if( this->settings.FskPacketHandler.PreambleDetected == false )
            {
                this->settings.FskPacketHandler.PreambleDetected = true;
            }    
        }
        break;
#endif  //#if (INAIR_ENABLE_FSK==1)
    case MODEM_LORA:
        break;
    default:
        break;
    }
}

void InAir::OnDio5Irq( void )
{
    switch( this->settings.Modem )
    {
    case MODEM_FSK:
        break;
    case MODEM_LORA:
        break;
    default:
        break;
    }
}
*/

void InAir::Reset( void )
{
    reset.output();
    reset = 0;
    wait_ms( 1 );
    reset.input();
    wait_ms( 6 );
}

void InAir::Write( uint8_t addr, uint8_t data )
{
    Write( addr, &data, 1 );
}

uint8_t InAir::Read( uint8_t addr )
{
    uint8_t data;
    Read( addr, &data, 1 );
    return data;
}

void InAir::Write( uint8_t addr, uint8_t *buffer, uint8_t size )
{
    uint8_t i;

    nss = 0;
    spi.write( addr | 0x80 );
    for( i = 0; i < size; i++ )
    {
        spi.write( buffer[i] );
    }
    nss = 1;
}

void InAir::Read( uint8_t addr, uint8_t *buffer, uint8_t size )
{
    uint8_t i;

    nss = 0;
    spi.write( addr & 0x7F );
    for( i = 0; i < size; i++ )
    {
        buffer[i] = spi.write( 0 );
    }
    nss = 1;
}

void InAir::WriteFifo( uint8_t *buffer, uint8_t size )
{
    Write( 0, buffer, size );
}

void InAir::ReadFifo( uint8_t *buffer, uint8_t size )
{
    Read( 0, buffer, size );
}


//-------------------------------------------------------------------------
//                      Board relative functions
//-------------------------------------------------------------------------

void InAir::IoInit( void )
{
    SpiInit( );
}

static const RadioRegisters_t RadioRegsInit[] =
{
    { MODEM_FSK , REG_LNA                , 0x23 },
    { MODEM_FSK , REG_RXCONFIG           , 0x1E },
    { MODEM_FSK , REG_RSSICONFIG         , 0xD2 },
    { MODEM_FSK , REG_PREAMBLEDETECT     , 0xAA },
    { MODEM_FSK , REG_OSC                , 0x07 },
    { MODEM_FSK , REG_SYNCCONFIG         , 0x12 },
    { MODEM_FSK , REG_SYNCVALUE1         , 0xC1 },
    { MODEM_FSK , REG_SYNCVALUE2         , 0x94 },
    { MODEM_FSK , REG_SYNCVALUE3         , 0xC1 },
    { MODEM_FSK , REG_PACKETCONFIG1      , 0xD8 },
    { MODEM_FSK , REG_FIFOTHRESH         , 0x8F },
    { MODEM_FSK , REG_IMAGECAL           , 0x02 },
    { MODEM_FSK , REG_DIOMAPPING1        , 0x00 },
    { MODEM_FSK , REG_DIOMAPPING2        , 0x30 },
    { MODEM_LORA, REG_LR_PAYLOADMAXLENGTH, 0x40 },
};

void InAir::RadioRegistersInit( ) {
    uint8_t i = 0;
    for( i = 0; i < sizeof( RadioRegsInit ) / sizeof( RadioRegisters_t ); i++ )
    {
        SetModem( RadioRegsInit[i].Modem );
        Write( RadioRegsInit[i].Addr, RadioRegsInit[i].Value );
    }
}

void InAir::SpiInit( void )
{
    nss = 1;
    spi.format( 8,0 );
    uint32_t frequencyToSet = 8000000;
    #if( defined ( TARGET_NUCLEO_L152RE ) || defined (TARGET_NUCLEO_F401RE) || defined ( TARGET_LPC11U6X ) || defined (TARGET_K64F) || defined ( TARGET_NZ32ST1L ) || defined(TARGET_NZ32SC151) )
        spi.frequency( frequencyToSet );
    #elif( defined ( TARGET_KL25Z ) ) //busclock frequency is halved -> double the spi frequency to compensate
        spi.frequency( frequencyToSet * 2 );
    #else
        #warning "Check the board's SPI frequency"
    #endif
    wait(0.1);
}

void InAir::IoIrqInit()
{
    //TARGET_KL25Z board does not have pulldown resistors, seems like TARGET_K64F does have them
    #if( defined ( TARGET_NUCLEO_L152RE )  || defined (TARGET_NUCLEO_F401RE) ||  defined ( TARGET_LPC11U6X ) || defined (TARGET_K64F) || defined ( TARGET_NZ32ST1L ) || defined(TARGET_NZ32SC151))
        dio0.mode(PullDown);
        dio1.mode(PullDown);
        dio2.mode(PullDown);
        dio3.mode(PullDown);
    #endif
#if(INAIR_DIO0_IS_INTERRUPT==1)
        dio0.rise(this, &InAir::OnDio0Irq);
#endif
#if(INAIR_DIO1_IS_INTERRUPT==1)
        dio1.rise( this, &InAir::OnDio1Irq);
#endif
#if(INAIR_DIO2_IS_INTERRUPT==1)
        dio2.rise( this, &InAir::OnDio2Irq);
#endif
#if(INAIR_DIO3_IS_INTERRUPT==1)
        dio3.rise( this, &InAir::OnDio3Irq);
#endif
}

void InAir::IoDeInit( void )
{
    //nothing
}

uint8_t InAir::GetPaSelect( uint32_t channel )
{
    if(boardConnected == BOARD_INAIR9B) {
        return RF_PACONFIG_PASELECT_PABOOST;
    }
    else {
        return RF_PACONFIG_PASELECT_RFO;
    }
}

bool InAir::CheckRfFrequency( uint32_t frequency )
{
    //TODO: Implement check, currently all frequencies are supported
    return true;
}
