#include <boost/date_time/posix_time/posix_time.hpp>

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
const std::string LAPTOP_ID = "0";
const std::string SEP = "|";
boost::posix_time::ptime epoch(boost::gregorian::date(2010, 1, 1));
double ave = -56;

double SignalToDistanceMeters(double dbm, int freq) {
  double exp = (27.55 - (20 * log(freq)/log(10.0)) + fabs(dbm*1.0)) / 20.0;
  return pow(10.0, exp); 
}

bool callback(const PDU &pdu) {
  try {
    const RadioTap *rad = pdu.find_pdu<RadioTap>();
    if (!rad) {
      std::cerr << "No radiotap!\n";
      return true;
    }
    int strength = (int)rad->dbm_signal();
    int freq = (int)rad->channel_freq();

    std::string src = "";
    std::string dst = "";
    std::string header_type = "";

    const Dot11Data *dot = pdu.find_pdu<Dot11Data>();
    if (dot) {
      src = dot->src_addr().to_string();
      dst = dot->dst_addr().to_string();
      header_type = "dot";
    }

    const Dot11ManagementFrame *dmf = pdu.find_pdu<Dot11ManagementFrame>();
    if (dmf) {
      src = dmf->addr2().to_string();
      dst = dmf->addr1().to_string();
      header_type = "dot_management";
    }

    const EthernetII *eth = pdu.find_pdu<EthernetII>();
    if (eth) {
      src = eth->src_addr().to_string();
      dst = eth->dst_addr().to_string();
      header_type = "eth";
    }
  
    if (src == "" && dst == "") {
      return true;
    }

    std::string target_a = "38:71:de:4c:84:2d";
    std::string target_b = "d0:22:be:65:f4:a9";
    if (dst == "58:f3:9c:e1:30:bd") {
      return true;
    }
    if (src == target_a || src == target_b) {
      ave = strength * 0.8 + ave * 0.2;
      double distance = SignalToDistanceMeters(strength, freq);

      std::string msg_str = LAPTOP_ID + SEP +
                            src + SEP +
                            std::to_string(distance) + SEP +
                            std::to_string((boost::posix_time::second_clock::local_time()-epoch).total_milliseconds());

      msg_in->Body(msg_str);
      channel->BasicPublish("amq.direct", LAPTOP_ID, msg_in);

      std::cout << src << " -> " << dst
                << " signal: " << strength
                << " dbm. Estimated distance: "
                << distance << " m. Header type: "
                << header_type << std::endl;
    }
  } catch(const field_not_present &e) {
    // Nothing 
  } catch(const std::exception &e) {
    std::cerr << "Exception " << e.what() << std::endl;
  }

  return true;
}

int main() {
  std::string broker_address = "18.111.46.141";//18.111.117.170";
  channel = AmqpClient::Channel::Create(broker_address, 5672, "a", "a");
  channel->DeclareQueue("6.857-" + LAPTOP_ID, true);
  channel->BindQueue("6.857-" + LAPTOP_ID, "amq.direct", LAPTOP_ID);
  msg_in = AmqpClient::BasicMessage::Create();

  SnifferConfiguration config;
  config.set_promisc_mode(true);
  config.set_rfmon(true);
  config.set_buffer_size(80 << 20);
  config.set_snap_len(2048);
  //config.set_filter("ether host d0:22:be:65:f4:a9");
  std::cerr << "Started!\n";
  Sniffer("en0", config).sniff_loop(callback);
}
