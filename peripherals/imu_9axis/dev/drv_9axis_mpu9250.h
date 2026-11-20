#ifndef _DRV_9AXIS_MPU9250_H_
#define _DRV_9AXIS_MPU9250_H_

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif /*__cplusplus*/


// documentation:
//   https://www.invensense.com/products/motion-tracking/9-axis/mpu-9250/
// 人体姿态捕捉建议加速度量程 ±8g；陀螺仪量程 ±2000
#define MPU9250_PIN_AD0_LEVEL                   (0)         // AD0 引脚高低
#define MPU9250_I2C_ADDR                        (0x68|MPU9250_PIN_AD0_LEVEL)
#define AK8963_MAGN_ADDRESS                     (0x0C)


/////////////////////////////MPU9250 REGISTER/////////////////////////////////////////
// 陀螺仪出厂自测结果
#define MPU9250_0X00_GYRO_SELF_TEST_X           (0x00)
#define MPU9250_0X01_GYRO_SELF_TEST_Y           (0x01)
#define MPU9250_0X02_GYRO_SELF_TEST_Z           (0x02)
// 加速度出厂自测结果
#define MPU9250_0X0d_ACCEL_SELF_TEST_X          (0x0d)
#define MPU9250_0X0e_ACCEL_SELF_TEST_Y          (0x0e)
#define MPU9250_0X0f_ACCEL_SELF_TEST_Z          (0x0f)
// 陀螺仪校准数值，单位 0.0305 dps; 最大值999.969dps，最小值-1000dps
#define MPU9250_0X13_XG_OFFS_USRH               (0x13)      // 陀螺仪X轴校准值高字节
#define MPU9250_0X14_XG_OFFS_USRL               (0x14)      // 陀螺仪X轴校准值低字节
#define MPU9250_0X15_YG_OFFS_USRH               (0x15)      // 陀螺仪Y轴校准值高字节
#define MPU9250_0X16_YG_OFFS_USRL               (0x16)      // 陀螺仪Y轴校准值低字节
#define MPU9250_0X17_ZG_OFFS_USRH               (0x17)      // 陀螺仪Z轴校准值高字节
#define MPU9250_0X18_ZG_OFFS_USRL               (0x18)      // 陀螺仪Z轴校准值低字节
// 采样率
#define MPU9250_0X19_SAMPLE_DIV                 (0x19)
// 配置寄存器
#define MPU9250_0X1A_CONFIG                     (0x1A)
    #define CONFIG_BIT_FIFO_MODE                (0x40)      // 使能时，fifo满后，添加的数据将覆盖旧的数据
    #define CONFIG_MSK_EXT_SYNC_SET             (0x38)      // 
    #define CONFIG_EXT_SYNC_DISABLE             (0x00)
    #define CONFIG_EXT_SYNC_TEMP_OUT_EN         (0x08)
    #define CONFIG_EXT_SYNC_GYRO_XOUT_EN        (0x10)
    #define CONFIG_EXT_SYNC_GYRO_YOUT_EN        (0x18)
    #define CONFIG_EXT_SYNC_GYRO_ZOUT_EN        (0x20)
    #define CONFIG_EXT_SYNC_ACCEL_XOUT_EN       (0x28)
    #define CONFIG_EXT_SYNC_ACCEL_YOUT_EN       (0x30)
    #define CONFIG_EXT_SYNC_ACCEL_ZOUT_EN       (0x38)
    #define GYRO_CFG1_MSK_DLPFCFG               (0x07)          // 陀螺仪低通滤波配置
    #define GYRO_DLPFCFG_0                      (0x00)
    #define GYRO_DLPFCFG_1                      (0x01)
    #define GYRO_DLPFCFG_2                      (0x02)
    #define GYRO_DLPFCFG_3                      (0x03)
    #define GYRO_DLPFCFG_4                      (0x04)
    #define GYRO_DLPFCFG_5                      (0x05)
    #define GYRO_DLPFCFG_6                      (0x06)
    #define GYRO_DLPFCFG_7                      (0x07)
// 陀螺仪配置寄存器
#define MPU9250_0X1B_GYRO_CONFIG                (0x1B)
    #define GYRO_CFG_BIT_XCT_EN                 (0x80)          // 使能陀螺仪 X 轴自检
    #define GYRO_CFG_BIT_YCT_EN                 (0x40)          // 使能陀螺仪 Y 轴自检
    #define GYRO_CFG_BIT_ZCT_EN                 (0x20)          // 使能陀螺仪 Z 轴自检
    #define GYRO_CFG_MSK_FS_SEL                 (0x18)          // 陀螺仪量程范围, 0->250dps; 1->500dps; 2->1000dps; 3->2000dps; 
    #define GYRO_CFG_FS_250                     (0x00)
    #define GYRO_CFG_FS_500                     (0x08)
    #define GYRO_CFG_FS_1000                    (0x10)
    #define GYRO_CFG_FS_2000                    (0x18)
    #define GYRO_CFG_MSK_FCHOICE_B              (0x03)
// 加速度配置寄存器
#define MPU9250_0X1C_ACCEL_CONFIG1              (0x1C)
    #define ACCEL_CFG1_BIT_XCT_EN               (0x80)          // 使能加速度 X 轴自检
    #define ACCEL_CFG1_BIT_YCT_EN               (0x40)          // 使能加速度 Y 轴自检
    #define ACCEL_CFG1_BIT_ZCT_EN               (0x20)          // 使能加速度 Z 轴自检
    #define ACCEL_CFG1_MSK_FS_SEL               (0x18)          // 加速度量程范围; 0->2g; 1->4g; 2->8g; 3->16g; 
    #define ACCEL_CFG1_FS_2G                    (0x00)
    #define ACCEL_CFG1_FS_4G                    (0x08)
    #define ACCEL_CFG1_FS_8G                    (0x10)
    #define ACCEL_CFG1_FS_16G                   (0x18)
// 加速度配置寄存器2
#define MPU9250_0X1D_ACCEL_CONFIG2              (0x1D)
    #define ACCEL_CFG2_BIT_FCHOICE_B            (0x08)
    #define ACCEL_CFG2_MSK_DLPFCFG              (0x07)          // 加速度低通滤波配置
    #define ACCEL_DLPFCFG_0                     (0x00)
    #define ACCEL_DLPFCFG_1                     (0x01)
    #define ACCEL_DLPFCFG_2                     (0x02)
    #define ACCEL_DLPFCFG_3                     (0x03)
    #define ACCEL_DLPFCFG_4                     (0x04)
    #define ACCEL_DLPFCFG_5                     (0x05)
    #define ACCEL_DLPFCFG_6                     (0x06)
    #define ACCEL_DLPFCFG_7                     (0x07)
// 加速度低功耗输出速率控制
#define MPU9250_0X1E_LP_ACCEL_ODR_CTRL          (0x1e)
// 设置加速度唤醒阈值，单位4mg, 取值范围0~255
#define MPU9250_0X1F_ACCEL_WOM_THR              (0x1f)
// 使能 fifo
#define MPU9250_0X23_FIFO_EN                    (0x23)
    #define FIFO_EN_BIT_TEMP_OUT_EN             (0x80)      // 使能温度高低字节输出到fifo
    #define FIFO_EN_BIT_GYRO_XOUT_EN            (0x40)      // 使能陀螺仪X轴高低字节输出到fifo
    #define FIFO_EN_BIT_GYRO_YOUT_EN            (0x20)      // 使能陀螺仪Y轴高低字节输出到fifo
    #define FIFO_EN_BIT_GYRO_ZOUT_EN            (0x10)      // 使能陀螺仪Z轴高低字节输出到fifo
    #define FIFO_EN_BIT_ACCEL_EN                (0x08)      // 使能加速度XYZ轴高低字节输出到fifo
    #define FIFO_EN_BIT_SLV2_EN                 (0x04)      // 使能外部控制器2数据输出到fifo
    #define FIFO_EN_BIT_SLV1_EN                 (0x02)      // 使能外部控制器1数据输出到fifo
    #define FIFO_EN_BIT_SLV0_EN                 (0x01)      // 使能外部控制器0数据输出到fifo
// i2c 多主模式设置寄存器
#define MPU9250_0X24_I2C_MST_CTRL               (0x24)
    #define I2C_MST_CTRL_BIT_EN                 (0x80)      // 使能 i2c 多主模式
    #define I2C_MST_CTRL_BIT_WAIT_FOR_ES        (0x40)      // 延迟数据中断，直到外部数据加载完毕
    #define I2C_MST_CTRL_BIT_SLV3_FIFO_EN       (0x20)      // 使能 slv3 fifo
    #define I2C_MST_CTRL_BIT_2C_MST_P_NSR       (0x10)      // 该位控制I2C主机从一个从读到下一个从读的转换
    #define I2C_MST_CTRL_MSK_CLK                (0x0f)      // i2c 时钟设置
    #define I2C_MST_CTRL_CLK_348_KHZ            (0x00)      // 8MHz; 分频 23
    #define I2C_MST_CTRL_CLK_333_KHZ            (0x01)      // 8MHz; 分频 24
    #define I2C_MST_CTRL_CLK_320_KHZ            (0x02)      // 8MHz; 分频 25
    #define I2C_MST_CTRL_CLK_308_KHZ            (0x03)      // 8MHz; 分频 26
    #define I2C_MST_CTRL_CLK_296_KHZ            (0x04)      // 8MHz; 分频 27
    #define I2C_MST_CTRL_CLK_286_KHZ            (0x05)      // 8MHz; 分频 28
    #define I2C_MST_CTRL_CLK_276_KHZ            (0x06)      // 8MHz; 分频 29
    #define I2C_MST_CTRL_CLK_267_KHZ            (0x07)      // 8MHz; 分频 30
    #define I2C_MST_CTRL_CLK_258_KHZ            (0x08)      // 8MHz; 分频 31
    #define I2C_MST_CTRL_CLK_500_KHZ            (0x09)      // 8MHz; 分频 16
    #define I2C_MST_CTRL_CLK_471_KHZ            (0x0a)      // 8MHz; 分频 17
    #define I2C_MST_CTRL_CLK_444_KHZ            (0x0b)      // 8MHz; 分频 18
    #define I2C_MST_CTRL_CLK_421_KHZ            (0x0c)      // 8MHz; 分频 19
    #define I2C_MST_CTRL_CLK_400_KHZ            (0x0d)      // 8MHz; 分频 20
    #define I2C_MST_CTRL_CLK_381_KHZ            (0x0e)      // 8MHz; 分频 21
    #define I2C_MST_CTRL_CLK_364_KHZ            (0x0e)      // 8MHz; 分频 22
// I2C slave0 配置寄存器
#define MPU9250_0X25_I2C_SLV0_ADDR              (0x25)      // 选择i2c slv0
#define MPU9250_0X26_I2C_SLV0_REG               (0x26)      // 设置i2c slv0地址 
#define MPU9250_0X27_I2C_SLV0_CTRL              (0x27)      // 控制i2c slv0;
#define MPU9250_0X63_I2C_SLV0_DO                (0x63)      // 设置 i2c slv0 数据
#define MPU9250_0X49_I2C_SLV0_DI                (0x49)      // 读取 i2c slv0 数据
// I2C slave1 配置寄存器
#define MPU9250_0X28_I2C_SLV1_ADDR              (0x28)
#define MPU9250_0X29_I2C_SLV1_REG               (0x29)
#define MPU9250_0X2A_I2C_SLV1_CTRL              (0x2a)
#define MPU9250_0X64_I2C_SLV1_DO                (0x64)
// I2C slave2 配置寄存器
#define MPU9250_0X2B_I2C_SLV2_ADDR              (0x2b)
#define MPU9250_0X2C_I2C_SLV2_REG               (0x2c)
#define MPU9250_0X2D_I2C_SLV2_CTRL              (0x2d)
#define MPU9250_0X65_I2C_SLV2_DO                (0x65)
// I2C slave3 配置寄存器
#define MPU9250_0X2E_I2C_SLV3_ADDR              (0x2e)
#define MPU9250_0X2F_I2C_SLV3_REG               (0x2f)
#define MPU9250_0X30_I2C_SLV3_CTRL              (0x30)
#define MPU9250_0X66_I2C_SLV3_DO                (0x66)
// I2C slave4 配置寄存器
#define MPU9250_0X31_I2C_SLV4_ADDR              (0x31)
#define MPU9250_0X32_I2C_SLV4_REG               (0x32)
#define MPU9250_0X33_I2C_SLV4_CTRL              (0x34)
#define MPU9250_0X34_I2C_SLV4_DO                (0x33)
#define MPU9250_0X35_I2C_SLV4_DI                (0x35)
// I2C 主模式状态
#define MPU9250_0X36_I2C_MST_STATUS             (0x36)
    #define I2C_MST_STATUS_BIT_PASS_THROUGH     (0x80)
    #define I2C_MST_STATUS_BIT_I2C_SLV4_DONE    (0x40)          // 当 i2c slv4 传输完成是产生中断
    #define I2C_MST_STATUS_BIT_I2C_LOST_ARB     (0x20)
    #define I2C_MST_STATUS_BIT_I2C_SLV4_NACK    (0x10)          // 当没有收到i2c slv4 的ack时, 且 INT_ENABLE_BIT_I2C_MST_INT_EN 使能时，会触发中断
    #define I2C_MST_STATUS_BIT_I2C_SLV3_NACK    (0x08)          // 当没有收到i2c slv3 的ack时, 且 INT_ENABLE_BIT_I2C_MST_INT_EN 使能时，会触发中断
    #define I2C_MST_STATUS_BIT_I2C_SLV2_NACK    (0x04)          // 当没有收到i2c slv2 的ack时, 且 INT_ENABLE_BIT_I2C_MST_INT_EN 使能时，会触发中断
    #define I2C_MST_STATUS_BIT_I2C_SLV1_NACK    (0x02)          // 当没有收到i2c slv1 的ack时, 且 INT_ENABLE_BIT_I2C_MST_INT_EN 使能时，会触发中断
    #define I2C_MST_STATUS_BIT_I2C_SLV0_NACK    (0x01)          // 当没有收到i2c slv0 的ack时, 且 INT_ENABLE_BIT_I2C_MST_INT_EN 使能时，会触发中断
// 中断引脚配置寄存器
#define MPU9250_0X37_INT_PIN_CFG                (0x37)
    #define INT_CFG_BIT_ACTL                    (0x80)          // 配置中断电平为低电平，默认为高电平
    #define INT_CFG_BIT_OPEN                    (0x40)          // 配置中断为开漏模式，默认为推挽
    #define INT_CFG_BIT_LATCH_INT_EN            (0x20)          // 配置中断电平持续到中断状态清除，默认中断电平持续50us
    #define INT_CFG_BIT_INT_ANYRD_2CLEAR        (0x10)          // 配置为读取数据后中断状态清除，默认为读取中断状态位后再清除
    #define INT_CFG_BIT_ACTL_FSYNC              (0x08)          // 配置 FSY 引脚中断电源为低电平，默认为高电平
    #define INT_CFG_BIT_FSYNC_INT_MODE_EN       (0x04)          // 配置 FSY 中断使能，默认关闭
    #define INT_CFG_BIT_BYPASS_EN               (0x02)          // 配置ES_CL/ES_DA 进入旁路模式，一般使用SPI模式时要使能该位
// 使能中断
#define MPU9250_0X38_INT_ENABLE                 (0x38)
    #define INT_ENABLE_BIT_WOM_EN               (0x20)          // 使能运动时产生中断
    #define INT_ENABLE_BIT_FIFO_OVERFLOW_EN     (0x10)          // 使能 fifo 溢出时产生中断
    #define INT_ENABLE_BIT_FSYNC_EN             (0x08)          // 使能 FSYNC 中断
    #define INT_ENABLE_BIT_RAW_ROY_EN           (0x01)          // 使能数据中断，当有一组数据完成时便会产生中断
// 中断状态
#define MPU9250_0X39_INT_STATUS                 (0x3a)
    #define INT_STATUS_BIT_WOM_INT              (0x40)
    #define INT_STATUS_BIT_FIFO_OVERFLOW_INT    (0x10)
    #define INT_STATUS_BIT_FSYNC_INT            (0x08)
    #define INT_STATUS_BIT_RAW_DATA_RDY_INT     (0x01)
// 加速度数据地址
#define MPU9250_0X3B_ACCEL_XOUT_H               (0x3B)
#define MPU9250_0X3C_ACCEL_XOUT_L               (0x3C)
#define MPU9250_0X3D_ACCEL_YOUT_H               (0x3D)
#define MPU9250_0X3E_ACCEL_YOUT_L               (0x3E)
#define MPU9250_0X3F_ACCEL_ZOUT_H               (0x3F)
#define MPU9250_0X40_ACCEL_ZOUT_L               (0x40)
// 温度数据地址
#define MPU9250_0X41_TEMPERATURE_H              (0x41)
#define MPU9250_0X42_TEMPERATURE_L              (0x42)
// 陀螺仪数据地址
#define MPU9250_0X43_GYRO_XOUT_H                (0x43)
#define MPU9250_0X44_GYRO_XOUT_L                (0x44)
#define MPU9250_0X45_GYRO_YOUT_H                (0x45)
#define MPU9250_0X46_GYRO_YOUT_L                (0x46)
#define MPU9250_0X47_GYRO_ZOUT_H                (0x47)
#define MPU9250_0X48_GYRO_ZOUT_L                (0x48)
// 扩展传感器数据(地磁数据)地址0x49~0x60
#define MPU9250_0X49_EXTEN_DATA                 (0x49)
// I2C延迟控制
#define MPU9250_0X67_I2C_MST_DELAY_CTRL         (0x67)
// 信号复位
#define MPU9250_0X68_SIGNAL_PATH_RESET          (0x68)
    #define SIGNAL_PATH_RESET_BIT_GYRO_RST      (0x04)
    #define SIGNAL_PATH_RESET_BIT_ACCEL_RST     (0x02)
    #define SIGNAL_PATH_RESET_BIT_TEMP_RST      (0x01)
// 加速度中断控制
#define ICM20948B2_0x69_ACCEL_INT_CTRL          (0x69)          
    #define ACCEL_INT_CTRL_BIT_EN               (0x80)          // 使能加速度唤醒功能
    #define ACCEL_INT_CTRL_BIT_MODE_INT         (0x40)          // 设置唤醒方式；0和初始化时的值进行对比，1和上一个值进行对比
// 用户控制寄存器
#define MPU9250_0X6A_USER_CTRL                  (0x6A)
    #define USER_CTRL_BIT_FIFO_EN               (0x40)          // 使能fifo
    #define USER_CTRL_BIT_I2C_MST_EN            (0x20)          // 使能 i2c 主模式
    #define USER_CTRL_BIT_I2C_IF_DIS            (0x10)          // 关闭 i2c 从模式，只能使用 spi 单一模式
    #define USER_CTRL_BIT_FIFO_RST              (0x04)          // 复位 fifo，清空队列数据
    #define USER_CTRL_BIT_I2C_MST_RST           (0x02)          // 复位 i2c 主模式，
    #define USER_CTRL_BIT_SIG_COND_RST          (0x01)
// 电源管理控制寄存器1
#define MPU9250_0X6B_PWR_MGMT1                  (0x6B)
    #define PWR_MGMT1_BIT_RESET                 (0x80)          // 复位整个芯片，所有数值恢复默认值
    #define PWR_MGMT1_BIT_SLEEP                 (0x40)          // 休眠标记位，设置后将进入休眠状态
    #define PWR_MGMT1_BIT_CYCLE                 (0x20)          // 循环模式，当休眠位和待机位没有置位时，芯片将处于工作和休眠来回切换
    #define PWR_MGMT1_BIT_GYRO_STANDBY          (0x10)          // 设置后，陀螺驱动和pll电路启用，但检测路径禁用。这是一种低功率模式，允许快速启用陀螺仪
    #define PWR_MGMT1_BIT_PD_PTAT               (0x08)          // 关闭内部PTAT电压发生器和PTAT ADC的电源
    #define PWR_MGMT1_MSK_CLOCK_SRC             (0x07)          // 时钟源
                                                                // 0/6: 内部20M时钟源
                                                                // 7: 停止时钟并保持定时发生器复位
// 电源管理控制寄存器2
#define MPU9250_0X6C_PWR_MGMT2                  (0x6C)
    #define PWR_MGMT2_BIT_AX_DISABLE            (0x20)          // 关闭加速度X轴
    #define PWR_MGMT2_BIT_AY_DISABLE            (0x10)          // 关闭加速度Y轴
    #define PWR_MGMT2_BIT_AZ_DISABLE            (0x08)          // 关闭加速度Z轴
    #define PWR_MGMT2_BIT_GX_DISABLE            (0x04)          // 关闭陀螺仪X轴
    #define PWR_MGMT2_BIT_GY_DISABLE            (0x02)          // 关闭陀螺仪Y轴
    #define PWR_MGMT2_BIT_GZ_DISABLE            (0x01)          // 关闭陀螺仪Z轴
// FIFO 数量
#define MPU9250_0X72_FIFO_COUNTH                (0x72)          // fifo 高字节, bit0~4
#define MPU9250_0X73_FIFO_COUNTL                (0x73)          // fifo 地字节, bit0~7
#define MPU9250_0X74_FIFO_R_W                   (0x74)
// 设备id
#define MPU9250_0X75_WHO_AM_I                   (0x75)          // 默认值0x71

// 加速度校准值，以+-16g为基准，单位 0.98mg
#define MPU9250_0X76_XA_OFFS_H                  (0x76)          // 高字节
#define MPU9250_0X77_XA_OFFS_L                  (0x77)          // 低字节，有效位bit1~bit7
#define MPU9250_0X78_YA_OFFS_H                  (0x78)
#define MPU9250_0X79_YA_OFFS_L                  (0x79)
#define MPU9250_0X7A_ZA_OFFS_H                  (0x7a)
#define MPU9250_0X7B_ZA_OFFS_L                  (0x7b)



/////////////////////////////AK8963 REGISTER/////////////////////////////////////////
// 设备id
#define AK8963_0X00_WIA                         (0x00)          // 默认0x48
// 设备信息
#define AK8963_0X01_INFO                        (0x01)          
// 设备状态
#define AK8963_0X02_ST1                         (0x02)
    #define AK8963_ST1_DRDY                     (1)
#define AK8963_0X03_XOUT_L                      (0x03)
#define AK8963_0X04_XOUT_H                      (0x04)
#define AK8963_0X05_YOUT_L                      (0x05)
#define AK8963_0X06_YOUT_H                      (0x06)
#define AK8963_0X07_ZOUT_L                      (0x07)
#define AK8963_0X08_ZOUT_H                      (0x08)
// 设备状态2
#define AK8963_0X09_ST2                         (0x09)
#define AK8963_0X0A_CNTL                        (0x0A)
    #define AK8963_CNTL_BIT_OUTPUT              (4)             // 0->输出14bit; 1->输出16dbit
    #define AK8963_CNTL_MODE_SETTING            (0x0f)          // 操作模式：0掉电模式；1单次测量模式；2连续测量模式1, 6连续测量模式2；8自测模式；15保险访问模式
    #define AK8963_CNTL_MODE_POWER_DOWN         (0x00)          // 关机模式
    #define AK8963_CNTL_MODE_SINGLE_MEASURE     (0x01)          // 单次测量模式
    #define AK8963_CNTL_MODE_CONTINUE1_MEASURE  (0x02)          // 连续测量模式1
    #define AK8963_CNTL_MODE_CONTINUE2_MEASURE  (0x06)          // 连续测量模式2
    #define AK8963_CNTL_MODE_EXTERNAL_MEASURE   (0x04)          
    #define AK8963_CNTL_MODE_SELF_TEST          (0x08)          // 自测模式
    #define AK8963_CNTL_MODE_FUSE_ROM           (0x0f)          // 保险访问模式
#define AK8963_0X0B_CNTL2                       (0x0B)
    #define AK8963_CNTL2_SRST                   (0x01)
#define AK8963_0X0C_ASTC                        (0x0C)
#define AK8963_0X0D_TS1                         (0x0D)
#define AK8963_0X0E_TS2                         (0x0E)  
#define AK8963_0X0F_I2CDIS                      (0x0F)          //
#define AK8963_0X10_ASAX                        (0x10)          // mx 轴校准值，只有在 fuse rom 下才能读取有效
#define AK8963_0X11_ASAY                        (0x11)          // my 轴校准值，只有在 fuse rom 下才能读取有效
#define AK8963_0X12_ASAZ                        (0x12)          // mz 轴校准值，只有在 fuse rom 下才能读取有效


typedef enum {
    // set reg29 Bit[3] & Bit[2:0] Hz
    MPU9250_Accel_DLPFBandwidth_460 = 0x00,    // delay 1.94ms
    MPU9250_Accel_DLPFBandwidth_184,           // delay 5.80ms
    MPU9250_Accel_DLPFBandwidth_92,            // delay 7.80ms
    MPU9250_Accel_DLPFBandwidth_41,            // delay 11.80ms
    MPU9250_Accel_DLPFBandwidth_20,            // delay 19.80ms
    MPU9250_Accel_DLPFBandwidth_10,            // delay 35.70ms
    MPU9250_Accel_DLPFBandwidth_5,             // delay 66.96ms
    MPU9250_Accel_DLPFBandwidth_460_2,         // delay 1.94ms
    MPU9250_Accel_DLPFBandwidth_1_13k = 0x08,  // delay 0.75ms
} MPU9250_Accel_DLPFBandwidth;


// 加速度值
typedef struct {
    int16_t z;
    int16_t y;
    int16_t x;
} accelerometer_t;

// 陀螺仪值
typedef struct {
    int16_t z;
    int16_t y;
    int16_t x;
} gyroscope_t;

// 地磁偏转角值
typedef struct {
    int16_t x;
    int16_t y;
    int16_t z;
} mag_val_t;

// 温度
typedef int16_t temperature_t;

typedef enum {
    LP_ACCEL_ODR_0_24HZ = 0,
    LP_ACCEL_ODR_0_49HZ,
    LP_ACCEL_ODR_0_98HZ,
    LP_ACCEL_ODR_1_95HZ,
    LP_ACCEL_ODR_3_91HZ,
    LP_ACCEL_ODR_7_81HZ,
    LP_ACCEL_ODR_15_63HZ,
    LP_ACCEL_ODR_31_25HZ,
    LP_ACCEL_ODR_62_50HZ,
    LP_ACCEL_ODR_125HZ,
    LP_ACCEL_ODR_250HZ,
    LP_ACCEL_ODR_500HZ,
} mpu9250_sample_rate_div_t;



#ifdef __cplusplus
}
#endif /*__cplusplus*/
#endif /*_DRV_9AXIS_MPU9250_H_*/

