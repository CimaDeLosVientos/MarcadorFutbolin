#include <LiquidCrystal.h>
LiquidCrystal lcd(7, 8, 9, 10, 11, 12); //    ( RS, EN, d4, d5, d6, d7)

// Pines
int atleti=5; // Porteria equipo 1
int madrid=6; // Porteria equipo 2
int buzzer = 3;
int controlBoton = 13;

// Constantes
int delayPosGol = 2000; // Tiempo que pasa desde un gol hasta que se reanuda el juego.
int tiempoPreHimno = 2000;  // Tiempo que pasa desde que termina el partido hasta que
                            // suena un himno.
int maxTime = 10; // minutos maximo que puede durar el partido
int controlDelay = 700; // Tiempo que se debe mantener pulsado el boton de control
                        // para resetear.
int frecuenciaNota= 2272;

// Variables
int controlState = 1; // 0=pausado; 1=corriendo
int i = 0;
unsigned long tiempoInicial=0; // Tiempo que dura el partido
unsigned long countDown;
unsigned long tiempoComienzo; // Variable utilizada para ajustar el tiempo restante en
                              // relación al reloj del micro.

bool lectura1;
bool lectura2;
int golesatleti = 0;
int golesmadrid = 0;

//////COSAS DE  LA MUSICA//////////

#define  c3    7634
#define  d3    6803
#define  e3    6061
#define  f3    5714
#define  g3    5102
#define  a3    4545
#define  b3    4049
#define  c4    3816    // 261 Hz 
#define  d4    3401    // 294 Hz 
#define  e4    3030    // 329 Hz 
#define  f4    2865    // 349 Hz 
#define  g4    2551    // 392 Hz 
#define  a4    2272    // 440 Hz 
#define  a4s   2146
#define  b4    2028    // 493 Hz 
#define  c5    1912    // 523 Hz
#define  d5    1706
#define  d5s   1608
#define  e5    1517    // 659 Hz
#define  f5    1433    // 698 Hz
#define  g5    1276
#define  a5    1136
#define  a5s   1073
#define  b5    1012
#define  c6    955
 
#define  R     0      // Define a special note, 'R', to represent a rest
 
// MELODIES and TIMING //
//  melody[] is an array of notes, accompanied by beats[], 
//  which sets each note's relative length (higher #, longer note) 
 
// Melody 1: Star Wars Imperial March
int melody1[] = {  a4, R,  a4, R,  a4, R,  f4, R, c5, R,  a4, R,  f4, R, c5, R, a4, R,  e5, R,  e5, R,  e5, R,  f5, R, c5, R,  g5, R,  f5, R,  c5, R, a4, R};
int beats1[]  = {  50, 20, 50, 20, 50, 20, 40, 5, 20, 5,  60, 10, 40, 5, 20, 5, 60, 80, 50, 20, 50, 20, 50, 20, 40, 5, 20, 5,  60, 10, 40, 5,  20, 5, 60, 40};
 
// Melody 2: Star Wars Theme
int melody2[] = {  f4,  f4, f4,  a4s,   f5,  d5s,  d5,  c5, a5s, f5, d5s,  d5,  c5, a5s, f5, d5s, d5, d5s,   c5};
int beats2[]  = {  21,  21, 21,  128,  128,   21,  21,  21, 128, 64,  21,  21,  21, 128, 64,  21, 21,  21, 128 }; 
 
int MAX_COUNT = sizeof(melody1) / 2; // Melody length, for looping.
 
long tempo = 10000; // Set overall tempo
 
int pause = 1000; // Set length of pause between notes
 
int rest_count = 50; // Loop variable to increase Rest length (BLETCHEROUS HACK; See NOTES)
 
// Initialize core variables
int toneM = 0;
int beat = 0;
long duration  = 0;
int potVal = 0;

///////////// FUNCIONES PRINCIPALES /////////////

void setup() {
  /*
  Inicializamos los pines de salida, la pantalla, se llama a reset
  para hacer la configuracion de inicio y la espera de dos segundos
  antes de comenzar el partido.
  */  
  pinMode(atleti, INPUT_PULLUP);
  pinMode(madrid, INPUT_PULLUP);
  pinMode(controlBoton, INPUT_PULLUP);
  pinMode(buzzer, OUTPUT);
  lcd.begin(16, 2); // Fijar el numero de caracteres y de filas
  Serial.begin(9600);
  reset();
  lcd.clear();
  delay(2000);
}

void loop() {
  /*
  El bucle de juego consiste en leer si se han metido goles, comprobar si el
  partido a terminado, y por último dibujar en la pantalla.
  */
  pausar(); // Se comprueba si el partido ha sido pausado
  while(!controlState){ // Si el juego esta en pausa, solo se mostrara la informacion
                        // parpadeando
    parpadeoPausa();
  }
  lecturaGoles();
  comprobarFin();
  if (countDown != 0){ // No queremos tiempos negativos
    countDown=tiempoComienzo-mitime(); // countDown contiene el tiempo que le queda al partido
  }  
  report();
}

void lecturaGoles(){
  /*
  Cuando uno de los pines de lectura de gol se activa, se incrementan los goles
  de ese equipo, se pita el gol y se espera una pequeña cantidad de tiempo para
  reanudar.
  */
  lectura1= digitalRead(madrid);
  lectura2= digitalRead(atleti);
  if (lectura1 ==LOW){
    golesmadrid++;
    report();
    pitiGol();
    delay(delayPosGol);
    resume();
  }
  if (lectura2==LOW){
    golesatleti++;
    report();
    pitiGol();
    delay(delayPosGol);
    resume();
  }
}

void comprobarFin(){
  /*
  El partido acaba cuando se acaba el tiempo o se meten 7 goles.
  Se comprueba en cada ronda del bucle si esto pasa, y en cuyo caso
  se pita el final del partido y se resetea para volver a la
  configuracion de un nuevo partido.
  */
  if (golesatleti+golesmadrid==7){
    report();
    delay(1000);
    pitiFin();
    reset();
  }
  Serial.println(countDown);
  if(countDown <=0){
    if(golesmadrid != golesatleti){
      pitiFin();
      reset();
    } 
  }
}


///////////// FUNCIONES DE CONTROL /////////////

void reset(){
  /*
  El reset implica configurar el tiempo de juego, esto se hace con 2 botones
  (en esta version, los mismo que los de meter gol), uno configura los minutos
  y el otro los segundos en potencias de 10, el tiempo maximo del partido viene
  definido por la variable maxTime.
  El tiempo seleccionado se actualiza en la pantalla.
  Cuando el tiempo mostrado sea el deseado, se pulsa el boton de control para
  aceptar.
  */
  lcd.clear();
  lcd.setCursor(0,0);
  lcd.print(String("Tiempo de juego"));
  golesatleti = 0;
  golesmadrid = 0;
  while(digitalRead(controlBoton) == HIGH){
    if(digitalRead(madrid) == LOW){
      golesatleti = ++golesatleti%9; // Estamos reutilizando variables, golesatleti es minutos y madrid segundos
      delay(500);
    }  
    if(digitalRead(atleti) == LOW){
      golesmadrid = ++golesmadrid%6;
      delay(500);
    }
    tiempoInicial = min((golesatleti+1)*60+golesmadrid*10, 600);
    lcd.setCursor(0, 1);
    lcd.print(timeFormat(tiempoInicial));
  }
  
  golesatleti=0;
  golesmadrid=0;
  tiempoComienzo=mitime()+tiempoInicial; // Estas operaciones se realizan para
                                          // tener control del momento en el
                                          // que empezo el partido, ya que puede
                                          // puede resetearse en cualquier momento
  countDown = tiempoComienzo-mitime();
  controlState = 0;
  lcd.clear();
}

void pausar(){
  /*
  El juego se puede pausar, esta funcion lee el pin donde se activa la pausa y,
  controlando el rebote, pausa o reanuda el juego, o reinicia si se mantiene 
  activado.
  */
  if (digitalRead(controlBoton) == LOW){
    delay(controlDelay);
    if (digitalRead(controlBoton) == LOW){
      lcd.clear();
      delay(1500);
      reset();
    }  
    else{
      if (!controlState){ // Si esta parado
        resume();
      }else{ // Si est tirando paralo
       controlState = 0; // Cambiamos el stado de 0 a 1 y viceversa 
      }  
    }
  }
}
void resume(){
  /*
  Esta funcion reanuda el partido, para ello cambia el controlState y
  actualiza la variable de tiempo de comienzo para que la cuenta atras
  no se corrompa.
  */
  controlState = 1; // Siempre que usemos resume es para pasar de pausa a play
  tiempoComienzo=mitime()+countDown;
  pitiInicio();
}
 
unsigned long mitime(){ // Retorna millis en segundos
  return millis()/1000;
}


///////////// FUNCIONES DE PANTALLA /////////////

void report(){
  /*
  Pinta los goles y el tiempo restante en la pantalla.
  */
  if (((countDown%(tiempoInicial-(tiempoInicial-9))) == 0)){
    lcd.clear();
  }  
  lcd.setCursor(0, 0);   
  lcd.print(String(golesatleti)+"-"+String(golesmadrid));
  lcd.setCursor(0, 1);  // set the cursor to column 0, line 1
  lcd.print(timeFormat(countDown));  // print the number of seconds since reset:
}

String timeFormat(int time){
  /*
  Formatea el tiempo para que aparezca como minutos y segundos
  */
  int minutos = time/60;
  int seg = time%60;
  if(seg<10){
    return String(minutos) + ":0" + String(seg);
  }  
  return String(minutos) + ":" + String(seg);
}

void parpadeoPausa(){
  /*
  Cuando el partido esta en pausa, se muestra la informacion parpadeando.
  Es necesario comprobar con el metodo pausar() si se ha reanudado el juego.
  */
  lcd.clear();
    for(i = 0; i<500; i++){
      delay(1);
      pausar();
    }
    report();
    for(i = 0; i<500; i++){
      delay(1);
      pausar();
    }
}

///////////// FUNCIONES DE PITIDOS /////////////

void pitiGenerico(long duracion){
  long elapsed_time = 0;
  if (frecuenciaNota > 0) { // if this isn't a Rest beat, while the tone has 
    //  played less long than 'duration', pulse speaker HIGH and LOW
    while (elapsed_time < duracion) {
 
      digitalWrite(buzzer,HIGH);
      delayMicroseconds(frecuenciaNota/ 2);
 
      // DOWN
      digitalWrite(buzzer,LOW);
      delayMicroseconds(frecuenciaNota / 2);
 
      // Keep track of how long we pulsed
      elapsed_time += (frecuenciaNota);
    }
  }
}

void pitiGol(){
  pitiGenerico(500000);
  delay(250);
  pitiGenerico(500000);
}

void pitiInicio(){
  pitiGenerico(1000000);
}

void pitiFin(){
  report(); // Para evitar problemas de tiempo...
  pitiGenerico(500000);
  delay(250);
  pitiGenerico(500000);
  delay(250);
  pitiGenerico(1500000);
  delay(tiempoPreHimno);
  pitiHimno();
}

void pitiHimno(){
  if (golesatleti>golesmadrid){
    for (int i=0; i<sizeof(melody1)/2; i++) {
      toneM = melody1[i];
      beat = beats1[i];
   
      duration = beat * tempo; // Set up timing
   
      playTone(); // A pause between notes
      delayMicroseconds(pause);
    }
  }
  else{    // ... else play Melody2
    for (int i=0; i<sizeof(melody2)/2; i++) {
      toneM = melody2[i];
      beat = beats2[i];
   
      duration = beat * tempo; // Set up timing
   
      playTone(); // A pause between notes
      delayMicroseconds(pause);
    }
  }  
}


void playTone() {
  long elapsed_time = 0;
  if (toneM > 0) { // if this isn't a Rest beat, while the tone has 
    //  played less long than 'duration', pulse speaker HIGH and LOW
    while (elapsed_time < duration) {
 
      digitalWrite(buzzer,HIGH);
      delayMicroseconds(toneM / 2);
 
      // DOWN
      digitalWrite(buzzer, LOW);
      delayMicroseconds(toneM / 2);
 
      // Keep track of how long we pulsed
      elapsed_time += (toneM);
    } 
  }
  else { // Rest beat; loop times delay
    for (int j = 0; j < rest_count; j++) { // See NOTE on rest_count
      delayMicroseconds(duration);  
    }                                
  }                                 
}
