echo "V80 test1"
./waf --run "testMyhello --nWifis=1900 --NS2traceFile=/home/adhoc-labs-1/Desktop/Toon/urban_grid_4X4/g3_v80_1.tcl" > /home/adhoc-labs-1/Desktop/Toon/LogNS/test1_v80.txt &
echo "V80 test2"
./waf --run "testMyhello --nWifis=1900 --NS2traceFile=/home/adhoc-labs-1/Desktop/Toon/urban_grid_4X4/g3_v80_2.tcl" > /home/adhoc-labs-1/Desktop/Toon/LogNS/test2_v80.txt &
echo "V80 test3"
./waf --run "testMyhello --nWifis=1900 --NS2traceFile=/home/adhoc-labs-1/Desktop/Toon/urban_grid_4X4/g3_v80_3.tcl" > /home/adhoc-labs-1/Desktop/Toon/LogNS/test3_v80.txt 
echo "V80 test4"
./waf --run "testMyhello --nWifis=1900 --NS2traceFile=/home/adhoc-labs-1/Desktop/Toon/urban_grid_4X4/g3_v80_4.tcl" > /home/adhoc-labs-1/Desktop/Toon/LogNS/test4_v80.txt &
echo "V80 test5"
./waf --run "testMyhello --nWifis=1900 --NS2traceFile=/home/adhoc-labs-1/Desktop/Toon/urban_grid_4X4/g3_v80_5.tcl" > /home/adhoc-labs-1/Desktop/Toon/LogNS/test5_v80.txt 
echo "V80 Done"
