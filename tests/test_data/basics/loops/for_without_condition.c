// RETURN: 42
int main(void)
{
  for (int i = 4;; i = i - 1)
    if (i == 1) return 42;
  return 0;
}
