#!/bin/bash
source /home/claude/bpp/BetrockPlusPlus-main/ps2/env.sh
cd $ROOT
mips64r5900el-ps2-elf-g++ -T$PS2SDK/ee/startup/linkfile -Os -o ps2/build/betrock.elf ps2/build/obj/*.o \
  -L$PS2SDK/ee/lib -L$PS2SDK/ports/lib -Lps2/deps/lib -Wl,-zmax-page-size=128 \
  -Wl,--start-group -lps2_drivers -ldeflate -lnetman -lps2ip -lps2ips -lcdvd -lc -lcglue -lpthread -lpthreadglue -lkernel -lpatches -ldebug -lfileXio -lpoweroff -lhdd -lmc -lmtap -lpadx -laudsrv -lstdc++ -lm -Wl,--end-group "$@"
