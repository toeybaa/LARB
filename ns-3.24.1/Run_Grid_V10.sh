echo "V10 test1"
./waf --run "testMyhello --nWifis=239 --NS2traceFile=/home/adhoc-labs-1/Desktop/Toon/urban_grid_4X4/g3_v10_1.tcl" > /home/adhoc-labs-1/Desktop/Toon/LogNS/test1_v10.txt &
echo "V10 test2" 
./waf --run "testMyhello --nWifis=239 --NS2traceFile=/home/adhoc-labs-1/Desktop/Toon/urban_grid_4X4/g3_v10_2.tcl" > /home/adhoc-labs-1/Desktop/Toon/LogNS/test2_v10.txt &
echo "V10 test3" 
./waf --run "testMyhello --nWifis=239 --NS2traceFile=/home/adhoc-labs-1/Desktop/Toon/urban_grid_4X4/g3_v10_3.tcl" > /home/adhoc-labs-1/Desktop/Toon/LogNS/test3_v10.txt 
echo "V10 test4"
./waf --run "testMyhello --nWifis=239 --NS2traceFile=/home/adhoc-labs-1/Desktop/Toon/urban_grid_4X4/g3_v10_4.tcl" > /home/adhoc-labs-1/Desktop/Toon/LogNS/test4_v10.txt &
echo "V10 test5"
./waf --run "testMyhello --nWifis=239 --NS2traceFile=/home/adhoc-labs-1/Desktop/Toon/urban_grid_4X4/g3_v10_5.tcl" > /home/adhoc-labs-1/Desktop/Toon/LogNS/test5_v10.txt 
echo "V10 Done"
