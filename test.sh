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

assert 1 "main(){if (1 > 0) return 1; else return 0;}"
assert 0 "main(){if (1 < 0) return 1; else return 0;}"
assert 10 "main(){i = 0; while(i < 10) i=i+1; return i;}"
assert 55 "main(){sum = 0; for(i=1; i<=10; i=i+1) sum = sum + i; return sum;}"
assert 55 "main(){sum=0; i = 0; while(i<=10) {sum = sum+i; i=i+1;} return sum;}"
assert 3 "add(a, b) {return a+b;} main(){return add(1,2);}"
assert 120 "f(n){if(n==0) return 1; return n*f(n-1);} main(){return f(5);}"
assert 10 "main(){a=10; b=&a; return *b;}"
echo OK
