#pragma once

#ifdef GRUMPY_WEB_SETUP

#include <Arduino.h>
#include <FS.h>

class MyMesh;
class AsyncWebServer;
class DNSServer;

class GrumpyWebSetup {
public:
  void begin(MyMesh &mesh, fs::FS &filesystem);
  void loop();
  bool isActive() const { return _active; }

private:
  void startPortal();
  void stopPortal();

  MyMesh *_mesh = nullptr;
  fs::FS *_filesystem = nullptr;
  AsyncWebServer *_server = nullptr;
  DNSServer *_dns = nullptr;
  uint32_t _restart_at = 0;
  bool _active = false;
};

extern GrumpyWebSetup grumpy_web_setup;

#endif
