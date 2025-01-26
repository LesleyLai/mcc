// RETURN: 1
int main(void)
{
  int acc = 0;
  int y = 1;
  while (y <= 10) {
    int x = 1;
    while (x <= 10) {
      acc += x * y;

      x += 1;
    }

    y += 1;
  }

  // acc = 3025
  return acc % 42; // 1
}
