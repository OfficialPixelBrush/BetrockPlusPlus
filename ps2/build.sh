#!/bin/bash
# Resumable object build for the PS2 target. Skips objects that already exist.
source /home/claude/bpp/BetrockPlusPlus-main/ps2/env.sh
cd $ROOT; mkdir -p ps2/build/obj; : > ps2/build/errors.log
while read f; do
  o=ps2/build/obj/$(echo $f | tr '/' '_' | sed 's/\.cpp$/.o/')
  [ -f $o ] && continue
  if mips64r5900el-ps2-elf-g++ $PS2_CXXFLAGS -g0 -c $f -o $o 2>ps2/build/last.err; then echo "OK $f" >> ps2/build/progress
  else rm -f $o; echo "FAIL $f" >> ps2/build/progress; { echo "### $f"; grep -E "error|internal" ps2/build/last.err | head -5; } >> ps2/build/errors.log; fi
done < ps2/sources.txt
echo DONE >> ps2/build/progress
