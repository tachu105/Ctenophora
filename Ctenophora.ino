#include <Adafruit_NeoPixel.h>  //NumPixel制御用ライブラリ

#define NUM_PIXELROW 14 //LEDテープの本数（並列）
#define NUM_PIXELS 45  //LEDテープ1本あたりのLED数
#define MAX_BULLETS 100 // 同時に存在できる弾の最大数
#define DELAY_TIME 15 //1フレームあたりの最小遅延時間(更新時間)

//NeoPixelのインスタンス生成 （第2引数はNeoPixelのピン番号）
Adafruit_NeoPixel neoPixels[NUM_PIXELROW] = {
  Adafruit_NeoPixel(NUM_PIXELS, 22, NEO_GRB + NEO_KHZ800),  //0
  Adafruit_NeoPixel(NUM_PIXELS, 23, NEO_GRB + NEO_KHZ800),  //1
  Adafruit_NeoPixel(NUM_PIXELS, 24, NEO_GRB + NEO_KHZ800),  //2
  Adafruit_NeoPixel(NUM_PIXELS, 25, NEO_GRB + NEO_KHZ800),  //3
  Adafruit_NeoPixel(NUM_PIXELS, 26, NEO_GRB + NEO_KHZ800),  //4
  Adafruit_NeoPixel(NUM_PIXELS, 27, NEO_GRB + NEO_KHZ800),  //5
  Adafruit_NeoPixel(NUM_PIXELS, 28, NEO_GRB + NEO_KHZ800),  //6
  Adafruit_NeoPixel(NUM_PIXELS, 29, NEO_GRB + NEO_KHZ800),  //7
  Adafruit_NeoPixel(NUM_PIXELS, 30, NEO_GRB + NEO_KHZ800),  //8
  Adafruit_NeoPixel(NUM_PIXELS, 31, NEO_GRB + NEO_KHZ800),  //9
  Adafruit_NeoPixel(NUM_PIXELS, 32, NEO_GRB + NEO_KHZ800),  //10
  Adafruit_NeoPixel(NUM_PIXELS, 33, NEO_GRB + NEO_KHZ800),  //11
  Adafruit_NeoPixel(NUM_PIXELS, 34, NEO_GRB + NEO_KHZ800),  //12
  Adafruit_NeoPixel(NUM_PIXELS, 35, NEO_GRB + NEO_KHZ800)   //13
};

//センサーのピン番号//
byte sensorPins[NUM_PIXELROW] = {
  40, //0
  41, //1
  42, //2
  43, //3
  44, //4
  45, //5
  46, //6
  47, //7
  48, //8
  49, //9
  50, //10
  51, //11
  52, //12
  53, //13
};

struct Bullet {
  byte position; //先頭の光の現在位置
  byte rowNum; //出現するLEDテープの番号
  int colorH; //光の色相
};

Bullet bullets[MAX_BULLETS];  //各弾の情報を保持する配列
bool preSensorState[NUM_PIXELROW];  //センサーの前フレームの状態

unsigned long startFrameTime = 0; //1フレームの開始時間
unsigned long deltaTime = 0;  //1フレームの処理時間

byte bulletFrontIndex = 0;  //Bullets配列の処理が必要な最前要素番号
byte bulletDataCounter = 0; //Bullets配列の有効データ数


void setup() {
  for(byte i = 0; i < NUM_PIXELROW; i++ ){
    //NeoPixelの初期化//
    neoPixels[i].begin();
    neoPixels[i].show(); // 全てのLEDをオフにする

    //センサーピンの初期化//
    pinMode(sensorPins[i], INPUT);

    //前回のセンサー状態の配列を初期化（未検知時でHIGH）
    preSensorState[i] = true;
  }
  
  Serial.begin(9600); // シリアル通信の開始
}


void loop() {
  startFrameTime = millis();
  
  sensorInputer(); //センサーの読み取り

  updateBulletsPosition(); // 弾の位置を更新

  //更新遅延処理
  deltaTime = millis() - startFrameTime;
  if(deltaTime < DELAY_TIME)  delay(DELAY_TIME - deltaTime);
}


//センサーによる入力処理//
void sensorInputer(){
  for(byte i = 0; i < NUM_PIXELROW; i++){
    bool currentSensorState = digitalRead(sensorPins[i]); //センサー値読み取り

    //センサー状態に変化がない場合はスキップ
    if(currentSensorState == preSensorState[i]) continue;
    
    if(currentSensorState == LOW){
      activateBullet(i);  // 新しい弾をアクティブにする
      preSensorState[i] = false;
    } else {
      preSensorState[i] = true;
    }
  }
}


//シリアル通信による入力処理（デバッグ用）//
void serialInputer(){
  if (Serial.available() > 0) {
    String readData = Serial.readStringUntil('\n');  // 受信文字列の読み取り
    byte received = (byte)readData.toInt(); //byte型に変換
    
    if(-1 < received && received < NUM_PIXELROW){
      activateBullet(received); // 新しい弾をアクティブにする
      Serial.println(received);
    }
  }
}


// 新しい弾の情報を追加するメソッド//
void activateBullet(byte rowNum) {     
  if(bulletDataCounter < MAX_BULLETS)  {
    byte noUsedFrontIndex; //Bullets配列の使用されていない最前のインデックス
    convertToBulletsRange(noUsedFrontIndex = bulletFrontIndex + bulletDataCounter); 

    bullets[noUsedFrontIndex].position = 0;
    bullets[noUsedFrontIndex].colorH = (int)random(0,360);
    bullets[noUsedFrontIndex].rowNum = rowNum;

    bulletDataCounter ++;
  }
}


//弾の位置の更新処理//
void updateBulletsPosition() {  
  //全NeoPixelをリセット//
  for(byte i = 0; i < NUM_PIXELROW; i++ ){
    neoPixels[i].clear();
  }
  
  //位置更新処理//
  byte currentIndex = bulletFrontIndex;  //while内で処理中のindex
  byte whileCounter = 0; //while文の合計処理回数
  
  while(true){
    if(whileCounter == bulletDataCounter) break;
    
    //弾の位置データの更新処理//
    if (bullets[currentIndex].position < NUM_PIXELS) {
      bullets[currentIndex].position++;
    } else {  //末端の弾の処理
      bulletDataCounter --;
      whileCounter--;      
      
      convertToBulletsRange(++bulletFrontIndex); //Bullet更新の処理開始位置を更新
    }

    //NeoPixelデータに反映//
    neoPixels[bullets[currentIndex].rowNum].setPixelColor(bullets[currentIndex].position-1, HSBColor(bullets[currentIndex].colorH, 100, 50)); // 本体のLED設定
    neoPixels[bullets[currentIndex].rowNum].setPixelColor(bullets[currentIndex].position-2, HSBColor(bullets[currentIndex].colorH, 100, 10)); // 残像1のLED設定
    neoPixels[bullets[currentIndex].rowNum].setPixelColor(bullets[currentIndex].position-3, HSBColor(bullets[currentIndex].colorH, 100, 2)); // 残像2のLED設定

    whileCounter ++;
    convertToBulletsRange(++currentIndex);
  }

  //各NeoPixelに反映//
  for(byte i = 0; i < NUM_PIXELROW; i++ ){
    neoPixels[i].show();
  }
}


//Bullets配列の範囲内に変換するメソッド//
void convertToBulletsRange(byte &currentIndex){
  currentIndex = currentIndex % MAX_BULLETS;
}


// HSBでの色指定をサポートするメソッド//
uint32_t HSBColor(float H, float S, float B) {
  S = S / 100.0;
  B = B / 100.0;
  H = H / 360;
  int i = int(H * 6) ;
  float f = H * 6 - i;
  float p = B * (1.0 - S);
  float q = B * (1.0 - f * S);
  float t = B * (1.0 - (1.0 - f) * S);
  float r = 0, g = 0, b = 0;

  switch(i%6) {
    case 0: r = B, g = t, b = p; break;
    case 1: r = q, g = B, b = p; break;
    case 2: r = p, g = B, b = t; break;
    case 3: r = p, g = q, b = B; break;
    case 4: r = t, g = p, b = B; break;
    case 5: r = B, g = p, b = q; break;
  }

  uint8_t red = r * 255.0;
  uint8_t green = g * 255.0;
  uint8_t blue = b * 255.0;

  return Adafruit_NeoPixel::Color(red, green, blue);
}
