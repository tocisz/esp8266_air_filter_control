#ifndef TopicProvider_h
#define TopicProvider_h

#include <DHTesp.h>

class TopicProvider {
  const char * const TOPIC_PRESENCE_CMD = "preq";
  const char * const PREFIX_P = "p/";
  const char * const PREFIX_C = "c/";
  const char * const PREFIX_S = "s/";
  const char * const SEPARATOR = "/";
  const char * const INTERN = "i";
  const char * const ONLINE = "online";
  const char * const HT = "ht";
  char id[16];
  const char * const area;
  char buf[32];

  public:
  TopicProvider(const char *area) : area(area) {}
  const char *get_id() { return id; }
  void set_id(const char *id) { strlcpy(this->id, id, 16); }

  const char *presence_cmd() { return TOPIC_PRESENCE_CMD; }
  const char *presence() {
    size_t i;
    i  = strlcpy(buf,   PREFIX_P,  32);
    i += strlcpy(buf+i, area,      32-i);
    i += strlcpy(buf+i, SEPARATOR, 32-i);
    i += strlcpy(buf+i, id,        32-i);
    return buf;
  }
  const char *presence_internal() {
    size_t i;
    i  = strlcpy(buf,   PREFIX_P,  32);
    i += strlcpy(buf+i, area,      32-i);
    i += strlcpy(buf+i, SEPARATOR, 32-i);
    i += strlcpy(buf+i, id,        32-i);
    i += strlcpy(buf+i, SEPARATOR, 32-i);
    i += strlcpy(buf+i, INTERN,    32-i);
    return buf;
  }
  const char *presence_online() {
    size_t i;
    i  = strlcpy(buf,   PREFIX_P,  32);
    i += strlcpy(buf+i, area,      32-i);
    i += strlcpy(buf+i, SEPARATOR, 32-i);
    i += strlcpy(buf+i, id,        32-i);
    i += strlcpy(buf+i, SEPARATOR, 32-i);
    i += strlcpy(buf+i, ONLINE,    32-i);
    return buf;
  }
  const char *cmd() {
    size_t i;
    i  = strlcpy(buf,   PREFIX_C,  32);
    i += strlcpy(buf+i, area,      32-i);
    i += strlcpy(buf+i, SEPARATOR, 32-i);
    i += strlcpy(buf+i, id,        32-i);
    return buf;
  }
  const char *sensor_ht() {
    size_t i;
    i  = strlcpy(buf,   PREFIX_S,  32);
    i += strlcpy(buf+i, area,      32-i);
    i += strlcpy(buf+i, SEPARATOR, 32-i);
    i += strlcpy(buf+i, HT,        32-i);
    i += strlcpy(buf+i, SEPARATOR, 32-i);
    i += strlcpy(buf+i, id,        32-i);
    return buf;
  }
};

#endif
