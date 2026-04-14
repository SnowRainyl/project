#include "OLED_SSD1306.h"
#include "custom_i2c.h"  /* 使用自定义寄存器级 I2C */

void OLED_Write_Byte(uint8_t WData, uint8_t cmd_or_data);
static void OLED_SetPos(uint8_t x, uint8_t y);

/**
 * @function: static void OLED_Write_Byte(uint8_t WData, uint8_t cmd_or_data)
 * @description: OLED通过硬件I2C写一个字节操作函数(注:STM32CubeMX配置i2c时将i2c中断打开，并将优先级设为最高)
 * @param {uint8_t} WData待写入的一个字节数据
 * @param {uint8_t} cmd_or_data写入的是指令还是数据<CmdReg(0x00)/DataReg(0x40)>
 * @return {*}
 */
void OLED_Write_Byte(uint8_t WData, uint8_t cmd_or_data)
{
#if OLED_USING_HARDWARE_I2C
    /* 使用自定义寄存器操作的 I2C 写内存函数 */
    /* OLED_Addr 定义为 0x78 包含读/写位，传给函数时需要右移一位
       以得到 7 位地址 0x3C */
    i2c_write_memory(OLED_Addr >> 1, cmd_or_data, WData, 1);
#endif
#if OLED_USING_SOFTWARE_I2C
    Software_I2C_Start();
    Software_I2C_WriteByte(OLED_Addr & 0xFE); //写入器件地址(bit0=1读，bit0=0写)
    if (Software_I2C_WaitACK())
        Software_I2C_Stop();
    Software_I2C_WriteByte(cmd_or_data); //指明写入指令还是数据
    if (Software_I2C_WaitACK())
        Software_I2C_Stop();
    Software_I2C_WriteByte(WData); //写入数据
    if (Software_I2C_WaitACK())
        Software_I2C_Stop();
    Software_I2C_Stop();
#endif
}

/**
 * @function: static void OLED_SetPos(uint8_t x, uint8_t y)
 * @description: 设置显示起始坐标位置
 * @param {uint8_t} x起始显示横坐标
 * @param {uint8_t} y起始显示纵坐标
 * @return {*}
 */
static void OLED_SetPos(uint8_t x, uint8_t y)
{
    OLED_Write_Byte(0xb0 + y, CmdReg);               //起始显示的页
    OLED_Write_Byte((x & 0xf0) >> 4 | 0x10, CmdReg); //设置高位列地址
    OLED_Write_Byte((x & 0x0f) | 0x01, CmdReg);      //设置低位列地址
}

/**
 * @function: void OLED_Fill(uint8_t Fill_Data)
 * @description: 全屏显示相同数据(主要用去清屏)
 * @param {uint8_t} Fill_Data待写入的数据<Bright(0xFF)/Dark(0x00)>
 * @return {*}
 */
void OLED_Fill(uint8_t Fill_Data)
{
    uint8_t i, j;
    for (i = 0; i < 8; i++) //8页，共需要循环显示8次
    {
        OLED_Write_Byte(0xb0 + i, CmdReg); //页地址0~7
        OLED_Write_Byte(0x10, CmdReg);     //设置高位列地址
        OLED_Write_Byte(0x00, CmdReg);     //设置低位列地址
        for (j = 0; j < OLED_Width; j++)   //每页有128列需要显示
            OLED_Write_Byte(Fill_Data, DataReg);
    }
}

/**
 * @function: void OLED_LocalFill(uint8_t x0, uint8_t y0, uint8_t x1, uint8_t y1, uint8_t LocalFillData)
 * @description: 局部写相同数据(主要用于局部清屏/亮屏)
 * @param {uint8_t} x0开始显示横坐标
 * @param {uint8_t} y0开始显示纵坐标
 * @param {uint8_t} x1结束显示横坐标
 * @param {uint8_t} y1结束显示纵坐标
 * @param {uint8_t} LocalFillData显示的数据
 * @return {*}
 */
void OLED_LocalFill(uint8_t x0, uint8_t y0, uint8_t x1, uint8_t y1, uint8_t LocalFillData)
{
    uint8_t i, j;
    for (i = y0; i <= y1; i++)
    {
        OLED_SetPos(x0, i);
        for (j = x0; j < x1; j++)
            OLED_Write_Byte(LocalFillData, DataReg);
    }
}

/**
 * @function: void OLED_Init(void)
 * @description: OLED初始化函数
 * @param {*}
 * @return {*}
 */
void OLED_Init(void)
{
#if OLED_USING_SOFTWARE_I2C
    Software_I2C_GPIO_Init();
#endif
    OLED_Delay_ms(200);
    OLED_Write_Byte(0xae, CmdReg); //关闭显示器 display off
    OLED_Write_Byte(0x20, CmdReg); //设置内存寻址模式Set Memory Addressing Mode
    OLED_Write_Byte(0x10, CmdReg); //00，水平寻址模式；01，垂直寻址模式；10，页面寻址模式（复位）；11，无效
    OLED_Write_Byte(0x00, CmdReg); //设置低位列地址 set low column address
    OLED_Write_Byte(0x10, CmdReg); //设置高位列地址 set high column address
    OLED_Write_Byte(0x40, CmdReg); //设置起始行地址 set start line address
    OLED_Write_Byte(0xb0, CmdReg); //设置页地址 set page address
    OLED_Write_Byte(0x81, CmdReg); //设置对比度
    OLED_Write_Byte(0xff, CmdReg); //对比度,数值越大对比度越高
    OLED_Write_Byte(0xc8, CmdReg); //扫描方向 不上下翻转Com scan direction
    OLED_Write_Byte(0xa1, CmdReg); //设置段重新映射 不左右翻转set segment remap
    OLED_Write_Byte(0xa6, CmdReg); //设置正常/反向 normal / reverse
    OLED_Write_Byte(0xa8, CmdReg); //设置多路复用比(1-64) set multiplex ratio(1 to 64)
    OLED_Write_Byte(0x3f, CmdReg); //设定值1/32  1/32 duty
    OLED_Write_Byte(0xd3, CmdReg); //设置显示偏移 set display offset
    OLED_Write_Byte(0x00, CmdReg); //
    OLED_Write_Byte(0xd5, CmdReg); //设置osc分区 set osc division
    OLED_Write_Byte(0x80, CmdReg); //
    OLED_Write_Byte(0xd8, CmdReg); //关闭区域颜色模式 set area color mode off
    OLED_Write_Byte(0x05, CmdReg); //
    OLED_Write_Byte(0xd9, CmdReg); //设置预充电期 Set Pre-Charge Period
    OLED_Write_Byte(0xf1, CmdReg); //
    OLED_Write_Byte(0xda, CmdReg); //设置com引脚配置 set com pin configuartion
    OLED_Write_Byte(0x12, CmdReg); //
    OLED_Write_Byte(0xdb, CmdReg); //设置vcomh set Vcomh
    OLED_Write_Byte(0x30, CmdReg); //
    OLED_Write_Byte(0x8d, CmdReg); //设置电源泵启用 set charge pump enable
    OLED_Write_Byte(0x14, CmdReg); //
    OLED_Write_Byte(0xa4, CmdReg); //设置全局显示开启；bit0，1开启，0关闭(白屏/黑屏)
#if OLED_INVERSE_COLOR
    OLED_Write_Byte(0xa7, CmdReg); //设置显示方式，bit0，1反相显示，0正常显示
#else
    OLED_Write_Byte(0xa6, CmdReg); //设置显示方式，bit0，1反相显示，0正常显示
#endif
    OLED_Write_Byte(0xaf, CmdReg); //打开oled面板 turn on oled panel
    OLED_Fill(Dark);               //清屏
    OLED_Delay_ms(100);
}

/**
 * @function: void OLED_ShowChar(uint8_t x, uint8_t y, uint8_t DisplayChar, uint8_t FontSize,uint8_t Color_Turn)
 * @description: 在OLED12864特定位置开始显示一个字符
 * @param {uint8_t} x字符开始显示的横坐标
 * @param {uint8_t} y字符开始显示的纵坐标
 * @param {uint8_t} DisplayChar待显示的字符
 * @param {uint8_t} FontSize待显示字符的字体大小(FontSize6x8/FontSize8x16)
 * @param {uint8_t} Color_Turn是否反相显示(1反相、0不反相)
 * @return {*}
 */
void OLED_ShowChar(uint8_t x, uint8_t y, uint8_t DisplayChar, uint8_t FontSize, uint8_t Color_Turn)
{
    uint8_t c = 0, i;
    c = DisplayChar - 32; //等同于减去个空格c = DisplayChar - ' '; 得到显示字符在库中的偏移地址
    if (x > 128 - 1)
    {
        x = 0;
        y += 2;
    }
    switch (FontSize)
    {
    case 1:
        OLED_SetPos(x, y); //开始显示位置
        for (i = 0; i < 6; i++)
        {
            if (Color_Turn)
                OLED_Write_Byte(~F6X8[c * 6 + i], DataReg); //显示一个6X8字符
            else
                OLED_Write_Byte(F6X8[c * 6 + i], DataReg); //显示一个6X8字符
        }
        break;
    case 2:
        OLED_SetPos(x, y);      //开始显示位置
        for (i = 0; i < 8; i++) //显示一个8X16字符的上半部分
        {
            if (Color_Turn)
                OLED_Write_Byte(~F8X16[c * 16 + i], DataReg); //循环显示数组中前8个数据
            else
                OLED_Write_Byte(F8X16[c * 16 + i], DataReg); //循环显示数组中前8个数据
        }
        OLED_SetPos(x, y + 1);  //换到下一页，显示字符下半部分
        for (i = 0; i < 8; i++) //显示8X16字符的下半部分
        {
            if (Color_Turn)
                OLED_Write_Byte(~F8X16[c * 16 + i + 8], DataReg); //循环显示数组中后8个数据
            else
                OLED_Write_Byte(F8X16[c * 16 + i + 8], DataReg); //循环显示数组中后8个数据
        }
        break;
    }
}

/**
 * @function: void OLED_ShowStr(uint8_t x, uint8_t y, uint8_t *DisplayStr, uint8_t FontSize, uint8_t Color_Turn)
 * @description: 在OLED12864特定位置开始显示字符串
 * @param {uint8_t} x待显示字符串的开始横坐标
 * @param {uint8_t} y待显示字符串的开始纵坐标
 * @param {uint8_t} *DisplayStr待显示的字符串
 * @param {uint8_t} FontSize待显示字符串的字体大小(FontSize6x8/FontSize8x16)
 * @param {uint8_t} Color_Turn是否反相显示(1反相、0不反相)
 * @return {*}
 */
void OLED_ShowStr(uint8_t x, uint8_t y, uint8_t *DisplayStr, uint8_t FontSize, uint8_t Color_Turn)
{
    uint8_t j = 0;
    while (DisplayStr[j] != '\0') //判断字符串是否显示完成
    {
        OLED_ShowChar(x, y, DisplayStr[j], FontSize, Color_Turn);
        if (FontSize == 1) //6X8的字体列加6，显示下一个字符
            x += 6;
        else if (FontSize == 2) //8X16的字体列加8，显示下一个字符
            x += 8;

        if (x > 122 && FontSize == 1) //TextSize6x8如果一行不够显示了，从下一行继续显示
        {
            x = 0;
            y++;
        }
        if (x > 120 && FontSize == 2) //TextSize8x16如果一行不够显示了，从下一行继续显示
        {
            x = 0;
            y++;
        }
        j++;
    }
}

/**
 * @function: void OLED_ShowCN(uint8_t x, uint8_t y, uint8_t Num, uint8_t Color_Turn)
 * @description: 在OLED特定位置开始显示16X16汉字
 * @param {uint8_t} x待显示的汉字其实横坐标
 * @param {uint8_t} y待显示的汉字其实纵坐标
 * @param {uint8_t} Num待显示的汉字编号
 * @param {uint8_t} Color_Turn是否反相显示(1反相、0不反相)
 * @return {*}
 */
//void OLED_ShowCN(uint8_t x, uint8_t y, uint8_t Num, uint8_t Color_Turn)
//{
//    uint8_t i;
//    OLED_SetPos(x, y);
//    for (i = 0; i < 16; i++)
//    {
//        if (Color_Turn)
//            OLED_Write_Byte(~FontCN[2 * Num][i], DataReg); //显示汉字的上半部分
//        else
//            OLED_Write_Byte(FontCN[2 * Num][i], DataReg); //显示汉字的上半部分
//    }
//    OLED_SetPos(x, y + 1);
//    for (i = 0; i < 16; i++)
//    {
//        if (Color_Turn)
//            OLED_Write_Byte(~FontCN[2 * Num + 1][i], DataReg); //显示汉字的下半部分
//        else
//            OLED_Write_Byte(FontCN[2 * Num + 1][i], DataReg); //显示汉字的下半部分
//    }
//}

/**
 * @function: void OLED_ShowNum20X40(uint8_t x, uint8_t y, uint8_t Num, uint8_t Color_Turn)
 * @description: 显示0~9数字(20X40)
 * @param {uint8_t} x待显示的数字起始横坐标
 * @param {uint8_t} y待显示的数字起始横坐标
 * @param {uint8_t} Num待显示的数字编号
 * @param {uint8_t} Color_Turn是否反相显示(1反相、0不反相)
 * @return {*}
 */
#if 0
void OLED_ShowNum20X40(uint8_t x, uint8_t y, uint8_t Num, uint8_t Color_Turn)
{
    uint8_t i, j, k = 0;
    OLED_SetPos(x, y);      //设置显示起始位置
    for (j = 0; j < 5; j++) //20X40共要显示5页
    {
        for (i = 0; i < 20; i++) //20X40每页显示20个数据
        {
            if (Color_Turn)
                OLED_Write_Byte(~Num20X40[Num][k++], DataReg); //显示第Num维中的第k个数据
            else
                OLED_Write_Byte(Num20X40[Num][k++], DataReg); //显示第Num维中的第k个数据
        }
        OLED_SetPos(x, y + j + 1); //切换显示起始位置到下一页
    }
}
#endif

/**
 * @function: void OLED_ShowBMP(uint8_t x0, uint8_t y0, uint8_t x1, uint8_t y1, uint8_t *BMP)
 * @description: 在OLED特定区域显示BMP图片
 * @param {uint8_t} x0图像开始显示横坐标
 * @param {uint8_t} y0图像开始显示纵坐标
 * @param {uint8_t} x1图像结束显示横坐标
 * @param {uint8_t} y1图像结束显示纵坐标
 * @param {uint8_t} *BMP待显示的图像数据
 * @return {*}
 */
#if 1
void OLED_ShowBMP(uint8_t x0, uint8_t y0, uint8_t x1, uint8_t y1, uint8_t *BMP)
{
    uint32_t j = 0;
    uint8_t x = 0, y = 0;
    if (y1 % 8 == 0) //用于确定显示几页
        y = y1 / 8;
    else
        y = y1 / 8 + 1;
    for (y = y0; y < y1; y++) //从设置的起始页开始显示到结束页
    {
        OLED_SetPos(x0, y);                     //进入下一页显示起始位置
        for (x = x0; x < x1; x++)               //从设置开始列显示到结束列
            OLED_Write_Byte(BMP[j++], DataReg); //逐个传入显示数据
    }
}
#endif
