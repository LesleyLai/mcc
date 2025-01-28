// RETURN: 25
int main(void)
{
  int i = 0;
  int acc = 0;
  // 1 + 3 + 5 + 7 + 9
  while (i < 10) {
    i += 1;
    if (i % 2 == 0) continue;
    acc += i;
  }

  return acc;
}
