TARGET_DIR := ".."
BSP_DIR := ${TARGET_DIR}/"bcm963xx"
ROOT_DIR := $(shell pwd)
MESH_DIR := ../mesh
AIRTIES_BUILDSYS := airties_buildsys.tar
AIRTIES_PACKAGE := airties_package.tar.bz2
BUILDSYS_DIR := export
PATCH_SRC := ${BUILDSYS_DIR}/patches
PATCH_DIR := airties_sky.patch

.PHONY: install patch clean

all:	install

install: ${BSP_DIR}/.airties_installed

${BSP_DIR}/.airties_installed:
	if [ ! -f ${AIRTIES_BUILDSYS} ]; then echo error: ${AIRTIES_BUILDSYS} is missing; exit 1; fi
	if [ ! -f ${AIRTIES_PACKAGE} ]; then echo error: ${AIRTIES_PACKAGE} is missing; exit 1; fi
	tar xvf ${AIRTIES_BUILDSYS}
	mv ${BUILDSYS_DIR}/airties/Makefile.skybuild ${BUILDSYS_DIR}/airties/Makefile
	rm -rf ${BUILDSYS_DIR}/airties/Makefile.airties ${BUILDSYS_DIR}/airties/Makefile.releaseversion
	cp -R ${BUILDSYS_DIR}/airties ${BSP_DIR}
	ln -sf `pwd`/${AIRTIES_PACKAGE} ${BSP_DIR}/airties/${AIRTIES_PACKAGE}
	if [ ! -d ${PATCH_DIR} ]; then mkdir ${PATCH_DIR}; fi
	cp ${PATCH_SRC}/airties_bsp.patch ${PATCH_DIR}/airties_bsp.patch
	cp -R ${BUILDSYS_DIR}/plc_fw ${PATCH_DIR}/plc_fw
	rm -rf ${BUILDSYS_DIR}
	rm -rf tmp
	touch ${BSP_DIR}/.airties_installed

patch: .patched

.patched:
	make install
	echo -e "\n\n******* Applying patches *******\n\n"
	cp ${PATCH_DIR}/plc_fw/*bin ${BSP_DIR}/userspace/private/apps/plcboot/
	# patches now applied using the 'git apply' command instead of
	# individual patch commands.
	(cd ${BSP_DIR}; git apply ${PATCH_DIR}/airties_bsp.patch )
	touch .patched

clean:
	rm -rf tmp
	rm -rf ${PATCH_DIR}
	rm -rf ${BSP_DIR}/airties
	rm -f ${BSP_DIR}/.airties_installed
