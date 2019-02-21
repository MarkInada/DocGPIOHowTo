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
次に実際に制御するコードを書いてみます。

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

.. blockdiag::

   blockdiag
   {
      GPIO_18 ->  220Ω -> Diode -> GND;
   }


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

.. blockdiag::

   blockdiag
   {
      3.3V出力 -> 10kΩ -> PushButton -> 220Ω -> GPIO_17;
   }


.. image:: _static/pic/image1.jpeg
   :scale: 100%
   :align: center

**ここまででGPIOの仮想ファイルの扱いは終了です。割り込みを使う準備もできたので次章で割り込みを使った簡単な制御コードを作ってみます。**

4. GPIO制御のソースコード解説
==================

手順3で使用した回路を使ってPush Buttonを押している間、LEDが点灯するコードをC++で書いてみます。
GPIOファイル制御を行うクラスをまず作成し、それらをインスタンスメソッドとして使って割り込み検出からLED制御まで行います。

4-1. 仮想ファイルの生成
--------------

手順3-1で行った仮想ファイルの生成をGPIOクラスのインスタンス生成時にコンストラクタで行います。
GPIO番号に紐付いたディレクトリを確認し、存在しなければexportファイルを使ってGPIOファイル群を生成します。
生成されたdirection, valueファイルはm_gpioDirectionFileとm_gpioValueFileで保持し、各Pinのインスタンスからデータの設定および取得できるようにします。

.. code-block:: c

   class GPIO
   {
   public:
   ...

     GPIO(int pin)
       : m_pin(pin)
     {
       char FileName[64];
   
       snprintf(FileName, sizeof(FileName), GPIO_DIRECTORY_PATH, pin);
       if (access(FileName, F_OK) != 0)
       {
         FILE* ExportFile = fopen(GPIO_EXPORT_PATH, "w");
         if (ExportFile == NULL)
         {
           exit(EXIT_FAILURE);
         }
         fprintf(ExportFile, "%d", pin);
         fflush(ExportFile);
         fclose(ExportFile);
       }
   
       snprintf(FileName, sizeof(FileName), GPIO_DIRECTION_PATH, pin);
       m_gpioDirectionFile = open(FileName, O_WRONLY | O_CLOEXEC);
   
       snprintf(FileName, sizeof(FileName), GPIO_VALUE_PATH, pin);
       m_gpioValueFile = open(FileName, O_RDWR | O_CLOEXEC);
   
       if ((m_gpioDirectionFile < 0) || (m_gpioValueFile < 0))
       {
         exit(EXIT_FAILURE);
       }
     }

4-2. ファイルの設定と取得
---------------

GPIO_18 の出力とGPIO_17の入力と割り込みの取得を行うので、以下の通り3つの制御用のメソッドを準備しました。

.. code-block:: c

   class GPIO
   {
   public:
   ...
   
     void setOutput(bool high)
     {
       if (high)
       {
         write(m_gpioDirectionFile, "high", 4);
       }
       else
       {
         write(m_gpioDirectionFile, "low", 3);
       }
     }
   
     bool getValue(void)
     {
       unsigned char value;
   
       lseek(m_gpioValueFile, SEEK_SET, 0);
       read(m_gpioValueFile, &value, 1);
   
       if (value == '0')
       {
         return false;
       }
   
       return true;
     }
   
     int setEdge(void)
     {
       char FileName[64];
   
       snprintf(FileName, sizeof(FileName), GPIO_EDGE_PATH, m_pin);
       int EdgeFile = open(FileName, O_WRONLY);
       if (EdgeFile < 0)
       {
         exit(EXIT_FAILURE);
       }
       write(EdgeFile, "both", 4);
       close(EdgeFile);
   
       return m_gpioValueFile;
     }


4-3. 処理概要
---------

まず、4-1, 4-2で作成したクラスのインスタンスをGPIO_17とGPIO_18それぞれ生成します。
unique_ptrはmain文を抜けるとメモリは自動的に解放されます。開始時のGPIO_17の入力レベルをcurrentValueで保持しておきます。
while文内のpoll(ファイルディスクリプタのイベント待ち)で割り込みを監視します。while文は5分以上経過後の割り込み検出で抜けるようにタイマを設定しておきます。
pollの設定時にGPIO_17のEdgeファイルに"both"を書き込み、立ち下がり立ち上がり両方のエッジで割り込みの検出を行います。
pollに設定するのはファイルディスクリプタ（今回はGPIO_17のValueファイル）と要求イベント(今回はPOLLPRIで緊急イベントを要求)し、
イベントの検出までpollで待ち受けます。Push Buttonを押したり離したりすると割り込みを検出してpollを抜けます。
GPIO_17の入力レベルに変化があれば、50msまって再度確認し問題なければ立ち上がり検出ならGPIO_18をHighに設定。立ち下り検出ならLowに設定してLEDを制御します。
While文を抜けて処理を終えるときはLEDを消灯します。

.. code-block:: c

   int main (void)
   {
     // Create instances for each GPIO pin
     std::unique_ptr<GPIO> m_outputPin = std::unique_ptr<GPIO>(new GPIO(GPIO_NUMBER_FOR_TEST));
     std::unique_ptr<GPIO> m_interruptPin = std::unique_ptr<GPIO>(new GPIO(GPIO_NUMBER_FOR_INTERRUPT));
   
     bool currentValue = m_interruptPin->getValue();
     bool nextValue;
   
     // Test for 5 min
     std::chrono::system_clock::time_point start = std::chrono::system_clock::now();
     while (std::chrono::duration_cast<std::chrono::seconds>
       ((std::chrono::system_clock::now())-start).count() < 300)
     { 
       // Set poll for interruption
       struct pollfd pfd;
       pfd.fd = m_interruptPin->setEdge();
       pfd.events = POLLPRI;
   
       // Polling
       if (poll(&pfd, 1, -1) <= 0)
       {
         exit(EXIT_FAILURE);
       }
   
       if (pfd.revents & POLLPRI)
       {
         nextValue = m_interruptPin->getValue();
         if (currentValue != nextValue)
         {
            // Wait for debounce/chattering time 50ms. This code doesn't assume continuous hits of push button.
            std::this_thread::sleep_for(std::chrono::milliseconds(50));
         
            if (nextValue == m_interruptPin->getValue())
            {
              currentValue = nextValue;
              if (nextValue)
              {
                // Turn LED on
                m_outputPin->setOutput(true);
              }
              else
              {
                // Turn LED off
                m_outputPin->setOutput(false);
              }
            }
         }
       }
     }
   
     // Turn LED off at the end
     m_outputPin->setOutput(false);
   }


5. 完成したソースコードサンプル
=================

完成したソースコードは以下。ラズパイにてviで実装後”g++ -std=c++11 gpio.cpp”でコンパイルして動作を確認しました。ボタン連打は非対応。

.. code-block:: c

   ////////////////////////////////////////////////////////////////////////////////////////////////////////
   ///
   /// For simple testing GPIO.
   /// How to Compile: g++ -std=c++11 gpio.cpp
   ///
   ////////////////////////////////////////////////////////////////////////////////////////////////////////
   
   #include <stdio.h>
   #include <stdlib.h>
   #include <unistd.h>
   #include <fcntl.h>
   #include <poll.h>
   #include <memory>
   #include <chrono>
   #include <thread>
   
   #define GPIO_EXPORT_PATH "/sys/class/gpio/export"
   #define GPIO_DIRECTORY_PATH "/sys/class/gpio/gpio%d"
   #define GPIO_DIRECTION_PATH "/sys/class/gpio/gpio%d/direction"
   #define GPIO_VALUE_PATH "/sys/class/gpio/gpio%d/value"
   #define GPIO_EDGE_PATH "/sys/class/gpio/gpio%d/edge"
   
   // Set GPIO pin number that you will use
   #define GPIO_NUMBER_FOR_TEST 18
   #define GPIO_NUMBER_FOR_INTERRUPT 17
   
   class GPIO
   {
   public:
     ////////////////////////////////////////////////////////////////////////////////////////////////////////
     /// @fn GPIO
     ///
     /// Create GPIO file using /sys/class/gpio/export. Then open direction file and value file
     ///
     /// @param Pin   a gpio pin number
     ////////////////////////////////////////////////////////////////////////////////////////////////////////
     GPIO(int pin)
       : m_pin(pin)
     {
       char FileName[64];
   
       snprintf(FileName, sizeof(FileName), GPIO_DIRECTORY_PATH, pin);
       if (access(FileName, F_OK) != 0)
       {
         FILE* ExportFile = fopen(GPIO_EXPORT_PATH, "w");
         if (ExportFile == NULL)
         {
           exit(EXIT_FAILURE);
         }
         fprintf(ExportFile, "%d", pin);
         fflush(ExportFile);
         fclose(ExportFile);
       }
   
       snprintf(FileName, sizeof(FileName), GPIO_DIRECTION_PATH, pin);
       m_gpioDirectionFile = open(FileName, O_WRONLY | O_CLOEXEC);
   
       snprintf(FileName, sizeof(FileName), GPIO_VALUE_PATH, pin);
       m_gpioValueFile = open(FileName, O_RDWR | O_CLOEXEC);
   
       if ((m_gpioDirectionFile < 0) || (m_gpioValueFile < 0))
       {
         exit(EXIT_FAILURE);
       }
     }
   
     ////////////////////////////////////////////////////////////////////////////////////////////////////////
     /// @fn setOutput
     ///
     /// Write high or low to direction file.
     ///
     /// @high   Set output level whether high or not
     ////////////////////////////////////////////////////////////////////////////////////////////////////////
     void setOutput(bool high)
     {
       if (high)
       {
         write(m_gpioDirectionFile, "high", 4);
       }
       else
       {
         write(m_gpioDirectionFile, "low", 3);
       }
     }
   
     ////////////////////////////////////////////////////////////////////////////////////////////////////////
     /// @fn getValue
     ///
     /// Get value from value file
     ///
     /// @return value is whether high or not
     ////////////////////////////////////////////////////////////////////////////////////////////////////////
     bool getValue(void)
     {
       unsigned char value;
   
       lseek(m_gpioValueFile, SEEK_SET, 0);
       read(m_gpioValueFile, &value, 1);
   
       if (value == '0')
       {
         return false;
       }
   
       return true;
     }
   
     ////////////////////////////////////////////////////////////////////////////////////////////////////////
     /// @fn setEdge
     ///
     /// Set interruption type (both) to Edge file
     ///
     /// @return Value File
     ////////////////////////////////////////////////////////////////////////////////////////////////////////
     int setEdge(void)
     {
       char FileName[64];
   
       snprintf(FileName, sizeof(FileName), GPIO_EDGE_PATH, m_pin);
       int EdgeFile = open(FileName, O_WRONLY);
       if (EdgeFile < 0)
       {
         exit(EXIT_FAILURE);
       }
       write(EdgeFile, "both", 4);
       close(EdgeFile);
   
       return m_gpioValueFile;
     }
   
   private:
     int m_pin;
     int m_gpioDirectionFile;
     int m_gpioValueFile;
   };
   
   int main (void)
   {
     // Create instances for each GPIO pin
     std::unique_ptr<GPIO> m_outputPin = std::unique_ptr<GPIO>(new GPIO(GPIO_NUMBER_FOR_TEST));
     std::unique_ptr<GPIO> m_interruptPin = std::unique_ptr<GPIO>(new GPIO(GPIO_NUMBER_FOR_INTERRUPT));
   
     bool currentValue = m_interruptPin->getValue();
     bool nextValue;
   
     // Test for 5 min
     std::chrono::system_clock::time_point start = std::chrono::system_clock::now();
     while (std::chrono::duration_cast<std::chrono::seconds>
       ((std::chrono::system_clock::now())-start).count() < 300)
     { 
       // Set poll for interruption
       struct pollfd pfd;
       pfd.fd = m_interruptPin->setEdge();
       pfd.events = POLLPRI;
   
       // Polling
       if (poll(&pfd, 1, -1) <= 0)
       {
         exit(EXIT_FAILURE);
       }
   
       if (pfd.revents & POLLPRI)
       {
         nextValue = m_interruptPin->getValue();
         if (currentValue != nextValue)
         {
            // Wait for debounce/chattering time 50ms. This code doesn't assume continuous hits of push button.
            std::this_thread::sleep_for(std::chrono::milliseconds(50));
         
            if (nextValue == m_interruptPin->getValue())
            {
              currentValue = nextValue;
              if (nextValue)
              {
                // Turn LED on
                m_outputPin->setOutput(true);
              }
              else
              {
                // Turn LED off
                m_outputPin->setOutput(false);
              }
            }
         }
       }
     }
   
     // Turn LED off at the end
     m_outputPin->setOutput(false);
   }

6. システムコールpollで返されるPOLLINとPOLLPRIの違い
====================================

本コードではfeventで返されるビットはPOLLPRI(緊急データ)のみチェックしている。POLLINは読み出し可能なデータがあることを示すが、
Edgeにnoneを書き込んでテストしたがValueにHigh/Lowの変化があってもpollは何もデータを検出できずにポーリングし続けた。


.. [Ref] 引用資料


1. Linuxカーネルドライバ仕様 `1 link`_.

.. _1 link: https://manual.atmark-techno.com/armadillo-4x0/armadillo-400_series_software_manual_ja-1.3.0/ch08.html

2. Raspberry PiのGPIO制御方法を確認する `2 link`_.

.. _2 link: https://tool-lab.com/2013/12/raspi-gpio-controlling-command-1/


3. Linux GPIOを通して割り込みを取得する `3 link`_.

.. _3 link: https://jp.linux.com/news/linuxcom-exclusive/415008-lco2014032602