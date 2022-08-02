#include <Wire.h> 
#include <LiquidCrystal_I2C.h>

LiquidCrystal_I2C lcd(0x27, 16, 2);
// SVM Weights
double w[] = {2.8985932476477125, -0.9270252682646416};
double c = -0.1931830992959192;
// Standar Scaler
double u = 288.58450704225353;
double p = 0.1361894949363624;
// Variable to help compute
int index = 0;
bool compute_svm = false;
float data_ecg[330];
int start = 0;
int finish = 0 ;
// Waves Value
float peak_value = 0;
int peak_index = 0;
float q_wave_value = 0;
float s_wave_value = 0;
float t_wave_value = 0;
int s_wave_index = 0;
int t_wave_index = 0;
float st_segment_value = 0;
// Eksponential Filter
float alpha=0.55;
float x_start;
float y_start =0;;
float y_prev = 0;


float mean(){
  float temp = 0;
  for(int i = 0; i <330; i++){
    temp += data_ecg[i];
  }

  float mean_value = temp / 330;
  u = mean_value;
}

float std_deviation(){
  float temp = 0;
  for(int i = 0; i <330; i++){
    temp += pow(data_ecg[i] - u, 2);
  }
  float dev = temp / 330;
  float std_dev = sqrt(dev);
  p = std_dev;
}

//SVM Compute
float svm_value(int q_wave, int st_segment){
  double temp = 0;
  temp += ((q_wave * w[0]));
  temp += ((st_segment * w[1]));
  return temp;
}

void svm(int svm_result){
  if (svm_result > c){
    lcd.clear();
    Serial.println("Report : Myocardial Infraction");
    Serial.print("Q Waves : ");
    Serial.println(q_wave_value);
    Serial.print("ST Segment : ");
    Serial.println(st_segment_value);
    Serial.print("Mean : ");
    Serial.println(u);
    Serial.print("Std dev : ");
    Serial.println(p);
    lcd.setCursor(0,0);
    lcd.print("Report: MI");
    compute_svm= true;
  }
  else {
    lcd.clear();
    Serial.println("Report : Noraml");
    Serial.print("Q Waves : ");
    Serial.println(q_wave_value);
    Serial.print("ST Segment : ");
    Serial.println(st_segment_value);
    Serial.print("Mean : ");
    Serial.println(u);
    Serial.print("Std dev : ");
    Serial.println(p);
    lcd.setCursor(0,0);
    lcd.print("Report: Normal");
    compute_svm= true;
  }
}

// Get R Waves Position
void get_peak_index(float value, int value_index){
  if (value > peak_value) {
    peak_value = value;
    peak_index = value_index;
  }
}

// Get Q Waves Value
float search_q_wave(int peaks_index){
  q_wave_value = data_ecg[peaks_index];
  for (int i = peaks_index-1; i > 0; i--){
    int temp = data_ecg[i];
    if (q_wave_value > temp){
      q_wave_value = temp;
    }
  }
  return (q_wave_value - u) * p;
}

// Get S Waves Position
float search_s_wave_index(int peaks_index) {
  s_wave_value = data_ecg[peaks_index];
  s_wave_index = 0;
  for (int i = peaks_index; i < 348; i++){
    int temp = data_ecg[i];
    if (s_wave_value > temp){
      s_wave_value = temp;
      s_wave_index = i;
    }
  }

  return s_wave_index;
}

// Get T Waves Position
float search_t_wave_index(int s_index) {
  t_wave_value = data_ecg[s_index];
  t_wave_index = 0;
  for (int i = s_index; i < 348; i++){
    int temp = data_ecg[i];
    if (t_wave_value < temp){
      t_wave_value = temp;
      t_wave_index = i;
    }
  }

  return t_wave_index;
}

// Compute ST Segment Value Mean
float calculate_st_segment(float s_index, float t_index){
  float temp = 0;
  for (int x=s_index; x<=t_index; x++){
    temp = temp + (data_ecg[x]- u) * p;
  }
  st_segment_value = temp / (t_index - s_index);
  return st_segment_value;
}

// Compute Time
void calculate_time(float start, float end){
  float time = end - start;
  Serial.print("Time : ");
  Serial.println(time);
}

// Reset All Variable
void reset_variables(){
  memset(data_ecg, 0, sizeof(data_ecg));
  peak_value = 0;
  peak_index = 0;
  q_wave_value = 0;
  s_wave_value = 0;
  t_wave_value = 0;
  s_wave_index = 0;
  t_wave_index = 0;
  st_segment_value = 0;
  start = 0;
  finish = 0;
  index = 0;
  compute_svm= false;
  delay(10);
}


void setup() {
  Serial.begin(9600);
  lcd.begin();
  pinMode(10, INPUT); // Setup for leads off detection LO +
  pinMode(11, INPUT); // Setup for leads off detection LO -
}

void loop() {
  if((digitalRead(10) == 1)||(digitalRead(11) == 1)){
    Serial.println('ECG Sensor Error');
  }
  else {
    // send the value of analog input 0:
    int temporary_value = analogRead(A0);

    x_start = temporary_value;
    y_start = (alpha * x_start) + (1 - alpha) * y_prev;

    get_peak_index(y_start, index);
    data_ecg[index] = analogRead(y_start);
    
    y_prev = y_start;
    

    if (index <= 327) {
      index += 1;
      delay(10);
    }
    else if (compute_svm == true) {
      reset_variables();
      delay(10);
    }
    else if (index >= 328) {
      start = millis();
      mean();
      std_deviation();
      float q_wave_now = search_q_wave(peak_index);
      float s_index_now = search_s_wave_index(peak_index);
      float t_index_now = search_t_wave_index(s_index_now);
      float st_segment_now = calculate_st_segment(s_index_now, t_index_now);
      float svm_equation = svm_value(q_wave_now, st_segment_now);
      svm(svm_equation);
      finish = millis();
      calculate_time(start, finish);
      delay(10);
    }
  }
}
