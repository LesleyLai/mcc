// RETURN: 22
int main(void)
{
  int x = 1;
  int acc = 0;
  do {
    x = x * 2;
    if (x == 8) continue;
    acc += x;
  } while (x < 11);

  return acc;
}
