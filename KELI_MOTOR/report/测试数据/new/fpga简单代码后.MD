# FPGA 简单版 SPI 接收测试记录

## 本次修改

- 保存原 2 级同步链版本为 `MOTOR_TEST/spi_slave_2stage_baseline.vhd`，作为 baseline。
- 当前工作版 `MOTOR_TEST/spi_slave.vhd` 改为简单版：直接使用外部 `sck` 的上升沿接收 `mosi`。
- `top.vhd` 和 `pwm_gen.vhd` 未改，因为 `spi_slave` 端口保持不变。
- `clk` 端口仍保留，只是简单版内部不使用，方便继续兼容顶层连接。

## LA2016 测试结果

测试方式：

- LA2016 使用 KingstVIS。
- CH 接到 `pwm_out / TB6612 PWMA`。
- GND 接系统地。
- 先测到 FPGA 输出 PWM，再确认 LA2016 应并联到 PWMA，不能拔掉原本 FPGA 到 TB6612 的连接。

关键结果：

```text
PWM Frequency ≈ 12.207 kHz
PWM DutyCycle ≈ 51.78%
STM32 串口 duty ≈ 2119~2121，对应 51.7%~51.8%
```

结论：

```text
STM32 duty -> SPI2 -> FPGA 简单版 spi_slave -> pwm_gen -> pwm_out
```

这条链路在当前测试条件下是通的，PWM 频率和占空比都与串口打印一致。

## Flash 固化记录

为了断电后仍能自动运行，使用 iMPACT 生成并写入 SPI Flash：

- 板载配置 Flash：`M25P16-VMN3PB`
- PROM 格式：`MCS`
- 选择路径：`SPI Flash -> Configure Single FPGA`
- Storage Device：`16M`
- Attached Flash：`SPI PROM / M25P16`
- Data Width：`1`
- 生成文件：`/home/ise/moter_new/top_simple_spi.mcs`

注意：

```text
top.bit = 临时下载到 FPGA，断电丢失
top_simple_spi.mcs = 写入 M25P16 配置 Flash，断电保存
```

## 原始串口数据

设置24.2
[IDLE ] set=0.0 setR=0.0 RPM=0.0 duty=   0 (0.0%) adc=  10 curr_raw=1997 iset=0.0mA curr=0.0mA
[IDLE ] set=0.0 setR=0.0 RPM=0.0 duty=   0 (0.0%) adc=  10 curr_raw=1996 iset=0.0mA curr=0.0mA
[IDLE ] set=0.0 setR=0.0 RPM=0.0 duty=   0 (0.0%) adc=  10 curr_raw=1996 iset=0.0mA curr=0.0mA
[IDLE ] set=0.0 setR=0.0 RPM=0.0 duty=   0 (0.0%) adc=  10 curr_raw=1996 iset=0.0mA curr=0.0mA
[IDLE ] set=0.0 setR=0.0 RPM=0.0 duty=   0 (0.0%) adc=  11 curr_raw=1995 iset=0.0mA curr=0.0mA
[IDLE ] set=0.0 setR=0.0 RPM=0.0 duty=   0 (0.0%) adc=  11 curr_raw=2023 iset=0.0mA curr=0.0mA
[IDLE ] set=0.0 setR=0.0 RPM=0.0 duty=   0 (0.0%) adc=  17 curr_raw=2056 iset=0.0mA curr=0.0mA
[START] set=22.5 setR=22.5 RPM=0.0 duty= 107 (2.6%) adc= 681 curr_raw=2216 iset=5.8mA curr=4.3mA
[START] set=24.1 setR=24.1 RPM=0.0 duty= 149 (3.6%) adc= 752 curr_raw=2274 iset=14.2mA curr=13.8mA
[START] set=24.1 setR=24.1 RPM=0.0 duty= 158 (3.9%) adc= 752 curr_raw=2326 iset=22.8mA curr=22.4mA
[START] set=24.1 setR=24.1 RPM=0.0 duty= 190 (4.6%) adc= 751 curr_raw=2377 iset=31.5mA curr=30.8mA
[START] set=24.1 setR=24.2 RPM=0.0 duty= 226 (5.5%) adc= 753 curr_raw=2434 iset=39.9mA curr=39.6mA
[START] set=24.1 setR=24.1 RPM=0.0 duty= 276 (6.7%) adc= 753 curr_raw=2489 iset=48.6mA curr=48.2mA
[START] set=24.1 setR=24.1 RPM=0.0 duty= 307 (7.5%) adc= 751 curr_raw=2535 iset=57.0mA curr=56.6mA
[START] set=24.2 setR=24.1 RPM=0.0 duty= 342 (8.4%) adc= 754 curr_raw=2594 iset=65.7mA curr=65.2mA
[START] set=24.2 setR=24.1 RPM=0.5 duty= 408 (10.0%) adc= 754 curr_raw=2563 iset=74.3mA curr=66.0mA
[RUN  ] set=24.2 setR=24.2 RPM=24.1 duty= 808 (19.7%) adc= 757 curr_raw=2305 iset=31.6mA curr=31.3mA
[RUN  ] set=24.2 setR=24.2 RPM=35.6 duty=1016 (24.8%) adc= 756 curr_raw=2273 iset=28.3mA curr=18.8mA
[RUN  ] set=24.3 setR=24.3 RPM=39.1 duty=1072 (26.2%) adc= 759 curr_raw=2271 iset=23.2mA curr=23.4mA
[RUN  ] set=24.2 setR=24.3 RPM=36.6 duty= 998 (24.4%) adc= 757 curr_raw=2237 iset=18.4mA curr=14.4mA
[RUN  ] set=24.2 setR=24.2 RPM=32.1 duty= 871 (21.3%) adc= 757 curr_raw=2236 iset=15.1mA curr=29.5mA
[RUN  ] set=24.2 setR=24.2 RPM=27.1 duty= 750 (18.3%) adc= 755 curr_raw=2351 iset=13.8mA curr=10.0mA
[RUN  ] set=24.2 setR=24.2 RPM=22.6 duty= 659 (16.1%) adc= 757 curr_raw=2233 iset=14.0mA curr=20.7mA
[RUN  ] set=24.2 setR=24.2 RPM=21.1 duty= 622 (15.2%) adc= 755 curr_raw=2218 iset=14.9mA curr=25.0mA
[RUN  ] set=24.2 setR=24.2 RPM=20.6 duty= 646 (15.8%) adc= 756 curr_raw=2272 iset=16.1mA curr=10.4mA
[RUN  ] set=24.2 setR=24.2 RPM=22.1 duty= 667 (16.3%) adc= 755 curr_raw=2452 iset=17.0mA curr=12.9mA
[RUN  ] set=24.2 setR=24.2 RPM=23.6 duty= 715 (17.5%) adc= 754 curr_raw=2264 iset=17.3mA curr=11.4mA
[RUN  ] set=24.1 setR=24.1 RPM=25.1 duty= 754 (18.4%) adc= 753 curr_raw=2242 iset=17.1mA curr=15.0mA
[RUN  ] set=24.2 setR=24.2 RPM=26.1 duty= 774 (18.9%) adc= 755 curr_raw=2255 iset=16.3mA curr=18.7mA
[RUN  ] set=24.2 setR=24.2 RPM=26.6 duty= 766 (18.7%) adc= 754 curr_raw=2500 iset=15.3mA curr=15.6mA
[RUN  ] set=24.2 setR=24.2 RPM=24.1 duty= 697 (17.0%) adc= 754 curr_raw=2250 iset=15.0mA curr=30.4mA
[RUN  ] set=24.1 setR=24.1 RPM=22.6 duty= 660 (16.1%) adc= 753 curr_raw=2284 iset=15.5mA curr=21.3mA
[RUN  ] set=24.2 setR=24.2 RPM=22.1 duty= 649 (15.8%) adc= 754 curr_raw=2207 iset=16.3mA curr=9.2mA
[RUN  ] set=24.2 setR=24.1 RPM=22.6 duty= 667 (16.3%) adc= 754 curr_raw=2228 iset=17.0mA curr=21.0mA
[RUN  ] set=24.1 setR=24.1 RPM=23.6 duty= 700 (17.1%) adc= 753 curr_raw=2257 iset=17.3mA curr=15.2mA
[RUN  ] set=24.1 setR=24.1 RPM=24.6 duty= 716 (17.5%) adc= 752 curr_raw=2274 iset=17.4mA curr=32.2mA
[RUN  ] set=24.1 setR=24.1 RPM=24.6 duty= 730 (17.8%) adc= 753 curr_raw=2365 iset=17.2mA curr=29.9mA
[RUN  ] set=24.1 setR=24.1 RPM=25.1 duty= 727 (17.8%) adc= 753 curr_raw=2244 iset=16.9mA curr=31.8mA
[RUN  ] set=24.2 setR=24.2 RPM=24.6 duty= 718 (17.5%) adc= 755 curr_raw=2222 iset=16.6mA curr=29.1mA
[RUN  ] set=24.1 setR=24.1 RPM=24.6 duty= 724 (17.7%) adc= 752 curr_raw=2273 iset=16.5mA curr=30.0mA
[RUN  ] set=24.1 setR=24.1 RPM=25.6 duty= 750 (18.3%) adc= 752 curr_raw=2243 iset=16.0mA curr=7.4mA
[RUN  ] set=24.2 setR=24.2 RPM=26.1 duty= 756 (18.5%) adc= 755 curr_raw=2289 iset=15.4mA curr=20.0mA
[RUN  ] set=24.1 setR=24.2 RPM=25.1 duty= 737 (18.0%) adc= 753 curr_raw=2259 iset=14.9mA curr=18.1mA
[RUN  ] set=24.2 setR=24.1 RPM=25.1 duty= 720 (17.6%) adc= 754 curr_raw=2201 iset=14.5mA curr=9.3mA
[RUN  ] set=24.1 setR=24.2 RPM=24.1 duty= 703 (17.2%) adc= 753 curr_raw=2244 iset=14.4mA curr=18.2mA
[RUN  ] set=24.1 setR=24.2 RPM=23.6 duty= 701 (17.1%) adc= 753 curr_raw=2260 iset=14.5mA curr=21.1mA
[RUN  ] set=24.1 setR=24.2 RPM=23.6 duty= 699 (17.1%) adc= 752 curr_raw=2373 iset=14.6mA curr=7.6mA
[RUN  ] set=24.1 setR=24.1 RPM=24.1 duty= 709 (17.3%) adc= 752 curr_raw=2258 iset=14.7mA curr=10.1mA
[RUN  ] set=24.1 setR=24.1 RPM=23.6 duty= 693 (16.9%) adc= 753 curr_raw=2209 iset=14.9mA curr=15.4mA
[RUN  ] set=24.1 setR=24.1 RPM=24.1 duty= 699 (17.1%) adc= 750 curr_raw=2213 iset=14.9mA curr=17.8mA
[RUN  ] set=24.2 setR=24.2 RPM=24.1 duty= 715 (17.5%) adc= 754 curr_raw=2251 iset=14.9mA curr=7.9mA
[RUN  ] set=24.2 setR=24.2 RPM=24.6 duty= 716 (17.5%) adc= 754 curr_raw=2237 iset=14.8mA curr=20.9mA
[RUN  ] set=24.1 setR=24.2 RPM=24.1 duty= 723 (17.7%) adc= 753 curr_raw=2305 iset=14.7mA curr=10.3mA
[RUN  ] set=24.1 setR=24.1 RPM=24.1 duty= 715 (17.5%) adc= 753 curr_raw=2267 iset=14.7mA curr=11.9mA
[RUN  ] set=24.1 setR=24.1 RPM=24.1 duty= 698 (17.0%) adc= 752 curr_raw=2206 iset=14.7mA curr=24.9mA
[RUN  ] set=24.1 setR=24.2 RPM=24.1 duty= 716 (17.5%) adc= 753 curr_raw=2249 iset=14.7mA curr=10.7mA
[RUN  ] set=24.1 setR=24.2 RPM=24.1 duty= 709 (17.3%) adc= 752 curr_raw=2337 iset=14.6mA curr=9.3mA
[RUN  ] set=24.1 setR=24.2 RPM=24.6 duty= 705 (17.2%) adc= 753 curr_raw=2361 iset=14.6mA curr=17.6mA
[RUN  ] set=24.1 setR=24.1 RPM=24.1 duty= 705 (17.2%) adc= 753 curr_raw=2282 iset=14.7mA curr=17.8mA
[RUN  ] set=24.1 setR=24.1 RPM=24.1 duty= 706 (17.2%) adc= 753 curr_raw=2241 iset=14.7mA curr=20.1mA
[RUN  ] set=24.1 setR=24.1 RPM=24.1 duty= 706 (17.2%) adc= 750 curr_raw=2227 iset=14.7mA curr=14.2mA
[RUN  ] set=24.2 setR=24.1 RPM=24.1 duty= 694 (16.9%) adc= 754 curr_raw=2201 iset=14.8mA curr=20.6mA
[RUN  ] set=24.2 setR=24.2 RPM=23.6 duty= 709 (17.3%) adc= 754 curr_raw=2234 iset=14.8mA curr=14.2mA
[RUN  ] set=24.1 setR=24.2 RPM=24.1 duty= 709 (17.3%) adc= 753 curr_raw=2248 iset=14.8mA curr=6.6mA
[RUN  ] set=24.2 setR=24.2 RPM=24.1 duty= 707 (17.3%) adc= 754 curr_raw=2274 iset=14.8mA curr=19.1mA
[RUN  ] set=24.2 setR=24.1 RPM=24.6 duty= 701 (17.1%) adc= 754 curr_raw=2215 iset=14.7mA curr=17.6mA
[RUN  ] set=24.2 setR=24.2 RPM=23.6 duty= 701 (17.1%) adc= 754 curr_raw=2226 iset=14.8mA curr=15.1mA
[RUN  ] set=24.2 setR=24.2 RPM=24.1 duty= 717 (17.5%) adc= 754 curr_raw=2275 iset=14.8mA curr=8.9mA
[RUN  ] set=24.1 setR=24.1 RPM=24.1 duty= 710 (17.3%) adc= 753 curr_raw=2288 iset=14.7mA curr=10.9mA
[RUN  ] set=24.2 setR=24.2 RPM=24.1 duty= 704 (17.2%) adc= 754 curr_raw=2248 iset=14.7mA curr=29.7mA
[RUN  ] set=24.2 setR=24.2 RPM=23.6 duty= 705 (17.2%) adc= 754 curr_raw=2239 iset=14.8mA curr=16.0mA

从0-100-0
[IDLE ] set=0.0 setR=0.0 RPM=0.0 duty=   0 (0.0%) adc=  20 curr_raw=1779 iset=0.0mA curr=0.0mA
[IDLE ] set=0.0 setR=0.0 RPM=0.0 duty=   0 (0.0%) adc=  20 curr_raw=1712 iset=0.0mA curr=0.0mA
[IDLE ] set=0.0 setR=0.0 RPM=0.0 duty=   0 (0.0%) adc=  21 curr_raw=1676 iset=0.0mA curr=0.0mA
[IDLE ] set=0.0 setR=0.0 RPM=0.0 duty=   0 (0.0%) adc=  19 curr_raw=1670 iset=0.0mA curr=0.0mA
[IDLE ] set=0.0 setR=0.0 RPM=0.0 duty=   0 (0.0%) adc=  17 curr_raw=1669 iset=0.0mA curr=0.0mA
[IDLE ] set=0.0 setR=0.0 RPM=0.0 duty=   0 (0.0%) adc=  27 curr_raw=1664 iset=0.0mA curr=0.0mA
[IDLE ] set=0.0 setR=0.0 RPM=0.0 duty=   0 (0.0%) adc=  26 curr_raw=1543 iset=0.0mA curr=0.0mA
[IDLE ] set=0.0 setR=0.0 RPM=0.0 duty=   0 (0.0%) adc=  25 curr_raw=1542 iset=0.0mA curr=0.0mA
[IDLE ] set=0.0 setR=0.0 RPM=0.0 duty=   0 (0.0%) adc=  25 curr_raw=1678 iset=0.0mA curr=0.0mA
[IDLE ] set=0.0 setR=0.0 RPM=0.0 duty=   0 (0.0%) adc=  25 curr_raw=1667 iset=0.0mA curr=0.0mA
[IDLE ] set=0.0 setR=0.0 RPM=0.0 duty=   0 (0.0%) adc=  73 curr_raw=1779 iset=0.0mA curr=0.0mA
[IDLE ] set=0.0 setR=0.0 RPM=0.0 duty=   0 (0.0%) adc= 139 curr_raw=1652 iset=0.0mA curr=0.0mA
[IDLE ] set=0.0 setR=0.0 RPM=0.0 duty=   0 (0.0%) adc= 190 curr_raw=1671 iset=0.0mA curr=0.0mA
[IDLE ] set=0.0 setR=0.0 RPM=0.0 duty=   0 (0.0%) adc= 238 curr_raw=2016 iset=0.0mA curr=0.0mA
[IDLE ] set=0.0 setR=0.0 RPM=0.0 duty=   0 (0.0%) adc= 276 curr_raw=2017 iset=0.0mA curr=0.0mA
[IDLE ] set=0.0 setR=0.0 RPM=0.0 duty=   0 (0.0%) adc= 316 curr_raw=2016 iset=0.0mA curr=0.0mA
[START] set=15.1 setR=15.1 RPM=0.0 duty=  44 (1.1%) adc= 354 curr_raw=2096 iset=2.6mA curr=0.0mA
[START] set=15.9 setR=15.9 RPM=0.5 duty= 139 (3.4%) adc= 390 curr_raw=2237 iset=8.0mA curr=7.8mA
[START] set=16.3 setR=16.4 RPM=0.0 duty= 155 (3.8%) adc= 409 curr_raw=2271 iset=13.8mA curr=13.1mA
[START] set=16.9 setR=16.9 RPM=0.0 duty= 177 (4.3%) adc= 434 curr_raw=2307 iset=19.8mA curr=19.5mA
[START] set=17.7 setR=17.7 RPM=0.0 duty= 184 (4.5%) adc= 468 curr_raw=2348 iset=25.9mA curr=25.9mA
[START] set=18.2 setR=18.2 RPM=0.0 duty= 202 (4.9%) adc= 491 curr_raw=2387 iset=32.4mA curr=32.1mA
[START] set=18.9 setR=19.0 RPM=0.0 duty= 214 (5.2%) adc= 524 curr_raw=2428 iset=39.0mA curr=38.7mA
[START] set=19.5 setR=19.5 RPM=0.0 duty= 235 (5.7%) adc= 548 curr_raw=2469 iset=45.9mA curr=45.7mA
[START] set=19.8 setR=19.8 RPM=0.0 duty= 258 (6.3%) adc= 560 curr_raw=2516 iset=53.0mA curr=52.6mA
[START] set=20.0 setR=20.0 RPM=0.0 duty= 280 (6.8%) adc= 569 curr_raw=2556 iset=60.0mA curr=59.8mA
[START] set=20.6 setR=20.6 RPM=0.0 duty= 303 (7.4%) adc= 598 curr_raw=2606 iset=67.4mA curr=67.0mA
[RUN  ] set=20.8 setR=20.8 RPM=14.5 duty= 431 (10.5%) adc= 607 curr_raw=2207 iset=29.4mA curr=9.0mA
[RUN  ] set=21.1 setR=21.1 RPM=30.6 duty= 913 (22.3%) adc= 618 curr_raw=2275 iset=28.0mA curr=20.5mA
[RUN  ] set=21.9 setR=21.9 RPM=37.1 duty=1038 (25.3%) adc= 654 curr_raw=2416 iset=22.9mA curr=20.8mA
[RUN  ] set=22.4 setR=22.4 RPM=37.1 duty= 997 (24.3%) adc= 675 curr_raw=2260 iset=17.6mA curr=12.1mA
[RUN  ] set=22.7 setR=22.7 RPM=32.1 duty= 859 (21.0%) adc= 688 curr_raw=2202 iset=13.6mA curr=18.6mA
[RUN  ] set=22.7 setR=22.7 RPM=25.6 duty= 717 (17.5%) adc= 689 curr_raw=2214 iset=12.0mA curr=9.9mA
[RUN  ] set=22.9 setR=22.9 RPM=21.1 duty= 615 (15.0%) adc= 697 curr_raw=2254 iset=12.2mA curr=28.6mA
[RUN  ] set=23.4 setR=23.4 RPM=19.1 duty= 576 (14.1%) adc= 722 curr_raw=2419 iset=13.5mA curr=24.7mA
[RUN  ] set=23.7 setR=23.8 RPM=19.1 duty= 592 (14.5%) adc= 735 curr_raw=2270 iset=15.2mA curr=8.3mA
[RUN  ] set=24.3 setR=24.3 RPM=20.6 duty= 627 (15.3%) adc= 760 curr_raw=2251 iset=16.8mA curr=8.0mA
[RUN  ] set=24.9 setR=24.8 RPM=22.6 duty= 683 (16.7%) adc= 784 curr_raw=2333 iset=17.7mA curr=11.2mA
[RUN  ] set=25.4 setR=25.4 RPM=25.1 duty= 736 (18.0%) adc= 808 curr_raw=2281 iset=18.0mA curr=32.2mA
[RUN  ] set=25.8 setR=25.8 RPM=26.6 duty= 777 (19.0%) adc= 827 curr_raw=2260 iset=17.8mA curr=12.0mA
[RUN  ] set=26.5 setR=26.5 RPM=27.6 duty= 775 (18.9%) adc= 857 curr_raw=2216 iset=17.4mA curr=20.3mA
[RUN  ] set=26.9 setR=26.9 RPM=27.6 duty= 801 (19.6%) adc= 875 curr_raw=2421 iset=17.2mA curr=14.1mA
[RUN  ] set=27.2 setR=27.2 RPM=27.6 duty= 796 (19.4%) adc= 888 curr_raw=2249 iset=17.0mA curr=17.0mA
[RUN  ] set=27.9 setR=27.9 RPM=27.6 duty= 794 (19.4%) adc= 918 curr_raw=2420 iset=17.0mA curr=12.3mA
[RUN  ] set=28.1 setR=28.1 RPM=27.6 duty= 792 (19.3%) adc= 926 curr_raw=2222 iset=17.0mA curr=30.9mA
[RUN  ] set=28.3 setR=28.4 RPM=28.1 duty= 796 (19.4%) adc= 938 curr_raw=2258 iset=17.1mA curr=20.9mA
[RUN  ] set=28.9 setR=28.9 RPM=28.1 duty= 809 (19.8%) adc= 962 curr_raw=2228 iset=17.3mA curr=20.5mA
[RUN  ] set=29.2 setR=29.2 RPM=28.6 duty= 819 (20.0%) adc= 977 curr_raw=2406 iset=17.5mA curr=11.7mA
[RUN  ] set=29.5 setR=29.5 RPM=29.1 duty= 832 (20.3%) adc= 987 curr_raw=2426 iset=17.6mA curr=28.6mA
[RUN  ] set=30.0 setR=30.0 RPM=30.1 duty= 849 (20.7%) adc=1012 curr_raw=2254 iset=17.7mA curr=13.0mA
[RUN  ] set=30.5 setR=30.4 RPM=30.6 duty= 861 (21.0%) adc=1032 curr_raw=2480 iset=17.8mA curr=26.8mA
[RUN  ] set=30.8 setR=30.8 RPM=31.1 duty= 864 (21.1%) adc=1047 curr_raw=2412 iset=17.8mA curr=12.1mA
[RUN  ] set=31.2 setR=31.1 RPM=30.6 duty= 876 (21.4%) adc=1063 curr_raw=2272 iset=17.9mA curr=20.2mA
[RUN  ] set=31.8 setR=31.7 RPM=31.6 duty= 880 (21.5%) adc=1088 curr_raw=2534 iset=18.0mA curr=19.0mA
[RUN  ] set=32.5 setR=32.5 RPM=31.6 duty= 894 (21.8%) adc=1121 curr_raw=2344 iset=18.3mA curr=29.3mA
[RUN  ] set=32.9 setR=32.8 RPM=32.6 duty= 911 (22.2%) adc=1137 curr_raw=2461 iset=18.5mA curr=15.7mA
[RUN  ] set=33.5 setR=33.5 RPM=34.6 duty= 976 (23.8%) adc=1163 curr_raw=2218 iset=18.3mA curr=12.1mA
[RUN  ] set=33.9 setR=33.8 RPM=36.6 duty=1022 (25.0%) adc=1181 curr_raw=2372 iset=17.4mA curr=16.7mA
[RUN  ] set=34.7 setR=34.7 RPM=37.1 duty=1021 (24.9%) adc=1216 curr_raw=2530 iset=16.4mA curr=15.3mA
[RUN  ] set=35.4 setR=35.4 RPM=37.1 duty=1006 (24.6%) adc=1247 curr_raw=2213 iset=15.8mA curr=16.2mA
[RUN  ] set=36.1 setR=36.0 RPM=35.6 duty= 984 (24.0%) adc=1278 curr_raw=2260 iset=15.8mA curr=20.5mA
[RUN  ] set=37.2 setR=37.3 RPM=35.6 duty= 985 (24.1%) adc=1330 curr_raw=2210 iset=16.2mA curr=9.4mA
[RUN  ] set=38.3 setR=38.3 RPM=36.1 duty= 985 (24.1%) adc=1378 curr_raw=2210 iset=16.9mA curr=21.8mA
[RUN  ] set=39.3 setR=39.3 RPM=37.1 duty=1029 (25.1%) adc=1419 curr_raw=2309 iset=17.7mA curr=17.6mA
[RUN  ] set=40.4 setR=40.4 RPM=38.6 duty=1057 (25.8%) adc=1468 curr_raw=2239 iset=18.3mA curr=11.0mA
[RUN  ] set=41.8 setR=41.8 RPM=40.1 duty=1094 (26.7%) adc=1530 curr_raw=2245 iset=18.8mA curr=11.6mA
[RUN  ] set=42.9 setR=42.8 RPM=41.6 duty=1146 (28.0%) adc=1579 curr_raw=2388 iset=19.2mA curr=19.2mA
[RUN  ] set=44.8 setR=44.7 RPM=43.1 duty=1156 (28.2%) adc=1662 curr_raw=2369 iset=19.5mA curr=22.7mA
[RUN  ] set=46.2 setR=46.2 RPM=44.6 duty=1202 (29.4%) adc=1723 curr_raw=2461 iset=20.1mA curr=28.6mA
[RUN  ] set=47.1 setR=47.1 RPM=46.1 duty=1243 (30.4%) adc=1765 curr_raw=2202 iset=20.5mA curr=11.6mA
[RUN  ] set=48.7 setR=48.6 RPM=48.1 duty=1282 (31.3%) adc=1836 curr_raw=2317 iset=20.7mA curr=23.4mA
[RUN  ] set=50.5 setR=50.5 RPM=49.1 duty=1321 (32.3%) adc=1914 curr_raw=2450 iset=21.1mA curr=21.3mA
[RUN  ] set=53.0 setR=53.0 RPM=50.1 duty=1349 (32.9%) adc=2026 curr_raw=2365 iset=21.9mA curr=32.5mA
[RUN  ] set=55.4 setR=55.4 RPM=52.1 duty=1387 (33.9%) adc=2130 curr_raw=2263 iset=23.0mA curr=27.9mA
[RUN  ] set=60.1 setR=60.1 RPM=55.6 duty=1498 (36.6%) adc=2339 curr_raw=2230 iset=24.6mA curr=11.3mA
[RUN  ] set=64.3 setR=64.1 RPM=62.2 duty=1626 (39.7%) adc=2520 curr_raw=2233 iset=25.3mA curr=23.4mA
[RUN  ] set=67.5 setR=67.5 RPM=62.7 duty=1642 (40.1%) adc=2665 curr_raw=2423 iset=26.8mA curr=25.1mA
[RUN  ] set=70.0 setR=69.9 RPM=63.2 duty=1644 (40.1%) adc=2773 curr_raw=2178 iset=29.0mA curr=15.0mA
[RUN  ] set=71.4 setR=71.4 RPM=63.7 duty=1653 (40.4%) adc=2834 curr_raw=2264 iset=31.6mA curr=24.7mA
[RUN  ] set=73.3 setR=73.3 RPM=64.2 duty=1666 (40.7%) adc=2917 curr_raw=2189 iset=34.7mA curr=23.6mA
[RUN  ] set=75.9 setR=75.9 RPM=65.2 duty=1685 (41.1%) adc=3034 curr_raw=2189 iset=38.4mA curr=23.1mA
[RUN  ] set=77.5 setR=77.5 RPM=65.7 duty=1703 (41.6%) adc=3105 curr_raw=2209 iset=42.5mA curr=12.4mA
[RUN  ] set=80.4 setR=80.4 RPM=66.7 duty=1738 (42.4%) adc=3232 curr_raw=2676 iset=47.1mA curr=21.0mA
[RUN  ] set=83.2 setR=83.1 RPM=67.7 duty=1749 (42.7%) adc=3353 curr_raw=2210 iset=52.4mA curr=27.1mA
[RUN  ] set=85.2 setR=85.2 RPM=68.7 duty=1787 (43.6%) adc=3442 curr_raw=2611 iset=58.2mA curr=23.0mA
[RUN  ] set=87.9 setR=87.8 RPM=70.2 duty=1819 (44.4%) adc=3560 curr_raw=2588 iset=64.4mA curr=22.7mA
[RUN  ] set=89.9 setR=89.9 RPM=71.2 duty=1833 (44.8%) adc=3651 curr_raw=2193 iset=70.9mA curr=22.3mA
[RUN  ] set=92.1 setR=92.1 RPM=72.2 duty=1874 (45.8%) adc=3749 curr_raw=2304 iset=77.7mA curr=22.9mA
[RUN  ] set=94.6 setR=94.6 RPM=74.2 duty=1909 (46.6%) adc=3858 curr_raw=2209 iset=84.9mA curr=18.9mA
[RUN  ] set=97.3 setR=97.3 RPM=75.2 duty=1952 (47.7%) adc=3978 curr_raw=2478 iset=92.6mA curr=25.9mA
[RUN  ] set=100.0 setR=100.0 RPM=77.7 duty=1994 (48.7%) adc=4093 curr_raw=2213 iset=100.4mA curr=21.4mA
[RUN  ] set=100.0 setR=100.0 RPM=78.7 duty=2034 (49.7%) adc=4094 curr_raw=2204 iset=108.1mA curr=21.0mA
[RUN  ] set=100.0 setR=100.0 RPM=80.2 duty=2057 (50.2%) adc=4093 curr_raw=2191 iset=115.2mA curr=24.4mA
[RUN  ] set=100.0 setR=100.0 RPM=82.2 duty=2109 (51.5%) adc=4094 curr_raw=2714 iset=121.7mA curr=25.2mA
[RUN  ] set=100.0 setR=100.0 RPM=82.7 duty=2117 (51.7%) adc=4094 curr_raw=2543 iset=128.0mA curr=29.5mA
[RUN  ] set=100.0 setR=100.0 RPM=83.7 duty=2141 (52.3%) adc=4094 curr_raw=2304 iset=133.9mA curr=27.0mA
[RUN  ] set=100.0 setR=100.0 RPM=85.2 duty=2166 (52.9%) adc=4094 curr_raw=2302 iset=139.2mA curr=27.8mA
[RUN  ] set=100.0 setR=100.0 RPM=86.2 duty=2203 (53.8%) adc=4093 curr_raw=2657 iset=144.3mA curr=23.8mA
[RUN  ] set=100.0 setR=100.0 RPM=86.7 duty=2220 (54.2%) adc=4094 curr_raw=2346 iset=149.0mA curr=25.9mA
[RUN  ] set=100.0 setR=100.0 RPM=87.7 duty=2243 (54.8%) adc=4094 curr_raw=2176 iset=153.4mA curr=21.9mA
[RUN  ] set=100.0 setR=100.0 RPM=89.2 duty=2262 (55.2%) adc=4094 curr_raw=2316 iset=157.3mA curr=25.4mA
[RUN  ] set=100.0 setR=100.0 RPM=89.7 duty=2282 (55.7%) adc=4094 curr_raw=2221 iset=161.1mA curr=24.6mA
[RUN  ] set=100.0 setR=100.0 RPM=90.2 duty=2293 (56.0%) adc=4094 curr_raw=2209 iset=164.7mA curr=21.3mA
[RUN  ] set=100.0 setR=100.0 RPM=91.2 duty=2308 (56.4%) adc=4094 curr_raw=2177 iset=167.8mA curr=12.2mA
[RUN  ] set=99.8 setR=99.8 RPM=91.7 duty=2333 (57.0%) adc=4088 curr_raw=2183 iset=170.9mA curr=20.8mA
[RUN  ] set=100.0 setR=100.0 RPM=92.7 duty=2342 (57.2%) adc=4094 curr_raw=2511 iset=173.7mA curr=18.5mA
[RUN  ] set=100.0 setR=100.0 RPM=92.7 duty=2369 (57.9%) adc=4094 curr_raw=2171 iset=176.2mA curr=22.3mA
[RUN  ] set=100.0 setR=100.0 RPM=93.8 duty=2380 (58.1%) adc=4094 curr_raw=2174 iset=178.4mA curr=22.2mA
[RUN  ] set=100.0 setR=100.0 RPM=94.3 duty=2388 (58.3%) adc=4094 curr_raw=2395 iset=180.4mA curr=26.5mA
[RUN  ] set=100.0 setR=100.0 RPM=94.8 duty=2399 (58.6%) adc=4094 curr_raw=2143 iset=182.2mA curr=17.2mA
[RUN  ] set=100.0 setR=100.0 RPM=95.3 duty=2416 (59.0%) adc=4094 curr_raw=2201 iset=184.0mA curr=12.7mA
[RUN  ] set=100.0 setR=100.0 RPM=95.3 duty=2410 (58.9%) adc=4094 curr_raw=2258 iset=185.8mA curr=24.6mA
[RUN  ] set=100.0 setR=100.0 RPM=95.8 duty=2426 (59.2%) adc=4093 curr_raw=2377 iset=187.3mA curr=21.9mA
[RUN  ] set=100.0 setR=100.0 RPM=95.8 duty=2440 (59.6%) adc=4094 curr_raw=2273 iset=188.8mA curr=18.2mA
[RUN  ] set=100.0 setR=100.0 RPM=96.3 duty=2437 (59.5%) adc=4093 curr_raw=2156 iset=190.2mA curr=20.9mA
[RUN  ] set=100.0 setR=100.0 RPM=96.8 duty=2440 (59.6%) adc=4094 curr_raw=2229 iset=191.4mA curr=10.6mA
[RUN  ] set=100.0 setR=100.0 RPM=97.3 duty=2436 (59.5%) adc=4094 curr_raw=2434 iset=192.5mA curr=37.4mA
[RUN  ] set=100.0 setR=100.0 RPM=97.3 duty=2434 (59.4%) adc=4094 curr_raw=2696 iset=193.6mA curr=27.2mA
[RUN  ] set=100.0 setR=100.0 RPM=96.8 duty=2440 (59.6%) adc=4094 curr_raw=2265 iset=194.7mA curr=29.7mA
[RUN  ] set=100.0 setR=100.0 RPM=96.8 duty=2450 (59.8%) adc=4094 curr_raw=2412 iset=195.8mA curr=27.6mA
[RUN  ] set=100.0 setR=100.0 RPM=96.8 duty=2450 (59.8%) adc=4094 curr_raw=2277 iset=196.8mA curr=14.6mA
[RUN  ] set=100.0 setR=100.0 RPM=97.3 duty=2447 (59.8%) adc=4094 curr_raw=2370 iset=197.7mA curr=28.3mA
[RUN  ] set=100.0 setR=100.0 RPM=97.3 duty=2462 (60.1%) adc=4093 curr_raw=2287 iset=198.6mA curr=26.0mA
[RUN  ] set=100.0 setR=100.0 RPM=97.3 duty=2456 (60.0%) adc=4094 curr_raw=2230 iset=199.5mA curr=17.9mA
[RUN  ] set=99.9 setR=99.9 RPM=97.8 duty=2466 (60.2%) adc=4092 curr_raw=2451 iset=200.2mA curr=30.1mA
[RUN  ] set=100.0 setR=100.0 RPM=97.8 duty=2470 (60.3%) adc=4094 curr_raw=2274 iset=200.9mA curr=25.5mA
[RUN  ] set=100.0 setR=100.0 RPM=98.3 duty=2474 (60.4%) adc=4093 curr_raw=2704 iset=201.5mA curr=31.1mA
[RUN  ] set=100.0 setR=100.0 RPM=98.3 duty=2476 (60.5%) adc=4094 curr_raw=2316 iset=202.2mA curr=26.1mA
[RUN  ] set=100.0 setR=100.0 RPM=97.3 duty=2475 (60.4%) adc=4094 curr_raw=2258 iset=203.0mA curr=19.4mA
[RUN  ] set=100.0 setR=100.0 RPM=97.8 duty=2466 (60.2%) adc=4094 curr_raw=2167 iset=203.6mA curr=22.0mA
[RUN  ] set=100.0 setR=100.0 RPM=97.8 duty=2468 (60.3%) adc=4094 curr_raw=2201 iset=204.3mA curr=30.0mA
[RUN  ] set=99.9 setR=99.9 RPM=97.8 duty=2466 (60.2%) adc=4092 curr_raw=2249 iset=205.0mA curr=31.7mA
[RUN  ] set=100.0 setR=100.0 RPM=98.3 duty=2474 (60.4%) adc=4094 curr_raw=2283 iset=205.6mA curr=31.9mA
[RUN  ] set=100.0 setR=100.0 RPM=97.8 duty=2469 (60.3%) adc=4094 curr_raw=2403 iset=206.3mA curr=36.2mA
[RUN  ] set=100.0 setR=100.0 RPM=99.3 duty=2488 (60.8%) adc=4094 curr_raw=2254 iset=206.7mA curr=20.8mA
[RUN  ] set=100.0 setR=100.0 RPM=99.3 duty=2479 (60.5%) adc=4094 curr_raw=2227 iset=207.1mA curr=20.7mA
[RUN  ] set=100.0 setR=100.0 RPM=98.8 duty=2492 (60.9%) adc=4094 curr_raw=2297 iset=207.6mA curr=23.7mA
[RUN  ] set=100.0 setR=100.0 RPM=98.3 duty=2494 (60.9%) adc=4094 curr_raw=2259 iset=208.2mA curr=19.9mA
[RUN  ] set=100.0 setR=100.0 RPM=98.8 duty=2496 (61.0%) adc=4094 curr_raw=2307 iset=208.5mA curr=19.6mA
[RUN  ] set=100.0 setR=100.0 RPM=98.8 duty=2488 (60.8%) adc=4094 curr_raw=2512 iset=208.9mA curr=23.4mA
[RUN  ] set=100.0 setR=100.0 RPM=99.3 duty=2484 (60.7%) adc=4094 curr_raw=2329 iset=209.1mA curr=27.1mA
[RUN  ] set=100.0 setR=100.0 RPM=99.3 duty=2491 (60.8%) adc=4093 curr_raw=2234 iset=209.4mA curr=21.2mA
[RUN  ] set=100.0 setR=100.0 RPM=99.3 duty=2491 (60.8%) adc=4094 curr_raw=2775 iset=209.6mA curr=31.5mA
[RUN  ] set=100.0 setR=100.0 RPM=99.3 duty=2496 (61.0%) adc=4094 curr_raw=2760 iset=210.0mA curr=30.2mA
[RUN  ] set=100.0 setR=100.0 RPM=99.8 duty=2501 (61.1%) adc=4094 curr_raw=2238 iset=210.2mA curr=29.9mA
[RUN  ] set=100.0 setR=100.0 RPM=99.3 duty=2493 (60.9%) adc=4094 curr_raw=2212 iset=210.4mA curr=31.8mA
[RUN  ] set=100.0 setR=100.0 RPM=99.8 duty=2502 (61.1%) adc=4094 curr_raw=2523 iset=210.5mA curr=42.3mA
[RUN  ] set=100.0 setR=100.0 RPM=99.3 duty=2505 (61.2%) adc=4094 curr_raw=2425 iset=210.6mA curr=29.5mA
[RUN  ] set=100.0 setR=100.0 RPM=99.8 duty=2494 (60.9%) adc=4093 curr_raw=2539 iset=210.7mA curr=33.8mA
[RUN  ] set=100.0 setR=100.0 RPM=99.8 duty=2498 (61.0%) adc=4094 curr_raw=2463 iset=210.8mA curr=23.6mA
[RUN  ] set=100.0 setR=100.0 RPM=99.8 duty=2516 (61.4%) adc=4094 curr_raw=2549 iset=210.9mA curr=25.0mA
[RUN  ] set=100.0 setR=100.0 RPM=99.8 duty=2508 (61.2%) adc=4094 curr_raw=2259 iset=211.0mA curr=25.7mA
[RUN  ] set=100.0 setR=100.0 RPM=100.3 duty=2509 (61.3%) adc=4094 curr_raw=2323 iset=211.0mA curr=25.7mA
[RUN  ] set=100.0 setR=100.0 RPM=99.8 duty=2500 (61.1%) adc=4094 curr_raw=2231 iset=211.2mA curr=26.4mA
[RUN  ] set=100.0 setR=100.0 RPM=99.8 duty=2499 (61.0%) adc=4094 curr_raw=2214 iset=211.3mA curr=27.1mA
[RUN  ] set=100.0 setR=100.0 RPM=99.8 duty=2504 (61.1%) adc=4094 curr_raw=2226 iset=211.3mA curr=19.3mA
[RUN  ] set=100.0 setR=100.0 RPM=99.3 duty=2522 (61.6%) adc=4094 curr_raw=2575 iset=211.4mA curr=26.6mA
[RUN  ] set=100.0 setR=100.0 RPM=99.8 duty=2503 (61.1%) adc=4094 curr_raw=2392 iset=211.4mA curr=32.1mA
[RUN  ] set=100.0 setR=100.0 RPM=99.8 duty=2496 (61.0%) adc=4094 curr_raw=2899 iset=211.5mA curr=38.8mA
[RUN  ] set=100.0 setR=100.0 RPM=99.8 duty=2498 (61.0%) adc=4093 curr_raw=2469 iset=211.6mA curr=31.5mA
[RUN  ] set=100.0 setR=100.0 RPM=99.8 duty=2510 (61.3%) adc=4094 curr_raw=2438 iset=211.6mA curr=32.0mA
[RUN  ] set=100.0 setR=100.0 RPM=99.8 duty=2508 (61.2%) adc=4094 curr_raw=2394 iset=211.7mA curr=24.2mA
[RUN  ] set=100.0 setR=100.0 RPM=100.3 duty=2506 (61.2%) adc=4094 curr_raw=2314 iset=211.7mA curr=32.2mA
[RUN  ] set=100.0 setR=100.0 RPM=100.3 duty=2511 (61.3%) adc=4094 curr_raw=2197 iset=211.7mA curr=29.7mA
[RUN  ] set=100.0 setR=100.0 RPM=99.8 duty=2500 (61.1%) adc=4094 curr_raw=2232 iset=211.8mA curr=31.7mA
[RUN  ] set=100.0 setR=100.0 RPM=99.8 duty=2516 (61.4%) adc=4094 curr_raw=2300 iset=211.8mA curr=28.3mA
[RUN  ] set=100.0 setR=100.0 RPM=100.3 duty=2507 (61.2%) adc=4094 curr_raw=2494 iset=211.8mA curr=22.5mA
[RUN  ] set=100.0 setR=100.0 RPM=99.8 duty=2499 (61.0%) adc=4094 curr_raw=2439 iset=211.8mA curr=36.2mA
[RUN  ] set=94.8 setR=94.8 RPM=99.3 duty=2504 (61.1%) adc=3868 curr_raw=2315 iset=211.0mA curr=32.0mA
[RUN  ] set=91.3 setR=91.3 RPM=98.8 duty=2478 (60.5%) adc=3713 curr_raw=2235 iset=208.4mA curr=34.4mA
[RUN  ] set=90.1 setR=90.1 RPM=98.8 duty=2486 (60.7%) adc=3661 curr_raw=2284 iset=205.2mA curr=28.4mA
[RUN  ] set=89.2 setR=89.2 RPM=97.8 duty=2469 (60.3%) adc=3620 curr_raw=2265 iset=202.3mA curr=20.4mA
[RUN  ] set=87.8 setR=87.9 RPM=97.8 duty=2440 (59.6%) adc=3557 curr_raw=2543 iset=198.7mA curr=34.3mA
[RUN  ] set=86.6 setR=86.6 RPM=96.8 duty=2421 (59.1%) adc=3506 curr_raw=2262 iset=195.1mA curr=34.4mA
[RUN  ] set=85.2 setR=85.3 RPM=95.8 duty=2407 (58.8%) adc=3445 curr_raw=2321 iset=191.4mA curr=30.1mA
[RUN  ] set=84.0 setR=84.0 RPM=95.3 duty=2403 (58.7%) adc=3391 curr_raw=2289 iset=187.2mA curr=16.7mA
[RUN  ] set=83.0 setR=82.9 RPM=94.3 duty=2384 (58.2%) adc=3344 curr_raw=2210 iset=183.2mA curr=26.3mA
[RUN  ] set=81.5 setR=81.5 RPM=93.8 duty=2353 (57.5%) adc=3282 curr_raw=2243 iset=178.7mA curr=28.7mA
[RUN  ] set=80.5 setR=80.5 RPM=92.7 duty=2338 (57.1%) adc=3238 curr_raw=2256 iset=174.2mA curr=15.8mA
[RUN  ] set=80.1 setR=80.2 RPM=91.7 duty=2301 (56.2%) adc=3220 curr_raw=2498 iset=170.0mA curr=29.7mA
[RUN  ] set=79.2 setR=79.2 RPM=90.2 duty=2281 (55.7%) adc=3179 curr_raw=2511 iset=165.9mA curr=23.5mA
[RUN  ] set=78.4 setR=78.4 RPM=89.2 duty=2264 (55.3%) adc=3143 curr_raw=2274 iset=162.0mA curr=20.2mA
[RUN  ] set=77.8 setR=77.8 RPM=88.7 duty=2248 (54.9%) adc=3118 curr_raw=2241 iset=157.9mA curr=19.4mA
[RUN  ] set=77.2 setR=77.2 RPM=88.7 duty=2231 (54.5%) adc=3090 curr_raw=2273 iset=153.8mA curr=18.7mA
[RUN  ] set=76.7 setR=76.8 RPM=87.2 duty=2203 (53.8%) adc=3069 curr_raw=2373 iset=150.1mA curr=31.8mA
[RUN  ] set=76.3 setR=76.4 RPM=86.7 duty=2198 (53.7%) adc=3052 curr_raw=2229 iset=146.2mA curr=26.6mA
[RUN  ] set=76.0 setR=75.7 RPM=86.2 duty=2175 (53.1%) adc=3038 curr_raw=2344 iset=142.5mA curr=29.2mA
[RUN  ] set=75.4 setR=75.4 RPM=85.2 duty=2160 (52.7%) adc=3011 curr_raw=2529 iset=138.9mA curr=21.3mA
[RUN  ] set=74.7 setR=74.7 RPM=84.7 duty=2149 (52.5%) adc=2979 curr_raw=2504 iset=135.2mA curr=24.0mA
[RUN  ] set=74.1 setR=74.1 RPM=83.7 duty=2128 (52.0%) adc=2952 curr_raw=2350 iset=131.7mA curr=21.0mA
[RUN  ] set=73.1 setR=73.1 RPM=83.2 duty=2109 (51.5%) adc=2911 curr_raw=2432 iset=128.1mA curr=21.0mA
[RUN  ] set=72.4 setR=72.4 RPM=82.7 duty=2089 (51.0%) adc=2881 curr_raw=2387 iset=124.4mA curr=27.5mA
[RUN  ] set=71.2 setR=71.2 RPM=82.2 duty=2076 (50.7%) adc=2825 curr_raw=2237 iset=120.6mA curr=25.7mA
[RUN  ] set=69.4 setR=69.4 RPM=80.7 duty=2058 (50.3%) adc=2749 curr_raw=2435 iset=116.6mA curr=28.2mA
[RUN  ] set=67.3 setR=67.3 RPM=80.2 duty=2028 (49.5%) adc=2653 curr_raw=2167 iset=112.1mA curr=15.6mA
[RUN  ] set=65.1 setR=65.1 RPM=79.2 duty=2010 (49.1%) adc=2558 curr_raw=2463 iset=107.0mA curr=18.6mA
[RUN  ] set=63.3 setR=63.3 RPM=78.2 duty=1978 (48.3%) adc=2477 curr_raw=2501 iset=101.8mA curr=23.2mA
[RUN  ] set=62.0 setR=62.0 RPM=77.2 duty=1960 (47.9%) adc=2421 curr_raw=2816 iset=96.3mA curr=26.9mA
[RUN  ] set=60.1 setR=60.1 RPM=76.2 duty=1928 (47.1%) adc=2339 curr_raw=2188 iset=90.7mA curr=13.6mA
[RUN  ] set=58.0 setR=58.0 RPM=74.7 duty=1909 (46.6%) adc=2245 curr_raw=2224 iset=84.8mA curr=17.4mA
[RUN  ] set=55.8 setR=55.8 RPM=73.7 duty=1874 (45.8%) adc=2148 curr_raw=2350 iset=78.5mA curr=18.0mA
[RUN  ] set=53.3 setR=53.4 RPM=72.2 duty=1835 (44.8%) adc=2036 curr_raw=2311 iset=72.0mA curr=28.2mA
[RUN  ] set=50.1 setR=50.1 RPM=71.2 duty=1817 (44.4%) adc=1898 curr_raw=2221 iset=64.7mA curr=21.2mA
[RUN  ] set=47.0 setR=47.0 RPM=69.2 duty=1775 (43.3%) adc=1762 curr_raw=2254 iset=56.7mA curr=27.7mA
[RUN  ] set=44.7 setR=44.8 RPM=67.7 duty=1738 (42.4%) adc=1660 curr_raw=2339 iset=48.7mA curr=17.2mA
[RUN  ] set=41.8 setR=41.8 RPM=65.7 duty=1693 (41.3%) adc=1530 curr_raw=2775 iset=40.1mA curr=20.2mA
[RUN  ] set=38.7 setR=38.8 RPM=64.7 duty=1645 (40.2%) adc=1395 curr_raw=2188 iset=31.2mA curr=21.7mA
[RUN  ] set=35.4 setR=35.5 RPM=62.2 duty=1605 (39.2%) adc=1251 curr_raw=2238 iset=21.7mA curr=24.0mA
[RUN  ] set=32.3 setR=32.3 RPM=58.7 duty=1472 (35.9%) adc=1113 curr_raw=2162 iset=11.9mA curr=18.9mA
[RUN  ] set=28.3 setR=28.4 RPM=46.1 duty=1126 (27.5%) adc= 936 curr_raw=2143 iset=4.8mA curr=7.2mA
[RUN  ] set=25.1 setR=25.1 RPM=28.6 duty= 744 (18.2%) adc= 794 curr_raw=2226 iset=2.2mA curr=8.7mA
[RUN  ] set=21.5 setR=21.5 RPM=17.0 duty= 490 (12.0%) adc= 636 curr_raw=2204 iset=3.1mA curr=0.4mA
[RUN  ] set=19.1 setR=19.2 RPM=10.5 duty= 349 (8.5%) adc= 532 curr_raw=2250 iset=5.9mA curr=15.0mA
[RUN  ] set=15.3 setR=15.3 RPM=10.0 duty= 348 (8.5%) adc= 364 curr_raw=2241 iset=8.2mA curr=15.8mA
[RUN  ] set=15.0 setR=15.0 RPM=12.0 duty= 422 (10.3%) adc= 270 curr_raw=2169 iset=9.4mA curr=0.0mA
[RUN  ] set=15.0 setR=15.0 RPM=14.5 duty= 483 (11.8%) adc= 270 curr_raw=2301 iset=9.9mA curr=11.5mA
[RUN  ] set=15.0 setR=15.0 RPM=15.5 duty= 490 (12.0%) adc= 273 curr_raw=2202 iset=9.8mA curr=12.0mA
[IDLE ] set=0.0 setR=0.0 RPM=0.0 duty=   0 (0.0%) adc=  90 curr_raw=2007 iset=0.0mA curr=0.0mA
[IDLE ] set=0.0 setR=0.0 RPM=0.0 duty=   0 (0.0%) adc=  13 curr_raw=2007 iset=0.0mA curr=0.0mA
[IDLE ] set=0.0 setR=0.0 RPM=0.0 duty=   0 (0.0%) adc=  13 curr_raw=2007 iset=0.0mA curr=0.0mA
[IDLE ] set=0.0 setR=0.0 RPM=0.0 duty=   0 (0.0%) adc=  13 curr_raw=2007 iset=0.0mA curr=0.0mA
[IDLE ] set=0.0 setR=0.0 RPM=0.0 duty=   0 (0.0%) adc=  12 curr_raw=2007 iset=0.0mA curr=0.0mA
[IDLE ] set=0.0 setR=0.0 RPM=0.0 duty=   0 (0.0%) adc=  13 curr_raw=2010 iset=0.0mA curr=0.0mA
[IDLE ] set=0.0 setR=0.0 RPM=0.0 duty=   0 (0.0%) adc=  13 curr_raw=2003 iset=0.0mA curr=0.0mA
[IDLE ] set=0.0 setR=0.0 RPM=0.0 duty=   0 (0.0%) adc=  13 curr_raw=2002 iset=0.0mA curr=0.0mA
[IDLE ] set=0.0 setR=0.0 RPM=0.0 duty=   0 (0.0%) adc=  10 curr_raw=1997 iset=0.0mA curr=0.0mA
[IDLE ] set=0.0 setR=0.0 RPM=0.0 duty=   0 (0.0%) adc=   9 curr_raw=1996 iset=0.0mA curr=0.0mA
[IDLE ] set=0.0 setR=0.0 RPM=0.0 duty=   0 (0.0%) adc=   9 curr_raw=1987 iset=0.0mA curr=0.0mA
[IDLE ] set=0.0 setR=0.0 RPM=0.0 duty=   0 (0.0%) adc=  10 curr_raw=1991 iset=0.0mA curr=0.0mA
[IDLE ] set=0.0 setR=0.0 RPM=0.0 duty=   0 (0.0%) adc=  13 curr_raw=1993 iset=0.0mA curr=0.0mA
[IDLE ] set=0.0 setR=0.0 RPM=0.0 duty=   0 (0.0%) adc=  12 curr_raw=1993 iset=0.0mA curr=0.0mA
[IDLE ] set=0.0 setR=0.0 RPM=0.0 duty=   0 (0.0%) adc=  12 curr_raw=1993 iset=0.0mA curr=0.0mA
[IDLE ] set=0.0 setR=0.0 RPM=0.0 duty=   0 (0.0%) adc=  13 curr_raw=1993 iset=0.0mA curr=0.0mA
[IDLE ] set=0.0 setR=0.0 RPM=0.0 duty=   0 (0.0%) adc=  12 curr_raw=1993 iset=0.0mA curr=0.0mA
[IDLE ] set=0.0 setR=0.0 RPM=0.0 duty=   0 (0.0%) adc=  12 curr_raw=1992 iset=0.0mA curr=0.0mA
[IDLE ] set=0.0 setR=0.0 RPM=0.0 duty=   0 (0.0%) adc=  15 curr_raw=1946 iset=0.0mA curr=0.0mA
[IDLE ] set=0.0 setR=0.0 RPM=0.0 duty=   0 (0.0%) adc=  13 curr_raw=1997 iset=0.0mA curr=0.0mA
[IDLE ] set=0.0 setR=0.0 RPM=0.0 duty=   0 (0.0%) adc= 211 curr_raw=1986 iset=0.0mA curr=0.0mA
[START] set=15.6 setR=15.6 RPM=0.0 duty=  55 (1.3%) adc= 377 curr_raw=2089 iset=2.8mA curr=0.0mA
[START] set=16.7 setR=16.6 RPM=0.0 duty= 151 (3.7%) adc= 424 curr_raw=2239 iset=8.7mA curr=8.3mA
[START] set=17.7 setR=17.7 RPM=0.0 duty= 164 (4.0%) adc= 471 curr_raw=2281 iset=14.9mA curr=14.9mA
[START] set=19.5 setR=19.5 RPM=0.0 duty= 181 (4.4%) adc= 548 curr_raw=2321 iset=21.6mA curr=21.3mA
[START] set=22.0 setR=22.0 RPM=0.0 duty= 200 (4.9%) adc= 660 curr_raw=2366 iset=29.2mA curr=28.9mA
[START] set=24.3 setR=24.2 RPM=0.0 duty= 224 (5.5%) adc= 759 curr_raw=2417 iset=37.5mA curr=37.3mA
[START] set=26.8 setR=26.7 RPM=0.0 duty= 253 (6.2%) adc= 869 curr_raw=2475 iset=46.9mA curr=46.5mA
[START] set=29.9 setR=29.9 RPM=0.0 duty= 286 (7.0%) adc=1006 curr_raw=2543 iset=57.3mA curr=56.6mA
[START] set=34.6 setR=34.5 RPM=0.0 duty= 329 (8.0%) adc=1212 curr_raw=2602 iset=68.9mA curr=67.5mA
[RUN  ] set=39.5 setR=39.5 RPM=23.1 duty= 818 (20.0%) adc=1431 curr_raw=2404 iset=34.7mA curr=25.7mA
[RUN  ] set=45.8 setR=45.7 RPM=51.6 duty=1530 (37.4%) adc=1705 curr_raw=2518 iset=34.5mA curr=29.9mA
[RUN  ] set=53.0 setR=52.9 RPM=63.7 duty=1641 (40.1%) adc=2024 curr_raw=2297 iset=29.8mA curr=26.2mA
[RUN  ] set=60.1 setR=60.1 RPM=63.2 duty=1640 (40.0%) adc=2335 curr_raw=2528 iset=27.9mA curr=23.5mA
[RUN  ] set=65.8 setR=65.8 RPM=63.2 duty=1648 (40.2%) adc=2589 curr_raw=2349 iset=28.3mA curr=20.3mA
[RUN  ] set=61.2 setR=61.4 RPM=63.7 duty=1652 (40.3%) adc=2387 curr_raw=2688 iset=28.3mA curr=22.5mA
[RUN  ] set=38.9 setR=39.7 RPM=62.7 duty=1624 (39.7%) adc=1402 curr_raw=2224 iset=22.4mA curr=11.2mA
[RUN  ] set=21.6 setR=21.8 RPM=60.7 duty=1510 (36.9%) adc= 640 curr_raw=2265 iset=9.4mA curr=14.8mA
[STOP ] set=0.0 setR=0.0 RPM=33.6 duty= 634 (15.5%) adc=  19 curr_raw=1978 iset=0.0mA curr=0.0mA
[IDLE ] set=0.0 setR=0.0 RPM=0.0 duty=   0 (0.0%) adc=  17 curr_raw=1983 iset=0.0mA curr=0.0mA
[IDLE ] set=0.0 setR=0.0 RPM=0.0 duty=   0 (0.0%) adc=  16 curr_raw=1981 iset=0.0mA curr=0.0mA
[IDLE ] set=0.0 setR=0.0 RPM=0.0 duty=   0 (0.0%) adc=  16 curr_raw=1985 iset=0.0mA curr=0.0mA
[IDLE ] set=0.0 setR=0.0 RPM=0.0 duty=   0 (0.0%) adc=  16 curr_raw=1986 iset=0.0mA curr=0.0mA
[IDLE ] set=0.0 setR=0.0 RPM=0.0 duty=   0 (0.0%) adc=  17 curr_raw=1987 iset=0.0mA curr=0.0mA
[IDLE ] set=0.0 setR=0.0 RPM=0.0 duty=   0 (0.0%) adc=  16 curr_raw=1985 iset=0.0mA curr=0.0mA
[IDLE ] set=0.0 setR=0.0 RPM=0.0 duty=   0 (0.0%) adc=  16 curr_raw=1985 iset=0.0mA curr=0.0mA
[IDLE ] set=0.0 setR=0.0 RPM=0.0 duty=   0 (0.0%) adc=  16 curr_raw=1986 iset=0.0mA curr=0.0mA
[IDLE ] set=0.0 setR=0.0 RPM=0.0 duty=   0 (0.0%) adc=  16 curr_raw=1987 iset=0.0mA curr=0.0mA
[IDLE ] set=0.0 setR=0.0 RPM=0.0 duty=   0 (0.0%) adc=  16 curr_raw=1987 iset=0.0mA curr=0.0mA
[IDLE ] set=0.0 setR=0.0 RPM=0.0 duty=   0 (0.0%) adc=  16 curr_raw=1988 iset=0.0mA curr=0.0mA
[IDLE ] set=0.0 setR=0.0 RPM=0.0 duty=   0 (0.0%) adc=  17 curr_raw=1989 iset=0.0mA curr=0.0mA
[IDLE ] set=0.0 setR=0.0 RPM=0.0 duty=   0 (0.0%) adc=  16 curr_raw=1992 iset=0.0mA curr=0.0mA
[IDLE ] set=0.0 setR=0.0 RPM=0.0 duty=   0 (0.0%) adc=  16 curr_raw=1994 iset=0.0mA curr=0.0mA
[IDLE ] set=0.0 setR=0.0 RPM=0.0 duty=   0 (0.0%) adc=  16 curr_raw=1994 iset=0.0mA curr=0.0mA

设置50
[IDLE ] set=0.0 setR=0.0 RPM=0.0 duty=   0 (0.0%) adc=  11 curr_raw=2020 iset=0.0mA curr=0.0mA
[IDLE ] set=0.0 setR=0.0 RPM=0.0 duty=   0 (0.0%) adc=  11 curr_raw=2021 iset=0.0mA curr=0.0mA
[IDLE ] set=0.0 setR=0.0 RPM=0.0 duty=   0 (0.0%) adc=  11 curr_raw=2021 iset=0.0mA curr=0.0mA
[IDLE ] set=0.0 setR=0.0 RPM=0.0 duty=   0 (0.0%) adc=  13 curr_raw=2018 iset=0.0mA curr=0.0mA
[IDLE ] set=0.0 setR=0.0 RPM=0.0 duty=   0 (0.0%) adc=  11 curr_raw=2022 iset=0.0mA curr=0.0mA
[IDLE ] set=0.0 setR=0.0 RPM=0.0 duty=   0 (0.0%) adc=  13 curr_raw=2034 iset=0.0mA curr=0.0mA
[START] set=16.7 setR=16.6 RPM=0.0 duty=  46 (1.1%) adc= 424 curr_raw=2126 iset=2.8mA curr=0.0mA
[START] set=23.3 setR=23.2 RPM=0.0 duty= 125 (3.1%) adc= 717 curr_raw=2250 iset=10.3mA curr=10.0mA
[START] set=31.9 setR=31.8 RPM=0.0 duty= 169 (4.1%) adc=1093 curr_raw=2315 iset=20.8mA curr=20.1mA
[START] set=38.0 setR=38.0 RPM=0.0 duty= 217 (5.3%) adc=1364 curr_raw=2392 iset=33.8mA curr=32.9mA
[START] set=40.0 setR=40.1 RPM=0.0 duty= 267 (6.5%) adc=1453 curr_raw=2479 iset=47.8mA curr=47.2mA
[START] set=40.2 setR=40.2 RPM=0.0 duty= 318 (7.8%) adc=1461 curr_raw=2567 iset=62.2mA curr=61.5mA
[RUN  ] set=41.4 setR=41.3 RPM=13.0 duty= 566 (13.8%) adc=1511 curr_raw=2317 iset=32.7mA curr=27.9mA
[RUN  ] set=45.6 setR=45.5 RPM=39.6 duty=1164 (28.4%) adc=1697 curr_raw=2348 iset=36.7mA curr=27.8mA
[RUN  ] set=48.9 setR=48.9 RPM=55.1 duty=1502 (36.7%) adc=1842 curr_raw=2253 iset=35.5mA curr=21.3mA
[RUN  ] set=51.6 setR=51.5 RPM=62.2 duty=1630 (39.8%) adc=1961 curr_raw=2657 iset=31.8mA curr=25.6mA
[RUN  ] set=54.1 setR=54.0 RPM=62.2 duty=1600 (39.1%) adc=2074 curr_raw=2370 iset=28.5mA curr=33.5mA
[RUN  ] set=54.9 setR=55.0 RPM=61.2 duty=1605 (39.2%) adc=2110 curr_raw=2438 iset=26.2mA curr=21.8mA
[RUN  ] set=53.7 setR=53.8 RPM=60.7 duty=1572 (38.4%) adc=2056 curr_raw=2240 iset=23.8mA curr=13.1mA
[RUN  ] set=48.7 setR=48.7 RPM=57.2 duty=1473 (36.0%) adc=1835 curr_raw=2373 iset=20.2mA curr=26.2mA
[RUN  ] set=48.7 setR=48.7 RPM=52.1 duty=1351 (33.0%) adc=1836 curr_raw=2211 iset=18.5mA curr=22.0mA
[RUN  ] set=48.8 setR=48.8 RPM=47.6 duty=1261 (30.8%) adc=1837 curr_raw=2210 iset=18.4mA curr=23.6mA
[RUN  ] set=48.7 setR=48.7 RPM=45.6 duty=1209 (29.5%) adc=1833 curr_raw=2214 iset=19.2mA curr=25.3mA
[RUN  ] set=48.7 setR=48.7 RPM=45.6 duty=1218 (29.7%) adc=1834 curr_raw=2337 iset=20.3mA curr=13.9mA
[RUN  ] set=48.7 setR=48.7 RPM=47.6 duty=1274 (31.1%) adc=1835 curr_raw=2222 iset=20.9mA curr=24.6mA
[RUN  ] set=48.7 setR=48.7 RPM=51.1 duty=1379 (33.7%) adc=1835 curr_raw=2245 iset=20.5mA curr=26.3mA
[RUN  ] set=48.7 setR=48.8 RPM=54.6 duty=1437 (35.1%) adc=1835 curr_raw=2405 iset=18.6mA curr=20.0mA
[RUN  ] set=48.7 setR=48.8 RPM=54.6 duty=1425 (34.8%) adc=1836 curr_raw=2281 iset=16.5mA curr=19.7mA
[RUN  ] set=48.7 setR=48.8 RPM=52.6 duty=1364 (33.3%) adc=1835 curr_raw=2406 iset=14.8mA curr=9.8mA
[RUN  ] set=48.7 setR=48.7 RPM=49.6 duty=1302 (31.8%) adc=1835 curr_raw=2213 iset=14.2mA curr=12.8mA
[RUN  ] set=48.7 setR=48.7 RPM=47.1 duty=1240 (30.3%) adc=1834 curr_raw=2184 iset=14.5mA curr=6.2mA
[RUN  ] set=48.7 setR=48.7 RPM=46.6 duty=1240 (30.3%) adc=1834 curr_raw=2427 iset=15.2mA curr=25.4mA
[RUN  ] set=48.7 setR=48.6 RPM=46.6 duty=1241 (30.3%) adc=1834 curr_raw=2179 iset=15.9mA curr=20.7mA
[RUN  ] set=48.7 setR=48.8 RPM=47.6 duty=1281 (31.3%) adc=1835 curr_raw=2253 iset=16.3mA curr=14.4mA
[RUN  ] set=48.7 setR=48.7 RPM=48.6 duty=1300 (31.7%) adc=1833 curr_raw=2191 iset=16.4mA curr=10.1mA
[RUN  ] set=48.7 setR=48.7 RPM=49.1 duty=1292 (31.6%) adc=1835 curr_raw=2235 iset=16.2mA curr=7.2mA
[RUN  ] set=48.7 setR=48.7 RPM=49.6 duty=1293 (31.6%) adc=1835 curr_raw=2214 iset=16.0mA curr=14.4mA
[RUN  ] set=48.7 setR=48.7 RPM=49.1 duty=1288 (31.5%) adc=1836 curr_raw=2390 iset=15.8mA curr=25.7mA
[RUN  ] set=48.7 setR=48.7 RPM=47.6 duty=1262 (30.8%) adc=1834 curr_raw=2185 iset=15.9mA curr=6.0mA
[RUN  ] set=48.7 setR=48.7 RPM=47.1 duty=1253 (30.6%) adc=1835 curr_raw=2265 iset=16.4mA curr=12.1mA
[RUN  ] set=48.7 setR=48.7 RPM=47.1 duty=1250 (30.5%) adc=1834 curr_raw=2190 iset=17.0mA curr=8.1mA
[RUN  ] set=48.7 setR=48.7 RPM=47.6 duty=1274 (31.1%) adc=1833 curr_raw=2486 iset=17.4mA curr=18.4mA
[RUN  ] set=48.7 setR=48.7 RPM=48.6 duty=1279 (31.2%) adc=1835 curr_raw=2179 iset=17.5mA curr=9.7mA
[RUN  ] set=48.7 setR=48.7 RPM=49.6 duty=1298 (31.7%) adc=1835 curr_raw=2408 iset=17.2mA curr=26.9mA
[RUN  ] set=48.7 setR=48.7 RPM=49.1 duty=1295 (31.6%) adc=1834 curr_raw=2348 iset=16.9mA curr=25.3mA
[RUN  ] set=48.7 setR=48.7 RPM=49.1 duty=1286 (31.4%) adc=1835 curr_raw=2202 iset=16.7mA curr=21.9mA
[RUN  ] set=48.7 setR=48.7 RPM=48.6 duty=1282 (31.3%) adc=1834 curr_raw=2176 iset=16.7mA curr=7.4mA
[RUN  ] set=48.7 setR=48.7 RPM=48.6 duty=1291 (31.5%) adc=1835 curr_raw=2222 iset=16.7mA curr=22.3mA
[RUN  ] set=48.7 setR=48.7 RPM=48.6 duty=1290 (31.5%) adc=1834 curr_raw=2265 iset=16.7mA curr=15.6mA
[RUN  ] set=48.7 setR=48.7 RPM=48.6 duty=1285 (31.4%) adc=1835 curr_raw=2298 iset=16.7mA curr=15.6mA
[RUN  ] set=48.7 setR=48.7 RPM=48.6 duty=1273 (31.1%) adc=1836 curr_raw=2323 iset=16.7mA curr=25.3mA
[RUN  ] set=48.7 setR=48.7 RPM=49.1 duty=1290 (31.5%) adc=1835 curr_raw=2355 iset=16.6mA curr=25.4mA
[RUN  ] set=48.7 setR=48.7 RPM=48.6 duty=1281 (31.3%) adc=1834 curr_raw=2231 iset=16.5mA curr=15.8mA
[RUN  ] set=48.7 setR=48.7 RPM=48.6 duty=1286 (31.4%) adc=1835 curr_raw=2603 iset=16.6mA curr=22.1mA
[RUN  ] set=48.7 setR=48.7 RPM=48.1 duty=1279 (31.2%) adc=1836 curr_raw=2220 iset=16.7mA curr=12.7mA
[RUN  ] set=48.7 setR=48.7 RPM=48.6 duty=1273 (31.1%) adc=1835 curr_raw=2225 iset=16.8mA curr=22.0mA
[RUN  ] set=48.7 setR=48.7 RPM=48.6 duty=1286 (31.4%) adc=1836 curr_raw=2187 iset=16.8mA curr=7.4mA
[RUN  ] set=48.7 setR=48.7 RPM=49.1 duty=1292 (31.6%) adc=1835 curr_raw=2217 iset=16.7mA curr=18.2mA
[RUN  ] set=48.7 setR=48.7 RPM=49.1 duty=1304 (31.8%) adc=1834 curr_raw=2442 iset=16.6mA curr=27.7mA
[RUN  ] set=48.7 setR=48.7 RPM=48.6 duty=1298 (31.7%) adc=1833 curr_raw=2251 iset=16.5mA curr=14.0mA
[RUN  ] set=48.7 setR=48.7 RPM=48.6 duty=1291 (31.5%) adc=1833 curr_raw=2204 iset=16.4mA curr=11.4mA
[RUN  ] set=48.7 setR=48.7 RPM=48.1 duty=1270 (31.0%) adc=1834 curr_raw=2173 iset=16.6mA curr=22.0mA
[RUN  ] set=48.7 setR=48.7 RPM=48.6 duty=1281 (31.3%) adc=1835 curr_raw=2198 iset=16.6mA curr=11.4mA
[RUN  ] set=48.7 setR=48.7 RPM=49.1 duty=1296 (31.6%) adc=1834 curr_raw=2215 iset=16.5mA curr=21.9mA
[RUN  ] set=48.7 setR=48.7 RPM=49.1 duty=1302 (31.8%) adc=1836 curr_raw=2333 iset=16.4mA curr=15.5mA
[RUN  ] set=48.7 setR=48.7 RPM=49.1 duty=1303 (31.8%) adc=1836 curr_raw=2423 iset=16.3mA curr=17.5mA
[RUN  ] set=48.7 setR=48.7 RPM=49.1 duty=1290 (31.5%) adc=1835 curr_raw=2201 iset=16.1mA curr=11.2mA
[RUN  ] set=48.7 setR=48.7 RPM=48.6 duty=1271 (31.0%) adc=1836 curr_raw=2272 iset=16.2mA curr=19.6mA
[RUN  ] set=48.6 setR=48.7 RPM=48.6 duty=1281 (31.3%) adc=1832 curr_raw=2522 iset=16.2mA curr=13.7mA
[RUN  ] set=48.7 setR=48.7 RPM=48.6 duty=1282 (31.3%) adc=1835 curr_raw=2218 iset=16.3mA curr=21.5mA
[RUN  ] set=48.7 setR=48.7 RPM=48.1 duty=1285 (31.4%) adc=1833 curr_raw=2484 iset=16.4mA curr=17.6mA
设置80
[IDLE ] set=0.0 setR=0.0 RPM=0.0 duty=   0 (0.0%) adc=   9 curr_raw=1999 iset=0.0mA curr=0.0mA
[IDLE ] set=0.0 setR=0.0 RPM=0.0 duty=   0 (0.0%) adc=   9 curr_raw=2001 iset=0.0mA curr=0.0mA
[IDLE ] set=0.0 setR=0.0 RPM=0.0 duty=   0 (0.0%) adc=   7 curr_raw=1992 iset=0.0mA curr=0.0mA
[IDLE ] set=0.0 setR=0.0 RPM=0.0 duty=   0 (0.0%) adc=   8 curr_raw=1991 iset=0.0mA curr=0.0mA
[IDLE ] set=0.0 setR=0.0 RPM=0.0 duty=   0 (0.0%) adc=   6 curr_raw=1954 iset=0.0mA curr=0.0mA
[START] set=28.2 setR=28.1 RPM=0.0 duty= 156 (3.8%) adc= 933 curr_raw=2230 iset=7.9mA curr=6.7mA
[START] set=33.6 setR=33.6 RPM=0.0 duty= 188 (4.6%) adc=1170 curr_raw=2321 iset=19.6mA curr=20.4mA
[START] set=39.6 setR=39.5 RPM=0.0 duty= 203 (5.0%) adc=1433 curr_raw=2394 iset=32.7mA curr=33.5mA
[START] set=42.3 setR=42.3 RPM=0.0 duty= 244 (6.0%) adc=1555 curr_raw=2480 iset=47.6mA curr=47.1mA
[START] set=47.0 setR=47.0 RPM=3.5 duty= 415 (10.1%) adc=1760 curr_raw=2496 iset=63.4mA curr=39.7mA
[RUN  ] set=54.0 setR=53.8 RPM=25.6 duty= 898 (21.9%) adc=2067 curr_raw=2298 iset=36.9mA curr=21.2mA
[RUN  ] set=57.4 setR=57.4 RPM=55.6 duty=1605 (39.2%) adc=2218 curr_raw=2269 iset=40.1mA curr=25.4mA
[RUN  ] set=61.0 setR=61.0 RPM=65.2 duty=1690 (41.3%) adc=2377 curr_raw=2345 iset=38.0mA curr=21.4mA
[RUN  ] set=65.7 setR=65.6 RPM=65.2 duty=1688 (41.2%) adc=2582 curr_raw=2250 iset=37.6mA curr=23.5mA
[RUN  ] set=67.9 setR=67.9 RPM=65.7 duty=1671 (40.8%) adc=2679 curr_raw=2173 iset=38.1mA curr=18.1mA
[RUN  ] set=70.6 setR=70.6 RPM=65.2 duty=1684 (41.1%) adc=2801 curr_raw=2170 iset=39.7mA curr=15.0mA
[RUN  ] set=73.1 setR=73.1 RPM=64.7 duty=1699 (41.5%) adc=2909 curr_raw=2232 iset=42.3mA curr=20.8mA
[RUN  ] set=74.4 setR=74.4 RPM=66.2 duty=1709 (41.7%) adc=2966 curr_raw=2197 iset=45.2mA curr=23.3mA
[RUN  ] set=75.3 setR=75.3 RPM=66.7 duty=1719 (42.0%) adc=3008 curr_raw=2514 iset=48.1mA curr=15.9mA
[RUN  ] set=77.3 setR=77.3 RPM=67.7 duty=1740 (42.5%) adc=3094 curr_raw=2196 iset=51.4mA curr=23.2mA
[RUN  ] set=77.5 setR=77.6 RPM=68.2 duty=1761 (43.0%) adc=3105 curr_raw=2217 iset=54.7mA curr=17.4mA
[RUN  ] set=78.4 setR=78.3 RPM=68.7 duty=1772 (43.3%) adc=3142 curr_raw=2183 iset=58.0mA curr=17.0mA
[RUN  ] set=80.3 setR=80.3 RPM=69.7 duty=1786 (43.6%) adc=3228 curr_raw=2173 iset=61.6mA curr=15.3mA
[RUN  ] set=80.3 setR=80.3 RPM=70.7 duty=1820 (44.4%) adc=3226 curr_raw=2248 iset=65.1mA curr=11.6mA
[RUN  ] set=80.3 setR=80.3 RPM=71.7 duty=1847 (45.1%) adc=3228 curr_raw=2422 iset=68.2mA curr=20.8mA
[RUN  ] set=80.3 setR=80.2 RPM=72.2 duty=1858 (45.4%) adc=3225 curr_raw=2182 iset=71.0mA curr=15.2mA
[RUN  ] set=80.3 setR=80.3 RPM=72.7 duty=1856 (45.3%) adc=3226 curr_raw=2151 iset=73.7mA curr=23.8mA
[RUN  ] set=80.3 setR=80.3 RPM=73.7 duty=1879 (45.9%) adc=3227 curr_raw=2752 iset=76.1mA curr=21.0mA
[RUN  ] set=80.3 setR=80.3 RPM=73.7 duty=1884 (46.0%) adc=3227 curr_raw=2307 iset=78.5mA curr=21.5mA
[RUN  ] set=80.3 setR=80.3 RPM=74.2 duty=1897 (46.3%) adc=3227 curr_raw=2200 iset=80.7mA curr=24.1mA
[RUN  ] set=80.3 setR=80.3 RPM=74.7 duty=1912 (46.7%) adc=3227 curr_raw=2172 iset=82.7mA curr=21.9mA
[RUN  ] set=80.3 setR=80.3 RPM=75.2 duty=1916 (46.8%) adc=3227 curr_raw=2222 iset=84.5mA curr=19.4mA
[RUN  ] set=80.3 setR=80.3 RPM=75.7 duty=1929 (47.1%) adc=3228 curr_raw=2224 iset=86.2mA curr=21.6mA
[RUN  ] set=80.3 setR=80.3 RPM=75.7 duty=1930 (47.1%) adc=3228 curr_raw=2438 iset=87.8mA curr=15.1mA
[RUN  ] set=80.3 setR=80.3 RPM=76.7 duty=1951 (47.6%) adc=3227 curr_raw=2488 iset=89.3mA curr=20.6mA
[RUN  ] set=80.3 setR=80.3 RPM=76.2 duty=1951 (47.6%) adc=3228 curr_raw=2178 iset=90.6mA curr=20.9mA
[RUN  ] set=80.3 setR=80.3 RPM=76.7 duty=1963 (47.9%) adc=3228 curr_raw=2184 iset=91.8mA curr=13.6mA
[RUN  ] set=80.3 setR=80.3 RPM=77.2 duty=1966 (48.0%) adc=3229 curr_raw=2165 iset=92.9mA curr=21.0mA
[RUN  ] set=80.3 setR=80.3 RPM=77.2 duty=1963 (47.9%) adc=3228 curr_raw=2391 iset=94.0mA curr=20.2mA
[RUN  ] set=80.3 setR=80.3 RPM=77.2 duty=1961 (47.9%) adc=3228 curr_raw=2348 iset=95.2mA curr=25.5mA
[RUN  ] set=80.3 setR=80.3 RPM=77.2 duty=1972 (48.2%) adc=3228 curr_raw=2203 iset=96.3mA curr=24.3mA
[RUN  ] set=80.3 setR=80.4 RPM=77.7 duty=1969 (48.1%) adc=3227 curr_raw=2253 iset=97.2mA curr=15.9mA
[RUN  ] set=80.3 setR=80.3 RPM=78.2 duty=1981 (48.4%) adc=3228 curr_raw=2361 iset=98.0mA curr=21.4mA
[RUN  ] set=80.3 setR=80.3 RPM=77.7 duty=1987 (48.5%) adc=3228 curr_raw=2188 iset=98.9mA curr=23.7mA
[RUN  ] set=80.3 setR=80.3 RPM=77.7 duty=1989 (48.6%) adc=3229 curr_raw=2424 iset=99.7mA curr=15.5mA
[RUN  ] set=80.3 setR=80.3 RPM=78.2 duty=1982 (48.4%) adc=3228 curr_raw=2181 iset=100.4mA curr=23.8mA
[RUN  ] set=80.3 setR=80.3 RPM=78.2 duty=1991 (48.6%) adc=3227 curr_raw=2587 iset=101.1mA curr=23.7mA
[RUN  ] set=80.3 setR=80.3 RPM=78.2 duty=1994 (48.7%) adc=3228 curr_raw=2378 iset=101.7mA curr=24.4mA
[RUN  ] set=80.3 setR=80.3 RPM=78.7 duty=2006 (49.0%) adc=3227 curr_raw=2722 iset=102.3mA curr=24.6mA
[RUN  ] set=80.3 setR=80.3 RPM=78.2 duty=2007 (49.0%) adc=3228 curr_raw=2343 iset=102.9mA curr=15.2mA
[RUN  ] set=80.3 setR=80.3 RPM=79.2 duty=2003 (48.9%) adc=3228 curr_raw=2156 iset=103.3mA curr=14.0mA
[RUN  ] set=80.3 setR=80.3 RPM=79.2 duty=2003 (48.9%) adc=3228 curr_raw=2503 iset=103.8mA curr=23.0mA
[RUN  ] set=80.3 setR=80.3 RPM=79.2 duty=2003 (48.9%) adc=3229 curr_raw=2167 iset=104.2mA curr=25.3mA
[RUN  ] set=80.3 setR=80.3 RPM=79.2 duty=2007 (49.0%) adc=3227 curr_raw=2312 iset=104.6mA curr=26.3mA
[RUN  ] set=80.3 setR=80.3 RPM=79.2 duty=2010 (49.1%) adc=3229 curr_raw=2362 iset=105.0mA curr=26.0mA
[RUN  ] set=80.3 setR=80.3 RPM=79.2 duty=2016 (49.2%) adc=3227 curr_raw=2201 iset=105.4mA curr=22.0mA
[RUN  ] set=80.3 setR=80.3 RPM=79.2 duty=2015 (49.2%) adc=3227 curr_raw=2179 iset=105.7mA curr=22.5mA
[RUN  ] set=80.3 setR=80.3 RPM=79.2 duty=2016 (49.2%) adc=3227 curr_raw=2185 iset=106.0mA curr=22.7mA
[RUN  ] set=80.3 setR=80.3 RPM=79.7 duty=2018 (49.3%) adc=3228 curr_raw=2193 iset=106.2mA curr=23.1mA
[RUN  ] set=80.3 setR=80.3 RPM=79.2 duty=2018 (49.3%) adc=3227 curr_raw=2424 iset=106.5mA curr=26.7mA
[RUN  ] set=80.3 setR=80.3 RPM=79.7 duty=2017 (49.3%) adc=3226 curr_raw=2306 iset=106.7mA curr=23.2mA
[RUN  ] set=80.3 setR=80.3 RPM=79.7 duty=2016 (49.2%) adc=3228 curr_raw=2174 iset=106.9mA curr=25.7mA
[RUN  ] set=80.3 setR=80.3 RPM=79.7 duty=2015 (49.2%) adc=3227 curr_raw=2250 iset=107.1mA curr=26.9mA
[RUN  ] set=80.3 setR=80.3 RPM=79.7 duty=2027 (49.5%) adc=3226 curr_raw=2309 iset=107.3mA curr=16.0mA
[RUN  ] set=80.3 setR=80.3 RPM=79.2 duty=2023 (49.4%) adc=3227 curr_raw=2144 iset=107.5mA curr=11.4mA
[RUN  ] set=80.3 setR=80.3 RPM=79.7 duty=2029 (49.5%) adc=3227 curr_raw=2417 iset=107.7mA curr=15.5mA
[RUN  ] set=80.3 setR=80.3 RPM=80.2 duty=2034 (49.7%) adc=3226 curr_raw=2200 iset=107.8mA curr=15.6mA
[RUN  ] set=80.3 setR=80.3 RPM=79.7 duty=2027 (49.5%) adc=3227 curr_raw=2450 iset=108.0mA curr=22.5mA
[RUN  ] set=80.3 setR=80.3 RPM=79.7 duty=2025 (49.5%) adc=3227 curr_raw=2199 iset=108.1mA curr=23.7mA
[RUN  ] set=80.3 setR=80.3 RPM=79.7 duty=2032 (49.6%) adc=3228 curr_raw=2174 iset=108.2mA curr=18.8mA
[RUN  ] set=80.3 setR=80.3 RPM=80.2 duty=2026 (49.5%) adc=3226 curr_raw=2600 iset=108.3mA curr=26.0mA
[RUN  ] set=80.3 setR=80.3 RPM=79.7 duty=2031 (49.6%) adc=3226 curr_raw=2214 iset=108.4mA curr=22.1mA
[RUN  ] set=80.3 setR=80.3 RPM=80.2 duty=2027 (49.5%) adc=3228 curr_raw=2169 iset=108.4mA curr=24.5mA
[RUN  ] set=80.3 setR=80.3 RPM=79.7 duty=2027 (49.5%) adc=3227 curr_raw=2198 iset=108.6mA curr=12.9mA
[RUN  ] set=80.3 setR=80.3 RPM=79.7 duty=2029 (49.5%) adc=3227 curr_raw=2423 iset=108.7mA curr=15.3mA
[RUN  ] set=80.3 setR=80.3 RPM=79.7 duty=2040 (49.8%) adc=3226 curr_raw=2499 iset=108.7mA curr=23.0mA
[RUN  ] set=80.3 setR=80.3 RPM=79.7 duty=2034 (49.7%) adc=3227 curr_raw=2176 iset=108.8mA curr=24.6mA
[RUN  ] set=80.3 setR=80.3 RPM=79.7 duty=2034 (49.7%) adc=3226 curr_raw=2162 iset=108.9mA curr=20.0mA
[RUN  ] set=80.3 setR=80.3 RPM=79.7 duty=2032 (49.6%) adc=3226 curr_raw=2358 iset=108.9mA curr=22.6mA
[RUN  ] set=80.3 setR=80.3 RPM=79.7 duty=2032 (49.6%) adc=3227 curr_raw=2256 iset=108.9mA curr=25.0mA
[RUN  ] set=80.3 setR=80.3 RPM=80.2 duty=2033 (49.6%) adc=3227 curr_raw=2208 iset=108.9mA curr=14.3mA
[RUN  ] set=80.3 setR=80.3 RPM=80.2 duty=2038 (49.8%) adc=3225 curr_raw=2310 iset=108.9mA curr=16.3mA
[RUN  ] set=80.3 setR=80.3 RPM=80.2 duty=2031 (49.6%) adc=3227 curr_raw=2475 iset=109.0mA curr=23.3mA
[RUN  ] set=80.3 setR=80.3 RPM=80.2 duty=2029 (49.5%) adc=3228 curr_raw=2377 iset=109.0mA curr=27.7mA
[RUN  ] set=80.3 setR=80.3 RPM=80.2 duty=2030 (49.6%) adc=3227 curr_raw=2515 iset=109.1mA curr=26.6mA
[RUN  ] set=80.3 setR=80.3 RPM=80.2 duty=2038 (49.8%) adc=3226 curr_raw=2208 iset=109.1mA curr=21.5mA
[RUN  ] set=80.3 setR=80.3 RPM=79.7 duty=2029 (49.5%) adc=3227 curr_raw=2269 iset=109.1mA curr=26.8mA
[RUN  ] set=80.3 setR=80.3 RPM=79.7 duty=2036 (49.7%) adc=3226 curr_raw=2175 iset=109.2mA curr=13.1mA
[RUN  ] set=80.3 setR=80.3 RPM=79.7 duty=2039 (49.8%) adc=3227 curr_raw=2251 iset=109.2mA curr=16.0mA
[RUN  ] set=80.3 setR=80.3 RPM=80.2 duty=2036 (49.7%) adc=3227 curr_raw=2177 iset=109.2mA curr=23.4mA
[RUN  ] set=80.3 setR=80.3 RPM=80.2 duty=2039 (49.8%) adc=3227 curr_raw=2187 iset=109.2mA curr=17.6mA
[RUN  ] set=80.3 setR=80.3 RPM=79.7 duty=2031 (49.6%) adc=3227 curr_raw=2189 iset=109.2mA curr=22.6mA
[RUN  ] set=80.3 setR=80.3 RPM=80.2 duty=2031 (49.6%) adc=3228 curr_raw=2166 iset=109.2mA curr=24.5mA
[RUN  ] set=80.3 setR=80.3 RPM=80.2 duty=2032 (49.6%) adc=3227 curr_raw=2148 iset=109.2mA curr=11.9mA
[RUN  ] set=80.3 setR=80.3 RPM=80.2 duty=2046 (50.0%) adc=3225 curr_raw=2212 iset=109.3mA curr=16.8mA
[RUN  ] set=80.3 setR=80.3 RPM=80.2 duty=2035 (49.7%) adc=3227 curr_raw=2172 iset=109.2mA curr=22.6mA
[RUN  ] set=80.3 setR=80.3 RPM=80.2 duty=2041 (49.8%) adc=3227 curr_raw=2208 iset=109.2mA curr=18.4mA
[RUN  ] set=80.3 setR=80.3 RPM=80.2 duty=2035 (49.7%) adc=3227 curr_raw=2207 iset=109.2mA curr=22.1mA
[RUN  ] set=80.3 setR=80.3 RPM=80.2 duty=2028 (49.5%) adc=3229 curr_raw=2261 iset=109.2mA curr=26.8mA
[RUN  ] set=80.3 setR=80.3 RPM=80.2 duty=2036 (49.7%) adc=3228 curr_raw=1839 iset=109.2mA curr=13.1mA
[RUN  ] set=80.3 setR=80.3 RPM=80.2 duty=2041 (49.8%) adc=3228 curr_raw=2435 iset=109.2mA curr=20.9mA
[RUN  ] set=80.3 setR=80.3 RPM=80.2 duty=2034 (49.7%) adc=3226 curr_raw=2298 iset=109.2mA curr=25.9mA
[RUN  ] set=80.3 setR=80.3 RPM=80.2 duty=2038 (49.8%) adc=3226 curr_raw=2829 iset=109.2mA curr=27.5mA
[RUN  ] set=80.3 setR=80.3 RPM=80.2 duty=2034 (49.7%) adc=3227 curr_raw=2418 iset=109.2mA curr=25.2mA
[RUN  ] set=80.3 setR=80.3 RPM=80.2 duty=2031 (49.6%) adc=3228 curr_raw=2411 iset=109.2mA curr=20.1mA
[RUN  ] set=80.3 setR=80.1 RPM=79.7 duty=2043 (49.9%) adc=3227 curr_raw=2459 iset=109.2mA curr=17.5mA
[RUN  ] set=80.3 setR=80.3 RPM=80.2 duty=2043 (49.9%) adc=3227 curr_raw=2524 iset=109.2mA curr=24.8mA
[RUN  ] set=80.3 setR=80.3 RPM=80.7 duty=2034 (49.7%) adc=3225 curr_raw=2240 iset=109.1mA curr=22.5mA
[RUN  ] set=80.3 setR=80.3 RPM=80.2 duty=2031 (49.6%) adc=3226 curr_raw=2308 iset=109.1mA curr=22.9mA
[RUN  ] set=80.3 setR=80.3 RPM=80.2 duty=2031 (49.6%) adc=3225 curr_raw=2186 iset=109.1mA curr=22.7mA
