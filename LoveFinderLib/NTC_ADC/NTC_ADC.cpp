/**
 * @file NTC_ADC.cpp
 * @brief NTC Thermistor ADC Temperature Measurement Library Implementation
 */

#include "NTC_ADC.hpp"
#include <cmath>

/*============================================================================
 * NTC_ADC Class Implementation
 *============================================================================*/

NTC_ADC::NTC_ADC(ADC_HandleTypeDef* hadc, e_NTC_Type type, float vref)
{
    init(hadc, type, vref);
}

void NTC_ADC::init(ADC_HandleTypeDef* hadc, e_NTC_Type type, float vref)
{
    m_hadc = hadc;
    m_type = type;
    m_vref = vref;
    m_lastTemp = NTCConfig::INVALID_TEMP;
    m_lastResistance = 0.0f;
    m_dmaBuffer = nullptr;
    
    loadParams(type);
}

void NTC_ADC::setDmaBuffer(volatile uint16_t* dmaBuffer)
{
    m_dmaBuffer = dmaBuffer;
}

void NTC_ADC::loadParams(e_NTC_Type type)
{
    switch (type) {
        case e_NTC_Type::NTC_10K_3950_103AT:
            m_params = NTCDatabase::NTC_10K_3950_103AT;
            break;
            
        case e_NTC_Type::NTC_10K_3380:
            m_params = NTCDatabase::NTC_10K_3380;
            break;
            
        case e_NTC_Type::NTC_10K_3435:
            m_params = NTCDatabase::NTC_10K_3435;
            break;
            
        case e_NTC_Type::NTC_10K_3977:
            m_params = NTCDatabase::NTC_10K_3977;
            break;
            
        case e_NTC_Type::NTC_100K_3950:
            m_params = NTCDatabase::NTC_100K_3950;
            break;
            
        case e_NTC_Type::NTC_100K_4250:
            m_params = NTCDatabase::NTC_100K_4250;
            break;
            
        default:
            // Default to 103AT parameters
            m_params = NTCDatabase::NTC_10K_3950_103AT;
            break;
    }
}

void NTC_ADC::setCustomParams(float R25, float B, float R_series)
{
    m_type = e_NTC_Type::NTC_CUSTOM;
    m_params.name = "Custom";
    m_params.R25 = R25;
    m_params.B = B;
    m_params.R_series = R_series;
    m_params.T_min = -50.0f;
    m_params.T_max = 300.0f;
}

uint16_t NTC_ADC::readRaw()
{
    if (!m_hadc) {
        return 0;
    }
    
    // If DMA buffer is set, read from DMA buffer (non-blocking)
    if (m_dmaBuffer != nullptr) {
        return *m_dmaBuffer;
    }
    
    // Fallback to blocking read (for non-DMA configurations)
    // For STM32G0, sampling time is configured globally during ADC init
    // Just start conversion and read
    HAL_ADC_Start(m_hadc);
    HAL_ADC_PollForConversion(m_hadc, 100);
    uint16_t value = HAL_ADC_GetValue(m_hadc);
    HAL_ADC_Stop(m_hadc);
    
    return value;
}

uint16_t NTC_ADC::readAveraged(uint16_t samples)
{
    if (samples == 0) {
        samples = 1;
    }
    
    uint32_t sum = 0;
    for (uint16_t i = 0; i < samples; i++) {
        sum += readRaw();
    }
    
    return static_cast<uint16_t>(sum / samples);
}

float NTC_ADC::calculateResistance(uint16_t adcValue) const
{
    if (adcValue == 0 || adcValue >= NTCConfig::ADC_RESOLUTION) {
        return 0.0f;  // Invalid reading
    }
    
    // Voltage divider: V_ntc = Vref * R_ntc / (R_ntc + R_series)
    // ADC = V_ntc * 4095 / Vref
    // Therefore: R_ntc = R_series * ADC / (4095 - ADC)
    
    float adcFloat = static_cast<float>(adcValue);
    float R_ntc = m_params.R_series * adcFloat / (NTCConfig::ADC_RESOLUTION - adcFloat);
    
    return R_ntc;
}

float NTC_ADC::calculateTemperature(float resistance) const
{
    if (resistance <= 0.0f) {
        return NTCConfig::INVALID_TEMP;
    }
    
    // Using Beta equation (Steinhart-Hart simplified)
    // 1/T = 1/T0 + (1/B) * ln(R/R0)
    // Where T0 = 298.15K (25°C), R0 = R25, B = Beta coefficient
    
    float T = NTCUtils::steinhartHart(resistance, m_params.R25, m_params.B);
    
    return T;
}

float NTC_ADC::readTemperature()
{
    uint16_t adcValue = readRaw();
    m_lastResistance = calculateResistance(adcValue);
    
    if (m_lastResistance <= 0.0f) {
        m_lastTemp = NTCConfig::INVALID_TEMP;
        return m_lastTemp;
    }
    
    m_lastTemp = calculateTemperature(m_lastResistance);
    return m_lastTemp;
}

float NTC_ADC::readTemperatureAveraged(uint16_t samples)
{
    uint16_t adcValue = readAveraged(samples);
    m_lastResistance = calculateResistance(adcValue);
    
    if (m_lastResistance <= 0.0f) {
        m_lastTemp = NTCConfig::INVALID_TEMP;
        return m_lastTemp;
    }
    
    m_lastTemp = calculateTemperature(m_lastResistance);
    return m_lastTemp;
}

const char* NTC_ADC::getTypeName() const
{
    return m_params.name;
}

bool NTC_ADC::isTemperatureValid(float temp) const
{
    return (temp >= m_params.T_min && temp <= m_params.T_max);
}

/*============================================================================
 * C API Compatibility Layer
 *============================================================================*/

extern "C" {

void v_NTC_Init(NTC_ADC* ntc, ADC_HandleTypeDef* hadc, e_NTC_Type type, float vref)
{
    ntc->init(hadc, type, vref);
}

float f_NTC_ReadTemperature(NTC_ADC* ntc)
{
    return ntc->readTemperature();
}

uint16_t us_NTC_ReadRaw(NTC_ADC* ntc)
{
    return ntc->readRaw();
}

} // extern "C"
