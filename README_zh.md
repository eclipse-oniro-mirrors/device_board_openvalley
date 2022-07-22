# device_board_openvalley

## 介绍
本仓用于放置openvalley开发板相关内容，各开发板的安装教程和使用说明可点击下表开发板名称查看。

|           开发板名称            |   SoC型号    |       应用领域       | 设备互联类型 |
| :-----------------------------: | :----------: | :------------------: | :----------: |
| [Niobeu4](niobeu4/README_zh.md) | `ESP32U4WDH` | 教育、工业、智能家居 |  Wifi、蓝牙  |

## 目录结构
```
device/board/openvalley
├── niobeu4                               # 开发板名称
    ├── figures                           # 文档目录，用于存放README_zh.md图片
    ├── liteos_m                          # LiteOS SDK目录
        ├── arch                          # 内核指令架构层目录
        ├── driver                        # 驱动目录
        ├── hals                          # 硬件抽象层目录
        ├── hdf_config                    # hdf配置目录
        ├── include                       # 板级配置头文件目录
        ├── littlefs                      # littlefs适配目录
        ├── target                        # 启动文件目录

├── xxx                                   # 其它板型，持续开发中...                    
```

#### 参与贡献

1.  Fork 本仓库
2.  新建 Feat_xxx 分支
3.  提交代码
4.  新建 Pull Request
