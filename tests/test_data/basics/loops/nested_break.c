// RETURN: 18
int main(void)
{
  int acc = 0;
  for (int y = 1; y < 10; y += 1) {
    for (int x = 1; x < 10; x += 1) {
      if (x == y % 2) break;
      acc += x * y;
    }
  }

  // acc = 900
  return acc % 42;
}
