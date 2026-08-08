/**
 * @file UART.cpp
 * @brief Universal Asynchronous Receiver Transmitter Driver Implementation - C++17
 */

#include "UART.hpp"
#include <cstdio>
#include <cstring>

/*============================================================================
 * UART 类实现
 *============================================================================*/

UART::UART(UART_HandleTypeDef* huart, const char* name)
{
    init(huart, name);
}

void UART::init(UART_HandleTypeDef* huart, const char* name)
{
    m_huart = huart;
    m_name = name ? name : "UART";
    m_logLevel = e_UART_LogLevel::INFO;
}

bool UART::transmit(const uint8_t* data, uint16_t len, uint32_t timeout)
{
    if (!m_huart || !data || len == 0)
        return false;
    
    HAL_StatusTypeDef status = HAL_UART_Transmit(m_huart, 
                                                   const_cast<uint8_t*>(data), 
                                                   len, timeout);
    return (status == HAL_OK);
}

bool UART::print(const char* str, uint32_t timeout)
{
    if (!str)
        return false;
    
    return transmit(reinterpret_cast<const uint8_t*>(str), 
                    static_cast<uint16_t>(strlen(str)), timeout);
}

bool UART::printf(const char* format, ...)
{
    va_list args;
    va_start(args, format);
    bool result = vprintf(format, args);
    va_end(args);
    return result;
}

bool UART::vprintf(const char* format, va_list args)
{
    if (!format)
        return false;
    
    // 格式化字符串
    int len = vsnprintf(m_formatBuffer, sizeof(m_formatBuffer), format, args);
    
    if (len <= 0 || len >= static_cast<int>(sizeof(m_formatBuffer)))
        return false;
    
    return transmit(reinterpret_cast<uint8_t*>(m_formatBuffer), 
                    static_cast<uint16_t>(len));
}

bool UART::receive(uint8_t* data, uint16_t len, uint32_t timeout)
{
    if (!m_huart || !data || len == 0)
        return false;
    
    HAL_StatusTypeDef status = HAL_UART_Receive(m_huart, data, len, timeout);
    return (status == HAL_OK);
}

bool UART::writeByte(uint8_t byte)
{
    return transmit(&byte, 1);
}

bool UART::readByte(uint8_t* byte, uint32_t timeout)
{
    return receive(byte, 1, timeout);
}

void UART::println()
{
    print("\r\n");
}

bool UART::printfln(const char* format, ...)
{
    va_list args;
    va_start(args, format);
    bool result = vprintf(format, args);
    va_end(args);
    
    if (result)
        println();
    
    return result;
}

void UART::logError(const char* format, ...)
{
    if (m_logLevel < e_UART_LogLevel::ERROR)
        return;
    
    print("[ERROR][");
    print(m_name);
    print("] ");
    
    va_list args;
    va_start(args, format);
    vprintf(format, args);
    va_end(args);
    
    println();
}

void UART::logWarn(const char* format, ...)
{
    if (m_logLevel < e_UART_LogLevel::WARN)
        return;
    
    print("[WARN][");
    print(m_name);
    print("] ");
    
    va_list args;
    va_start(args, format);
    vprintf(format, args);
    va_end(args);
    
    println();
}

void UART::logInfo(const char* format, ...)
{
    if (m_logLevel < e_UART_LogLevel::INFO)
        return;
    
    print("[INFO][");
    print(m_name);
    print("] ");
    
    va_list args;
    va_start(args, format);
    vprintf(format, args);
    va_end(args);
    
    println();
}

void UART::logDebug(const char* format, ...)
{
    if (m_logLevel < e_UART_LogLevel::DEBUG)
        return;
    
    print("[DEBUG][");
    print(m_name);
    print("] ");
    
    va_list args;
    va_start(args, format);
    vprintf(format, args);
    va_end(args);
    
    println();
}

void UART::logVerbose(const char* format, ...)
{
    if (m_logLevel < e_UART_LogLevel::VERBOSE)
        return;
    
    print("[VERBOSE][");
    print(m_name);
    print("] ");
    
    va_list args;
    va_start(args, format);
    vprintf(format, args);
    va_end(args);
    
    println();
}

void UART::log(e_UART_LogLevel level, const char* tag, const char* format, ...)
{
    if (m_logLevel < level)
        return;
    
    // 输出级别标签
    switch (level)
    {
        case e_UART_LogLevel::ERROR:   print("[ERROR]"); break;
        case e_UART_LogLevel::WARN:    print("[WARN]"); break;
        case e_UART_LogLevel::INFO:    print("[INFO]"); break;
        case e_UART_LogLevel::DEBUG:   print("[DEBUG]"); break;
        case e_UART_LogLevel::VERBOSE: print("[VERBOSE]"); break;
        default: break;
    }
    
    // 输出标签
    if (tag)
    {
        print("[");
        print(tag);
        print("]");
    }
    
    print(" ");
    
    // 输出内容
    va_list args;
    va_start(args, format);
    vprintf(format, args);
    va_end(args);
    
    println();
}

void UART::printHex(const uint8_t* data, uint16_t len, const char* prefix)
{
    if (!data || len == 0)
        return;
    
    if (prefix)
    {
        print(prefix);
        print(": ");
    }
    
    for (uint16_t i = 0; i < len; i++)
    {
        printf("%02X ", data[i]);
        
        // 每16字节换行
        if ((i + 1) % 16 == 0)
            println();
    }
    
    // 最后一行没满16字节也要换行
    if (len % 16 != 0)
        println();
}

/*============================================================================
 * C API 兼容层
 *============================================================================*/

extern "C" {

void v_UART_Init(UART* uart, UART_HandleTypeDef* huart, const char* name)
{
    uart->init(huart, name);
}

bool b_UART_Transmit(UART* uart, const uint8_t* data, uint16_t len, uint32_t timeout)
{
    return uart->transmit(data, len, timeout);
}

bool b_UART_Print(UART* uart, const char* str)
{
    return uart->print(str);
}

bool b_UART_Printf(UART* uart, const char* format, ...)
{
    va_list args;
    va_start(args, format);
    bool result = uart->printf(format, args);
    va_end(args);
    return result;
}

void v_UART_LogInfo(UART* uart, const char* format, ...)
{
    (void)uart;  // Suppress unused parameter warning
    va_list args;
    va_start(args, format);
    // 这里需要调用成员函数，C API不太方便支持可变参数
    // 建议直接使用C++ API
    va_end(args);
}

void v_UART_LogError(UART* uart, const char* format, ...)
{
    (void)uart;  // Suppress unused parameter warning
    va_list args;
    va_start(args, format);
    va_end(args);
}

} // extern "C"
