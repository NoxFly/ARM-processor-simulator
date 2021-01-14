#!/bin/sh

if [ ! -x ./arm_simulator -a ! -d student ]
then
	echo missing simulator or student version
	exit 1
fi

rm -r tests/trace
mkdir tests/trace
for file in tests/*.s
do
  base=`expr "$file" : 'tests/\(.*\)\.s'`
  ./arm_simulator --gdb-port 58000 --trace-registers --trace-memory \
                  >tests/trace/trace_$base &
  gdb-multiarch -ex "file tests/$base" -x gdb_commands --batch
done
zip -9 -j student/traces.zip tests/trace/*
