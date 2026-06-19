/*****************************************************************************
* | File      	:   Readme_CN.txt
* | Author      :   
* | Function    :   Help with use
* | Info        :
*----------------
* |	This version:   V1.0
* | Date        :   2022-08-29
* | Info        :   在这里提供一个中文版本的使用文档，以便你的快速使用
******************************************************************************/
这个文件是帮助您使用本例程。
在这里简略的描述本工程的使用：

1.基本信息：
本例程使用RGB-Matrix-P3-64x32进行了验证，你可以在工程的中查看对应的测试例程;

2.管脚连接：
R1  -> GP13
G1  -> R2(数据输出)
B1  -> G2(数据输出)
R2  -> R1(数据输出)
G2  -> G1(数据输出)
B2  -> B1(数据输出)
A    ->  GP19
B    ->  GP23
C    ->  GP18
D    ->  GP5
E     ->  GP15
CLK ->  GP14	  
STB/LAT->  GP22
OE   ->  GP0


3.基本使用：
    1): 将libraries里面的文件全部拷贝到Arduino IDE的安装路径中的libraries下
        
    2): 用Arduino IDE打开工程EzTimeTetrisClockESP32.ino
    
    3): 点击文件->附加开发板管理器地址->输入：
"https://raw.githubusercontent.com/espressif/arduino-esp32/gh-pages/package_esp32_index.json"
->好
        
    4): 点击工具->开发板->开发板管理器->搜索ESP32->下载1.0.6的版本->关闭->ESP32 Arduino->NodeMCU-32S
        
    5): 设置好wifi名跟密码（在56行），连接好ESP32,选择对应的端口，然后点击编辑下面的箭头编译下载即可
    