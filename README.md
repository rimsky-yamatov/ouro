# Ouro

自分自身をコンパイルできる**最小のプログラミング言語**です。名前はウロボロス(自分の尾を噛む蛇)に由来します。

- 型は 64bit 整数 1 種類だけ
- 関数、`if`、`while`、グローバル配列、バイト/ワード単位のメモリアクセスのみ
- x86-64 Linux 用の静的 ELF を直接出力(アセンブラもリンカも不要)

```
fn gcd(a, b) {
  while (b) { var t; t = b; b = a % b; a = t; }
  return a;
}
fn main() { putc('0' + gcd(12, 18)); putc('\n'); return 0; }
```

言語仕様は [docs/spec.md](docs/spec.md) を参照してください。

## ステータス

| 段階 | 内容 | 状態 |
|---|---|---|
| stage0 | C で書いたブートストラップ用コンパイラ `ouroc0` | 実装済み |
| stage1 | Ouro で書いたコンパイラ `ouroc.ou` | 未着手 |
| stage2 | 固定点の検証(`ouroc1` と `ouroc2` のバイト一致) | 未着手 |

## ビルド

CMake か make のどちらかで、C99 コンパイラだけでビルドできます。

```
cmake -S . -B build && cmake --build build --config Release
# または
make
```

## 使い方

```
ouroc0 < prog.ou > prog
chmod +x prog
./prog
```

`ouroc0` はクロスコンパイラです。どの OS・CPU で動かしても、出力は同じ x86-64 Linux 用 ELF になります(実行できるのは x86-64 Linux のみ)。

## テスト

```
make test                       # または: ctest --test-dir build -C Release
bash tests/run.sh path/to/ouroc0
```

- `tests/cases/*.ou`: コンパイルして実行し、標準出力と終了コードを比較(x86-64 Linux 以外では ELF の検証のみ)
- `tests/errors/*.ou`: コンパイルエラーのメッセージを比較

## CI とリリース

- `main` への push と PR で、Linux / macOS / Windows(x86-64・arm64)の 6 構成でビルドとテストを実行します。さらに全ホストで出力がバイト単位で一致することを確認します。
- `v*` のタグを push すると、6 構成のバイナリをビルドし、コミット履歴からリリースノートを生成して GitHub Release に添付します(SHA256SUMS 付き)。

```
git tag v0.1.0
git push origin v0.1.0
```

コミットメッセージは [Conventional Commits](https://www.conventionalcommits.org/)(`feat:`, `fix:`, `docs:` など)に従うと、リリースノートが種類別に整理されます。

## ライセンス

[MIT](LICENSE)
