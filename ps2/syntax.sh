#!/bin/bash
source /home/claude/bpp/BetrockPlusPlus-main/ps2/env.sh
cd $ROOT; : > ps2/syntax.log
while read f; do
  out=$(mips64r5900el-ps2-elf-g++ $PS2_CXXFLAGS -fsyntax-only $f 2>&1)
  if [ -n "$(echo "$out" | grep -E 'error|internal compiler')" ]; then echo "### $f" >> ps2/syntax.log; echo "$out" | grep -E "error|internal compiler" | head -4 >> ps2/syntax.log; echo "FAIL $f" >> ps2/syntax.progress; else echo "OK $f" >> ps2/syntax.progress; fi
done < ps2/sources.txt
echo DONE >> ps2/syntax.progress
