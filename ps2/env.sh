export PS2DEV=/home/claude/ps2dev PS2SDK=/home/claude/ps2dev/ps2sdk
export PATH=$PATH:$PS2DEV/ee/bin:$PS2DEV/iop/bin:$PS2SDK/bin
ROOT=/home/claude/bpp/BetrockPlusPlus-main
PS2_INC="-I$ROOT -I$ROOT/include -I$ROOT/src/bpp_server -I$ROOT/src/bpp_shared -I$ROOT/src/bpp_utilities -I$ROOT/src/bpp_shared/enums -I$ROOT/src/bpp_shared/enums/network -I$ROOT/src/bpp_shared/strings -I$ROOT/src/bpp_shared/logger -I$ROOT/src/bpp_shared/world -I$ROOT/src/bpp_shared/world/noise -I$ROOT/src/bpp_shared/helpers/java -I$ROOT/src/bpp_shared/helpers -I$ROOT/ps2/deps/include -I$PS2SDK/ee/include -I$PS2SDK/common/include -I$PS2SDK/ports/include"
PS2_DEFS="-D_EE -DBUILD_SERVER -DNOMINMAX -DPS2 -DLIBCGLUE_SYS_SOCKET_ALIASES=1 -DGENERATION_PRECISION_DOUBLE"
PS2_CXXFLAGS="-std=c++23 -G0 -Os -Wno-attributes @$ROOT/ps2/typefix.rsp $PS2_DEFS -I$ROOT/ps2/compat -include $ROOT/ps2/compat/ps2_compat.h -include format -include $ROOT/src/bpp_shared/nbt/nbt.h -include $ROOT/src/bpp_server/server.h $PS2_INC"
