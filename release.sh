#!/bin/bash
export LIB=ucm
export DATE=$(date +%Y%m%d)
rm -rf ${LIB}-${DATE}
mkdir ${LIB}-${DATE}

# Linux binaries
mkdir -p ${LIB}-${DATE}/linux/rel
cp -rp linux/rel/*.so ${LIB}-${DATE}/linux/rel/
cp -rp linux/rel/${LIB} ${LIB}-${DATE}/linux/rel/
mkdir -p ${LIB}-${DATE}/linux/dbg
cp -rp linux/dbg/*.so ${LIB}-${DATE}/linux/dbg/
cp -rp linux/dbg/${LIB} ${LIB}-${DATE}/linux/dbg/

# MacOS binaries
mkdir -p ${LIB}-${DATE}/darwin/rel
cp -rp darwin/rel/*.dylib ${LIB}-${DATE}/darwin/rel/
cp -rp darwin/rel/${LIB} ${LIB}-${DATE}/darwin/rel/
mkdir -p ${LIB}-${DATE}/darwin/dbg
cp -rp darwin/dbg/*.dylib ${LIB}-${DATE}/darwin/dbg/
cp -rp darwin/dbg/${LIB} ${LIB}-${DATE}/darwin/dbg/

# Windows binaries
mkdir -p ${LIB}-${DATE}/win32/rel
cp -rp win32/rel/*.dll ${LIB}-${DATE}/win32/rel/
cp -rp win32/rel/*.pdb ${LIB}-${DATE}/win32/rel/
cp -rp win32/rel/*.exe ${LIB}-${DATE}/win32/rel/
mkdir -p ${LIB}-${DATE}/win32/dbg
cp -rp win32/dbg/*.dll ${LIB}-${DATE}/win32/dbg/
cp -rp win32/dbg/*.pdb ${LIB}-${DATE}/win32/dbg/
cp -rp win32/dbg/*.exe ${LIB}-${DATE}/win32/dbg/

cp -p LICENSE.txt ${LIB}-${DATE}/
cat > ${LIB}-${DATE}/README <<EOF
UCM Binary Distribution

UCM is a small utility for communicating with the control endpoint of a LibUSB device.
EOF

# Package it up
tar zcf ${LIB}-${DATE}.tar.gz ${LIB}-${DATE}
rm -rf ${LIB}-${DATE}
cp -p ${LIB}-${DATE}.tar.gz /mnt/ukfsn/bin/
