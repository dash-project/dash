

int main(int argc, char **argv)
{
  int val = 0;
  int res = __sync_add_and_fetch(&val, 1);

  if (res != 1) return 1;

  return 0;
}
