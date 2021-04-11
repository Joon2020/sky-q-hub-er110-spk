#!/bin/sh

CONFIG_SKY_ETHAN=_set_by_buildFS_
SKY_ROUTER_MODEL=_set_by_buildFS_
READ_EXTERNAL_REG_1=0x2c00
REG_1C=28
#set RGMII mode to 2.5V
RGMII_MODE_25V=0xac0d

case $1 in
    start )
        echo "applyiing SKY specifig config..."
        echo 1 >/sys/module/printk/parameters/time
#        echo "CONFIG_SKY_ETHAN - $CONFIG_SKY_ETHAN";
#       echo "SKY_ROUTER_MODEL - $SKY_ROUTER_MODEL";
#        mount -t jffs2 /dev/mtdblock1 /var/auxfs
#        mkdir -p /tmp/skydiags
	ln -s /data /var/auxfs
	ln -s /log /var/log
        mkdir -p /var/auxfs/diags
        # mkdir -p /var/airties
        # echo > /var/auxfs/udhcpd.leases
        ethswctl -c pause -p 8 -v3
        export VALGRIND_LIB=/lib/valgrind/
        if [ "$SKY_ROUTER_MODEL" = "ER110" ]; then
            echo " access and read the register 1C (External Control 1 Register) parameters "

            #with help of shadow register selector, register 1C (28 in decimal) will act as multiple registers.
            #Value 0x2C will select External Control 1 Register.
            #0x2c00 will read values of values of External Control 1 Register.Refer table 65 of B50612E data sheet
            #following command will display the default values of external control 1 register  of External PHY B50612E.
            #current value of external control 1 register is 0x2c15.set by hardware bootstrap.

            ethctl eth1 reg "$REG_1C" "$READ_EXTERNAL_REG_1"

            echo "write reg 1C MODE_SEL[1] = 0 and MODE_SEL[0] = 1 to set voltage from 1.8v to 2.5v"

            #Bit 15  shall be set to 1 to perform write operation
            #Bit 14:10 shall be set to 01011 to select External Control 1 Register
            #Bit  4 (MODE_SEL[1]) and 3 ( MODE_SEL[0] )of External Control register 1 shall be to set 0 and 1 respectively to select RGMII mode to 2.5 v.
            #Bit 2:0 are 101 by default. Not changed.
            #Above settings will make up bit map 0xac0d to set RGMII mode to 2.5V

            ethctl eth1 reg "$REG_1C"  "$RGMII_MODE_25V"
        fi
        if [ "$CONFIG_SKY_ETHAN" = y ]; then
            echo "set PLC link status down"
            ethswctl -c regaccess -v 0x5c -l 1 -d 0x4a

            echo "Disable txclk on RGMII2"
            ethswctl -c regaccess -v 0x65 -l 1 -d 0x51
            echo "Disable txclk on RGMII4"
            ethswctl -c regaccess -v 0x67 -l 1 -d 0x51

            echo "Disable RGMII2 port"
            ethswctl -c regaccess -v 0x5D -l 1 -d 0x4A
            echo "Disable RGMII4 port"
            ethswctl -c regaccess -v 0x5F -l 1 -d 0x4A

            # Copy log hoover command file into /var/log
            echo "****************** COPIED LOG HOOVER COMMANDS FILE *****************"
            cp -a /etc/log_hoover_cmd.sample /var/log_hoover_cmd.txt
        fi
        if [ "$SKY_ROUTER_MODEL" = "EE120" ]; then
            echo "Disable MII3_GTX_CLK"
            ethswctl -c regaccess -v 0x66 -l 1 -d 0x51
        fi
        exit 0
        ;;

    stop )
        echo "unconfig SKY config not implemented..."
        exit 1
        ;;
    *)
        echo "$0: unrecognised option $1"
        exit 1
        ;;
esac

