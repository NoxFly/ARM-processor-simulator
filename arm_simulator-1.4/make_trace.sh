#!/bin/sh

if [ ! -x ./arm_simulator -a ! -d student ]
then
	echo missing simulator or student version
	exit 1
fi

rm -r Examples/trace
mkdir Examples/trace
for file in Examples/*.s
do
  base=`expr "$file" : 'Examples/\(.*\)\.s'`
  ./arm_simulator --gdb-port 58000 --trace-registers --trace-memory \
                  >Examples/trace/trace_$base &
  gdb-multiarch -ex "file Examples/$base" -x gdb_commands --batch
done
zip -9 -j student/traces.zip Examples/trace/*
