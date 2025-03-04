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

assert 1 "int main(){if (1 > 0) return 1; else return 0;}"
assert 0 "int main(){if (1 < 0) return 1; else return 0;}"
assert 10 "int main(){int i; i=0; while(i < 10) i=i+1; return i;}"
assert 55 "int main(){int Sum_0; Sum_0 = 0; int i; for(i=1; i<=10; i=i+1) Sum_0 = Sum_0 + i; return Sum_0;}"
assert 55 "int main(){int sum; sum=0; int i; i = 0; while(i<=10) {sum = sum+i; i=i+1;} return sum;}"
assert 3 "int add(int a, int b) {return a+b;} int main(){return add(1,2);}"
assert 120 "int f(int n){if(n==0) return 1; return n*f(n-1);} int main(){return f(5);}"
assert 3 "int main(){int a; int *b; b = &a; *b = 3; return a;}"
assert 8 "int main(){int *a; return sizeof(a);}"
assert 10 "int main(){int a[3]; *(a+2) = 10; int *p; p = a+2; return *p;}"
assert 10 "int main(){int a[3]; a[2] = 10; return a[2];}"
assert 10 "int main(){int a[3][3]; a[2][2] = 10; return a[2][2];}"
echo OK
