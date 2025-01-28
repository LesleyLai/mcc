// RETURN: 30
int main(void)
{
  int acc = 0;
  // 0 + 2 + 4 + 6 + 8 + 10
  for (int i = 0; i <= 10; i += 1) {
    if (i % 2) continue;
    acc += i;
  }
  return acc;
}
