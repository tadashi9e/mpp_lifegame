# -*- coding: utf-8; mode:python -*-
# Title: ライフゲームのパターンファイルフォーマットについて
# Tag: Python, ライフゲーム
'''
## 目的

ライフゲームの分野においては、先人達が多くの興味深いパターンを発見しています。

ライフゲームプログラムを実行するときにそうしたパターンを画面上から毎回投入するのは大変です。パターンをテキストファイルから取り込んだり、あるいはテキストファイルに書き出したりできるようにしたいと思いました。

## いくつかのパターンファイルフォーマット

ライフゲームのパターンファイルとして使われているフォーマットにどんなものがあるか調べてみました。

* Life 1.05 (*.LIF/*.LIFE)
* Life 1.06 (*.LIF/*.LIFE)
* XLife (*.LIF/*.LIFE)
* RLE (*.RLE)
* MCLife (*.MCL)

.LIF あるいは .LIFE の拡張子を採用したものが多くて、拡張子を見ただけではどのフォーマットか区別がつかなそうですね。

### Life 1.05 (*.LIF/*.LIFE)

生きているセルを '*' で、死んでいるセルを '.' でテキストで表した、直感的に分かりやすい形式です。
テキストエディタの画面を超えるサイズになってくると直感的な分かりやすさは損なわれるかもしれません。

### Life 1.06 (*.LIF/*.LIFE)

生きているセルの X 座標と Y 座標のペアからなる行を並べたものです。プログラムで読み書きするにはとても扱いやすい形式です。

### XLife (*.LIF/*.LIFE)

XLife は 1989 年からある歴史あるコマンドです。その xlife で使われているファイルフォーマットです。Life 1.05 をさらに強力にした感じのフォーマットです。

XLife のホームページを見るとパターンファイルが大量に置いてあって、これらを利用できると便利そうです。

### RLE (*.RLE)

Run Length Encoded フォーマットだそうです。ランレングス法を使っているので巨大なパターンを格納するのに適しているとのこと。

### MCLife (*.MCL)

Life 1.05 と RLE を組み合わせたフォーマットだそうです。

ざっと見て、一番簡単で分かりやすそうな Life 1.05 フォーマットの読み書きを実装してみることにしました。直感的に分かりやすいので、ちょっとした画面表示にも使えそうです。


## Life 1.05 フォーマットの読み書き

Life 1.05 ファイルは、改行コードは CRLF で、'.' と '*' でパターンを描いたもの、というのが基本です。空行は無視することになっています。
さらに細かい仕様として '#D' で始まる行は説明文を書く、だとか '#P' で始まる行はオフセット座標を示す、だとか、挙句は '#R' で近傍セルのルールを記述できます、だとかいろいろあるのですが、そのあたりはざっくり無視して '#' で始まる行はコメント扱いにしてしまいます。

まずは、ストリームからの読み込みから。
vvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvv'''
import sys

def _parse_line(line, x0, y):
    u'''与えられた行に含まれる '*' を生きている状態として返す。
    '''
    return {(x0 + i, y) for i in range(len(line)) if line[i] == '*'}

def read_life_105(x0, y0, stream):
    u'''Life 1.05 形式のファイルの読み込み
    --------
    x0 : int
        読み込み開始位置(X 座標)
    y0 : int
        読み込み開始位置(Y 座標)
    stream : stream
        読み込み対象ストリーム
    '''
    dots = set()
    y = y0
    for line in stream:
        if line.startswith('#'):
            continue
        if not line.rstrip('\r\n'):
            continue
        dots |= _parse_line(line, x0, y)
        y += 1
    return dots
'''^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
同じくストリームへの書き出しを定義します。デフォルトの出力先は標準出力なので、簡単なデバッグ出力に用いることができます。
vvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvv'''
def write_life_105(dots, stream=sys.stdout):
    u'''Life 1.05 形式でのストリームへの書き出し
    --------
    dots : set[Tuple[int, int]]
        「生」セルの座標のセット
    stream : stream
        書き込み対象ストリーム
    '''
    x_min = min([x for (x, _) in dots])
    y_min = min([y for (_, y) in dots])
    x_max = max([x for (x, _) in dots])
    y_max = max([y for (_, y) in dots])
    print('#P {} {}'.format(x_min, y_min))
    for y in range(y_min, y_max + 1):
        line = ''.join([('*' if (x, y) in dots else '.')
                        for x in range(x_min, x_max + 1)])
        stream.write(line + '\n')
'''^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
上で定義したのはストリーム相手の操作なので、ファイルアクセス用の関数として定義しなおしておきます。
vvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvv'''
def read_life_105_file(x0, y0, from_path):
    u'''
    --------
    x0 : int
        読み込み開始位置(X 座標)
    y0 : int
        読み込み開始位置(Y 座標)
    from_path : str
        読み込み対象ファイル
    '''
    with open(from_path, 'r') as f:
        return read_life_105(x0, y0, f)

def write_life_105_file(dots, to_path):
    u'''Life 1.05 形式のファイルへの書き込み
    --------
    dots : set[Tuple[int, int]]
        「生」セルの座標のセット
    stream : str
        書き込み対象ストリーム
    '''
    with open(to_path, 'w', newline='\r\n') as f:
        f.write('#Life 1.05\n')
        write_life_105(dots, f)
'''^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
## まとめ

ライフゲームのパターンファイルに使われるファイルフォーマットを調べてみました。

Life 1.05 形式のパターンファイルの読み込み関数を書いてみました。
同じく、Life 1.05 形式で表示・ファイル出力する関数を書きました。

'''
