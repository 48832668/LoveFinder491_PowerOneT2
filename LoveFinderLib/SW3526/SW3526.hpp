/**
 * @file SW3526.hpp
 * @brief SW3526 USB Power Delivery Controller Driver - C++17
 * @author LoveFinder
 * @date 2026
 * 
 * Multi-slave support with HW I2C or software bit-bang I2C
 * Register map based on SW3526S_REG V1.0.0 (2023-01-05)
 */

#ifndef SW3526_HPP
#define SW3526_HPP

#include "main.h"
#include <cstdint>
#include <optional>
#include <array>
#include <functional>

/*============================================================================
 * Constants
 *============================================================================*/

namespace SW3526Config {
    constexpr uint8_t I2C_ADDR = (0x3C << 1);  // 7-bit address shifted left
    constexpr uint32_t I2C_TIMEOUT = 0xFF;
    constexpr uint16_t BITBANG_DELAY_CYCLES = 160;  // ~2.5us half-period at 64MHz (~200kHz I2C)
}

/*============================================================================
 * Enumerations
 *============================================================================*/

/**
 * @brief I2C interface type
 */
enum class e_SW3526_I2C_Type : uint8_t {
    HW_I2C   = 0,   // Hardware I2C peripheral
    BITBANG  = 1    // Software bit-bang I2C
};

/**
 * @brief ADC measurement lane selection
 */
enum class e_SW3526_ADC_Lane : uint8_t {
    VIN  = 1,   // Input voltage (10mV steps)
    VOUT = 2,   // Output voltage (6mV steps)
    IOUT = 3    // Output current (2.5mA steps)
};

/**
 * @brief Fast charging protocol types (REG 0x06 [3:0])
 */
enum class e_SW3526_FastProt : uint8_t {
    NONE    = 0,
    QC2     = 1,    // Qualcomm Quick Charge 2.0
    QC3     = 2,    // Qualcomm Quick Charge 3.0
    FCP     = 3,    // Huawei Fast Charge Protocol
    SCP     = 4,    // Super Charge Protocol
    PD_FIX  = 5,    // USB PD Fixed Voltage
    PD_PPS  = 6,    // USB PD Programmable Power Supply
    PE11    = 7,    // MediaTek Pump Express 1.1
    PE20    = 8,    // MediaTek Pump Express 2.0
    VOOC    = 9,    // OPPO VOOC
    SFCP    = 10,   // Samsung Fast Charge Protocol
    AFC     = 11,   // Samsung Adaptive Fast Charging
    UNKNOWN = 12
};

/**
 * @brief Fast charge mode configuration
 */
enum class e_SW3526_FastMode : uint8_t {
    FAST_ON  = 0,   // Enable all fast charge
    ONLY_PD  = 1,   // Only PD protocol
    NO_FAST  = 2    // Disable fast charge
};

/**
 * @brief PD version (REG 0x06 [5:4])
 */
enum class e_SW3526_PD_Version : uint8_t {
    NONE   = 0,
    PD2_0  = 1,
    PD3_0  = 2
};

/**
 * @brief No-load voltage setting (REG 0xA0 [7:6])
 */
enum class e_SW3526_NoLoadVolt : uint8_t {
    V5_12 = 0,
    V5_08 = 1,
    V5_04 = 2,
    V5_00 = 3
};

/**
 * @brief SCP max power (REG 0xA2 [7])
 */
enum class e_SW3526_SCP_MaxPower : uint8_t {
    W20 = 0,
    W40 = 1
};

/**
 * @brief SCP max current (REG 0xAA [7:3] combined)
 */
enum class e_SW3526_SCP_MaxCurrent : uint8_t {
    A2_0 = 0,
    A2_2 = 1,
    A3_5 = 2,
    A4_0 = 3
};

/**
 * @brief Non-PD max output voltage (REG 0xAA [1:0])
 */
enum class e_SW3526_NonPdMaxVolt : uint8_t {
    SAME_AS_PD = 0,
    V9  = 1,
    V12 = 2,
    V20 = 3
};

/**
 * @brief Port type (REG 0xAB [0])
 */
enum class e_SW3526_PortType : uint8_t {
    TYPE_C = 0,
    TYPE_A = 1
};

/**
 * @brief Wire compensation size (REG 0xA6 [4])
 */
enum class e_SW3526_WireComp : uint8_t {
    MV_120_PER_A = 0,
    MV_60_PER_A  = 1
};

/**
 * @brief PD command (REG 0x70 [3:0])
 */
enum class e_SW3526_PDCmd : uint8_t {
    HARD_RESET = 1
};

/**
 * @brief Register addresses - complete map per SW3526S_REG V1.0.0
 */
enum class e_SW3526_Reg : uint8_t {
    // --- Read-only status registers ---
    CHIP_VER      = 0x01,  // Chip version
    BUCK_VOL_H    = 0x03,  // Buck output voltage high 8-bit [11:4]
    BUCK_VOL_L    = 0x04,  // Buck output voltage low 4-bit [3:0]
    CC_LIMIT      = 0x05,  // Buck output current limit
    FAST_STATUS   = 0x06,  // Fast charge protocol indication
    SYS_STATUS    = 0x07,  // System status (port open, buck on)
    CC_CONN       = 0x0A,  // CC1/CC2 connection status
    FAULT_STATUS  = 0x0B,  // Fault/abnormal status (UV/OV/OT/SC)
    MAX_POWER_RD  = 0x0D,  // Max output power (read-only, 1W/bit)

    // --- Write-protected control registers ---
    UNLOCK_1      = 0x12,  // I2C write enable (unlock sequence)
    BUCK_CTRL     = 0x13,  // Buck control (force off, wire comp)

    // --- ADC data registers ---
    VIN_L         = 0x30,  // ADC Vin data
    VOUT_L        = 0x31,  // ADC Vout data
    IOUT_L        = 0x33,  // ADC Iout data
    ADC_SENSE     = 0x3A,  // ADC config (lane select)
    ADC_DATA_H    = 0x3B,  // ADC data high 8-bit [11:4]
    ADC_LSB       = 0x3C,  // ADC data low 4-bit [3:0]

    // --- PD command registers ---
    PD_CMD        = 0x70,  // PD command request
    PD_SRC_CAP    = 0x73,  // PD Source Cap command

    // --- Power configuration ---
    WATT_CONFIG   = 0x68,  // Power config (same as MAX_POWER_RD mapping)

    // --- Fast charge configuration (0xA0~0xAF, write-protected) ---
    NOLOAD_VOLT   = 0xA0,  // No-load voltage setting
    CHG_CFG0      = 0xA2,  // Fast charge config 0 (SCP power, QC wire comp, PDO-Vin link)
    CHG_CFG1      = 0xA4,  // Fast charge config 1 (HV SCP, PE2.0 12V+)
    FREQ_CONFIG   = 0xA6,  // Buck freq + wire comp size
    POWER_CFG     = 0xA7,  // Max output power config (register mode)
    CHG_CFG2      = 0xA8,  // Fast charge config 2 (LV SCP, SFCP, QC2, QC3, FCP, AFC, PE)
    CHG_CFG3      = 0xA9,  // Fast charge config 3 (PPS, PD 20/15/12/9V, PD enable)
    CHG_CFG4      = 0xAA,  // Fast charge config 4 (SCP current, DPDM, non-PD max V)
    CHG_CFG5      = 0xAB,  // Fast charge config 5 (power source, port type)
    CHG_CFG6      = 0xAC,  // Fast charge config 6 (fast charge enable, PDO rebroadcast)
    CHG_CFG7      = 0xAD,  // Fast charge config 7 (dual-chip dynamic power)
    CHG_CFG8      = 0xAE,  // Fast charge config 8 (Apple mode, PD response time)
    CHG_CFG9      = 0xAF   // Fast charge config 9 (Apple/Samsung voltage, drive current)
};

/*============================================================================
 * SW3526 Fault Status Flags (REG 0x0B)
 *============================================================================*/

struct SW3526_FaultFlags {
    bool output_uv   : 1;  // Output under-voltage
    bool input_uv    : 1;  // Input under-voltage (inverted: 1=normal)
    bool input_ov    : 1;  // Input over-voltage
    bool die_ot_warn : 1;  // Die over-temperature warning
    bool die_ot_shdn : 1;  // Die over-temperature shutdown
    bool output_sc   : 1;  // Output short circuit
};

/*============================================================================
 * SW3526 System Status Flags (REG 0x07)
 *============================================================================*/

struct SW3526_SysStatus {
    bool port_open : 1;  // Port is open (device connected)
    bool buck_on   : 1;  // Buck converter is running
};

/*============================================================================
 * SW3526 Device Configuration
 *============================================================================*/

struct SW3526_Config {
    I2C_HandleTypeDef* i2c;         // I2C peripheral handle (HW_I2C only)
    e_SW3526_I2C_Type i2c_type;     // Interface type
    const char* name;               // Device name for logging
    
    // GPIO pins for bit-bang I2C (BITBANG only)
    GPIO_TypeDef* sda_port;         // SDA GPIO port
    uint16_t sda_pin;               // SDA GPIO pin
    GPIO_TypeDef* scl_port;         // SCL GPIO port
    uint16_t scl_pin;               // SCL GPIO pin
};

/*============================================================================
 * SW3526 Measurement Result
 *============================================================================*/

struct SW3526_Measurement {
    uint16_t vin_mv;    // Input voltage in mV
    uint16_t vout_mv;   // Output voltage in mV
    uint16_t iout_ma;   // Output current in mA
    uint16_t power_mw;  // Power in mW (calculated)
};

/*============================================================================
 * SW3526 Class
 *============================================================================*/

class SW3526 {
public:
    /**
     * @brief Default constructor
     */
    SW3526() = default;
    
    /**
     * @brief Constructor for HW I2C
     * @param i2c I2C handle pointer
     * @param name Device name
     */
    SW3526(I2C_HandleTypeDef* i2c, const char* name = "SW3526");
    
    /**
     * @brief Constructor for bit-bang I2C
     * @param sda_port SDA GPIO port
     * @param sda_pin SDA GPIO pin
     * @param scl_port SCL GPIO port
     * @param scl_pin SCL GPIO pin
     * @param name Device name
     */
    SW3526(GPIO_TypeDef* sda_port, uint16_t sda_pin,
           GPIO_TypeDef* scl_port, uint16_t scl_pin, const char* name = "SW3526");
    
    /**
     * @brief Initialize for HW I2C
     * @param i2c I2C handle pointer
     * @param name Device name
     */
    void init(I2C_HandleTypeDef* i2c, const char* name = "SW3526");
    
    /**
     * @brief Initialize for bit-bang I2C
     * @param sda_port SDA GPIO port
     * @param sda_pin SDA GPIO pin
     * @param scl_port SCL GPIO port
     * @param scl_pin SCL GPIO pin
     * @param name Device name
     */
    void initBitbang(GPIO_TypeDef* sda_port, uint16_t sda_pin,
                     GPIO_TypeDef* scl_port, uint16_t scl_pin, const char* name = "SW3526");
    
    /**
     * @brief Set current sense resistor value for correct current readback.
     *        SW3526 ADC is calibrated for 10mΩ. If a different sense resistor
     *        is used on the PCB, the raw ADC reading needs scaling.
     * @param milliOhm Sense resistor value in milliohms (default: 10).
     *        e.g. 5 for 5mΩ, 10 for 10mΩ.
     * 
     * @note With a 5mΩ sense resistor: actual current = ADC reading × 2
     *       because V = I × R, half the R gives half the voltage at same current.
     */
    void setSenseResistor(uint16_t milliOhm) { m_senseResistorMilliOhm = milliOhm; }
    
    // ========== Register Operations ==========
    
    /**
     * @brief Write to register
     * @param reg Register address
     * @param value Value to write
     * @return true on success
     */
    bool writeReg(e_SW3526_Reg reg, uint8_t value);
    bool writeReg(uint8_t reg, uint8_t value);
    
    /**
     * @brief Read from register
     * @param reg Register address
     * @return Register value or nullopt on error
     */
    std::optional<uint8_t> readReg(e_SW3526_Reg reg);
    std::optional<uint8_t> readReg(uint8_t reg);
    
    // ========== Chip Information ==========
    
    /**
     * @brief Get chip version
     * @return Chip version (typically 2)
     */
    uint8_t getChipVersion();
    
    // ========== Voltage/Current Measurements ==========
    
    /**
     * @brief Get input voltage
     * @return Voltage in mV
     */
    uint16_t getVin();
    
    /**
     * @brief Get output voltage
     * @return Voltage in mV
     */
    uint16_t getVout();
    
    /**
     * @brief Get output current
     * @return Current in mA
     */
    uint16_t getIout();
    
    /**
     * @brief Get all measurements at once
     * @return Measurement struct
     */
    SW3526_Measurement getAllMeasurements();
    
    /**
     * @brief Get Buck target output voltage (REG 0x03/0x04)
     * @return Target voltage in mV (buck_vol * 10mV)
     */
    uint16_t getBuckTargetVoltage();
    
    /**
     * @brief Get CC current limit (REG 0x05)
     * @return Current limit in mA (1000 + reg * 50mA)
     */
    uint16_t getCCLimit();
    
    /**
     * @brief Set CC current limit (REG 0x05, write-protected)
     * @param limitMa Current limit in mA (1000~4100, rounded to 50mA steps)
     * @return true on success
     */
    bool setCCLimit(uint16_t limitMa);
    
    // ========== System & Fault Status ==========
    
    /**
     * @brief Get system status (REG 0x07)
     * @return SysStatus struct with port_open and buck_on flags
     */
    SW3526_SysStatus getSysStatus();
    
    /**
     * @brief Get CC1/CC2 connection status (REG 0x0A)
     * @return Bit0=CC1 connected, Bit1=CC2 connected
     */
    uint8_t getCCConnection();
    
    /**
     * @brief Get fault/abnormal status (REG 0x0B)
     * @return FaultFlags struct with individual fault bits
     */
    SW3526_FaultFlags getFaultStatus();
    
    /**
     * @brief Get max output power readback (REG 0x0D, read-only)
     * @return Power in Watts (1W/bit)
     */
    uint8_t getMaxPowerReadback();
    
    // ========== Protocol Status ==========
    
    /**
     * @brief Get current fast charge protocol (REG 0x06)
     * @return Protocol enum
     */
    e_SW3526_FastProt getFastStatus();
    
    /**
     * @brief Get PD version (REG 0x06 [5:4])
     * @return PD version enum
     */
    e_SW3526_PD_Version getPDVersion();
    
    /**
     * @brief Check if currently in fast charge (REG 0x06 [7])
     * @return true if fast charge protocol is active
     */
    bool isFastCharging();
    
    /**
     * @brief Check if voltage is in fast charge range (REG 0x06 [6])
     * @return true if voltage is in fast charge
     */
    bool isVoltageFast();
    
    /**
     * @brief Get protocol name string
     * @param prot Protocol enum
     * @return Protocol name string
     */
    static const char* getProtocolName(e_SW3526_FastProt prot);
    
    // ========== Power Configuration ==========
    
    /**
     * @brief Get power configuration (REG 0x68)
     * @return Power in Watts
     */
    uint8_t getWattConfig();
    
    /**
     * @brief Set max output power via register (REG 0xA7)
     *        Requires REG 0xAB[2]=1 (register mode) to take effect.
     *        After modifying, need voltage adjustment or buck restart to apply.
     * @param watts Power in Watts (12~63 for 12W~63W, 0~7 for 64W~71W)
     * @return true on success
     */
    bool setMaxPower(uint8_t watts);
    
    /**
     * @brief Set power configuration source (REG 0xAB [2])
     * @param useRegister true=use register (REG 0xA7), false=use external resistor
     * @return true on success
     */
    bool setPowerSource(bool useRegister);
    
    /**
     * @brief Set port type (REG 0xAB [0])
     * @param portType TYPE_C or TYPE_A
     * @return true on success
     */
    bool setPortType(e_SW3526_PortType portType);
    
    // ========== Buck Control ==========
    
    /**
     * @brief Unlock protected registers (REG 0x12)
     */
    void unlockRegisters();
    
    /**
     * @brief Force buck converter off for 1 second (REG 0x13 [7])
     */
    void forceBuckOff1S();
    
    /**
     * @brief Set whether CC1/CC2 disconnect during forced buck off (REG 0x13 [6])
     * @param disconnect true=CC1/CC2 disconnect, false=keep connected
     * @return true on success
     */
    bool setBuckOffCCDisconnect(bool disconnect);
    
    /**
     * @brief Enable/disable wire compensation (REG 0x13 [5])
     * @param enable true=enable wire compensation
     * @return true on success
     */
    bool setWireCompEnable(bool enable);
    
    /**
     * @brief Set Buck switching frequency (REG 0xA6 [7])
     * @param freqKhz 125 for 125kHz, 312 for 312kHz
     * @return true on success
     */
    bool setBuckFreq(uint16_t freqKhz);
    
    /**
     * @brief Set wire compensation size (REG 0xA6 [4])
     * @param comp 120mV/A or 60mV/A
     * @return true on success
     */
    bool setWireCompSize(e_SW3526_WireComp comp);
    
    // ========== No-load Voltage ==========
    
    /**
     * @brief Set no-load output voltage (REG 0xA0 [7:6])
     * @param volt No-load voltage enum
     * @return true on success
     */
    bool setNoLoadVoltage(e_SW3526_NoLoadVolt volt);
    
    // ========== Fast Charge Protocol Control ==========
    
    /**
     * @brief Set fast charge mode (controls REG 0xA8/0xA9/0xAC)
     * @param mode Fast charge mode
     * @return Mode name string
     */
    const char* setFastMode(e_SW3526_FastMode mode);
    
    /**
     * @brief Enable/disable fast charge on port (REG 0xAC [2])
     * @param enable true to enable fast charge
     * @return true on success
     */
    bool setFastChargeEnable(bool enable);
    
    /**
     * @brief Enable/disable PD protocol (REG 0xA9 [0])
     * @param enable true to enable
     * @return true on success
     */
    bool setPDEnable(bool enable);
    
    /**
     * @brief Enable/disable PD voltage levels (REG 0xA9 [5:2])
     * @param voltage9 true=enable 9V PDO
     * @param voltage12 true=enable 12V PDO
     * @param voltage15 true=enable 15V PDO
     * @param voltage20 true=enable 20V PDO
     * @return true on success
     */
    bool setPDVoltageEnable(bool voltage9, bool voltage12, bool voltage15, bool voltage20);
    
    /**
     * @brief Enable/disable PPS (REG 0xA9 [7:6])
     * @param pps0 true=enable PPS0
     * @param pps1 true=enable PPS1
     * @return true on success
     */
    bool setPPSEnable(bool pps0, bool pps1);
    
    /**
     * @brief Enable/disable SCP protocol (REG 0xA8 [7] for LV SCP)
     * @param enable true to enable
     * @return true on success
     */
    bool setSCPEnable(bool enable);
    
    /**
     * @brief Set SCP max power (REG 0xA2 [7])
     * @param power 20W or 40W
     * @return true on success
     */
    bool setSCPMaxPower(e_SW3526_SCP_MaxPower power);
    
    /**
     * @brief Enable/disable HV SCP (REG 0xA4 [6])
     * @param enable true to enable
     * @return true on success
     */
    bool setHVSCPEnable(bool enable);
    
    /**
     * @brief Enable/disable PE2.0 12V+ (REG 0xA4 [5])
     * @param enable true to enable
     * @return true on success
     */
    bool setPE20HighVoltEnable(bool enable);
    
    /**
     * @brief Enable/disable QC2.0 (REG 0xA8 [4])
     * @param enable true to enable
     * @return true on success
     */
    bool setQC2Enable(bool enable);
    
    /**
     * @brief Enable/disable QC3.0 (REG 0xA8 [3])
     * @param enable true to enable
     * @return true on success
     */
    bool setQC3Enable(bool enable);
    
    /**
     * @brief Enable/disable FCP (REG 0xA8 [2])
     * @param enable true to enable
     * @return true on success
     */
    bool setFCPEnable(bool enable);
    
    /**
     * @brief Enable/disable AFC (REG 0xA8 [1])
     * @param enable true to enable
     * @return true on success
     */
    bool setAFCEnable(bool enable);
    
    /**
     * @brief Enable/disable PE (REG 0xA8 [0])
     * @param enable true to enable
     * @return true on success
     */
    bool setPEEnable(bool enable);
    
    /**
     * @brief Enable/disable SFCP (REG 0xA8 [5])
     * @param enable true to enable
     * @return true on success
     */
    bool setSFCPEnable(bool enable);
    
    /**
     * @brief Enable/disable DPDM (REG 0xAA [5])
     * @param enable true to enable (Apple 2.7V + Samsung 1.2V + fast charge)
     * @return true on success
     */
    bool setDPDMEnable(bool enable);
    
    /**
     * @brief Set SCP max current (REG 0xAA [7:3])
     * @param current SCP max current enum
     * @return true on success
     */
    bool setSCPMaxCurrent(e_SW3526_SCP_MaxCurrent current);
    
    /**
     * @brief Set non-PD max output voltage (REG 0xAA [1:0])
     * @param volt Max voltage enum
     * @return true on success
     */
    bool setNonPdMaxVoltage(e_SW3526_NonPdMaxVolt volt);
    
    /**
     * @brief Set QC2/QC3 wire comp and offset (REG 0xA2 [6])
     * @param enable true=enable wire comp and offset
     * @return true on success
     */
    bool setQCWireCompEnable(bool enable);
    
    /**
     * @brief Set PDO-Vin linkage (REG 0xA2 [5])
     * @param link true=PDO linked to Vin (won't broadcast PDO > Vin)
     * @return true on success
     */
    bool setPdoVinLink(bool link);
    
    /**
     * @brief Enable 5V/2A PDO rebroadcast (REG 0xAC [0])
     * @param enable true to enable
     * @return true on success
     */
    bool setPdo5V2ARebroadcast(bool enable);
    
    // ========== PD Commands ==========
    
    /**
     * @brief Send PD command (REG 0x70)
     * @param cmd PD command enum
     * @return true on success
     */
    bool sendPDCmd(e_SW3526_PDCmd cmd);
    
    /**
     * @brief Send PD Source Capability (REG 0x73)
     * @return true on success
     */
    bool sendPDSourceCap();
    
    // ========== Dual-chip Dynamic Power ==========
    
    /**
     * @brief Set dual-chip reset power strategy (REG 0xAD [7:6])
     * @param resetAll true=all fast charge reset, false=only non-PD reset
     * @param enable true=enable reset power on resistance change
     * @return true on success
     */
    bool setDualChipResetPower(bool resetAll, bool enable);
    
    // ========== Apple/Samsung Configuration ==========
    
    /**
     * @brief Enable Apple adapter mode (REG 0xAE [2])
     * @param enable true to enable
     * @return true on success
     */
    bool setAppleModeEnable(bool enable);
    
    /**
     * @brief Set non-PD max power to 18W (REG 0xAE [6])
     * @param limit18W true=limit to 18W, false=use system max power
     * @return true on success
     */
    bool setNonPdPowerLimit18W(bool limit18W);
    
    /**
     * @brief Set PD Request-to-Accept response time (REG 0xAE [0])
     * @param fast true=1ms, false=4ms
     * @return true on success
     */
    bool setPDResponseTime(bool fast);
    
    /**
     * @brief Set Apple-to-Samsung reference voltage (REG 0xAF [3:2])
     * @param voltage 0=1.05V, 1=1.3V, 2=1.4V, 3=1.7V
     * @return true on success
     */
    bool setAppleSamsungRefVolt(uint8_t voltage);
    
    /**
     * @brief Set Apple mode drive current (REG 0xAF [1:0])
     * @param current 0=16uA, 1=25uA, 2=38uA, 3=47uA
     * @return true on success
     */
    bool setAppleDriveCurrent(uint8_t current);
    
    // ========== ADC Configuration ==========
    
    /**
     * @brief Set ADC measurement lane
     * @param lane ADC lane to measure
     */
    void setADCLane(e_SW3526_ADC_Lane lane);
    
    // ========== Utility ==========
    
    /**
     * @brief Get device name
     * @return Device name string
     */
    const char* getName() const { return m_name; }
    
    /**
     * @brief Check if device is initialized
     * @return true if initialized
     */
    bool isInitialized() const { return (m_i2cType == e_SW3526_I2C_Type::HW_I2C && m_i2c != nullptr) ||
                                          (m_i2cType == e_SW3526_I2C_Type::BITBANG && m_sdaPort != nullptr); }

private:
    I2C_HandleTypeDef* m_i2c = nullptr;
    e_SW3526_I2C_Type m_i2cType = e_SW3526_I2C_Type::HW_I2C;
    GPIO_TypeDef* m_sdaPort = nullptr;
    uint16_t m_sdaPin = 0;
    GPIO_TypeDef* m_sclPort = nullptr;
    uint16_t m_sclPin = 0;
    const char* m_name = "SW3526";
    e_SW3526_FastProt m_currentProt = e_SW3526_FastProt::NONE;
    uint16_t m_senseResistorMilliOhm = 10;  // Default: 10mΩ per datasheet
    
    /**
     * @brief Bit-bang I2C delay
     */
    static void bitbangDelay();
    
    /**
     * @brief Bit-bang SDA/SCL output control
     */
    static void bitbangSdaLow(GPIO_TypeDef* port, uint16_t pin);
    static void bitbangSdaHigh(GPIO_TypeDef* port, uint16_t pin);
    static void bitbangSclLow(GPIO_TypeDef* port, uint16_t pin);
    static void bitbangSclHigh(GPIO_TypeDef* port, uint16_t pin);
    static uint8_t bitbangReadSda(GPIO_TypeDef* port, uint16_t pin);
    
    /**
     * @brief Bit-bang I2C primitives
     */
    void bitbangStart();
    void bitbangStop();
    bool bitbangWriteByte(uint8_t data);
    uint8_t bitbangReadByte(bool ack);
    
    /**
     * @brief Internal read for HW I2C
     */
    bool readRegHwI2c(uint8_t reg, uint8_t* value);
    
    /**
     * @brief Internal write for HW I2C
     */
    bool writeRegHwI2c(uint8_t reg, uint8_t value);
    
    /**
     * @brief Internal read with bit-bang
     */
    bool readRegBitbang(uint8_t reg, uint8_t* value);
    
    /**
     * @brief Internal write with bit-bang
     */
    bool writeRegBitbang(uint8_t reg, uint8_t value);
    
    /**
     * @brief Internal read (dispatches to HW or bit-bang)
     */
    bool readRegInternal(uint8_t reg, uint8_t* value);
    
    /**
     * @brief Internal write (dispatches to HW or bit-bang)
     */
    bool writeRegInternal(uint8_t reg, uint8_t value);
    
    /**
     * @brief Helper: read-modify-write a single bit in a register
     * @param reg Register address
     * @param bit Bit position (0-7)
     * @param value true=set bit, false=clear bit
     * @return true on success
     */
    bool modifyRegBit(e_SW3526_Reg reg, uint8_t bit, bool value);
    
    /**
     * @brief Helper: read-modify-write bit field in a register
     * @param reg Register address
     * @param shift Bit shift amount
     * @param mask Bit mask (already shifted)
     * @param value Value to write (already shifted)
     * @return true on success
     */
    bool modifyRegField(e_SW3526_Reg reg, uint8_t shift, uint8_t mask, uint8_t value);
};

/*============================================================================
 * SW3526 Manager - Multi-device Support
 *============================================================================*/

class SW3526_Manager {
public:
    static constexpr size_t MAX_DEVICES = 4;
    
    /**
     * @brief Add device to manager
     * @param device Pointer to SW3526 instance
     * @return Device index, or -1 on failure
     */
    int8_t addDevice(SW3526* device);
    
    /**
     * @brief Get device by index
     * @param index Device index
     * @return Pointer to device or nullptr
     */
    SW3526* getDevice(size_t index);
    
    /**
     * @brief Get device count
     * @return Number of devices
     */
    size_t getDeviceCount() const { return m_count; }
    
    /**
     * @brief Iterate over all devices
     * @param callback Function to call for each device
     */
    void forEach(std::function<void(SW3526*, size_t)> callback);
    
private:
    std::array<SW3526*, MAX_DEVICES> m_devices = {};
    size_t m_count = 0;
};

/*============================================================================
 * C API Compatibility Layer
 *============================================================================*/

#ifdef __cplusplus
extern "C" {
#endif

void v_SW3526_Init(SW3526* dev, I2C_HandleTypeDef* i2c, const char* name);
void v_SW3526_InitBitbang(SW3526* dev, GPIO_TypeDef* sda_port, uint16_t sda_pin,
                          GPIO_TypeDef* scl_port, uint16_t scl_pin, const char* name);
uint16_t us_SW3526_GetVin(SW3526* dev);
uint16_t us_SW3526_GetVout(SW3526* dev);
uint16_t us_SW3526_GetIout(SW3526* dev);

#ifdef __cplusplus
}
#endif

#endif // SW3526_HPP
