/**
 * @file SW3526.cpp
 * @brief SW3526 USB Power Delivery Controller Driver Implementation
 *        Register map based on SW3526S_REG V1.0.0 (2023-01-05)
 */

#include "SW3526.hpp"
#include <cstring>

/*============================================================================
 * SW3526 Class Implementation
 *============================================================================*/

SW3526::SW3526(I2C_HandleTypeDef* i2c, const char* name)
{
    init(i2c, name);
}

SW3526::SW3526(GPIO_TypeDef* sda_port, uint16_t sda_pin,
               GPIO_TypeDef* scl_port, uint16_t scl_pin, const char* name)
{
    initBitbang(sda_port, sda_pin, scl_port, scl_pin, name);
}

void SW3526::init(I2C_HandleTypeDef* i2c, const char* name)
{
    m_i2c = i2c;
    m_i2cType = e_SW3526_I2C_Type::HW_I2C;
    m_name = name ? name : "SW3526";
    m_currentProt = e_SW3526_FastProt::NONE;
}

void SW3526::initBitbang(GPIO_TypeDef* sda_port, uint16_t sda_pin,
                         GPIO_TypeDef* scl_port, uint16_t scl_pin, const char* name)
{
    m_i2c = nullptr;
    m_i2cType = e_SW3526_I2C_Type::BITBANG;
    m_sdaPort = sda_port;
    m_sdaPin = sda_pin;
    m_sclPort = scl_port;
    m_sclPin = scl_pin;
    m_name = name ? name : "SW3526";
    m_currentProt = e_SW3526_FastProt::NONE;
    
    // Configure SDA and SCL as open-drain output with pull-up
    GPIO_InitTypeDef GPIO_InitStruct = {0};
    GPIO_InitStruct.Pin = sda_pin | scl_pin;
    GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_OD;
    GPIO_InitStruct.Pull = GPIO_PULLUP;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;
    HAL_GPIO_Init(sda_port, &GPIO_InitStruct);
    
    // Initialize bus to idle state: both HIGH (open-drain = high-Z, pulled up)
    sda_port->BSRR = sda_pin;
    scl_port->BSRR = scl_pin;
    bitbangDelay();
}

/*============================================================================
 * Bit-bang I2C Primitives
 *============================================================================*/

void SW3526::bitbangDelay()
{
    // Simple NOP-loop delay for ~5us half-period at 64MHz
    for (volatile uint16_t i = 0; i < SW3526Config::BITBANG_DELAY_CYCLES; i++) {
        // NOP
    }
}

void SW3526::bitbangSdaLow(GPIO_TypeDef* port, uint16_t pin)
{
    // Open-drain: write 0 to output -> drives LOW
    port->BRR = pin;
}

void SW3526::bitbangSdaHigh(GPIO_TypeDef* port, uint16_t pin)
{
    // Open-drain: write 1 to output -> high-Z, pulled HIGH by external resistor
    port->BSRR = pin;
}

void SW3526::bitbangSclLow(GPIO_TypeDef* port, uint16_t pin)
{
    port->BRR = pin;
}

void SW3526::bitbangSclHigh(GPIO_TypeDef* port, uint16_t pin)
{
    port->BSRR = pin;
}

uint8_t SW3526::bitbangReadSda(GPIO_TypeDef* port, uint16_t pin)
{
    // Read input data register (open-drain output = can read back)
    return (uint8_t)((port->IDR & pin) ? 1 : 0);
}

void SW3526::bitbangStart()
{
    // SCL=HIGH, SDA=HIGH -> SDA=LOW (while SCL=HIGH)
    bitbangSdaHigh(m_sdaPort, m_sdaPin);
    bitbangDelay();
    bitbangSclHigh(m_sclPort, m_sclPin);
    bitbangDelay();
    bitbangSdaLow(m_sdaPort, m_sdaPin);
    bitbangDelay();
    bitbangSclLow(m_sclPort, m_sclPin);
    bitbangDelay();
}

void SW3526::bitbangStop()
{
    // SCL=LOW, SDA=LOW -> SCL=HIGH -> SDA=HIGH (while SCL=HIGH)
    bitbangSdaLow(m_sdaPort, m_sdaPin);
    bitbangDelay();
    bitbangSclHigh(m_sclPort, m_sclPin);
    bitbangDelay();
    bitbangSdaHigh(m_sdaPort, m_sdaPin);
    bitbangDelay();
}

bool SW3526::bitbangWriteByte(uint8_t data)
{
    // MSB first
    for (uint8_t i = 0; i < 8; i++) {
        if (data & 0x80) {
            bitbangSdaHigh(m_sdaPort, m_sdaPin);
        } else {
            bitbangSdaLow(m_sdaPort, m_sdaPin);
        }
        data <<= 1;
        bitbangDelay();
        bitbangSclHigh(m_sclPort, m_sclPin);
        bitbangDelay();
        bitbangSclLow(m_sclPort, m_sclPin);
        bitbangDelay();
    }
    
    // Release SDA for ACK
    bitbangSdaHigh(m_sdaPort, m_sdaPin);
    bitbangDelay();
    bitbangSclHigh(m_sclPort, m_sclPin);
    bitbangDelay();
    
    // Read ACK (SDA should be LOW for ACK)
    bool ack = (bitbangReadSda(m_sdaPort, m_sdaPin) == 0);
    
    bitbangSclLow(m_sclPort, m_sclPin);
    bitbangDelay();
    
    return ack;
}

uint8_t SW3526::bitbangReadByte(bool ack)
{
    uint8_t data = 0;
    
    // Release SDA (set high, let slave drive)
    bitbangSdaHigh(m_sdaPort, m_sdaPin);
    
    for (uint8_t i = 0; i < 8; i++) {
        data <<= 1;
        bitbangDelay();
        bitbangSclHigh(m_sclPort, m_sclPin);
        bitbangDelay();
        if (bitbangReadSda(m_sdaPort, m_sdaPin)) {
            data |= 0x01;
        }
        bitbangSclLow(m_sclPort, m_sclPin);
        bitbangDelay();
    }
    
    // Send ACK or NACK
    if (ack) {
        bitbangSdaLow(m_sdaPort, m_sdaPin);
    } else {
        bitbangSdaHigh(m_sdaPort, m_sdaPin);
    }
    bitbangDelay();
    bitbangSclHigh(m_sclPort, m_sclPin);
    bitbangDelay();
    bitbangSclLow(m_sclPort, m_sclPin);
    bitbangDelay();
    
    // Release SDA
    bitbangSdaHigh(m_sdaPort, m_sdaPin);
    
    return data;
}

/*============================================================================
 * I2C Communication
 *============================================================================*/

bool SW3526::writeRegHwI2c(uint8_t reg, uint8_t value)
{
    uint8_t data[1] = {value};
    HAL_StatusTypeDef status = HAL_I2C_Mem_Write(
        m_i2c, SW3526Config::I2C_ADDR, reg, I2C_MEMADD_SIZE_8BIT,
        data, 1, SW3526Config::I2C_TIMEOUT
    );
    return (status == HAL_OK);
}

bool SW3526::readRegHwI2c(uint8_t reg, uint8_t* value)
{
    HAL_StatusTypeDef status = HAL_I2C_Mem_Read(
        m_i2c, SW3526Config::I2C_ADDR, reg, I2C_MEMADD_SIZE_8BIT,
        value, 1, SW3526Config::I2C_TIMEOUT
    );
    return (status == HAL_OK);
}

bool SW3526::writeRegBitbang(uint8_t reg, uint8_t value)
{
    bitbangStart();
    
    // Send device address + write
    if (!bitbangWriteByte(SW3526Config::I2C_ADDR & 0xFE)) {
        bitbangStop();
        return false;
    }
    
    // Send register address
    if (!bitbangWriteByte(reg)) {
        bitbangStop();
        return false;
    }
    
    // Send data
    if (!bitbangWriteByte(value)) {
        bitbangStop();
        return false;
    }
    
    bitbangStop();
    return true;
}

bool SW3526::readRegBitbang(uint8_t reg, uint8_t* value)
{
    bitbangStart();
    
    // Send device address + write (to set register pointer)
    if (!bitbangWriteByte(SW3526Config::I2C_ADDR & 0xFE)) {
        bitbangStop();
        return false;
    }
    
    // Send register address
    if (!bitbangWriteByte(reg)) {
        bitbangStop();
        return false;
    }
    
    // Repeated START
    bitbangStart();
    
    // Send device address + read
    if (!bitbangWriteByte(SW3526Config::I2C_ADDR | 0x01)) {
        bitbangStop();
        return false;
    }
    
    // Read byte with NACK (last byte)
    *value = bitbangReadByte(false);
    
    bitbangStop();
    return true;
}

bool SW3526::writeRegInternal(uint8_t reg, uint8_t value)
{
    if (m_i2cType == e_SW3526_I2C_Type::HW_I2C) {
        return writeRegHwI2c(reg, value);
    } else {
        return writeRegBitbang(reg, value);
    }
}

bool SW3526::readRegInternal(uint8_t reg, uint8_t* value)
{
    if (m_i2cType == e_SW3526_I2C_Type::HW_I2C) {
        return readRegHwI2c(reg, value);
    } else {
        return readRegBitbang(reg, value);
    }
}

bool SW3526::writeReg(e_SW3526_Reg reg, uint8_t value)
{
    return writeReg(static_cast<uint8_t>(reg), value);
}

bool SW3526::writeReg(uint8_t reg, uint8_t value)
{
    return writeRegInternal(reg, value);
}

std::optional<uint8_t> SW3526::readReg(e_SW3526_Reg reg)
{
    return readReg(static_cast<uint8_t>(reg));
}

std::optional<uint8_t> SW3526::readReg(uint8_t reg)
{
    uint8_t value;
    if (readRegInternal(reg, &value)) {
        return value;
    }
    return std::nullopt;
}

/*============================================================================
 * Register Helpers
 *============================================================================*/

bool SW3526::modifyRegBit(e_SW3526_Reg reg, uint8_t bit, bool value)
{
    auto regVal = readReg(reg);
    if (!regVal.has_value()) {
        return false;
    }
    uint8_t v = regVal.value();
    if (value) {
        v |= (1 << bit);
    } else {
        v &= ~(1 << bit);
    }
    return writeReg(reg, v);
}

bool SW3526::modifyRegField(e_SW3526_Reg reg, uint8_t shift, uint8_t mask, uint8_t value)
{
    auto regVal = readReg(reg);
    if (!regVal.has_value()) {
        return false;
    }
    uint8_t v = regVal.value();
    v &= ~mask;
    v |= (value << shift) & mask;
    return writeReg(reg, v);
}

/*============================================================================
 * Chip Information
 *============================================================================*/

uint8_t SW3526::getChipVersion()
{
    auto value = readReg(e_SW3526_Reg::CHIP_VER);
    return value.has_value() ? (value.value() & 0x03) : 0;
}

/*============================================================================
 * Voltage/Current Measurements
 *============================================================================*/

void SW3526::setADCLane(e_SW3526_ADC_Lane lane)
{
    auto value = readReg(e_SW3526_Reg::ADC_SENSE);
    if (value.has_value()) {
        uint8_t temp = value.value() & 0xF8;  // Clear lower 3 bits
        temp |= static_cast<uint8_t>(lane);
        writeReg(e_SW3526_Reg::ADC_SENSE, temp);
    }
}

uint16_t SW3526::getVin()
{
    setADCLane(e_SW3526_ADC_Lane::VIN);
    
    auto temp1 = readReg(e_SW3526_Reg::VIN_L);
    auto temp2 = readReg(e_SW3526_Reg::ADC_LSB);
    
    if (!temp1.has_value() || !temp2.has_value()) {
        return 0;
    }
    
    // VIN: 160mV steps, LSB provides fine adjustment (10mV)
    uint16_t vin = temp1.value() * 160 + (temp2.value() & 0x0F) * 10;
    return vin;
}

uint16_t SW3526::getVout()
{
    setADCLane(e_SW3526_ADC_Lane::VOUT);
    
    auto temp1 = readReg(e_SW3526_Reg::VOUT_L);
    auto temp2 = readReg(e_SW3526_Reg::ADC_LSB);
    
    if (!temp1.has_value() || !temp2.has_value()) {
        return 0;
    }
    
    // VOUT: 96mV steps, LSB provides fine adjustment (6mV)
    uint16_t vout = temp1.value() * 96 + (temp2.value() & 0x0F) * 6;
    return vout;
}

uint16_t SW3526::getIout()
{
    setADCLane(e_SW3526_ADC_Lane::IOUT);
    
    auto temp1 = readReg(e_SW3526_Reg::IOUT_L);
    auto temp2 = readReg(e_SW3526_Reg::ADC_LSB);
    
    if (!temp1.has_value() || !temp2.has_value()) {
        return 0;
    }
    
    // IOUT (10mΩ reference): 40mA steps, LSB provides fine adjustment (2.5mA)
    // Scale by sense resistor ratio when using non-default value (e.g. 5mΩ)
    float raw = temp1.value() * 40.0f + temp2.value() * 2.5f;
    uint16_t iout = static_cast<uint16_t>(raw * 10.0f / m_senseResistorMilliOhm);
    return iout;
}

SW3526_Measurement SW3526::getAllMeasurements()
{
    SW3526_Measurement meas;
    meas.vin_mv = getVin();
    meas.vout_mv = getVout();
    meas.iout_ma = getIout();
    meas.power_mw = (meas.vout_mv * meas.iout_ma) / 1000;
    return meas;
}

uint16_t SW3526::getBuckTargetVoltage()
{
    auto h = readReg(e_SW3526_Reg::BUCK_VOL_H);
    auto l = readReg(e_SW3526_Reg::BUCK_VOL_L);
    if (!h.has_value() || !l.has_value()) {
        return 0;
    }
    // buck_vol[11:0] = REG0x03[7:0] << 4 | REG0x04[7:4]
    uint16_t buck_vol = (static_cast<uint16_t>(h.value()) << 4) | ((l.value() >> 4) & 0x0F);
    return buck_vol * 10;  // 10mV per bit
}

uint16_t SW3526::getCCLimit()
{
    auto value = readReg(e_SW3526_Reg::CC_LIMIT);
    if (value.has_value()) {
        // Base 1000mA + register[5:0] * 50mA
        return 1000 + (value.value() & 0x3F) * 50;
    }
    return 0;
}

bool SW3526::setCCLimit(uint16_t limitMa)
{
    if (limitMa < 1000 || limitMa > 4100) {
        return false;
    }
    // ctrl_icc = (limitMa - 1000) / 50
    uint8_t ctrl_icc = static_cast<uint8_t>((limitMa - 1000) / 50);
    // Read current register, preserve upper bits
    auto value = readReg(e_SW3526_Reg::CC_LIMIT);
    if (!value.has_value()) {
        return false;
    }
    uint8_t reg = value.value();
    reg = (reg & 0xC0) | (ctrl_icc & 0x3F);
    return writeReg(e_SW3526_Reg::CC_LIMIT, reg);
}

/*============================================================================
 * System & Fault Status
 *============================================================================*/

SW3526_SysStatus SW3526::getSysStatus()
{
    SW3526_SysStatus status = {};
    auto value = readReg(e_SW3526_Reg::SYS_STATUS);
    if (value.has_value()) {
        status.port_open = (value.value() & 0x02) != 0;  // Bit1
        status.buck_on   = (value.value() & 0x01) != 0;  // Bit0
    }
    return status;
}

uint8_t SW3526::getCCConnection()
{
    auto value = readReg(e_SW3526_Reg::CC_CONN);
    if (value.has_value()) {
        // Bit7=CC1 connected, Bit6=CC2 connected
        uint8_t v = value.value();
        return ((v & 0x80) ? 0x01 : 0x00)
             | ((v & 0x40) ? 0x02 : 0x00);
    }
    return 0;
}

SW3526_FaultFlags SW3526::getFaultStatus()
{
    SW3526_FaultFlags flags = {};
    auto value = readReg(e_SW3526_Reg::FAULT_STATUS);
    if (value.has_value()) {
        uint8_t v = value.value();
        flags.output_uv   = (v & 0x80) != 0;  // Bit7: output under-voltage
        flags.input_uv    = (v & 0x20) != 0;  // Bit5: input under-voltage (1=normal)
        flags.input_ov    = (v & 0x10) != 0;  // Bit4: input over-voltage
        flags.die_ot_warn = (v & 0x04) != 0;  // Bit2: die over-temp warning
        flags.die_ot_shdn = (v & 0x02) != 0;  // Bit1: die over-temp shutdown
        flags.output_sc   = (v & 0x01) != 0;  // Bit0: output short circuit
    }
    return flags;
}

uint8_t SW3526::getMaxPowerReadback()
{
    auto value = readReg(e_SW3526_Reg::MAX_POWER_RD);
    return value.has_value() ? (value.value() & 0x7F) : 0;
}

/*============================================================================
 * Protocol Status
 *============================================================================*/

e_SW3526_FastProt SW3526::getFastStatus()
{
    auto value = readReg(e_SW3526_Reg::FAST_STATUS);
    if (value.has_value()) {
        uint8_t protVal = value.value() & 0x0F;
        // Valid protocol range: 0~11 (NONE~AFC)
        // Values 12+ are reserved/invalid — retry once then treat as NONE
        if (protVal > 11) {
            // Possible I2C glitch — retry
            HAL_Delay(1);
            auto retry = readReg(e_SW3526_Reg::FAST_STATUS);
            if (retry.has_value()) {
                protVal = retry.value() & 0x0F;
            }
            if (protVal > 11) {
                protVal = 0;  // Fallback to NONE
            }
        }
        m_currentProt = static_cast<e_SW3526_FastProt>(protVal);
        return m_currentProt;
    }
    return e_SW3526_FastProt::NONE;  // Read failure: default NONE instead of UNKNOWN
}

e_SW3526_PD_Version SW3526::getPDVersion()
{
    auto value = readReg(e_SW3526_Reg::FAST_STATUS);
    if (value.has_value()) {
        uint8_t ver = (value.value() >> 4) & 0x03;
        return static_cast<e_SW3526_PD_Version>(ver);
    }
    return e_SW3526_PD_Version::NONE;
}

bool SW3526::isFastCharging()
{
    auto value = readReg(e_SW3526_Reg::FAST_STATUS);
    return value.has_value() && (value.value() & 0x80) != 0;
}

bool SW3526::isVoltageFast()
{
    auto value = readReg(e_SW3526_Reg::FAST_STATUS);
    return value.has_value() && (value.value() & 0x40) != 0;
}

const char* SW3526::getProtocolName(e_SW3526_FastProt prot)
{
    switch (prot) {
        case e_SW3526_FastProt::NONE:    return "NONE";
        case e_SW3526_FastProt::QC2:     return "QC2 ";
        case e_SW3526_FastProt::QC3:     return "QC3 ";
        case e_SW3526_FastProt::FCP:     return "FCP ";
        case e_SW3526_FastProt::SCP:     return "SCP ";
        case e_SW3526_FastProt::PD_FIX:  return "PDFX";
        case e_SW3526_FastProt::PD_PPS:  return "PPS ";
        case e_SW3526_FastProt::PE11:    return "PE11";
        case e_SW3526_FastProt::PE20:    return "PE20";
        case e_SW3526_FastProt::VOOC:    return "VOOC";
        case e_SW3526_FastProt::SFCP:    return "SFCP";
        case e_SW3526_FastProt::AFC:     return "AFC ";
        case e_SW3526_FastProt::UNKNOWN: return "UNKN";
        default:                         return "UNKN";
    }
}

/*============================================================================
 * Power Configuration
 *============================================================================*/

uint8_t SW3526::getWattConfig()
{
    auto value = readReg(e_SW3526_Reg::WATT_CONFIG);
    return value.has_value() ? (value.value() & 0x7F) : 0;
}

bool SW3526::setMaxPower(uint8_t watts)
{
    // REG 0xA7 [5:0]: 12~63 = 12W~63W, 0~7 = 64W~71W
    return writeReg(e_SW3526_Reg::POWER_CFG, watts & 0x3F);
}

bool SW3526::setPowerSource(bool useRegister)
{
    // REG 0xAB [2]: 0=external resistor, 1=register
    return modifyRegBit(e_SW3526_Reg::CHG_CFG5, 2, useRegister);
}

bool SW3526::setPortType(e_SW3526_PortType portType)
{
    // REG 0xAB [0]: 0=C port, 1=A port
    return modifyRegBit(e_SW3526_Reg::CHG_CFG5, 0, static_cast<bool>(portType));
}

/*============================================================================
 * Buck Control
 *============================================================================*/

void SW3526::unlockRegisters()
{
    writeReg(e_SW3526_Reg::UNLOCK_1, 0x20);
    HAL_Delay(1);
    writeReg(e_SW3526_Reg::UNLOCK_1, 0x40);
    HAL_Delay(1);
    writeReg(e_SW3526_Reg::UNLOCK_1, 0x80);
    HAL_Delay(1);
}

void SW3526::forceBuckOff1S()
{
    // REG 0x13 [7]: write 1 to force buck off for 1s, auto-clear
    auto buckCtrl = readReg(e_SW3526_Reg::BUCK_CTRL);
    if (buckCtrl.has_value()) {
        writeReg(e_SW3526_Reg::BUCK_CTRL, buckCtrl.value() | 0x80);
    }
}

bool SW3526::setBuckOffCCDisconnect(bool disconnect)
{
    // REG 0x13 [6]: 0=CC1/CC2 keep connected, 1=CC1/CC2 disconnect
    return modifyRegBit(e_SW3526_Reg::BUCK_CTRL, 6, disconnect);
}

bool SW3526::setWireCompEnable(bool enable)
{
    // REG 0x13 [5]: 0=enable wire comp, 1=disable wire comp
    return modifyRegBit(e_SW3526_Reg::BUCK_CTRL, 5, !enable);
}

bool SW3526::setBuckFreq(uint16_t freqKhz)
{
    // REG 0xA6 [7]: 0=125kHz, 1=312kHz
    return modifyRegBit(e_SW3526_Reg::FREQ_CONFIG, 7, freqKhz >= 312);
}

bool SW3526::setWireCompSize(e_SW3526_WireComp comp)
{
    // REG 0xA6 [4]: 0=120mV/A, 1=60mV/A
    return modifyRegBit(e_SW3526_Reg::FREQ_CONFIG, 4, static_cast<bool>(comp));
}

/*============================================================================
 * No-load Voltage
 *============================================================================*/

bool SW3526::setNoLoadVoltage(e_SW3526_NoLoadVolt volt)
{
    // REG 0xA0 [7:6]
    auto value = readReg(e_SW3526_Reg::NOLOAD_VOLT);
    if (!value.has_value()) {
        return false;
    }
    uint8_t reg = value.value() & 0x3F;  // Clear [7:6]
    reg |= (static_cast<uint8_t>(volt) << 6);
    return writeReg(e_SW3526_Reg::NOLOAD_VOLT, reg);
}

/*============================================================================
 * Fast Charge Protocol Control
 *============================================================================*/

const char* SW3526::setFastMode(e_SW3526_FastMode mode)
{
    unlockRegisters();
    
    switch (mode) {
        case e_SW3526_FastMode::FAST_ON: {
            // Enable all protocols: clear all disable bits in CHG_CFG2 (0xA8)
            // CHG_CFG2: bit=0 means enabled, bit=1 means disabled
            // Also enable PD in CHG_CFG3 (0xA9)
            // Also enable fast charge in CHG_CFG6 (0xAC)
            auto cfg2 = readReg(e_SW3526_Reg::CHG_CFG2);
            auto cfg3 = readReg(e_SW3526_Reg::CHG_CFG3);
            if (!cfg2.has_value() || !cfg3.has_value()) {
                return "Error";
            }
            // Clear all protocol disable bits in CHG_CFG2 (bits are active-high disable)
            // Keep reserved bits (bit6=1 default)
            writeReg(e_SW3526_Reg::CHG_CFG2, cfg2.value() & 0x40);
            // Enable PD in CHG_CFG3: clear bit0 (PD enable, 0=enable)
            writeReg(e_SW3526_Reg::CHG_CFG3, cfg3.value() & 0x02);
            // Enable fast charge port: clear bit2 in CHG_CFG6 (0=enable)
            auto cfg6 = readReg(e_SW3526_Reg::CHG_CFG6);
            if (cfg6.has_value()) {
                writeReg(e_SW3526_Reg::CHG_CFG6, cfg6.value() & ~0x04);
            }
            return "FastOn";
        }
        
        case e_SW3526_FastMode::ONLY_PD: {
            // Disable all non-PD protocols in CHG_CFG2, enable PD in CHG_CFG3
            auto cfg2 = readReg(e_SW3526_Reg::CHG_CFG2);
            auto cfg3 = readReg(e_SW3526_Reg::CHG_CFG3);
            if (!cfg2.has_value() || !cfg3.has_value()) {
                return "Error";
            }
            // Set all non-PD protocol disable bits (1=disable)
            writeReg(e_SW3526_Reg::CHG_CFG2, cfg2.value() | 0x3F);  // Disable SCP,SFCP,QC2,QC3,FCP,AFC,PE
            // Enable PD: clear bit0
            writeReg(e_SW3526_Reg::CHG_CFG3, cfg3.value() & 0x02);
            // Enable fast charge port
            auto cfg6 = readReg(e_SW3526_Reg::CHG_CFG6);
            if (cfg6.has_value()) {
                writeReg(e_SW3526_Reg::CHG_CFG6, cfg6.value() & ~0x04);
            }
            return "OnlyPD";
        }
        
        case e_SW3526_FastMode::NO_FAST: {
            // Disable all protocols
            auto cfg2 = readReg(e_SW3526_Reg::CHG_CFG2);
            auto cfg3 = readReg(e_SW3526_Reg::CHG_CFG3);
            if (!cfg2.has_value() || !cfg3.has_value()) {
                return "Error";
            }
            // Disable all non-PD protocols
            writeReg(e_SW3526_Reg::CHG_CFG2, cfg2.value() | 0x3F);
            // Disable PD: set bit0
            writeReg(e_SW3526_Reg::CHG_CFG3, cfg3.value() | 0x01);
            // Disable PPS: set bits [7:6]
            writeReg(e_SW3526_Reg::CHG_CFG3, cfg3.value() | 0xC0);
            // Disable fast charge port: set bit2 in CHG_CFG6
            auto cfg6 = readReg(e_SW3526_Reg::CHG_CFG6);
            if (cfg6.has_value()) {
                writeReg(e_SW3526_Reg::CHG_CFG6, cfg6.value() | 0x04);
            }
            return "NoFast";
        }
        
        default:
            return "";
    }
}

bool SW3526::setFastChargeEnable(bool enable)
{
    // REG 0xAC [2]: 0=enable, 1=disable
    return modifyRegBit(e_SW3526_Reg::CHG_CFG6, 2, !enable);
}

bool SW3526::setPDEnable(bool enable)
{
    // REG 0xA9 [0]: 0=enable, 1=disable
    return modifyRegBit(e_SW3526_Reg::CHG_CFG3, 0, !enable);
}

bool SW3526::setPDVoltageEnable(bool voltage9, bool voltage12, bool voltage15, bool voltage20)
{
    // REG 0xA9: [2]=9V, [3]=12V, [4]=15V, [5]=20V; 0=enable, 1=disable
    auto value = readReg(e_SW3526_Reg::CHG_CFG3);
    if (!value.has_value()) {
        return false;
    }
    uint8_t reg = value.value();
    if (!voltage9)  reg |= 0x04; else reg &= ~0x04;
    if (!voltage12) reg |= 0x08; else reg &= ~0x08;
    if (!voltage15) reg |= 0x10; else reg &= ~0x10;
    if (!voltage20) reg |= 0x20; else reg &= ~0x20;
    return writeReg(e_SW3526_Reg::CHG_CFG3, reg);
}

bool SW3526::setPPSEnable(bool pps0, bool pps1)
{
    // REG 0xA9: [6]=PPS0, [7]=PPS1; 0=enable, 1=disable
    auto value = readReg(e_SW3526_Reg::CHG_CFG3);
    if (!value.has_value()) {
        return false;
    }
    uint8_t reg = value.value();
    if (!pps0) reg |= 0x40; else reg &= ~0x40;
    if (!pps1) reg |= 0x80; else reg &= ~0x80;
    return writeReg(e_SW3526_Reg::CHG_CFG3, reg);
}

bool SW3526::setSCPEnable(bool enable)
{
    // REG 0xA8 [7]: LV SCP enable; 0=enable, 1=disable
    return modifyRegBit(e_SW3526_Reg::CHG_CFG2, 7, !enable);
}

bool SW3526::setSCPMaxPower(e_SW3526_SCP_MaxPower power)
{
    // REG 0xA2 [7]: 0=20W, 1=40W
    return modifyRegBit(e_SW3526_Reg::CHG_CFG0, 7, static_cast<bool>(power));
}

bool SW3526::setHVSCPEnable(bool enable)
{
    // REG 0xA4 [6]: 0=disable, 1=enable
    return modifyRegBit(e_SW3526_Reg::CHG_CFG1, 6, enable);
}

bool SW3526::setPE20HighVoltEnable(bool enable)
{
    // REG 0xA4 [5]: 0=PE doesn't support 12V+, 1=PE supports 12V+
    return modifyRegBit(e_SW3526_Reg::CHG_CFG1, 5, enable);
}

bool SW3526::setQC2Enable(bool enable)
{
    // REG 0xA8 [4]: 0=enable, 1=disable
    return modifyRegBit(e_SW3526_Reg::CHG_CFG2, 4, !enable);
}

bool SW3526::setQC3Enable(bool enable)
{
    // REG 0xA8 [3]: 0=enable, 1=disable
    return modifyRegBit(e_SW3526_Reg::CHG_CFG2, 3, !enable);
}

bool SW3526::setFCPEnable(bool enable)
{
    // REG 0xA8 [2]: 0=enable, 1=disable
    return modifyRegBit(e_SW3526_Reg::CHG_CFG2, 2, !enable);
}

bool SW3526::setAFCEnable(bool enable)
{
    // REG 0xA8 [1]: 0=enable, 1=disable
    return modifyRegBit(e_SW3526_Reg::CHG_CFG2, 1, !enable);
}

bool SW3526::setPEEnable(bool enable)
{
    // REG 0xA8 [0]: 0=enable, 1=disable
    return modifyRegBit(e_SW3526_Reg::CHG_CFG2, 0, !enable);
}

bool SW3526::setSFCPEnable(bool enable)
{
    // REG 0xA8 [5]: 0=enable, 1=disable
    return modifyRegBit(e_SW3526_Reg::CHG_CFG2, 5, !enable);
}

bool SW3526::setDPDMEnable(bool enable)
{
    // REG 0xAA [5]: 0=enable (Apple 2.7V + Samsung 1.2V + fast charge), 1=disable
    return modifyRegBit(e_SW3526_Reg::CHG_CFG4, 5, !enable);
}

bool SW3526::setSCPMaxCurrent(e_SW3526_SCP_MaxCurrent current)
{
    // REG 0xAA: [7]=SCP_max_current[1], [3]=SCP_max_current[0]
    // 0b00=2A, 0b01=2.2A, 0b10=3.5A, 0b11=4A
    uint8_t val = static_cast<uint8_t>(current);
    auto regVal = readReg(e_SW3526_Reg::CHG_CFG4);
    if (!regVal.has_value()) {
        return false;
    }
    uint8_t reg = regVal.value();
    // Clear bit7 and bit3
    reg &= ~0x88;
    // Set bit7 = val[1], bit3 = val[0]
    if (val & 0x02) reg |= 0x80;
    if (val & 0x01) reg |= 0x08;
    return writeReg(e_SW3526_Reg::CHG_CFG4, reg);
}

bool SW3526::setNonPdMaxVoltage(e_SW3526_NonPdMaxVolt volt)
{
    // REG 0xAA [1:0]
    auto value = readReg(e_SW3526_Reg::CHG_CFG4);
    if (!value.has_value()) {
        return false;
    }
    uint8_t reg = value.value() & 0xFC;  // Clear [1:0]
    reg |= static_cast<uint8_t>(volt) & 0x03;
    return writeReg(e_SW3526_Reg::CHG_CFG4, reg);
}

bool SW3526::setQCWireCompEnable(bool enable)
{
    // REG 0xA2 [6]: 0=enable wire comp and offset, 1=disable
    return modifyRegBit(e_SW3526_Reg::CHG_CFG0, 6, !enable);
}

bool SW3526::setPdoVinLink(bool link)
{
    // REG 0xA2 [5]: 0=PDO not linked to Vin, 1=PDO linked to Vin
    return modifyRegBit(e_SW3526_Reg::CHG_CFG0, 5, link);
}

bool SW3526::setPdo5V2ARebroadcast(bool enable)
{
    // REG 0xAC [0]: 0=disable, 1=enable
    return modifyRegBit(e_SW3526_Reg::CHG_CFG6, 0, enable);
}

/*============================================================================
 * PD Commands
 *============================================================================*/

bool SW3526::sendPDCmd(e_SW3526_PDCmd cmd)
{
    // REG 0x70: [7]=send enable (write 1, auto-clear), [3:0]=command 3:0]
    uint8_t reg = 0x80 | (static_cast<uint8_t>(cmd) & 0x0F);
    return writeReg(e_SW3526_Reg::PD_CMD, reg);
}

bool SW3526::sendPDSourceCap()
{
    // REG 0x73: [7]=send Source Cap command (write 1, auto-clear)
    return writeReg(e_SW3526_Reg::PD_SRC_CAP, 0x80);
}

/*============================================================================
 * Dual-chip Dynamic Power
 *============================================================================*/

bool SW3526::setDualChipResetPower(bool resetAll, bool enable)
{
    // REG 0xAD: [7]=resetAll (0=non-PD only, 1=all), [6]=enable (0=enable, 1=disable)
    auto value = readReg(e_SW3526_Reg::CHG_CFG7);
    if (!value.has_value()) {
        return false;
    }
    uint8_t reg = value.value() & 0x3F;  // Clear [7:6]
    if (resetAll) reg |= 0x80;
    if (!enable)  reg |= 0x40;  // 0=enable, 1=disable
    return writeReg(e_SW3526_Reg::CHG_CFG7, reg);
}

/*============================================================================
 * Apple/Samsung Configuration
 *============================================================================*/

bool SW3526::setAppleModeEnable(bool enable)
{
    // REG 0xAE [2]: 0=disable, 1=enable
    return modifyRegBit(e_SW3526_Reg::CHG_CFG8, 2, enable);
}

bool SW3526::setNonPdPowerLimit18W(bool limit18W)
{
    // REG 0xAE [6]: 0=system max power, 1=18W limit
    return modifyRegBit(e_SW3526_Reg::CHG_CFG8, 6, limit18W);
}

bool SW3526::setPDResponseTime(bool fast)
{
    // REG 0xAE [0]: 0=4ms, 1=1ms
    return modifyRegBit(e_SW3526_Reg::CHG_CFG8, 0, fast);
}

bool SW3526::setAppleSamsungRefVolt(uint8_t voltage)
{
    // REG 0xAF [3:2]: 0=1.05V, 1=1.3V, 2=1.4V, 3=1.7V
    auto value = readReg(e_SW3526_Reg::CHG_CFG9);
    if (!value.has_value()) {
        return false;
    }
    uint8_t reg = value.value() & 0xCF;  // Clear [3:2]
    reg |= (voltage & 0x03) << 2;
    return writeReg(e_SW3526_Reg::CHG_CFG9, reg);
}

bool SW3526::setAppleDriveCurrent(uint8_t current)
{
    // REG 0xAF [1:0]: 0=16uA, 1=25uA, 2=38uA, 3=47uA
    auto value = readReg(e_SW3526_Reg::CHG_CFG9);
    if (!value.has_value()) {
        return false;
    }
    uint8_t reg = value.value() & 0xFC;  // Clear [1:0]
    reg |= current & 0x03;
    return writeReg(e_SW3526_Reg::CHG_CFG9, reg);
}

/*============================================================================
 * SW3526_Manager Implementation
 *============================================================================*/

int8_t SW3526_Manager::addDevice(SW3526* device)
{
    if (m_count >= MAX_DEVICES || device == nullptr) {
        return -1;
    }
    
    m_devices[m_count] = device;
    return static_cast<int8_t>(m_count++);
}

SW3526* SW3526_Manager::getDevice(size_t index)
{
    if (index >= m_count) {
        return nullptr;
    }
    return m_devices[index];
}

void SW3526_Manager::forEach(std::function<void(SW3526*, size_t)> callback)
{
    for (size_t i = 0; i < m_count; i++) {
        if (m_devices[i]) {
            callback(m_devices[i], i);
        }
    }
}

/*============================================================================
 * C API Compatibility Layer
 *============================================================================*/

extern "C" {

void v_SW3526_Init(SW3526* dev, I2C_HandleTypeDef* i2c, const char* name)
{
    dev->init(i2c, name);
}

void v_SW3526_InitBitbang(SW3526* dev, GPIO_TypeDef* sda_port, uint16_t sda_pin,
                          GPIO_TypeDef* scl_port, uint16_t scl_pin, const char* name)
{
    dev->initBitbang(sda_port, sda_pin, scl_port, scl_pin, name);
}

uint16_t us_SW3526_GetVin(SW3526* dev)
{
    return dev->getVin();
}

uint16_t us_SW3526_GetVout(SW3526* dev)
{
    return dev->getVout();
}

uint16_t us_SW3526_GetIout(SW3526* dev)
{
    return dev->getIout();
}

} // extern "C"
