// RETURN: 10
int main(void)
{
  int acc = 0;
  for (int i = 4;; i = i - 1) {
    acc += i;
    if (i == 1) break;
  }
  return acc;
}
