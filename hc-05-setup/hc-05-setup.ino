char c = ' ';
 
void setup() 
{
    Serial.begin(9600);
    Serial.println("Arduino is ready");
 
    // HC-05 default serial speed for commincation mode is 9600
    Serial2.begin(38400);  
}
 
void loop()
{
 
    // Keep reading from HC-05 and send to Arduino Serial Monitor
    if (Serial2.available())
    {  
        c = Serial2.read();
        Serial.write(c);
    }
 
    // Keep reading from Arduino Serial Monitor and send to HC-05
    if (Serial.available())
    {
        c =  Serial.read();
        Serial2.write(c);  
    } 
}
