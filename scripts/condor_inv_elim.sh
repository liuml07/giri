#!/bin/bash

CUR_DIR=`pwd`
echo $CUR_DIR

   cp Invariants-squid-s1.txt Invariants-Combined-1.txt
   cp Invariants-squid-s1.txt Invariants.txt
   j=2;

   while [ $j -le 8 ] ; do
       /home/vadve/ssahoo2/research/sw-bug-diagnosis/llvm-2.6/src/projects/diagnosis/scripts/process-invariants.out Invariants.txt Invariants-squid-s"$j".txt  -o Invariants-Combined-"$j".txt &> /dev/null
      cp Invariants-Combined-"$j".txt Invariants.txt
      j=`expr $j + 1`
   done

   for j in 1 2 4 8 ; do
      /home/vadve/ssahoo2/research/sw-bug-diagnosis/llvm-2.6/src/projects/diagnosis/scripts/process-invariants.out  Invariants-squid-f1.txt  -e Invariants-Combined-"$j".txt  -o Invariants.temp  &> /dev/null

      TOTAL=`wc -l Invariants-squid-f1.txt | sed 's/Invariants-squid-f1.txt//g' | sed 's/ //g'`
      REM=`wc -l Invariants.temp | sed 's/Invariants.temp//g' | sed 's/ //g'`
      TOTAL=`expr $TOTAL / 2`
      REM=`expr $REM / 2` 
      FALSE_POS=`expr $TOTAL - $REM` ;
      PERCENTAGE=`expr $FALSE_POS \* 100 / $TOTAL`
      echo $k $TOTAL $REM     
   done
