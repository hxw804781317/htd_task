# 1、使用方式

git clone https://github.com/hxw804781317/htd_task

# 2、介绍

htd_task 是一款开源的轻量级嵌入式任务管理器，方便开发者基于裸机系统和RTOS系统的开发和移植。资源占用低，支持多种 MCU。

# 3、目录介绍

| 文件夹          | 功能      | 备注                                               |
|:---------------:| --------- | -------------------------------------------------- |
| **components**  | 功能组件  |                                                    |
| **device**      | MCU驱动   | 提供统一的MCU驱动接口，方便项目在不同的MCU之间迁移 |
| **example**     | 工程样例  |                                                    |
| **external**    | 开源模块  |                                                    |
| **mcu_sdk**     | MCU SDK包 |                                                    |
| **peripherals** | 外设驱动  |                                                    |
| **task**        | 任务调度  |                                                    |
| **tools**       | 脚本工具  |                                                    |

## 3.1 功能组件介绍，components 文件夹

| 文件夹      | 功能               | 描述                                                                                                                                                                                                                                                                                                                      |
| ----------- | ------------------ | ------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------- |
| htd_console | 控制台             | 1、提供命令行输入功能<br/>2、提供命令行历史保存功能                                                                                                                                                                                                                                                                       |
| htd_button  | 按键               | 1、通用按键事件驱动，GPIO/ADC驱动<br/>2、查理复用按键<br/>3、矩阵按键                                                                                                                                                                                                                                                     |
| htd_led     | LED 灯             | 1、通用LED灯，GPIO/PWM驱动<br/>2、查理复用 LED<br/>3、矩阵LED                                                                                                                                                                                                                                                             |
| htd_boot    | bootloader启动程序 | 1、支持YModem协议通信<br/>2、支持固件fastlz/quicklz压缩解压<br/>3、支持                                                                                                                                                                                                                                                   |
| htd_flash   | flash抽象接口      | 1、flashkv支持key-value为键值对的数据存储模式<br/>2、flashbox支持信息记录以序列顺序存储，支持同种数据格式存储为多个数据块<br/>3、flashlog 支持以数据流的模式存储数据，后面数据超出配置大小会占用前面存储的数据<br/>4、flashvar 支持单个变量存储，该变量可以是结构体<br/>5、磨损平衡，掉电保护<br/>6、内存占用极小，运行快 |
| htd_vfs     | 虚拟文件系统       | 1、支持文件系统 ramfs/romfs/fatfs/littlefs/spifs<br/>2、支持外部                                                                                                                                                                                                                                                          |

## 3.1.1 htd_flash_kv，键值对存储

把一片 flash 当成“小型配置数据库”：一个 key 对应一条数据，key 是 `0 ~ key_count-1` 的序号（上限 4094），值长度可变、最长 2032 字节（`FLASHKV_VAL_MAX_LEN`）。编译开关 `HTD_FLASH_KV_ENABLE`。

| 接口 | 说明 |
| ---- | ---- |
| `htd_flashkv_init(p_fkv, p_cfg, p_key_table, key_count)` | 初始化，扫描区域并重建索引表 |
| `htd_flashkv_uninit(p_fkv)` | 去初始化，清掉 RAM 侧状态 |
| `htd_flashkv_write(p_fkv, key, p_data, data_len)` | 写入/更新一个 key，`data_len=0` 即清空该 key |
| `htd_flashkv_read(p_fkv, key, p_data, dat_len)` | 读一个 key（带 crc 校验） |
| `htd_flashkv_data_len_read(p_fkv, key)` | 只取该 key 的数据长度，不读内容 |
| `htd_flashkv_read_direct(p_fkv, key, p_data, data_len)` | 只读数据前 `data_len` 字节，不校验 |
| `htd_flashkv_data_addr_get(p_fkv, key)` | 取该 key 数据在 flash 中的地址（可映射直读） |
| `htd_flashkv_delete(p_fkv, key)` | 删除一个 key（内部写一条空记录作废） |
| `htd_flashkv_clear(p_fkv)` | 清空所有 key |

使用要点：

- 区域至少 **2 个擦除单位**，`flash_config_t.start / end` 按擦除单位对齐；
- 索引表 `key_table_t[键数]` 常驻 RAM（每个 key 2 字节），`init` 时扫描重建；
- 写满自动做页交换，没有单独的 gc 接口；
- 掉电语义（做过逐字节掉电注入验证，39466 项检查 0 失败）：只允许“正在写的那条”丢或回退成旧值，**其它 key 不受影响，也不会永久写不进去**；
- 8 位 crc8 对“写到一半被掉电撕裂”的记录有 1/256 概率漏判成新值，属已知限制（掉电本身概率很低，未为此改 on-flash 格式）。

```c
static flashkv_t m_kv;
static key_table_t m_key_table[8];                  // 8 个 key
static const flash_config_t m_cfg = {
    .p_name = "norflash_4k",                        // HTD_FLASH_HANDLE_REGISTER 注册的名字
    .start = NORFLASH_MAP_START_ADDR,
    .end = NORFLASH_MAP_START_ADDR + (64 * 1024),
};

htd_flashkv_init(&m_kv, &m_cfg, m_key_table, ARRAY_COUNT(m_key_table));
htd_flashkv_write(&m_kv, 0, &net_cfg, sizeof(net_cfg));       // key 0 存一份配置
htd_flashkv_read(&m_kv, 0, &net_cfg, sizeof(net_cfg));
```

## 3.1.2 htd_flash_var，单变量存储

只存“一个变量”，变量可以是结构体，每次 set 都是覆盖更新，适合节点参数、校准值这类只有一份的数据。编译开关 `HTD_FLASH_VAR_ENABLE`。

| 接口 | 说明 |
| ---- | ---- |
| `htd_flashvar_init(p_var, p_cfg)` | 初始化，扫描区域找出最新记录并定位写指针 |
| `htd_flashvar_set(p_var, p_buff, buff_len)` | 写入/更新变量，成功返回写入长度 |
| `htd_flashvar_get(p_var, p_buff, buff_size)` | 读取当前变量，成功返回数据长度 |

使用要点：

- 区域至少 **2 个擦除单位**，`flash_config_t.start / end` 按擦除单位对齐；
- 掉电语义（逐字节掉电注入 + 1/2/4 页回绕用例，4338 项检查 0 失败）：重启后只会读到“旧值”或“新值”，不会读到半条，也不会永久写不进去；
- `get` 时缓冲小于实际长度返回 `ERR_FLASH_INVALID_ARG`，区域里还没有数据返回 `ERR_FLASH_NOT_FOUND`；
- 同样有 crc8 撕裂 1/256 漏判的已知限制。

```c
static flashvar_t m_var;
static const flash_config_t m_cfg = {
    .p_name = "norflash_4k",
    .start = NORFLASH_MAP_START_ADDR,
    .end = NORFLASH_MAP_START_ADDR + (2 * 4096),        // 至少 2 个擦除单位
};

htd_flashvar_init(&m_var, &m_cfg);
htd_flashvar_set(&m_var, &cfg, sizeof(cfg));            // 掉电最多丢这一条，回退旧值
htd_flashvar_get(&m_var, &cfg, sizeof(cfg));
```

## 3.2 MCU驱动，device文件夹

| 文件夹        | 功能           | 描述                                                                                                                                     |
| ------------- | -------------- | ---------------------------------------------------------------------------------------------------------------------------------------- |
| drv_stm32fxxx | stm32f系列驱动 | 1、gpio<br/>2、uart/uarts<br/>3、RTC<br/>4、SPI_Master<br/>5、I2C_Master<br/>6、CRC<br/>7、随机值RNG<br/>8、PM管理，支持低功耗休眠和唤醒 |
|               |                |                                                                                                                                          |
|               |                |                                                                                                                                          |

## 3.3 外设驱动，peripherals文件夹

## 3.4 任务管理，task文件夹

| 文件夹  | 功能                 | 描述                                                                                                                                                           |
| ------- | -------------------- | -------------------------------------------------------------------------------------------------------------------------------------------------------------- |
| common  | 通用的数据结构和功能 | 1、先进先出队列fifo<br/>2、双链表list和单链表slist<br/>3、时间转换<br/>4、base16/base32/base64<br/>5、格式化拼接功能<br/>6、bit位控制操作<br/>7、section块管理 |
| memheap | 内存堆               | 提供malloc/free接口                                                                                                                                            |
