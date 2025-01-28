// RETURN: 45
int main(void)
{
  int i = 0;
  int acc = 0;
  while (1) {
    if (i >= 10) break;

    acc += i;
    i += 1;
  }

  return acc;
}
