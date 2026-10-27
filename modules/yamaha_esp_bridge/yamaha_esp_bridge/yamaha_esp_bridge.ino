#include <WiFi.h>
#include <WebServer.h>
#include <WiFiManager.h>

WiFiManager wm;
WebServer server(80);
String lastNote = "";

void handleRoot() { server.send(200,"text/html","<h2>Yamaha ESP Bridge</h2><p>IP: "+WiFi.localIP().toString()+"</p><script>setInterval(()=>fetch('/midi'),300);</script>"); }
void handleMidi(){ server.send(200,"text/json","{\"note\":\""+lastNote+"\"}"); }

void setup() {
  Serial.begin(115200);
  wm.autoConnect("ESP_MIDIBridge");
  Serial.println(WiFi.localIP());
  server.on("/",handleRoot); server.on("/midi",handleMidi); server.begin();
}
void loop() {
  server.handleClient();
  if(Serial.available()>=3){ uint8_t b1=Serial.read(),b2=Serial.read(),b3=Serial.read();
    if((b1&0xF0)==0x90||(b1&0xF0)==0x80){ char buf[32]; snprintf(buf,sizeof(buf),"%02X:%02X:%02X",b1,b2,b3); lastNote=buf; Serial.println(buf); }
  }
}
