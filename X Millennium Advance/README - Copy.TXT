------------------------------------------------------------------------------

   X millennium Advance Demo version (ver0.20)

------------------------------------------------------------------------------



■未サポート機能
　・一部のスクリーンモード(width,10 及び width,20)
　・フロッピーへの書き込み
　・キーボード
　・SIO
　・メニュー(各種設定 / ディスク入れ換え)



■GBAイメージの作成。

　1. advtool.exe xmiladv.hdr のあるフォルダに以下のファイルをコピーします。

　　　iplrom.x1t font0808.x1 font0816.x1 font1616.x1

　2. 実行に挿入するフロッピーイメージ(d88形式のみ)を FDD0.D88 FDD1.D88に
　　リネームして、同じフォルダにコピーします。

　3. advtool.exeを実行すると xmiladv.gba が作成されます。



■実行

　GBA実機
　　xmiladv.gbaはNintendoよりライセンスを受けていません。
　　この為、実機で動作させる為のNintendoロゴは含まれていません。
　　実機で動作させる為には、各自の責任でNintendoロゴを書き込んで下さい。

　　…というかGBAのPDSを実機で動かせる人には説明要らないっしょ…


　Visual Boy Advance
　　実機BIOSを使用しない場合であれば、パッチなしで動作します。

　　また VBAで動作させる場合には、

　　　Options→Frame skipで 0,2,4,6,8のいずれかを選択し Automaticを外し、
　　　Options→Filter→Interframe Blending→Motion Blurを選択

　　して下さい。



■操作
　方向キー,Ａ,Ｂ … X1側のJOYPAD1に相当します。
　Ｌキー         … Ｌキーを押しながら 方向キーを押すと、
　　　　　　　　　　画面表示範囲を移動する事が出来ます。

　その他の機能はございません。

　故に、JOYPAD1のみで、かつディスク入れ換えを行なわないアプリケーションのみ
実行することが出来ます。



■結論
　NDSが非常に楽しみである。



-------------------------------------------------------------------------------

  X1 turbo spec

  Zilog Z80A, Z80DMA, Z80CTC and Z80SIO

  CPU:    Z80A 4MHz / 80C49 x 2
  Memory: 64KB
  BIOS:   32KB
  VRAM:    6KB - Text
          96KB - Graphic

           Text            Graphic
          40x25 or 40x12   320x100x8, 320x200x4 or 320x400x2
          80x25 or 80x12   640x100x4, 640x200x2 or 640x400x1
          40x20 or 40x10   underline only
          80x20 or 80x10   underline only

  FONT:     8KB ROM (8x8 ANK)
          256KB ROM (Japanese Charactors)
            6KB RAM (Programable Charactor Generator)

  Sound:  AY-3-8910 (Programable Sound Generator)

