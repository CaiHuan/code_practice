#include <iostream>

class Base {
public:
  void f(double i) {
    std::cout << i << std::endl;
  }
};

class Derived : public Base {
  using Base::f;
};

int main() {
  
  return 0;
}