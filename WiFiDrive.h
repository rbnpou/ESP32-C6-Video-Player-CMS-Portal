#ifndef WIFI_DRIVE_H
#define WIFI_DRIVE_H

#include <WiFi.h>
#include <WebServer.h>
#include <DNSServer.h>
#include <SD.h>

WebServer server(80);
DNSServer dnsServer;
const byte DNS_PORT = 53;

// Interfaz Web: Conversor en Cliente + Gestión de SD + Playlist
const char upload_html[] PROGMEM = R"rawliteral(
<!DOCTYPE html><html><head><meta charset="utf-8" name="viewport" content="width=device-width, initial-scale=1">
<title>ESP32 Manager</title>
<style>
body{font-family:sans-serif;background:#121212;color:#eee;text-align:center;padding:10px}
.card{background:#1e1e1e;padding:15px;border-radius:10px;margin-bottom:15px;border:1px solid #333}
h3{color:#00d4ff;margin:5px 0}
.progress-container{width:100%;background:#333;display:none;border-radius:5px;overflow:hidden;margin:10px 0}
#progressBar{width:0%;height:12px;background:linear-gradient(90deg,#0078d4,#00d4ff);transition:width .2s}
button{background:#0078d4;color:#fff;border:none;padding:10px;border-radius:5px;width:100%;cursor:pointer;font-weight:bold;margin-top:5px}
.btn-del{background:#d32f2f;width:auto;padding:4px 8px;font-size:11px;margin:0}
.item{display:flex;justify-content:space-between;padding:8px;border-bottom:1px solid #333;align-items:center;text-align:left}
#status{font-size:12px;color:#0f8;margin:5px 0;min-height:1.2em}
</style></head><body>
<div class="card">
<h3>📹 Conversor y Subida</h3>
<input type="file" id="fileInput" accept="video/*"><br>
<div id="status">Listo</div>
<div class="progress-container" id="pContainer"><div id="progressBar"></div></div>
<button id="startBtn" onclick="processAndUpload()">CONVERTIR Y ENVIAR</button>
</div>
<div class="card">
<h3>🎬 Gestión de Playlist</h3>
<div id="list">Cargando...</div>
<button style="background:#6c4ab6;margin-top:15px" onclick="savePlaylist()">GUARDAR SELECCIÓN</button>
</div>
<p><a href="/restart" style="color:#555;text-decoration:none;font-size:11px">Reiniciar Dispositivo</a></p>
<script>
async function processAndUpload(){
const file=document.getElementById('fileInput').files[0];if(!file)return alert("Selecciona video");
const btn=document.getElementById('startBtn'),stat=document.getElementById('status'),pBar=document.getElementById('progressBar');
btn.disabled=true;document.getElementById('pContainer').style.display='block';
const video=document.createElement('video');video.src=URL.createObjectURL(file);video.muted=true;video.playsInline=true;
await new Promise(r=>video.onloadedmetadata=r);
const canvas=document.createElement('canvas'),ctx=canvas.getContext('2d');canvas.width=320;canvas.height=240;
let frames=[];const fps=15,total=Math.floor(video.duration*fps);
for(let i=0;i<total;i++){
video.currentTime=i/fps;await new Promise(r=>video.onseeked=r);
ctx.drawImage(video,0,0,canvas.width,canvas.height);
const blob=await new Promise(r=>canvas.toBlob(r,'image/jpeg',0.7));frames.push(blob);
pBar.style.width=(i/total*50)+'%';stat.innerHTML=`Procesando: ${Math.round(i/total*100)}%`;
}
const fd=new FormData();fd.append("f",new Blob(frames,{type:'video/x-mjpeg'}),file.name.split('.')[0]+".mjpeg");
const xhr=new XMLHttpRequest();xhr.open("POST","/upload",true);
xhr.upload.onprogress=e=>{const p=50+Math.round(e.loaded/e.total*50);pBar.style.width=p+'%';stat.innerHTML=`Subiendo: ${p}%`;};
xhr.onload=()=>{alert("Éxito");location.reload();};xhr.send(fd);
}
function loadList(){
fetch('/list_data').then(r=>r.json()).then(data=>{
let html='';data.forEach(v=>{
html+=`<div class="item">
<div><input type="checkbox" class="v-check" value="${v.name}" ${v.active?'checked':''}> <span>${v.name}</span></div>
<button class="btn-del" onclick="deleteFile('${v.name}')">Borrar</button>
</div>`;
});
document.getElementById('list').innerHTML=html||'SD vacía';
});
}
function deleteFile(name){if(confirm('¿Borrar '+name+'?')) fetch('/delete?name='+name).then(()=>loadList());}
function savePlaylist(){
const sel=[];document.querySelectorAll('.v-check:checked').forEach(c=>sel.push(c.value));
fetch('/set_playlist',{method:'POST',body:sel.join('\n')}).then(()=>alert('Playlist guardada'));
}
loadList();
</script></body></html>)rawliteral";

void startWiFiDrive(const char* folder) {
    WiFi.softAP("ESP32-C6-DRIVE");
    dnsServer.start(DNS_PORT, "*", WiFi.softAPIP());

    server.on("/", HTTP_GET, []() {
        server.send(200, "text/html", upload_html);
    });

    server.on("/list_data", HTTP_GET, []() {
        String playlist = "";
        if(SD.exists("/playlist.conf")){
            File f = SD.open("/playlist.conf", FILE_READ);
            playlist = f.readString();
            f.close();
        }
        String json = "[";
        File root = SD.open("/mjpeg");
        File file = root.openNextFile();
        while(file) {
            if(!file.isDirectory()) {
                if(json != "[") json += ",";
                bool active = (playlist.indexOf(file.name()) != -1);
                json += "{\"name\":\"" + String(file.name()) + "\", \"active\":" + (active?"true":"false") + "}";
            }
            file = root.openNextFile();
        }
        server.send(200, "application/json", json + "]");
    });

    server.on("/delete", HTTP_GET, []() {
        String name = server.arg("name");
        if(name.length() > 0) {
            SD.remove("/mjpeg/" + name);
            server.send(200, "text/plain", "OK");
        }
    });

    server.on("/set_playlist", HTTP_POST, []() {
        String data = server.arg("plain");
        File f = SD.open("/playlist.conf", FILE_WRITE);
        f.print(data);
        f.close();
        server.send(200, "text/plain", "OK");
    });

    server.on("/upload", HTTP_POST, []() { server.send(200); }, []() {
        HTTPUpload& upload = server.upload();
        static File uploadFile;
        if (upload.status == UPLOAD_FILE_START) {
            uploadFile = SD.open("/mjpeg/" + upload.filename, FILE_WRITE);
        } else if (upload.status == UPLOAD_FILE_WRITE) {
            if (uploadFile) uploadFile.write(upload.buf, upload.currentSize);
        } else if (upload.status == UPLOAD_FILE_END) {
            if (uploadFile) uploadFile.close();
        }
    });

    server.on("/restart", []() {
        server.send(200, "text/plain", "Reiniciando...");
        delay(500); ESP.restart();
    });

    server.onNotFound([]() {
        server.sendHeader("Location", String("http://") + WiFi.softAPIP().toString(), true);
        server.send(302, "text/plain", "");
    });

    server.begin();
}

void stopWiFiDrive() {
    dnsServer.stop();
    server.stop();
    WiFi.softAPdisconnect(true);
    WiFi.mode(WIFI_OFF);
}

void handleWiFiDrive() {
    dnsServer.processNextRequest();
    server.handleClient();
}

#endif