============
LinuxのGPIO制御
============

:作成日: 2019/2/7
:作成者: Inada

1. はじめに
=======
本ページではLinuxでのGPIOの基本的な制御のメモです。
ITRONでは、あらかじめ定義されている変数(レジスタ)に数値を代入したり数値を読み込むことによりGPIOの制御ができるようになっていました。
Linuxでは、レジスタを直接制御するのではなく、仮想ファイルシステムを読み書きして制御するようになっています。
まずはターミナルでGPIOの仮想ファイルシステムを理解します。
次に実際に制御するコードを書き、最後に簡単なデバイスを作ってみます。

2. 評価環境について
===========
・Raspberry Pi 3 (OS: Raspbian Version 8.0)

・スマホアプリTermiusからWi-Fi経由のSSHで操作

・抵抗、プッシュボタン、LED

3. GPIO 仮想ファイルシステム
==================
ターミナル上でGPIOの仮想ファイルの動作を確認します。

3-1. 仮想ファイルの生成
--------------
最初はgpioディレクトリ内にGPIOのPinに当てはまる仮想ファイルは以下の通り存在しません。

.. code-block:: bash

   pi@raspberrypi ~ $ ls /sys/class/gpio/
   export       gpiochip0    gpiochip128      
   gpiochip100  unexport

GPIO18ピンを制御するには"18"をexportという仮想ファイルに書き込み、Pin 18を制御する仮想ファイルを生成します。

.. code-block:: bash

   pi@raspberrypi ~ $ echo 18 > /sys/class/gpio/export

結果、以下のようにgpio18という仮想ファイルが生成され、ここからPin 18の制御が可能になります。

.. code-block:: bash

   pi@raspberrypi ~ $ ls /sys/class/gpio/
   export    gpiochip0    gpiochip128      
   gpio18    gpiochip100  unexport

3-2. 仮想ファイルの概要
--------------
gpio18ディレクトリ内の仮想ファイルは以下のようになっています。

.. code-block:: bash

   pi@raspberrypi ~ $ ls /sys/class/gpio/gpio18/
   active_low  direction   subsystem   value
   device      edge        uevent

今回は3つ仮想ファイルのみ使います。

・direction・・・ファイルで入出力方向の設定を行うことができます。(in/out/low/high)

・value・・・ファイルで出力レベルの設定、入出力レベルの取得を行うことができます。(High(1)/Low(0))

・edge・・・割り込みタイプの設定を行うことができます。(none/falling/rising/both)

入出力方向の初期値などの詳細はLinuxカーネルドライバ仕様([Ref]_)を参照してください。


3-3. direction - 入出力方向の設定
-------------------------
gpio18のdirection（入出力方向）を取得してみます。

.. code-block:: bash

   pi@raspberrypi ~ $ cat /sys/class/gpio/gpio18/direction
   in

カーネルドライバ仕様([Ref]_)の定義通り初期値の"in"が出力されます。directionはin/outの2状態を持ち、以下の3通りの設定方法があります。

・in・・・入出力方向をin(入力)に設定します。入力レベルの取得を行うことができます。

・low・・・入出力方向をout(出力)に、value(出力レベル)に0(LOW)レベルを設定します。出力レベルの取得/設定を行うことができます。

・high・・・入出力方向をout(出力)に、value(出力レベル)に1(HIGH)レベルを設定します。出力レベルの取得/設定を行うことができます。

ではdirectionをlow(出力方向、出力レベル０)に設定してみます。

.. code-block:: bash

   pi@raspberrypi ~ $ echo low > /sys/class/gpio/gpio18/direction
   pi@raspberrypi ~ $ cat /sys/class/gpio/gpio18/direction
   out

注意：InputもしくはOutput中の入出力の変更は・・

3-4. value - 入出力レベルの設定
----------------------
gpio18のvalue（出力レベル）を取得してみます。3-3で入出力方向を"low(出力方向)"にしたので取得されるのは出力レベルです。

.. code-block:: bash

   pi@raspberrypi ~ $ cat /sys/class/gpio/gpio18/value
   0

現在は何も出力していないので0(Low)が出力されました。GPIO 18PinとGNDにLEDを繋げ、出力レベルを1(High)に設定してLEDが点灯するか確認します。

.. code-block:: bash

   pi@raspberrypi ~ $ echo 1 > /sys/class/gpio/gpio18/value

LEDの点灯が確認できました。

3-5. edge - 割り込みタイプの設定
----------------------
gpio17の仮想ファイルを生成し、gpio17のedge（割り込みタイプ）を取得してみます。

.. code-block:: bash

   pi@raspberrypi ~ $ echo 17 > /sys/class/gpio/export
   pi@raspberrypi ~ $ cat /sys/class/gpio/gpio17/edge
   none

初期値のnoneが出力されました。noneは割り込みの検出を行いません。割り込みタイプは以下の通りです。

・falling・・・立ち下がりエッジで割り込みの検出を行います。（今回は使わない。）

・rising・・・立ち上がりエッジで割り込みの検出を行います。（今回は使わない。）

・both・・・立ち下がり、立ち上がり両方のエッジで割り込みの検出を行います。

今回はnoneとbothのみ使用し、bothを設定するPinを割り込み検出専用Pinとします。では割り込みタイプを"both"に設定します。

.. code-block:: bash

   pi@raspberrypi ~ $ echo both > /sys/class/gpio/gpio17/edge
   pi@raspberrypi ~ $ cat /sys/class/gpio/gpio17/edge
   both

Push Buttonを使って割り込みの動作確認をします。
ボタンの一方の端子をヘッダーの 3.3V 出力ピンにつなぎ、もう一方の端子とGPIO 17を抵抗を通して接続します。
ボタンが期待とおりに動作しているかを確認するために、Value ファイルを読み込み、0 であることを確認します。
次に、ボタンを押下している間は Value が 1 になり、ボタンを離すと 0 になることを確認します。
これで、ボタンを押下したときと離したときに割り込みを受け取ることができます。

.. code-block:: bash

   pi@raspberrypi ~ $ cat /sys/class/gpio/gpio17/value
   0
   pi@raspberrypi ~ $ cat /sys/class/gpio/gpio17/value
   1
   pi@raspberrypi ~ $ cat /sys/class/gpio/gpio17/value
   0

**ここまででGPIOの仮想ファイルの扱いは終了です。割り込みを使う準備もできたので次章で割り込みを使った簡単なデバイスを作ってみます。**

4. GPIO制御のソースコードサンプル
====================
まずはソースコードにて、これまで説明してきた処理をclass(構造体)で実装します。

.. code-block:: python

   import RPi.GPIO as GPIO
   
   class ControlGPIO:
   
       def __init__(Pin):
           # コンストラクタ
           # cならここで/sys/class/gpio/exportで仮想ファイル作る
           GPIO.setmode(GPIO.BCM)
   
       def makeInput(self, x):
           W1, W2 = self.params['W1'], self.params['W2']
           b1, b2 = self.params['b1'], self.params['b2']
       
           a1 = np.dot(x, W1) + b1
           z1 = sigmoid(a1)
           a2 = np.dot(z1, W2) + b2
           y = softmax(a2)
           
           return y
           
       def makeOutput(self, x, t):
           y = self.predict(x)
           
           return cross_entropy_error(y, t)
       
       def setValue(self, x, t):
           y = self.predict(x)
           y = np.argmax(y, axis=1)
           t = np.argmax(t, axis=1)
           
           accuracy = np.sum(y == t) / float(x.shape[0])
           return accuracy
           
       def getValue(self, x, t):
           loss_W = lambda W: self.loss(x, t)
           
           grads = {}
           grads['W1'] = numerical_gradient(loss_W, self.params['W1'])
           grads['b1'] = numerical_gradient(loss_W, self.params['b1'])
           grads['W2'] = numerical_gradient(loss_W, self.params['W2'])
           grads['b2'] = numerical_gradient(loss_W, self.params['b2'])
           
           return grads
           
       def setIntrMode(self, x, t):
           W1, W2 = self.params['W1'], self.params['W2']
           b1, b2 = self.params['b1'], self.params['b2']
           grads = {}
           
           batch_num = x.shape[0]
           
           # forward
           a1 = np.dot(x, W1) + b1
           z1 = sigmoid(a1)
           a2 = np.dot(z1, W2) + b2
           y = softmax(a2)
           
           # backward
           dy = (y - t) / batch_num
           grads['W2'] = np.dot(z1.T, dy)
           grads['b2'] = np.sum(dy, axis=0)
           
           dz1 = np.dot(dy, W2.T)
           da1 = sigmoid_grad(a1) * dz1
           grads['W1'] = np.dot(x.T, da1)

5. リアルタイムの為替レート表示デバイス
=====================
リアルタイムの為替レートを毎秒表示更新しボタンが押されている間のみ最新24時間の平均レートを表示するデバイスを作ります。

・要件定義1・・・4-Digit 7-Segment LEDをGPIO制御し、少数第一位までの為替レートを毎秒表示する。

・要件定義2・・・Push Buttonが押されたら（割り込みを立ち上がりエッジで検出したら）、リアルタイムの為替レート表示を終了して平均レートを表示する。
Push Buttonが離されたら（割り込みを立ち下がりエッジで検出したら）、要件定義1の動作へ戻る。

実装上の前提：direction(入出力方向)がinで割り込みを設定するPinを割り込み専用GPIO Pinとしoutへは変更しません。

解説全体に関する補足
==========
"/sys/class/gpio/export"に対してのPin番号の書き込みはC++だとfopen⇒fprintf⇒fflush⇒fcloseで実現できます。
適宜コンパイラ言語に置き換えて処理速度を改善して最適化する必要がありますが
処理の全体の流れをつかみやすくするため、組み込みのプログラムですがPythonで作成、学習しました。
   
   
.. [Ref] 引用資料


1. Linuxカーネルドライバ仕様 `1 link`_.

.. _1 link: https://manual.atmark-techno.com/armadillo-4x0/armadillo-400_series_software_manual_ja-1.3.0/ch08.html

2. Raspberry PiのGPIO制御方法を確認する `2 link`_.

.. _2 link: https://tool-lab.com/2013/12/raspi-gpio-controlling-command-1/


3. Linux GPIOを通して割り込みを取得する `3 link`_.

.. _3 link: https://jp.linux.com/news/linuxcom-exclusive/415008-lco2014032602