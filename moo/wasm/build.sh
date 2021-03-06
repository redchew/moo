# temporary build script.
# must integrate the build process into automake

topdir=../../..
blddir=${topdir}/bld/emcc

emcc -Wall -O2 -g   \
	${blddir}/lib/.libs/libmoo.a \
	${blddir}/lib/.libs/libmoox.a \
	${topdir}/wasm/main.c \
	-DMOO_HAVE_CFG_H \
	-I${blddir}/lib \
	-I${topdir}/lib\
	-s WASM=1 \
	-s LINKABLE=1 \
	-s ALLOW_MEMORY_GROWTH=1 \
	-s EXTRA_EXPORTED_RUNTIME_METHODS="['ccall','cwrap']" \
	-o libmoo.js \
	--embed-file ${topdir}/kernel/ \
	--pre-js ${topdir}/wasm/moo.cb.js

##-s USE_PTHREADS=1 -s ERROR_ON_UNDEFINED_SYMBOLS=0 \
##cp -pf ${topdir}/wasm/moo.html .
##cp -pf ${topdir}/wasm/moo.worker.js .
ln -sf ${topdir}/wasm/moo.html moo.html
ln -sf ${topdir}/wasm/moo.worker.js moo.worker.js
