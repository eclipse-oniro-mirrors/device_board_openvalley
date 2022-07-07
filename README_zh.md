# device_board_openvalley

## 介绍
本仓用于放置openvalley开发板相关内容，各开发板的安装教程和使用说明可点击下表开发板名称查看。

|           开发板名称            |   SoC型号    |       应用领域       | 设备互联类型 |
| :-----------------------------: | :----------: | :------------------: | :----------: |
| [Niobeu4](niobeu4/README_zh.md) | `ESP32U4WDH` | 教育、工业、智能家居 |  Wifi、蓝牙  |

## 软件架构
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


#### 特技

1.  使用 Readme\_XXX.md 来支持不同的语言，例如 Readme\_en.md, Readme\_zh.md
2.  Gitee 官方博客 [blog.gitee.com](https://blog.gitee.com)
3.  你可以 [https://gitee.com/explore](https://gitee.com/explore) 这个地址来了解 Gitee 上的优秀开源项目
4.  [GVP](https://gitee.com/gvp) 全称是 Gitee 最有价值开源项目，是综合评定出的优秀开源项目
5.  Gitee 官方提供的使用手册 [https://gitee.com/help](https://gitee.com/help)
6.  Gitee 封面人物是一档用来展示 Gitee 会员风采的栏目 [https://gitee.com/gitee-stars/](https://gitee.com/gitee-stars/)
