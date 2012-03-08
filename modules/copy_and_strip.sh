rm *.ko

find $ANDROID_BUILD_TOP/out/target/product/olympus/ -name "*.ko" -exec cp -f {} ./ \;
find -name "*.ko" -exec arm-linux-androideabi-strip --strip-debug {} \;

