// RETURN: 55
int main(void)
{
  int x = 10;
  int y = 0;
  while (x) {
    y += x;
    x -= 1;
  }
  return y;
}
