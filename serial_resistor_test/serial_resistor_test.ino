float org_bat1 = 0;
float org_bat2 = 0;
float org_bat3 = 0;
float org_bat4 = 0;

void setup()
{
  Serial.begin(9600);

//  pinMode(A2, INPUT);
//  pinMode(A3, INPUT);
//  pinMode(A4, INPUT);
//  pinMode(A5, INPUT);
}

void loop()
{
  float bat1 = 0;
  float bat2 = 0;
  float bat3 = 0;
  float bat4 = 0;

  int test = analogRead(A2);
  org_bat1 = test * 0.05859375;
  org_bat2 = analogRead(A3) * 0.05859375;
  org_bat3 = analogRead(A4) * 0.05859375;
  org_bat4 = analogRead(A5) * 0.05859375;
  
  bat1 = org_bat1 <= 0 ? 0 : org_bat1;
  bat2 = org_bat2 - org_bat1 <= 0 ? 0 : org_bat2 - org_bat1;
  bat3 = org_bat3 - org_bat2 <= 0 ? 0 : org_bat3 - org_bat2;
  bat4 = org_bat4 - org_bat3 <= 0 ? 0 : org_bat4 - org_bat3;

  Serial.print("RAW : ");
  Serial.print(test);
  Serial.print(" -> Vol : ");
  Serial.println(org_bat1);
//  Serial.print("A2 = ");
//  Serial.println(bat1);
//  Serial.print(org_bat2);
//  Serial.print("A3 = ");
//  Serial.println(bat2);
//  Serial.print(org_bat3);
//  Serial.print("A4 = ");
//  Serial.println(bat3);
//  Serial.print(org_bat4);
//  Serial.print("A5 = ");
//  Serial.println(bat4);
  
  delay(500); // Wait for 3000 millisecond(s)
}
