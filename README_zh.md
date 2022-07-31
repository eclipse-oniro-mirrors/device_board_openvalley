# device_board_openvalley

## 介绍
本仓用于托管湖南开鸿智谷数字产业发展有限公司旗下Niobe系列开发板相关内容，各开发板的安装教程和使用说明可点击下表开发板名称查看。

|           开发板名称            |   SoC型号    |       应用领域       | 设备互联类型 |
| :-----------------------------: | :----------: | :------------------: | :----------: |
| [NiobeU4](niobeu4/README_zh.md) | `ESP32U4WDH` | 教育、工业、智能家居 |  Wifi、蓝牙  |

## 目录结构
```
device/board/openvalley
├── niobeu4                               # 开发板名称
    ├── figures                           # 文档图片目录
    ├── liteos_m                          # liteos_m适配目录
        ├── arch                          # 内核指令架构层目录
        ├── hals                          # 硬件抽象层目录
         	├── drivers					  # 驱动适配目录
            ├── iot_hardware			  # iothardware子系统适配
            ├── log						  # 日志输出适配
            ├── memory					  # 内存管理函数适配
            ├── syscalls				  # 系统调用函数适配
            ├── update				      # 程序升级子系统适配
            └── utils					  # 通用接口适配
        ├── hdf_config                    # hdf配置目录
        ├── target                        # 板级配置目录
        ├── third_party_adapter           # 三方库适配目录
        	├── littlefs				  # littlefs文件系统适配	
        	├── mbedtls					  # 轻量级加密库适配

├── xxx                                   # 其它板型，持续开发中...                    
```

#### 参与贡献

1.  Fork 本仓库
2.  新建 Feat_xxx 分支
3.  提交代码
4.  新建 Pull Request
