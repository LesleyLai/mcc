// RETURN: 16
int main(void)
{
  int x = 1;
  do {
    x = x * 2;
    if (x > 10) break;
  } while (1);

  return x;
}
