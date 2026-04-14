set_property IOSTANDARD LVCMOS33 [get_ports clk]
set_property IOSTANDARD LVCMOS33 [get_ports pwm_out]
set_property PACKAGE_PIN U20 [get_ports pwm_out]
set_property PACKAGE_PIN K17 [get_ports clk]

set_property PACKAGE_PIN R17 [get_ports cs_n]
set_property PACKAGE_PIN W20 [get_ports miso]
set_property PACKAGE_PIN V20 [get_ports mosi]
set_property PACKAGE_PIN T20 [get_ports sck]
set_property IOSTANDARD LVCMOS33 [get_ports cs_n]
set_property IOSTANDARD LVCMOS33 [get_ports miso]
set_property IOSTANDARD LVCMOS33 [get_ports mosi]
set_property IOSTANDARD LVCMOS33 [get_ports sck]

# ????????? sck ??? P20 ??????? (Error [Place 30-574])
set_property CLOCK_DEDICATED_ROUTE FALSE [get_nets sck_IBUF]

