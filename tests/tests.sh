#!/bin/bash

for i in pass*.json; do
  echo -n "$i: "
  ./test < "$i" && echo "OK" || echo "FAIL -- Wrong result, expected PASS"
done

for i in fail*.json; do
  echo -n "$i: "
  ./test < "$i" > /dev/null && echo "PASS -- Wrong result, expected FAIL" || echo "OK"
done

