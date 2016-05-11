#include <boost/algorithm/string.hpp>
#include <SimpleAmqpClient/BasicMessage.h>
#include <SimpleAmqpClient/Channel.h>
#include <SimpleAmqpClient/Util.h>

#include <iostream>
#include <vector>

#include "UIRenderer.hpp"
#include "Utils.hpp"

using namespace AmqpClient;

int main(int argc, char **argv) {
  ui::UIRenderer renderer(config::SCREEN_WIDTH, config::SCREEN_HEIGHT, argc, argv);
  utils::TimeAverager R[2];
  utils::TimeAverager X, Y;

  std::string broker_uri = "";
  Channel::ptr_t channel[2];
  channel[0] = Channel::Create(broker_uri, 5672, "a", "a");
  channel[1] = Channel::Create(broker_uri, 5672, "a", "a");

  channel[0]->BasicConsume("6.857-0", "consumertag-0");
  channel[1]->BasicConsume("6.857-1", "consumertag-1");
  while (true) {
    for (int channel_id = 0; channel_id < 2; ++channel_id) {
      BasicMessage::ptr_t msg_out = channel[channel_id]->BasicConsumeMessage("consumertag-" + std::to_string(channel_id))->Message();
      const std::string msg = msg_out->Body();

      std::cout << "Message text: " << msg << std::endl;
      std::vector<std::string> tokens;
      boost::split(tokens, msg, boost::is_any_of("|"));

      // Extract message details.
      assert(std::stoi(tokens[0]) == channel_id);
      const std::string mac = tokens[1];
      const utils::Notification notif(std::stod(tokens[2]), std::stod(tokens[3]));

      // Update one of the radii.
      R[channel_id].add(mac, notif);

      // Recalculate position estimates.
      utils::CalculatePositionEstimates(R, &X, &Y, notif.ts);
    }

    const auto &macsX = X.keys();
    const auto &macsY = Y.keys();
    for (const auto &mac : macsX) {
      if (macsY.find(mac) != macsY.end()) {
        // Estimate and add that to the UIRenderer.
        double dx, dy;
        X.get(mac, &dx);
        Y.get(mac, &dy);
        int x = int(dx * config::PIXEL_DISTANCE / config::LAPTOP_DISTANCE_M + 40);
        int y = int(dy * config::PIXEL_DISTANCE / config::LAPTOP_DISTANCE_M + config::SCREEN_HEIGHT / 2.0);

        std::cerr << "Calculated distance to be: " << dx << " " << dy << " pixelwise: " << x << " " << y << std::endl;

        int color;
        double r, g, b;
        if (mac[0] == 'd') {
          color = 0xff00ffff;
        } else {
          color = 0xff0000ff;
        }

        utils::Circle circle(x, y, 40, color);
        renderer.set(mac, circle);
      }
    }
    renderer.set("0", utils::Circle(40, config::SCREEN_HEIGHT / 2 - config::PIXEL_DISTANCE / 2, 20, 0xffff0000));
    renderer.set("1", utils::Circle(40, config::SCREEN_HEIGHT / 2 + config::PIXEL_DISTANCE / 2, 20, 0xffff0000));

    renderer.update();
  }

  return 0;
}
