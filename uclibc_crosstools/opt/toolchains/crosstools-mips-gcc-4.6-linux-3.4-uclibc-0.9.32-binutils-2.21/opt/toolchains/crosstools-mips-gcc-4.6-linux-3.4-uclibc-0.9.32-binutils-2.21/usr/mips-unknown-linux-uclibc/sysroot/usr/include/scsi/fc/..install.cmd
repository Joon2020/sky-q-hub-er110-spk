cmd_/home/dslsqa/rel0.6/crosstools-gcc-4.6-linux-3.4-uclibc-0.9.32-binutils-2.21/src/buildroot-2011.11/output/toolchain/linux/include/scsi/fc/.install := perl scripts/headers_install.pl /home/dslsqa/rel0.6/crosstools-gcc-4.6-linux-3.4-uclibc-0.9.32-binutils-2.21/src/buildroot-2011.11/output/toolchain/linux-3.4/include/scsi/fc /home/dslsqa/rel0.6/crosstools-gcc-4.6-linux-3.4-uclibc-0.9.32-binutils-2.21/src/buildroot-2011.11/output/toolchain/linux/include/scsi/fc mips fc_els.h fc_fs.h fc_gs.h fc_ns.h; perl scripts/headers_install.pl /home/dslsqa/rel0.6/crosstools-gcc-4.6-linux-3.4-uclibc-0.9.32-binutils-2.21/src/buildroot-2011.11/output/toolchain/linux-3.4/include/scsi/fc /home/dslsqa/rel0.6/crosstools-gcc-4.6-linux-3.4-uclibc-0.9.32-binutils-2.21/src/buildroot-2011.11/output/toolchain/linux/include/scsi/fc mips ; perl scripts/headers_install.pl /home/dslsqa/rel0.6/crosstools-gcc-4.6-linux-3.4-uclibc-0.9.32-binutils-2.21/src/buildroot-2011.11/output/toolchain/linux-3.4/include/generated/scsi/fc /home/dslsqa/rel0.6/crosstools-gcc-4.6-linux-3.4-uclibc-0.9.32-binutils-2.21/src/buildroot-2011.11/output/toolchain/linux/include/scsi/fc mips ; for F in ; do echo "\#include <asm-generic/$$F>" > /home/dslsqa/rel0.6/crosstools-gcc-4.6-linux-3.4-uclibc-0.9.32-binutils-2.21/src/buildroot-2011.11/output/toolchain/linux/include/scsi/fc/$$F; done; touch /home/dslsqa/rel0.6/crosstools-gcc-4.6-linux-3.4-uclibc-0.9.32-binutils-2.21/src/buildroot-2011.11/output/toolchain/linux/include/scsi/fc/.install
