// RETURN: 30
int main(void)
{
  int sum = 0;
  // 2 + 4 + 6 + 8 + 10
  for (int i = 0; i < 10;) {
    i = i + 1;
    if (i % 2) continue;
    sum = sum + i;
  }
  return sum;
}
