#include <cstdio>

void runPayloadTests();
void runRetryBackoffTests();

int main() {
  runPayloadTests();
  runRetryBackoffTests();
  std::puts("native tests passed");
  return 0;
}
