#! /bin/bash

if ! [[ -x wordcount ]]; then
    echo "wordcount executable does not exist"
    exit 1
fi

../tester/run-tests.sh $*


