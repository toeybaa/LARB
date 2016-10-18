echo "V60 test1"
./waf --run "testMyhello --nWifis=1439 --NS2traceFile=/home/adhoc-labs-1/Desktop/Toon/urban_grid_4X4/g3_v60_1.tcl" > /home/adhoc-labs-1/Desktop/Toon/LogNS/test1_v60.txt &
echo "V60 test2"
./waf --run "testMyhello --nWifis=1439 --NS2traceFile=/home/adhoc-labs-1/Desktop/Toon/urban_grid_4X4/g3_v60_2.tcl" > /home/adhoc-labs-1/Desktop/Toon/LogNS/test2_v60.txt &
echo "V60 test3"
./waf --run "testMyhello --nWifis=1439 --NS2traceFile=/home/adhoc-labs-1/Desktop/Toon/urban_grid_4X4/g3_v60_3.tcl" > /home/adhoc-labs-1/Desktop/Toon/LogNS/test3_v60.txt 
echo "V60 test4"
./waf --run "testMyhello --nWifis=1439 --NS2traceFile=/home/adhoc-labs-1/Desktop/Toon/urban_grid_4X4/g3_v60_4.tcl" > /home/adhoc-labs-1/Desktop/Toon/LogNS/test4_v60.txt &
echo "V60 test5"
./waf --run "testMyhello --nWifis=1439 --NS2traceFile=/home/adhoc-labs-1/Desktop/Toon/urban_grid_4X4/g3_v60_5.tcl" > /home/adhoc-labs-1/Desktop/Toon/LogNS/test5_v60.txt 
echo "V60 Done"
