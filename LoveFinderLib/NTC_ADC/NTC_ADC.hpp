/**
 * @file NTC_ADC.hpp
 * @brief NTC Thermistor ADC Temperature Measurement Library - C++17
 * @author LoveFinder
 * @date 2026
 * 
 * Supports multiple NTC thermistor types with Steinhart-Hart equation
 */

#ifndef NTC_ADC_HPP
#define NTC_ADC_HPP

#include "main.h"
#include <cstdint>
#include <cmath>
#include <array>
#include <optional>

/*============================================================================
 * Constants
 *============================================================================*/

namespace NTCConfig {
    constexpr uint16_t ADC_RESOLUTION = 4095;      // 12-bit ADC
    constexpr uint16_t SAMPLE_COUNT = 10;          // Average samples
    constexpr float KELVIN_OFFSET = 273.15f;       // Celsius to Kelvin
    constexpr float INVALID_TEMP = -999.0f;        // Invalid temperature indicator
}

/*============================================================================
 * NTC Thermistor Type Enumeration
 *============================================================================*/

enum class e_NTC_Type : uint8_t {
    NONE = 0,
    
    // 10K NTC Thermistors (R25 = 10KΩ)
    NTC_10K_3950_103AT = 1,    // 10K B=3950 (103AT series)
    NTC_10K_3380,              // 10K B=3380
    NTC_10K_3435,              // 10K B=3435
    NTC_10K_3977,              // 10K B=3977
    
    // 100K NTC Thermistors (R25 = 100KΩ)
    NTC_100K_3950,             // 100K B=3950
    NTC_100K_4250,             // 100K B=4250
    
    // Custom (user-defined parameters)
    NTC_CUSTOM
};

/*============================================================================
 * NTC Parameters Structure
 *============================================================================*/

struct NTC_Params {
    const char* name;           // Thermistor name
    float R25;                  // Resistance at 25°C (Ohms)
    float B;                    // B-value (Beta coefficient, Kelvin)
    float R_series;             // Series resistor value (Ohms)
    float T_min;                // Minimum temperature (°C)
    float T_max;                // Maximum temperature (°C)
};

/*============================================================================
 * NTC Parameter Database (Compile-time)
 *============================================================================*/

namespace NTCDatabase {
    // 103AT: 10K NTC with B=3950K
    constexpr NTC_Params NTC_10K_3950_103AT = {
        .name = "103AT",
        .R25 = 10000.0f,        // 10KΩ at 25°C
        .B = 3950.0f,           // B-value
        .R_series = 10000.0f,   // Series resistor 10K
        .T_min = -30.0f,        // Min temperature
        .T_max = 105.0f         // Max temperature
    };
    
    // Additional thermistor parameters can be added here
    constexpr NTC_Params NTC_10K_3380 = {
        .name = "10K-3380",
        .R25 = 10000.0f,
        .B = 3380.0f,
        .R_series = 10000.0f,
        .T_min = -40.0f,
        .T_max = 125.0f
    };
    
    constexpr NTC_Params NTC_10K_3435 = {
        .name = "10K-3435",
        .R25 = 10000.0f,
        .B = 3435.0f,
        .R_series = 10000.0f,
        .T_min = -40.0f,
        .T_max = 125.0f
    };
    
    constexpr NTC_Params NTC_10K_3977 = {
        .name = "10K-3977",
        .R25 = 10000.0f,
        .B = 3977.0f,
        .R_series = 10000.0f,
        .T_min = -40.0f,
        .T_max = 125.0f
    };
    
    constexpr NTC_Params NTC_100K_3950 = {
        .name = "100K-3950",
        .R25 = 100000.0f,
        .B = 3950.0f,
        .R_series = 100000.0f,
        .T_min = -40.0f,
        .T_max = 300.0f
    };
    
    constexpr NTC_Params NTC_100K_4250 = {
        .name = "100K-4250",
        .R25 = 100000.0f,
        .B = 4250.0f,
        .R_series = 100000.0f,
        .T_min = -40.0f,
        .T_max = 300.0f
    };
}

/*============================================================================
 * NTC_ADC Class
 *============================================================================*/

class NTC_ADC {
public:
    /**
     * @brief Default constructor
     */
    NTC_ADC() = default;
    
    /**
     * @brief Constructor with initialization
     * @param hadc ADC handle pointer
     * @param type NTC thermistor type
     * @param vref Reference voltage (mV), default 3300mV
     */
    NTC_ADC(ADC_HandleTypeDef* hadc, e_NTC_Type type, float vref = 3300.0f);
    
    /**
     * @brief Initialize the NTC ADC
     * @param hadc ADC handle pointer
     * @param type NTC thermistor type
     * @param vref Reference voltage (mV)
     */
    void init(ADC_HandleTypeDef* hadc, e_NTC_Type type, float vref = 3300.0f);
    
    /**
     * @brief Set custom NTC parameters
     * @param R25 Resistance at 25°C
     * @param B B-value (Beta coefficient)
     * @param R_series Series resistor value
     */
    void setCustomParams(float R25, float B, float R_series);
    
    /**
     * @brief Read raw ADC value
     * @return Raw ADC value (0-4095)
     */
    uint16_t readRaw();
    
    /**
     * @brief Read averaged ADC value
     * @param samples Number of samples to average
     * @return Averaged ADC value
     */
    uint16_t readAveraged(uint16_t samples = NTCConfig::SAMPLE_COUNT);
    
    /**
     * @brief Calculate NTC resistance from ADC value
     * @param adcValue ADC reading
     * @return NTC resistance in Ohms
     */
    float calculateResistance(uint16_t adcValue) const;
    
    /**
     * @brief Calculate temperature using Beta equation
     * @param resistance NTC resistance in Ohms
     * @return Temperature in Celsius
     */
    float calculateTemperature(float resistance) const;
    
    /**
     * @brief Read temperature (blocking)
     * @return Temperature in Celsius, or INVALID_TEMP on error
     */
    float readTemperature();
    
    /**
     * @brief Read temperature with averaging
     * @param samples Number of samples to average
     * @return Temperature in Celsius
     */
    float readTemperatureAveraged(uint16_t samples = NTCConfig::SAMPLE_COUNT);
    
    /**
     * @brief Get NTC type name
     * @return NTC type name string
     */
    const char* getTypeName() const;
    
    /**
     * @brief Get current NTC type
     * @return NTC type enum
     */
    e_NTC_Type getType() const { return m_type; }
    
    /**
     * @brief Get last temperature reading
     * @return Last temperature in Celsius
     */
    float getLastTemperature() const { return m_lastTemp; }
    
    /**
     * @brief Get last resistance reading
     * @return Last resistance in Ohms
     */
    float getLastResistance() const { return m_lastResistance; }
    
    /**
     * @brief Check if temperature is in valid range
     * @param temp Temperature to check
     * @return true if valid
     */
    bool isTemperatureValid(float temp) const;
    
    /**
     * @brief Get parameters
     * @return Current NTC parameters
     */
    const NTC_Params& getParams() const { return m_params; }
    
    /**
     * @brief Set DMA buffer for non-blocking ADC reads
     * @param dmaBuffer Pointer to volatile ADC DMA buffer
     */
    void setDmaBuffer(volatile uint16_t* dmaBuffer);

private:
    ADC_HandleTypeDef* m_hadc = nullptr;
    e_NTC_Type m_type = e_NTC_Type::NONE;
    float m_vref = 3300.0f;
    float m_lastTemp = NTCConfig::INVALID_TEMP;
    float m_lastResistance = 0.0f;
    NTC_Params m_params = {};
    volatile uint16_t* m_dmaBuffer = nullptr;  // DMA buffer pointer
    
    /**
     * @brief Load parameters for NTC type
     * @param type NTC type
     */
    void loadParams(e_NTC_Type type);
};

/*============================================================================
 * Utility Functions
 *============================================================================*/

namespace NTCUtils {
    /**
     * @brief Convert Celsius to Kelvin
     */
    constexpr float celsiusToKelvin(float celsius) {
        return celsius + NTCConfig::KELVIN_OFFSET;
    }
    
    /**
     * @brief Convert Kelvin to Celsius
     */
    constexpr float kelvinToCelsius(float kelvin) {
        return kelvin - NTCConfig::KELVIN_OFFSET;
    }
    
    /**
     * @brief Calculate temperature using Steinhart-Hart equation
     * @param R Measured resistance
     * @param R25 Resistance at 25°C
     * @param B B-value
     * @return Temperature in Celsius
     */
    inline float steinhartHart(float R, float R25, float B) {
        // Using simplified Beta equation: 1/T = 1/T0 + (1/B) * ln(R/R0)
        float T0 = celsiusToKelvin(25.0f);  // 25°C in Kelvin
        float T = 1.0f / (1.0f / T0 + (1.0f / B) * std::log(R / R25));
        return kelvinToCelsius(T);
    }
}

/*============================================================================
 * C API Compatibility Layer
 *============================================================================*/

#ifdef __cplusplus
extern "C" {
#endif

void v_NTC_Init(NTC_ADC* ntc, ADC_HandleTypeDef* hadc, e_NTC_Type type, float vref);
float f_NTC_ReadTemperature(NTC_ADC* ntc);
uint16_t us_NTC_ReadRaw(NTC_ADC* ntc);

#ifdef __cplusplus
}
#endif

#endif // NTC_ADC_HPP
