#include <util/delay.h>
#include <LiquidCrystal.h>
#include <Wire.h>
#include "RTClib.h"
#include "DHT.h"

LiquidCrystal lcd(12, 11, 9,8,7,6);
RTC_DS1307 rtc;
DHT dht11(5, DHT11);
float temp;
const int buzzerPin = 10;

//variabilele au si valori default pentru cazul in care apare o problema la modulul rtc
volatile int secunda = 0;
volatile int minut = 0;
volatile int ora = 7;
volatile int zi = 1;
volatile int luna = 1;
volatile int an = 2021;
volatile int zi_sapt = 5;

//retin separat datele despre alarma
volatile int m_alarm;
volatile int o_alarm = 7; //valoare default
volatile boolean set_alarm = false;
volatile byte am_pm_alarm;

volatile int zile_luna [] = {31, 29, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};
const char zile_sapt[9][4] = {"   ", "Mon", "Tue", "Wed", "Thu", "Fri", "Sat", "Sun"};
volatile byte  set_mode;
volatile byte am_pm; //setare mod 12/24 ore

//pentru a preveni schimbarea nedorita a valorilor in cadrul intreruperilor declansate de apasarea butoanelor, 
//verific cat timp a trecut de la ultima apasare
long int button1_time = 0;
long int button1_prev_time = 0;

long int button2_time = 0;
long int button2_prev_time = 0;

long int button3_time = 0;
long int button3_prev_time = 0;

byte celsius[8] = { // grad Celsius
  B00110,
  B01001,
  B01001,
  B00110,
  B00000,
  B00000,
  B00000,
};

byte alarm[8] = { // icon alarma
  B00100,
  B01110,
  B01110,
  B01110,
  B11111,
  B00000,
  B00100,
};

void setup() {
  // rezistente interne pull-up pentru butoane
  pinMode(2, INPUT_PULLUP);
  pinMode(3, INPUT_PULLUP);
  pinMode(4, INPUT_PULLUP);

  TIMSK1 = (1 << TOIE1); // activare timer overflow
  //TCNT1=0x0BDC;
  TCCR1A = 0; 
  TCCR1B = (1 << CS12); // timer start

  attachInterrupt(0,ISR_b1,FALLING);
  attachInterrupt(1,ISR_b2,FALLING);
  // activare PCI2 (buton 3 - PCINT20 - PCI2)
  PCICR  |= (1 << PCIE2);
  PCMSK2 |= (1<<PCINT20);
  
  // configurare LCD
  lcd.begin(16, 2);
  lcd.noCursor();

  set_mode = 0; //functionare normala, fara setari suplimentare
  am_pm = 0; //mod 24 ore
  am_pm_alarm = 0;
  
  //activare rtc
  rtc.begin();

  //activare senzor temperatura si formare caracter special
  dht11.begin();
  lcd.createChar(0, celsius);

  //formez caracterul special pentru alarma
  lcd.createChar(1, alarm);

  //se porneste cu timpul real, ce poate fi modificat ulterior
  zi = rtc.now().day();
  luna = rtc.now().month();
  an = rtc.now().year();

  ora = rtc.now().hour();
  minut = rtc.now().minute();
  secunda = rtc.now().second();
  zi_sapt = rtc.now().dayOfTheWeek();
}

void loop(){
  //afisez timpul daca nu doresc sa setez alarma
   if(set_alarm == false)
   {
     //ora, minut, secunda
     lcd.setCursor(0, 0);
     if (ora<10) lcd.print("0");
     lcd.print(ora);
     lcd.setCursor(2,0);
     lcd.print(":");
     if (minut<10) lcd.print("0");
     lcd.print(minut);
     lcd.setCursor(5,0);
     lcd.print(":");
     if (secunda<10) lcd.print("0");
     lcd.print(secunda);
     lcd.print(" "); 
     
     //zi, luna, an
     lcd.setCursor(0, 1);
     if (zi<10) lcd.print("0");
     lcd.print(zi);
     lcd.setCursor(2,1);
     lcd.print("/");
     if (luna<10) lcd.print("0");
     lcd.print(luna);
     lcd.setCursor(5,1);
     lcd.print("/");
     lcd.setCursor(6,1);
     lcd.print(an);

     //mod 24h/12h
     lcd.setCursor(8,0);
     if(am_pm == 1)
      lcd.print("AM");
     else if(am_pm == 2) 
      lcd.print("PM");
     else
     lcd.print("  ");

       //achizitie date de la senzor si afisare
      float temp = dht11.readTemperature();
      lcd.setCursor(12, 0);
      lcd.print(temp);
      lcd.setCursor(14, 0);
      lcd.write(byte(0)); //grad Celsius
      lcd.setCursor(15,0);
      lcd.print("C");

    //setare parametru
     lcd.setCursor(11, 1);
     if (set_mode!=0) 
     {  
       lcd.print("SET ");
       switch (set_mode) {
         case 1:
           lcd.print("O");
           break;
         case 2:
           lcd.print("M");
           break;
         case 3:
           lcd.print("S");
           break;
         case 4:
           lcd.print("T");
           break;
         case 5:
           lcd.print("Z");
           break;
         case 6:
           lcd.print("L");
           break;
         case 7:
           lcd.print("A");
           break;
      }
     }
      else
      {   //daca nu setez ceva, atunci afisez ziua din saptmana
          lcd.setCursor(11, 1);
          lcd.print("     ");
          lcd.setCursor(13,1);
          lcd.print(zile_sapt[zi_sapt]);
      }

    }

   //doresc sa setez o alaram
   if(set_alarm)
   {
      //afisare icon alarma
      lcd.clear();
      lcd.setCursor(0,0);
      lcd.write(byte(1)); //alarma
      lcd.setCursor(1,0);
      lcd.print(":");

      //ora, minut
      lcd.setCursor(3, 0);
     if (o_alarm<10) lcd.print("0");
     lcd.print(o_alarm);
     lcd.setCursor(5,0);
     lcd.print(":");
     if (m_alarm<10) lcd.print("0");
     lcd.print(m_alarm);

     //mod 24h/12h
     lcd.setCursor(8,0);
     if(am_pm_alarm == 1)
      lcd.print("AM");
     else if(am_pm_alarm == 2) 
      lcd.print("PM");
     else
     lcd.print("  ");
     
     //pot seta ora, minutul, modul
     if (set_mode!=0) 
     {  
       lcd.setCursor(11, 1);
       lcd.print("SET ");
       switch (set_mode) {
         case 1:
           lcd.print("O");
           break;
         case 2:
           lcd.print("M");
           break;
         case 3:
           lcd.print("T");
           break;       
       }
     }
     else
     {
       lcd.print("     ");
     }
   }

  //verific daca timpul indicat de ceas coincide cu timpul alarmei 
   if(o_alarm == ora && m_alarm == minut && am_pm == am_pm_alarm && secunda < 20 && set_alarm == false)
      tone(buzzerPin, 300, 100);
   else
      noTone(buzzerPin);

  delay(1000);
}

ISR(TIMER1_OVF_vect) {
  //TCNT1=0x0BDC;
  secunda++;
   if (secunda == 60)
   {
     secunda = 0;
     minut++;
     if (minut>=60)
     {
       minut = 0;
       ora ++;
       if (ora >= 24)
       {
         ora = 0;
         zi++;
         if(zi_sapt == 7) zi_sapt = 1;
         else
            zi_sapt++;
         if (zi > zile_luna[luna-1])
         {
           zi = 1;
           luna++;
           
           if (luna > 12)
           {
             luna = 1;
             an++;
             if (an%4 == 0) zile_luna[1] = 29;
             else zile_luna[1] = 28;
           }      
         }
       }
     }
   }
}

void ISR_b1() {

  //schimb valoarea lui set mode daca a trecut un interval de timp intre intreruperile cauzate de apasarea butoanelor
  button1_time = millis();
  if (button1_time - button1_prev_time > 300)
  {
    // "ecranul" principal
    if(set_alarm == false)
    {
      if (set_mode==7) set_mode=0;
      else 
        set_mode++;
    }
    //setare alarma
    else{
      if (set_mode==3) set_mode=0;
      else 
        set_mode++;
    }
  }
  //actualizez valorile de timp
  button1_prev_time = button1_time;
  
  _delay_ms(20);
}

void ISR_b2() 
{
  button2_time = millis();
  if (button2_time - button2_prev_time > 300)
  {
    //setez parametrii "ecranului" principal
    if (set_mode!=0 && set_alarm == false)
    switch (set_mode) {
      case 1:
        ora++;
        if (ora==24) ora=0;
        break;
      case 2:
        minut++;
        if (minut==60) minut=0;
        break;
      case 3:
        secunda++;
        if (secunda==60) secunda=0;
        break;
      case 4:
        if(am_pm == 2) //PM->24h
        {
          ora = ora + 12;
          am_pm = 0;
        } 
        else //mod 24 ore si AM
          am_pm++;
        if(am_pm!=0 && ora> 12) // AM PM
        {
          ora = ora - 12;
        }
        if(am_pm== 1 && ora> 12) // AM->PM
        {
          am_pm = 2;
        }
        break;
        
      case 5:
        zi++;
        if(zi_sapt == 7) zi_sapt = 1;
        else
            zi_sapt++;
        if (zi==zile_luna[luna-1]) zi=1;
        break;
      case 6: 
        //merg pana la ultima zi din luna, egala cu zi_sapt(peste x saptamani), determin cat este ultima zi din luna prin diferenta
        zi_sapt = zi_sapt + (zile_luna[luna-1] - zi) % 7;   
        if(zi_sapt > 7) 
          zi_sapt = zi_sapt - 7;
        //ar mai trebui sa adaug saptamanile intregi din noua luna pana la noua zi_sapt
        //pastrez doar zilele in plus
        zi_sapt += zi % 7;       
        if(zi_sapt >7) 
          zi_sapt = zi_sapt - 7;
        if(zi_sapt ==7)
          zi_sapt = 0;   
        if (luna==12) //daca nu incrementez anul, trebuie sa aflu o zi de la inceputul anului curent
        {
          luna=1;
          //am calculat deja ce zi este pentru luna ianuarie, dar anul viitor, care este cu o zi sau doua in plus fata de cea din anul curent
          if ( an%4 == 0 && an%100!=0 || an%400==0) //an bisect 366 zile
          {
            if(zi_sapt == 1) //luni
              zi_sapt = 7; //duminica
            else
              zi_sapt= zi_sapt - 2; 
          }
          else //365 zile
          {
            if(zi_sapt == 1) 
              zi_sapt = 7;
            else 
              zi_sapt= zi_sapt - 1;
          }   
        }
        else
          luna++;
        if (zi>zile_luna[luna-1]) zi=zile_luna[luna-1];
        break;
      case 7:
        an++;
        if (an==2030) 
          an=2021;
        if ( an%4 == 0 && an%100!=0 || an%400==0) 
        {
            zile_luna[1] = 29;
            zi_sapt = zi_sapt + 2;
            if(zi_sapt > 7)
              zi_sapt = zi_sapt - 7;
        }
        else
        {
          zile_luna[1] = 28;
          zi_sapt = zi_sapt + 1;
          if(zi_sapt > 7)
              zi_sapt = zi_sapt - 7;
        }
        
        if (zi>zile_luna[luna-1]) zi=zile_luna[luna-1];
        break;
    }

    //setare alarma
    if (set_mode!=0 && set_alarm == true)
    {
      switch (set_mode) {
      case 1:
        o_alarm++;
        if (o_alarm==24) o_alarm=0;
        
        break;
      case 2:
        m_alarm++;
        if (m_alarm==60) m_alarm=0;
        break;
      case 3:
        if(am_pm_alarm == 2)
          am_pm_alarm = 0;
        else
          am_pm_alarm++;
        if(am_pm_alarm!= 0 && o_alarm> 12) // AM si PM
        {
          o_alarm = o_alarm - 12;
        }
        break;
      }
    }
  
  }
  button2_prev_time = button2_time;
  _delay_ms(20);
  
}

ISR(PCINT2_vect) {
 _delay_ms(20);
 if (!(PIND & 0x10)) 
 {
   button3_time = millis();
   if (button3_time - button3_prev_time > 300){
     if (set_alarm) {
       set_alarm = false; 
       set_mode = 0;  // nu doresc sa apara vreun caz de set_mode de la setarea timpului din "ecranul" principal
     }
     else {
       set_alarm = true;
       set_mode = 0;
     }
   }
   button3_prev_time = button3_time;
 }
}
