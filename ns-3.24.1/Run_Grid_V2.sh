echo "V2 test1"
./waf --run "testMyhello --nWifis=47 --NS2traceFile=/home/adhoc-labs-1/Desktop/Toon/urban_grid_4X4/g3_v2_1.tcl" > /home/adhoc-labs-1/Desktop/Toon/LogNS/test1_v2.txt & 
echo "V2 test2"
./waf --run "testMyhello --nWifis=47 --NS2traceFile=/home/adhoc-labs-1/Desktop/Toon/urban_grid_4X4/g3_v2_2.tcl" > /home/adhoc-labs-1/Desktop/Toon/LogNS/test2_v2.txt &
echo "V2 test3"
./waf --run "testMyhello --nWifis=47 --NS2traceFile=/home/adhoc-labs-1/Desktop/Toon/urban_grid_4X4/g3_v2_3.tcl" > /home/adhoc-labs-1/Desktop/Toon/LogNS/test3_v2.txt 
echo "V2 test4" 
./waf --run "testMyhello --nWifis=47 --NS2traceFile=/home/adhoc-labs-1/Desktop/Toon/urban_grid_4X4/g3_v2_4.tcl" > /home/adhoc-labs-1/Desktop/Toon/LogNS/test4_v2.txt &
echo "V2 test5" 
./waf --run "testMyhello --nWifis=47 --NS2traceFile=/home/adhoc-labs-1/Desktop/Toon/urban_grid_4X4/g3_v2_5.tcl" > /home/adhoc-labs-1/Desktop/Toon/LogNS/test5_v2.txt 
echo "V2 Done"
