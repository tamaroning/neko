#include <stdio.h>

int main(void) {
  int a;
  scanf("%d", &a);

  // taint use
  for (int i = 0; i < a; i++) {
    // do nothing
    printf("Hello, World!\n");
  }
}
