#include "./discovery.h"

#define UDP_PACKETS_PER_LOOP 4

DeviceProtocol protocol;

Discovery discovery = {
    KEY_DISCOVERY,
};
uint8_t total = 0;
uint32_t lastTimeDiscovery = 0;
uint32_t lastTimeProtocol = 0;
WiFiUDP udp;

// Shared receive buffer instead of a 1500-byte local in udpReceive(): the
// ESP8266 has only ~1.5-2 KB of free stack.
static uint8_t rxBuffer[PACKET_BUFFER_SIZE];

bool udpSendPacket(const uint8_t* data, size_t length, uint32_t targetAddress) {
  IPAddress targetIp(targetAddress);
  udp.beginPacket(targetIp, settings.discoveryPort);
  udp.write(data, length);
  bool ok = udp.endPacket();
  // Serial.printf("[SEND] %d bytes -> %s:%d\n", length, targetIp.toString().c_str(), settings.discoveryPort);
  return ok;
}

void udpReceive() {
  // Bound the work per loop: a discovery flood must not stretch the iteration.
  for (uint8_t i = 0; i < UDP_PACKETS_PER_LOOP; i++) {
    const int packetSize = udp.parsePacket();
    if (packetSize <= 0) return;

    IPAddress senderIp = udp.remoteIP();
    const int len = udp.read(rxBuffer, sizeof(rxBuffer));

    if (packetSize > (int)sizeof(rxBuffer)) {
      while (udp.available() > 0) udp.read();
      continue;
    }

    if (len > 0) protocol.handleIncomingData((uint32_t)senderIp, rxBuffer, (size_t)len);
  }
}

void sendPeer(const PeerInfo& peer) {
  discovery.id = peer.id;
  discovery.ip = peer.address;
  discovery.lastSeen = peer.lastSeen;
  discovery.status = peer.status;
  wsSendAll((uint8_t*)&discovery, sizeof(discovery));
}

bool udpHandler(uint8_t paramCount, const Value* params, Value& result, void* userData) {
  if (paramCount < 2) return false;

  const uint32_t device = params[0].uintVal;
  const char* text = params[1].stringVal.data;

  result.type = VAL_INT;
  result.intVal = protocol.sendText(device, text);
  return true;
}

void setupDiscovery() {
  if (!settings.discovery) return;
  udp.begin(settings.discoveryPort);

  protocol.setup(infoFS.id);

  protocol.onSend([](const OutgoingPacket& p) {
    return udpSendPacket(p.data, p.length, p.targetAddress);
  });

  protocol.onPeerDiscovered([](const PeerInfo& peer) {
    sendPeer(peer);
  });

  protocol.onPeerLost([](const PeerInfo& peer) {
    sendPeer(peer);
  });

  protocol.onTextReceived([](uint32_t senderId, const String& text) {
#if VUESP_LOG_LEVEL >= 2
    Serial.printf("TEXT from 0x%08X: %s\n", senderId, text.c_str());
#else
    (void)senderId;
#endif
    // Incoming text is a script: register it in the temporary slot and run it.
    scriptRunner.registerScript(254, text.c_str());
    scriptRunner.runScript(254);
  });

  protocol.onBinaryReceived([](uint32_t senderId, const uint8_t* data, size_t len) {
    // Serial.printf("BINARY from 0x%08X, %d bytes: ", senderId, len);
    // for (size_t i = 0; i < len; i++) Serial.printf("%02X ", data[i]);
    // Serial.println();
  });
  scriptRunner.registerFunction("udp", udpHandler);
}

void commDiscovery() {
  if (discovery.command == CMD_GET) {
    total = protocol.getPeerCount();
  }
}

void loopDiscovery(uint32_t now) {
  if (!settings.discovery) return;
  if (!isConnected) return;
  udpReceive();
  protocol.update();

  if (now - lastTimeDiscovery > settings.discoveryInterval * 1000) {
    lastTimeDiscovery = now;
    protocol.sendAnnounce();
  }

  if (total && now - lastTimeProtocol > 100) {
    lastTimeProtocol = now;
    const PeerInfo* peer = protocol.getPeerByIndex(total - 1);
    if (peer) {
      sendPeer(*peer);
      total--;
    }
  }
}
