#!/bin/sh
#simulate Video capture use cases, default codec:H264, default rate control: CBR 
adb shell mix_encoder -w 1920 -h 1080 -f /data/record_1080P.264 
adb shell mix_encoder -w 1280 -h 720 -f /data/record_720P.264
adb shell mix_encoder -w 720 -h 480 -f /data/record_480P.264

#simulate Video edit use cases
adb shell mix_encoder -w 1920 -h 1080  -m 1 -f /data/videoedit_1080P.264
adb shell mix_encoder -w 1280 -h 720 -m 1 -f /data/videoedit_720P.264
adb shell mix_encoder -w 720 -h 480 -m 1 -f /data/videoedit_480P.264

#simulate WIDI use cases
adb shell mix_encoder -w 1920 -h 1080  -m 2 -f /data/widi_1080P.264
adb shell mix_encoder -w 1280 -h 720 -m 2 -f /data/widi_720P.264
adb shell mix_encoder -w 720 -h 480 -m 2 -f /data/widi_480P.264

for i in record videoedit widi
do
    for j in 1080P 720P 480P
    do
        TEMP_NAME=${i}_$j.264
        echo $TEMP_NAME
        adb pull /data/$TEMP_NAME .
   done
done
