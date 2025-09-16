# Сломанные символические ссылки

Программе в аргументах командной строки передаются пути к файлам. Каждый путь `PATH` следует напечатать на stdout в следующем виде:
- `PATH (missing)`, если файл не существует
- `PATH (broken symlink)`, если файл является символической ссылкой, указывающей на несуществующий путь
- `PATH` во всех остальных случаях

Например:

```bash
$ ln -s /non/existent symlink_bad
$ ln -s regular symlink_good
$ touch regular
$ ./yourprog regular symlink_good symlink_bad /non/existent
regular
symlink_good
symlink_bad (broken symlink)
/non/existent (missing)
```

Для обращения к аргументам командной строки используйте аргументы, передаваемые в `main`:

```cpp
int main(int argc, const char* argv[]) {
    // Аргумент с номером 0 нас обычно не интересует
    for (int i = 1; i < argc; ++i) {
        // argv[i] – i-й аргумент командной строки
        DoSmth(argv[i]);
    }
}
```

Для решения вам могут пригодиться функции семейства [`*stat`](https://www.man7.org/linux/man-pages/man2/stat.2.html).
