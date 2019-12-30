# 假定是在/PickDemo/bin文件夹下执行
cd ../../build
# rm -rf *
cmake ..
make -j8
make install
cd ../PickDemo/build
rm -rf *
cmake ..
make -j8
echo
echo
echo "=== All OK! ==="

