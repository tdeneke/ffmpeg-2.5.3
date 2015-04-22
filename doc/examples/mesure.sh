#!/bin/bash

ITERATIONS=5
TIMES_FILE="times"
CODEC=$1
FMT=$2
CORES=(1 2 4 6)
#ALGOS[0]="RR"
#ALGOS[1]="WLB"
#ALGOS[2]="KLR"

#SCHEDERS[0]="rr"
#SCHEDERS[1]="dd"

VIDEOS[0]="elephants_dream_1080p"
VIDEOS[1]="old_town_cross_1080p"
VIDEOS[2]="snow_mnt_1080p"
VIDEOS[3]="touchdown_pass_1080p"

RESOLUTIONS[0]="320	240	400000"
RESOLUTIONS[1]="720	480	1200000"
RESOLUTIONS[2]="1280	720	4500000"

echo "Removing previous time logs ..."
rm $TIMES_FILE
echo "Starting the mesurment experiment ..."

#echo -e "scheder\talgo\tcore\treal\tusr\tsys" >> $TIMES_FILE
echo -e "video\twidth\theight\tbitrate\tcore\treal\tusr\tsys" >> $TIMES_FILE

for video  in ${VIDEOS[@]} ;do
#for scheder in ${SCHEDERS[@]} ;do
#for algo in ${ALGOS[@]} ;do
for res in "${RESOLUTIONS[@]}" ;do
for core in ${CORES[@]} ;do
for ((iter=1; iter<=$ITERATIONS; iter++)) ;do
echo -ne "$video\t$res\t$core\t" >> $TIMES_FILE
/usr/bin/time -o $TIMES_FILE -a -p -f '%e\t%U\t%S' ./dataflow_transcoding_test  $video.$FMT ${CODEC}_${video}.h264 $res $core $CODEC
done
done
#done
#done
done
done
