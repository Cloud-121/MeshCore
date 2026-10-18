#include "GrumpyWebSetup.h"

#ifdef GRUMPY_WEB_SETUP

#include <DNSServer.h>
#include <ESPAsyncWebServer.h>
#include <WiFi.h>
#include <esp_system.h>

#include "MyMesh.h"

namespace {
constexpr char kCompleteMarker[] = "/grumpy_setup_complete";
constexpr float kGulfCoastFrequency = 910.525f;
constexpr float kGulfCoastBandwidth = 62.5f;
constexpr uint8_t kGulfCoastSpreadingFactor = 7;
constexpr uint8_t kGulfCoastCodingRate = 8;
constexpr uint8_t kPathHashMode = 1;
constexpr uint8_t kAgcResetInterval = 75; // Stored in four-second units: 300 seconds.
constexpr uint8_t kMultiAcks = 1;
constexpr uint16_t kDnsPort = 53;

const char kSetupPage[] PROGMEM = R"HTML(
<!doctype html><html lang="en"><head><meta charset="utf-8">
<meta name="viewport" content="width=device-width,initial-scale=1">
<title>GrumpyMesh Repeater Setup</title><style>
:root{--ink:#fff;--muted:#bfbfbf;--paper:#0b0b0b;--card:#111;--orange:#ff7a00;--line:#383838;--danger:#ff6b5f}
*{box-sizing:border-box}body{margin:0;background:radial-gradient(circle at 50% 20%,#ff7a002e,transparent 42%),linear-gradient(#111,#000);background-attachment:fixed;color:var(--ink);font:16px/1.5 Arial,Helvetica,sans-serif;min-height:100vh}
body:before{content:"";display:block;height:4px;background:var(--orange);box-shadow:0 3px 18px #ff7a0077}
main{width:min(680px,calc(100% - 28px));margin:34px auto}.brand{font-weight:900;letter-spacing:.12em;color:var(--orange)}
h1{font-size:clamp(2rem,8vw,3.5rem);line-height:1;margin:.45rem 0 1rem}h2{font-size:1.45rem;margin:0 0 .5rem}
.lead,.hint{color:var(--muted)}.card{background:var(--card);border:1px solid var(--line);border-radius:18px;padding:clamp(20px,5vw,36px);box-shadow:0 15px 40px #0009,0 0 25px #ff7a0018}
.step{display:none}.step.active{display:block}.progress{height:7px;background:#303030;border-radius:9px;margin:26px 0;overflow:hidden}.progress i{display:block;height:100%;background:var(--orange);box-shadow:0 0 12px #ff7a0099;transition:width .2s}
label{display:block;font-weight:750;margin:18px 0 6px}input,select{width:100%;padding:13px 14px;border:1px solid #555;border-radius:10px;background:#181818;font:inherit;color:var(--ink)}
input:focus,select:focus{outline:3px solid #ff7a0044;border-color:var(--orange)}.choice{display:grid;grid-template-columns:1fr 1fr;gap:12px}.choice label{margin:0;padding:16px;border:2px solid var(--line);border-radius:12px;cursor:pointer}.choice input{width:auto;margin-right:8px;accent-color:var(--orange)}
.custom{margin-top:20px;padding-top:4px;border-top:1px solid var(--line)}.grid{display:grid;grid-template-columns:1fr 1fr;gap:0 14px}
.check{display:flex;align-items:flex-start;gap:10px;padding:18px;border:2px solid var(--orange);border-radius:12px;background:#ff7a0014}.check input{width:auto;margin-top:5px;accent-color:var(--orange)}.check label{margin:0}
.actions{display:flex;gap:10px;justify-content:flex-end;margin-top:28px}.actions button{border:0;border-radius:10px;padding:12px 21px;font:800 1rem inherit;cursor:pointer}.back{background:#333;color:var(--ink)}.next,.finish{background:var(--orange);color:#111;box-shadow:0 8px 20px #ff7a0038}.finish:disabled{opacity:.55}
.summary{display:grid;grid-template-columns:auto 1fr;gap:8px 14px;padding:16px;background:#1b1b1b;border:1px solid #333;border-radius:12px}.summary dt{font-weight:800;color:var(--orange)}.summary dd{margin:0;overflow-wrap:anywhere}.error{color:var(--danger);font-weight:700;min-height:1.5em}.success{text-align:center;padding:30px 0}.success b{font-size:3rem;color:var(--orange)}
@media(max-width:520px){.choice,.grid{grid-template-columns:1fr}.actions button{flex:1}}
</style></head><body><main><div class="brand">GRUMPYMESH</div><h1>Repeater setup</h1>
<p class="lead">Thank you for buying a Grumpy board. Let&rsquo;s get it onto the mesh.</p><div class="progress"><i id="bar" style="width:20%"></i></div>
<form id="form" class="card">
<section class="step active"><h2>Name your repeater</h2><p class="hint">Pick a recognizable public name (24 bytes maximum because location is shared).</p><label for="name">Repeater name</label><input id="name" name="name" maxlength="24" required autocomplete="off" placeholder="Grumpy Gulf Repeater"><div class="actions"><button class="next" type="button">Continue</button></div></section>
<section class="step"><h2>Choose radio settings</h2><p class="hint">The Gulf Coast preset connects to the normal public Gulf Coast Mesh.</p><div class="choice"><label><input type="radio" name="profile" value="gulf" checked> Gulf Coast Mesh</label><label><input type="radio" name="profile" value="custom"> Custom</label></div>
<div class="custom"><div class="grid"><div><label for="freq">Frequency (MHz)</label><input id="freq" name="freq" type="number" min="902" max="928" step="0.001" value="910.525" required></div><div><label for="bw">Bandwidth (kHz)</label><select id="bw" name="bw"><option>7.8</option><option>10.4</option><option>15.6</option><option>20.8</option><option>31.25</option><option>41.7</option><option selected>62.5</option><option>125</option><option>250</option><option>500</option></select></div><div><label for="sf">Spreading factor</label><select id="sf" name="sf"><option>5</option><option>6</option><option selected>7</option><option>8</option><option>9</option><option>10</option><option>11</option><option>12</option></select></div><div><label for="cr">Coding rate</label><select id="cr" name="cr"><option>5</option><option>6</option><option>7</option><option selected>8</option></select></div></div></div><div class="actions"><button class="back" type="button">Back</button><button class="next" type="button">Continue</button></div></section>
<section class="step"><h2>Set its location</h2><p class="hint">These coordinates are included in public repeater adverts. Use the installed antenna&rsquo;s location.</p><div class="grid"><div><label for="lat">Latitude</label><input id="lat" name="lat" type="number" min="-90" max="90" step="0.000001" required placeholder="29.951066"></div><div><label for="lon">Longitude</label><input id="lon" name="lon" type="number" min="-180" max="180" step="0.000001" required placeholder="-90.071532"></div></div><div class="actions"><button class="back" type="button">Back</button><button class="next" type="button">Continue</button></div></section>
<section class="step"><h2>Check the antenna</h2><p>Transmitting without a correctly installed 902&ndash;928 MHz antenna can damage radio hardware.</p><div class="check"><input id="antenna" name="antenna" value="on" type="checkbox" required><label for="antenna">I have securely installed the correct antenna on the board.</label></div><div class="actions"><button class="back" type="button">Back</button><button class="next" type="button">Review</button></div></section>
<section class="step"><h2>Review and finish</h2><p class="hint">The board will save these settings, turn off setup Wi-Fi, and restart as a repeater.</p><dl class="summary" id="summary"></dl><p class="hint">Also applied: 2-byte path hashes, AGC reset every 300 seconds, and multi-ACKs.</p><p class="error" id="error"></p><div class="actions"><button class="back" type="button">Back</button><button class="finish" type="submit">Save &amp; reboot</button></div></section>
</form></main><script>
const steps=[...document.querySelectorAll('.step')],form=document.querySelector('#form'),bar=document.querySelector('#bar');let at=0;
function show(n){at=n;steps.forEach((s,i)=>s.classList.toggle('active',i===n));bar.style.width=((n+1)/steps.length*100)+'%';scrollTo(0,0);if(n===4)review()}
function valid(){const fields=[...steps[at].querySelectorAll('input,select')];for(const f of fields){if(!f.checkValidity()){f.reportValidity();return false}}return true}
document.querySelectorAll('.next').forEach(b=>b.onclick=()=>{if(valid())show(at+1)});document.querySelectorAll('.back').forEach(b=>b.onclick=()=>show(at-1));
document.querySelectorAll('input[name=profile]').forEach(r=>r.onchange=()=>{const off=r.value==='gulf'&&r.checked;['freq','bw','sf','cr'].forEach(id=>document.querySelector('#'+id).disabled=off);if(off){document.querySelector('#freq').value='910.525';document.querySelector('#bw').value='62.5';document.querySelector('#sf').value='7';document.querySelector('#cr').value='8'}});document.querySelector('input[name=profile]:checked').onchange();
function val(id){return document.querySelector('#'+id).value}function review(){const gulf=document.querySelector('input[name=profile]:checked').value==='gulf';document.querySelector('#summary').innerHTML='<dt>Name</dt><dd>'+esc(val('name'))+'</dd><dt>Network</dt><dd>'+(gulf?'Gulf Coast Mesh':'Custom')+'</dd><dt>Radio</dt><dd>'+val('freq')+' MHz / '+val('bw')+' kHz / SF'+val('sf')+' / CR'+val('cr')+'</dd><dt>Coordinates</dt><dd>'+val('lat')+', '+val('lon')+'</dd>'}function esc(s){const d=document.createElement('div');d.textContent=s;return d.innerHTML}
form.onsubmit=async e=>{e.preventDefault();if(!valid())return;const btn=document.querySelector('.finish'),err=document.querySelector('#error');btn.disabled=true;err.textContent='Saving…';const data=new URLSearchParams(new FormData(form));if(data.get('profile')==='gulf'){data.set('freq','910.525');data.set('bw','62.5');data.set('sf','7');data.set('cr','8')}try{const res=await fetch('/save',{method:'POST',headers:{'Content-Type':'application/x-www-form-urlencoded'},body:data});const msg=await res.text();if(!res.ok)throw Error(msg);form.innerHTML='<div class="success"><b>✓</b><h2>Setup complete</h2><p>Your repeater is restarting. You can close this page.</p></div>';bar.style.width='100%'}catch(x){err.textContent=x.message||'Could not save settings.';btn.disabled=false}};
</script></body></html>
)HTML";

bool hasInvalidNameCharacter(const String &name) {
  return name.indexOf('[') >= 0 || name.indexOf(']') >= 0 || name.indexOf('\\') >= 0 ||
         name.indexOf(':') >= 0 || name.indexOf(',') >= 0 || name.indexOf('?') >= 0 ||
         name.indexOf('*') >= 0;
}

bool parseFloatParam(AsyncWebServerRequest *request, const char *name, float &value) {
  if (!request->hasParam(name, true)) return false;
  const String raw = request->getParam(name, true)->value();
  char *end = nullptr;
  value = strtof(raw.c_str(), &end);
  return end != raw.c_str() && *end == '\0' && isfinite(value);
}

bool parseIntParam(AsyncWebServerRequest *request, const char *name, int &value) {
  if (!request->hasParam(name, true)) return false;
  const String raw = request->getParam(name, true)->value();
  char *end = nullptr;
  const long parsed = strtol(raw.c_str(), &end, 10);
  if (end == raw.c_str() || *end != '\0') return false;
  value = static_cast<int>(parsed);
  return true;
}

bool isSupportedBandwidth(float value) {
  constexpr float supported[] = {7.8f, 10.4f, 15.6f, 20.8f, 31.25f, 41.7f, 62.5f, 125.0f, 250.0f, 500.0f};
  for (float candidate : supported) {
    if (fabsf(value - candidate) < 0.01f) return true;
  }
  return false;
}
} // namespace

GrumpyWebSetup grumpy_web_setup;

void GrumpyWebSetup::begin(MyMesh &mesh, fs::FS &filesystem) {
  _mesh = &mesh;
  _filesystem = &filesystem;
  if (_filesystem->exists(kCompleteMarker)) {
    WiFi.mode(WIFI_OFF);
    return;
  }
  startPortal();
}

void GrumpyWebSetup::startPortal() {
  uint8_t mac[6];
  WiFi.macAddress(mac);
  char ssid[32];
  snprintf(ssid, sizeof(ssid), "GrumpyMesh-Setup-%02X%02X", mac[4], mac[5]);

  WiFi.mode(WIFI_AP);
  WiFi.softAP(ssid);

  _dns = new DNSServer();
  _dns->start(kDnsPort, "*", WiFi.softAPIP());
  _server = new AsyncWebServer(80);

  _server->on("/", HTTP_GET, [](AsyncWebServerRequest *request) {
    request->send_P(200, "text/html", kSetupPage);
  });

  _server->on("/save", HTTP_POST, [this](AsyncWebServerRequest *request) {
    if (_restart_at != 0) {
      request->send(409, "text/plain", "Setup has already been saved.");
      return;
    }

    if (!request->hasParam("name", true) || !request->hasParam("antenna", true)) {
      request->send(400, "text/plain", "Name and antenna confirmation are required.");
      return;
    }

    const String name = request->getParam("name", true)->value();
    const String profile = request->hasParam("profile", true)
                               ? request->getParam("profile", true)->value()
                               : String();
    float frequency, bandwidth, latitude, longitude;
    int spreading_factor, coding_rate;
    const bool valid_radio = parseFloatParam(request, "freq", frequency) && frequency >= 902.0f && frequency <= 928.0f &&
                             parseFloatParam(request, "bw", bandwidth) && isSupportedBandwidth(bandwidth) &&
                             parseIntParam(request, "sf", spreading_factor) && spreading_factor >= 5 && spreading_factor <= 12 &&
                             parseIntParam(request, "cr", coding_rate) && coding_rate >= 5 && coding_rate <= 8;
    if (name.isEmpty() || name.length() > 24 || hasInvalidNameCharacter(name) ||
        (profile != "gulf" && profile != "custom") || !valid_radio ||
        !parseFloatParam(request, "lat", latitude) || latitude < -90.0f || latitude > 90.0f ||
        !parseFloatParam(request, "lon", longitude) || longitude < -180.0f || longitude > 180.0f ||
        request->getParam("antenna", true)->value() != "on") {
      request->send(400, "text/plain", "One or more settings are invalid. Please review every step.");
      return;
    }

    // Never trust client-provided values for the guided public-network preset.
    if (profile == "gulf") {
      frequency = kGulfCoastFrequency;
      bandwidth = kGulfCoastBandwidth;
      spreading_factor = kGulfCoastSpreadingFactor;
      coding_rate = kGulfCoastCodingRate;
    }

    NodePrefs *prefs = _mesh->getNodePrefs();
    name.toCharArray(prefs->node_name, sizeof(prefs->node_name));
    prefs->freq = frequency;
    prefs->bw = bandwidth;
    prefs->sf = static_cast<uint8_t>(spreading_factor);
    prefs->cr = static_cast<uint8_t>(coding_rate);
    prefs->node_lat = latitude;
    prefs->node_lon = longitude;
    prefs->advert_loc_policy = ADVERT_LOC_PREFS;
    prefs->path_hash_mode = kPathHashMode;
    prefs->agc_reset_interval = kAgcResetInterval;
    prefs->multi_acks = kMultiAcks;
    _mesh->savePrefs();

    File saved_preferences = _filesystem->open("/com_prefs", "r");
    const bool preferences_written = saved_preferences && saved_preferences.size() >= 293;
    if (saved_preferences) saved_preferences.close();
    if (!preferences_written) {
      request->send(500, "text/plain", "Settings could not be verified on storage. Please try again.");
      return;
    }

    File marker = _filesystem->open(kCompleteMarker, "w");
    if (!marker) {
      request->send(500, "text/plain", "Settings were saved, but setup could not be finalized. Please try again.");
      return;
    }
    marker.print("1");
    marker.close();

    request->send(200, "text/plain", "Saved");
    _restart_at = millis() + 2000;
  });

  _server->onNotFound([](AsyncWebServerRequest *request) {
    request->redirect("/");
  });
  _server->begin();
  _active = true;

  Serial.printf("Grumpy setup: connect to %s and open http://%s/\n", ssid, WiFi.softAPIP().toString().c_str());
}

void GrumpyWebSetup::stopPortal() {
  if (_dns) _dns->stop();
  if (_server) _server->end();
  WiFi.softAPdisconnect(true);
  WiFi.mode(WIFI_OFF);
  _active = false;
}

void GrumpyWebSetup::loop() {
  if (!_active) return;
  _dns->processNextRequest();
  if (_restart_at != 0 && static_cast<int32_t>(millis() - _restart_at) >= 0) {
    stopPortal();
    ESP.restart();
  }
}

#endif
