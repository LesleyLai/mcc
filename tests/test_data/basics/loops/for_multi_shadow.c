// RETURN: 10
int main(void)
{
  int i = 0;
  int acc = 0;
  for (int i = 0; i < 10; i += 1) {
    int i = 42;
    acc += i;
  }
  i = acc % 41;
  return i;
}
