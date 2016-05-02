#include <SimpleAmqpClient/BasicMessage.h>
#include <SimpleAmqpClient/Channel.h>
#include <SimpleAmqpClient/Util.h>

#include <iostream>
#include <cstring>
#include <cmath>
#include <tins/tins.h>

using namespace Tins;

AmqpClient::Channel::ptr_t channel;
AmqpClient::BasicMessage::ptr_t msg_in;

double SignalToDistanceMeters(int dbm, int freq) {
  double exp = (27.55 - (20 * log(freq)/log(10.0)) + fabs(dbm*1.0)) / 20.0;
  return pow(10.0, exp); 
}

bool callback(const PDU &pdu) {
  try {
    const RadioTap *rad = pdu.find_pdu<RadioTap>();
    if (!rad) {
      return true;
    }
    int strength = (int)rad->dbm_signal();
    int freq = (int)rad->channel_freq();

    const Dot11Data *dot = pdu.find_pdu<Dot11Data>();
    if (!dot) {
      return true;
    }
    std::string src = dot->src_addr().to_string();
    std::string tar = dot->dst_addr().to_string();
    std::string target_a = "38:71:de:4c:84:2d";
    std::string target_b = "d0:22:be:65:f4:a9";
    if (src == target_a || src == target_b) {
      double distance = SignalToDistanceMeters(strength, freq);
      msg_in->Body("1|" + src + "|" + std::to_string(distance));
      channel->BasicPublish("amq.direct", "key", msg_in);
      std::cout << src << " -> " << tar
                << " signal: " << strength
                << " dbm. Estimated distance: "
                << distance << " m." << std::endl;
    }
  } catch(const field_not_present &e) {
    // Nothing 
  } catch(const std::exception &e) {
    std::cerr << "Exception " << e.what() << std::endl;
  }

  return true;
}

int main() {
  std::string broker_address = "18.111.42.239";
  channel = AmqpClient::Channel::Create(broker_address);
  channel->DeclareQueue("6.857", true);
  channel->BindQueue("6.857", "amq.direct", "key");
  msg_in = AmqpClient::BasicMessage::Create();

  SnifferConfiguration config;
  config.set_promisc_mode(true);
  config.set_rfmon(true);
  config.set_buffer_size(80 << 20);
  //config.set_filter("ether host d0:22:be:65:f4:a9");
  Sniffer("en0", config).sniff_loop(callback);
}
