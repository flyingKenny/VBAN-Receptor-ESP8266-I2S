#include <ESP8266WiFi.h>
#include <WiFiUdp.h>
#include <WiFiManager.h>
#include <i2s.h>
#include <i2s_reg.h>


#include "VBAN.h"
#include "packet.h"

#define VBAN_BIT_RESOLUTION_MAX 2
#define EINVAL 22


unsigned int localPort = 6980;      // local port to listen on

// buffers for receiving and sending data
int VBANBuffer[VBAN_PROTOCOL_MAX_SIZE];

WiFiUDP Udp;

void setup() {
  Serial.begin(115200);
  wifiman_setup();

  Serial.println("VBAN WiFi receptor");
  Serial.print("Connected! IP address: ");
  Serial.println(WiFi.localIP());
  Udp.begin(localPort);

  i2s_begin();
}


void wifiman_setup(void)
{
  //WiFiManager
  //Local intialization. Once its business is done, there is no need to keep it around
  WiFiManager wifiManager;
  //reset saved settings SSID and password
  //wifiManager.resetSettings();

  //fetches ssid and pass from eeprom and tries to connect
  //if it does not connect it starts an access point with the specified name
  //here  "AutoConnectAP"
  //and goes into a blocking loop awaiting configuration
  wifiManager.autoConnect("VBAN WiFi receptor");

  //if you get here you have connected to the WiFi
  Serial.println(F("connected"));
}


static int packet_pcm_check(char const* buffer, size_t size)
{
  /** the packet is already a valid vban packet and buffer already checked before */

  struct VBanHeader const* const hdr = PACKET_HEADER_PTR(buffer);
  enum VBanBitResolution const bit_resolution = (VBanBitResolution)(hdr->format_bit & VBAN_BIT_RESOLUTION_MASK);
  int const sample_rate   = hdr->format_SR & VBAN_SR_MASK;
  int const nb_samples    = hdr->format_nbs + 1;
  int const nb_channels   = hdr->format_nbc + 1;
  size_t sample_size      = 0;
  size_t payload_size     = 0;

  //logger_log(LOG_DEBUG, "%s: packet is vban: %u, sr: %d, nbs: %d, nbc: %d, bit: %d, name: %s, nu: %u",
  //    __func__, hdr->vban, hdr->format_SR, hdr->format_nbs, hdr->format_nbc, hdr->format_bit, hdr->streamname, hdr->nuFrame);

  if (bit_resolution >= VBAN_BIT_RESOLUTION_MAX)
  {
    Serial.println("invalid bit resolution");
    return -EINVAL;
  }

  if (sample_rate >= VBAN_SR_MAXNUMBER)
  {
    Serial.println("invalid sample rate");
    return -EINVAL;
  }

  sample_size = VBanBitResolutionSize[bit_resolution];
  payload_size = nb_samples * sample_size * nb_channels;

  if (payload_size != (size - VBAN_HEADER_SIZE))
  {
    //    logger_log(LOG_WARNING, "%s: invalid payload size, expected %d, got %d", __func__, payload_size, (size - VBAN_HEADER_SIZE));
    Serial.println("invalid payload size");
    return -EINVAL;
  }

  return 0;
}

int vban_packet_check(char const* streamname, char const* buffer, size_t size)
{
  struct VBanHeader const* const hdr = PACKET_HEADER_PTR(buffer);
  enum VBanProtocol protocol = VBAN_PROTOCOL_UNDEFINED_4;
  enum VBanCodec codec = (VBanCodec)VBAN_BIT_RESOLUTION_MAX;

  if ((streamname == 0) || (buffer == 0))
  {
    Serial.println("null pointer argument");
    return -EINVAL;
  }

  if (size <= VBAN_HEADER_SIZE)
  {
    Serial.println("packet too small");
    return -EINVAL;
  }

  if (hdr->vban != VBAN_HEADER_FOURC)
  {
    Serial.println("invalid vban magic fourc");
    return -EINVAL;
  }

  if (strncmp(streamname, hdr->streamname, VBAN_STREAM_NAME_SIZE))
  {
    Serial.println("different streamname");
    return -EINVAL;
  }

  /** check the reserved bit : it must be 0 */
  if (hdr->format_bit & VBAN_RESERVED_MASK)
  {
    Serial.println("reserved format bit invalid value");
    return -EINVAL;
  }

  /** check protocol and codec */
  protocol        = (VBanProtocol)(hdr->format_SR & VBAN_PROTOCOL_MASK);
  codec           = (VBanCodec)(hdr->format_bit & VBAN_CODEC_MASK);

  switch (protocol)
  {
    case VBAN_PROTOCOL_AUDIO:
      return (codec == VBAN_CODEC_PCM) ? packet_pcm_check(buffer, size) : -EINVAL;
      /*Serial.print("OK: VBAN_PROTOCOL_AUDIO");
        Serial.print(": nuFrame ");
        Serial.print(hdr->nuFrame);
        Serial.print(": format_SR ");
        Serial.print(hdr->format_SR);
        Serial.print(": format_nbs ");
        Serial.print(hdr->format_nbs);
        Serial.print(": format_nbc ");
        Serial.println(hdr->format_nbc);
      */
      return 0;
    case VBAN_PROTOCOL_SERIAL:
    case VBAN_PROTOCOL_TXT:
    case VBAN_PROTOCOL_UNDEFINED_1:
    case VBAN_PROTOCOL_UNDEFINED_2:
    case VBAN_PROTOCOL_UNDEFINED_3:
    case VBAN_PROTOCOL_UNDEFINED_4:
      /** not supported yet */
      return -EINVAL;

    default:
      Serial.println("packet with unknown protocol");
      return -EINVAL;
  }
}


uint8_t VBANSample = 0;
int* VBANData;
int16_t* VBANData1Ch;
int VBANValue;
void loop() {
  // if there's data available, read a packet
  int packetSize = Udp.parsePacket();
  if (packetSize) {
    // read the packet into VBANBuffer
    Udp.read((char*)VBANBuffer, VBAN_PROTOCOL_MAX_SIZE);

    if (vban_packet_check("Stream1", (char*)VBANBuffer, packetSize) == 0)
    {
      struct VBanHeader const* const hdr = PACKET_HEADER_PTR((char*)VBANBuffer);
      i2s_set_rate((int)VBanSRList[hdr->format_SR & VBAN_SR_MASK]);
      
      //copy the received data to i2s buffer
      VBANSample = 0;
      VBANData = (int*)(((char*)VBANBuffer) + VBAN_HEADER_SIZE);
      VBANData1Ch = (int16_t*)VBANData;
      for (VBANSample = 0; VBANSample < hdr->format_nbs;VBANSample++)
      {
        switch (hdr->format_nbc)
        {
          case 0:   //one channel
            VBANData1Ch = (int16_t*)VBANData;
            VBANValue =  (int)VBANData1Ch[VBANSample]<<16 | VBANData1Ch[VBANSample];
            break;
          case 1:    //two channels
            VBANValue =  VBANData[VBANSample];
            break;
        }
        i2s_write_sample(VBANValue);
      }
    }
    else
    {
      Serial.println("VBAN Error");
    }
  }
}
