echo "V30 test1"
./waf --run "testMyhello --nWifis=719 --NS2traceFile=/home/adhoc-labs-1/Desktop/Toon/urban_grid_4X4/g3_v30_1.tcl" > /home/adhoc-labs-1/Desktop/Toon/LogNS/test1_v30.txt &
echo "V30 test2"
./waf --run "testMyhello --nWifis=719 --NS2traceFile=/home/adhoc-labs-1/Desktop/Toon/urban_grid_4X4/g3_v30_2.tcl" > /home/adhoc-labs-1/Desktop/Toon/LogNS/test2_v30.txt &
echo "V30 test3"
./waf --run "testMyhello --nWifis=719 --NS2traceFile=/home/adhoc-labs-1/Desktop/Toon/urban_grid_4X4/g3_v30_3.tcl" > /home/adhoc-labs-1/Desktop/Toon/LogNS/test3_v30.txt 
echo "V30 test4"
./waf --run "testMyhello --nWifis=719 --NS2traceFile=/home/adhoc-labs-1/Desktop/Toon/urban_grid_4X4/g3_v30_4.tcl" > /home/adhoc-labs-1/Desktop/Toon/LogNS/test4_v30.txt &
echo "V30 test5"
./waf --run "testMyhello --nWifis=719 --NS2traceFile=/home/adhoc-labs-1/Desktop/Toon/urban_grid_4X4/g3_v30_5.tcl" > /home/adhoc-labs-1/Desktop/Toon/LogNS/test5_v30.txt 
echo "V30 Done"
