#!/bin/bash
assert() {
  expected="$1"
  input="$2"

  ./9cc "$input" > tmp.s
  cc -o tmp tmp.s
  ./tmp
  actual="$?"

  if [ "$actual" = "$expected" ]; then
    echo "$input => $actual"
  else
    echo "$input => $expected expected, but got $actual"
    exit 1
  fi
}

assert 0 "0;"
assert 42 "42;"
assert 21 "5+20-4;"
assert 41 " 12 + 34 - 5;"
assert 47 "5+6*7;"
assert 15 "5*(9-6);"
assert 4 "(3+5)/2;"
assert 7 "-5+3*+4;"
assert 1 "1 == 3<4 + 4 > 0;"
assert 1 "1 != 0;"
assert 0 "1 <= 1 != 1 >= 1;"
assert 6 "a = b = 1 + 2; a = a + b;"
assert 7 "abc = 1 + 2; d = 3; abc = abc + 1 + d;"
assert 4 "abc = 1 + 2; return d = abc + 1; abc = abc + 1 + d;"
assert 1 "if (1 > 0) return 1; else return 0;"
assert 0 "if (1 < 0) return 1; else return 0;"
assert 10 "i = 0; while(i < 10) i=i+1; return i;"
assert 55 "sum = 0; for(i=1; i<=10; i=i+1) sum = sum + i; return sum;"
echo OK
