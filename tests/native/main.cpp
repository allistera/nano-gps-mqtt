#include <cstdio>

void runPayloadTests();
void runRetryBackoffTests();
void runPublishScheduleTests();

int main() {
  runPayloadTests();
  runRetryBackoffTests();
  runPublishScheduleTests();
  std::puts("native tests passed");
  return 0;
}
