/**
 * @file UART.hpp
 * @brief Universal Asynchronous Receiver Transmitter Driver - C++17
 * @author LoveFinder
 * @date 2026
 * 
 * 支持多串口复用，用于日志输出和数据传输
 */

#ifndef UART_HPP
#define UART_HPP

#include "main.h"
#include <cstdint>
#include <cstdarg>

/*============================================================================
 * 常量定义 (constexpr)
 *============================================================================*/

namespace UARTConfig {
    constexpr uint16_t TX_BUFFER_SIZE = 512;    // 发送缓冲区大小
    constexpr uint16_t RX_BUFFER_SIZE = 256;    // 接收缓冲区大小
    constexpr uint32_t TIMEOUT_MS = 1000;        // 默认超时时间
}

/*============================================================================
 * 串口日志级别枚举
 *============================================================================*/

enum class e_UART_LogLevel : uint8_t {
    NONE    = 0,
    ERROR   = 1,
    WARN    = 2,
    INFO    = 3,
    DEBUG   = 4,
    VERBOSE = 5
};

// 向后兼容
#define UART_LOG_NONE    e_UART_LogLevel::NONE
#define UART_LOG_ERROR   e_UART_LogLevel::ERROR
#define UART_LOG_WARN    e_UART_LogLevel::WARN
#define UART_LOG_INFO    e_UART_LogLevel::INFO
#define UART_LOG_DEBUG   e_UART_LogLevel::DEBUG
#define UART_LOG_VERBOSE e_UART_LogLevel::VERBOSE

/*============================================================================
 * UART类
 *============================================================================*/

class UART {
public:
    /**
     * @brief 默认构造函数
     */
    UART() = default;
    
    /**
     * @brief 构造并初始化
     * @param huart UART句柄
     * @param name 串口名称 (用于日志标识)
     */
    UART(UART_HandleTypeDef* huart, const char* name);
    
    /**
     * @brief 初始化
     * @param huart UART句柄
     * @param name 串口名称
     */
    void init(UART_HandleTypeDef* huart, const char* name);
    
    /**
     * @brief 发送数据 (阻塞)
     * @param data 数据指针
     * @param len 数据长度
     * @param timeout 超时时间(ms)
     * @return true=成功
     */
    bool transmit(const uint8_t* data, uint16_t len, uint32_t timeout = UARTConfig::TIMEOUT_MS);
    
    /**
     * @brief 发送字符串 (阻塞)
     * @param str 字符串
     * @param timeout 超时时间(ms)
     * @return true=成功
     */
    bool print(const char* str, uint32_t timeout = UARTConfig::TIMEOUT_MS);
    
    /**
     * @brief 格式化输出 (阻塞)
     * @param format 格式字符串
     * @param ... 可变参数
     * @return true=成功
     */
    bool printf(const char* format, ...) __attribute__((format(printf, 2, 3)));
    
    /**
     * @brief 接收数据 (阻塞)
     * @param data 数据缓冲区
     * @param len 期望接收长度
     * @param timeout 超时时间(ms)
     * @return true=成功
     */
    bool receive(uint8_t* data, uint16_t len, uint32_t timeout = UARTConfig::TIMEOUT_MS);
    
    /**
     * @brief 发送单字节
     * @param byte 字节数据
     * @return true=成功
     */
    bool writeByte(uint8_t byte);
    
    /**
     * @brief 接收单字节
     * @param byte 字节输出
     * @param timeout 超时时间(ms)
     * @return true=成功
     */
    bool readByte(uint8_t* byte, uint32_t timeout = UARTConfig::TIMEOUT_MS);
    
    /**
     * @brief 发送换行
     */
    void println();
    
    /**
     * @brief 格式化输出并换行
     */
    bool printfln(const char* format, ...) __attribute__((format(printf, 2, 3)));
    
    // ========== 日志功能 ==========
    
    /**
     * @brief 设置日志级别
     * @param level 日志级别
     */
    void setLogLevel(e_UART_LogLevel level) { m_logLevel = level; }
    
    /**
     * @brief 获取日志级别
     * @return 当前日志级别
     */
    e_UART_LogLevel getLogLevel() const { return m_logLevel; }
    
    /**
     * @brief 输出错误日志
     * @param format 格式字符串
     */
    void logError(const char* format, ...) __attribute__((format(printf, 2, 3)));
    
    /**
     * @brief 输出警告日志
     * @param format 格式字符串
     */
    void logWarn(const char* format, ...) __attribute__((format(printf, 2, 3)));
    
    /**
     * @brief 输出信息日志
     * @param format 格式字符串
     */
    void logInfo(const char* format, ...) __attribute__((format(printf, 2, 3)));
    
    /**
     * @brief 输出调试日志
     * @param format 格式字符串
     */
    void logDebug(const char* format, ...) __attribute__((format(printf, 2, 3)));
    
    /**
     * @brief 输出详细日志
     * @param format 格式字符串
     */
    void logVerbose(const char* format, ...) __attribute__((format(printf, 2, 3)));
    
    /**
     * @brief 通用日志输出
     * @param level 日志级别
     * @param tag 标签
     * @param format 格式字符串
     */
    void log(e_UART_LogLevel level, const char* tag, const char* format, ...) __attribute__((format(printf, 4, 5)));
    
    /**
     * @brief 获取串口名称
     * @return 名称字符串
     */
    const char* getName() const { return m_name; }
    
    /**
     * @brief 输出十六进制数据
     * @param data 数据指针
     * @param len 数据长度
     * @param prefix 前缀字符串 (可选)
     */
    void printHex(const uint8_t* data, uint16_t len, const char* prefix = nullptr);

private:
    UART_HandleTypeDef* m_huart = nullptr;
    const char* m_name = "UART";
    e_UART_LogLevel m_logLevel = e_UART_LogLevel::INFO;
    
    // 格式化缓冲区
    char m_formatBuffer[UARTConfig::TX_BUFFER_SIZE];
    
    // 内部格式化输出
    bool vprintf(const char* format, va_list args);
};

// 向后兼容typedef
using U_ART = UART;

/*============================================================================
 * C API 兼容层
 *============================================================================*/

#ifdef __cplusplus
extern "C" {
#endif

void v_UART_Init(UART* uart, UART_HandleTypeDef* huart, const char* name);
bool b_UART_Transmit(UART* uart, const uint8_t* data, uint16_t len, uint32_t timeout);
bool b_UART_Print(UART* uart, const char* str);
bool b_UART_Printf(UART* uart, const char* format, ...);
void v_UART_LogInfo(UART* uart, const char* format, ...);
void v_UART_LogError(UART* uart, const char* format, ...);

#ifdef __cplusplus
}
#endif

#endif // UART_HPP
