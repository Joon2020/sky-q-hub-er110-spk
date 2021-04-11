#!/bin/bash

# VIPER by default
product="ER110"

case "$1" in
	"SKY_HUB")
		# product="SR101"
		product="IHR-SKY_HUB"
		;;

	"SKY_HUB_IPV6" | "SKY_HUB_IPV6_DEV")
		# product="SR101_IPV6"
		product="IHR-SKY_HUB_IPv6"
		;;

	"VDSL" | "VDSL_DEV")
		product="SR102"
		;;

	"VDSL_IPV6" | "VDSL_IPV6_DEV")
		product="SR102_IPV6"
		;;

	"VIPER" | "VIPER_DEV")
		product="ER110"
		;;

	"VIPER_IPV6" | "VIPER_IPV6_DEV" | "VIPER_IPV6_DIAG" | "VIPER_IPV6_WFA")
		product="ER110_IPV6"
		;;

	"EXTENDER" | "EXTENDER_DEV" | "EXTENDER_DIAG" | "EXTENDER_WFA")
		# product="EE120"
		product="EE120_IPV6"
		;;
	"NR701" | "NR701_DEV")
		product="NR701"
		;;

esac

echo -n $product
