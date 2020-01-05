void __declspec(noinline) __declspec(noreturn) NoReturn() {
  __asm int 3;
}

void __declspec(noinline) __declspec(noreturn) __declspec(naked) Parent() {
  __asm call NoReturn;
}

int main() {
  Parent();
}
